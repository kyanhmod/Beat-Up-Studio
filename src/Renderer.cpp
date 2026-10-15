#include "Renderer.h"
#include <d3dcompiler.h>
#include <cmath>
#include <algorithm>
#include <fstream>

namespace BeatUp {

const char* g_shaderHLSL = R"(
struct VS_INPUT {
    float2 pos : POSITION;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos.x = (input.pos.x / 1024.0f) * 2.0f - 1.0f;
    output.pos.y = 1.0f - (input.pos.y / 768.0f) * 2.0f;
    output.pos.z = 0.0f;
    output.pos.w = 1.0f;
    output.uv = input.uv;
    output.col = input.col;
    return output;
}

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

float4 PS(PS_INPUT input) : SV_Target {
    float4 texColor = g_texture.Sample(g_sampler, input.uv);
    return texColor * input.col;
}
)";

Renderer::Renderer() {
    m_batchVertices.reserve(MAX_BATCH_VERTICES);
}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Init(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight) {
    m_hWnd = hWnd;
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;

    if (!CreateDeviceAndSwapChain(hWnd)) return false;
    if (!CreateRenderTarget()) return false;
    if (!CreateShadersAndBuffers()) return false;
    if (!CreateStates()) return false;

    // Create 1x1 white pixel texture for primitive drawing
    uint32_t whitePixelData = 0xFFFFFFFF;
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = 1;
    texDesc.Height = 1;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subData = {};
    subData.pSysMem = &whitePixelData;
    subData.SysMemPitch = 4;

    if (SUCCEEDED(m_device->CreateTexture2D(&texDesc, &subData, &m_whitePixel.texture))) {
        m_device->CreateShaderResourceView(m_whitePixel.texture, nullptr, &m_whitePixel.srv);
        m_whitePixel.width = 1;
        m_whitePixel.height = 1;
    }

    InitFontAtlas();

    return true;
}

void Renderer::Shutdown() {
    m_songTitleTexture.Release();
    m_cachedSongTitle.clear();
    m_fontTexture.Release();
    m_whitePixel.Release();
    if (m_blendAlpha) { m_blendAlpha->Release(); m_blendAlpha = nullptr; }
    if (m_blendAdditive) { m_blendAdditive->Release(); m_blendAdditive = nullptr; }
    if (m_samplerLinear) { m_samplerLinear->Release(); m_samplerLinear = nullptr; }
    if (m_rasterizerState) { m_rasterizerState->Release(); m_rasterizerState = nullptr; }
    if (m_vertexBuffer) { m_vertexBuffer->Release(); m_vertexBuffer = nullptr; }
    if (m_inputLayout) { m_inputLayout->Release(); m_inputLayout = nullptr; }
    if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = nullptr; }
    if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }
    if (m_renderTargetView) { m_renderTargetView->Release(); m_renderTargetView = nullptr; }
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_context) { m_context->Release(); m_context = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
}

bool Renderer::CreateDeviceAndSwapChain(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Width = m_windowWidth;
    scd.BufferDesc.Height = m_windowHeight;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 0;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevels, ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION, &scd, &m_swapChain,
        &m_device, &featureLevel, &m_context
    );

    if (FAILED(hr)) {
        // Fallback to older swap effect if flip discard unsupported
        scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            featureLevels, ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION, &scd, &m_swapChain,
            &m_device, &featureLevel, &m_context
        );
    }

    return SUCCEEDED(hr);
}

bool Renderer::CreateRenderTarget() {
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr)) return false;

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);
    backBuffer->Release();
    return SUCCEEDED(hr);
}

bool Renderer::CreateShadersAndBuffers() {
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(g_shaderHLSL, strlen(g_shaderHLSL), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) errorBlob->Release();
        return false;
    }

    hr = D3DCompile(g_shaderHLSL, strlen(g_shaderHLSL), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        vsBlob->Release();
        if (errorBlob) errorBlob->Release();
        return false;
    }

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    if (FAILED(hr)) { vsBlob->Release(); psBlob->Release(); return false; }

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
    if (FAILED(hr)) { vsBlob->Release(); psBlob->Release(); return false; }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = m_device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    vsBlob->Release();
    psBlob->Release();
    if (FAILED(hr)) return false;

    // Create dynamic vertex buffer
    D3D11_BUFFER_DESC vbd = {};
    vbd.ByteWidth = sizeof(Vertex2D) * MAX_BATCH_VERTICES;
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_device->CreateBuffer(&vbd, nullptr, &m_vertexBuffer);
    return SUCCEEDED(hr);
}

bool Renderer::CreateStates() {
    // Sampler
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(m_device->CreateSamplerState(&sampDesc, &m_samplerLinear))) return false;

    // Blend: Alpha
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(m_device->CreateBlendState(&blendDesc, &m_blendAlpha))) return false;

    // Blend: Additive (for glows, sparks)
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    if (FAILED(m_device->CreateBlendState(&blendDesc, &m_blendAdditive))) return false;

    // Rasterizer
    D3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_NONE;
    rastDesc.ScissorEnable = FALSE;
    rastDesc.DepthClipEnable = FALSE;
    if (FAILED(m_device->CreateRasterizerState(&rastDesc, &m_rasterizerState))) return false;

    return true;
}

void Renderer::Resize(uint32_t newWidth, uint32_t newHeight) {
    if (!m_swapChain || newWidth == 0 || newHeight == 0 || (newWidth==m_windowWidth && newHeight==m_windowHeight)) return;

    if (m_renderTargetView) {
        m_renderTargetView->Release();
        m_renderTargetView = nullptr;
    }

    m_swapChain->ResizeBuffers(0, newWidth, newHeight, DXGI_FORMAT_UNKNOWN, 0);
    m_windowWidth = newWidth;
    m_windowHeight = newHeight;
    CreateRenderTarget();
}

void Renderer::BeginFrame(float r, float g, float b, float a) {
    float clearColor[4] = { r, g, b, a };
    m_context->ClearRenderTargetView(m_renderTargetView, clearColor);
    m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

    D3D11_VIEWPORT vp = {};
    // Preserve the 4:3 playfield on widescreen/fullscreen displays.
    float scale = (std::min)(static_cast<float>(m_windowWidth) / m_virtualWidth,
                            static_cast<float>(m_windowHeight) / m_virtualHeight);
    vp.Width = m_virtualWidth * scale;
    vp.Height = m_virtualHeight * scale;
    vp.TopLeftX = (m_windowWidth - vp.Width) * 0.5f;
    vp.TopLeftY = (m_windowHeight - vp.Height) * 0.5f;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);
    m_context->RSSetState(m_rasterizerState);

    m_context->IASetInputLayout(m_inputLayout);
    UINT stride = sizeof(Vertex2D);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);
    m_context->PSSetSamplers(0, 1, &m_samplerLinear);

    m_batchVertices.clear();
    m_currentBatchSRV = nullptr;
    m_currentBatchAdditive = false;
}

void Renderer::FlushBatch() {
    if (m_batchVertices.empty()) return;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(m_context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, m_batchVertices.data(), m_batchVertices.size() * sizeof(Vertex2D));
        m_context->Unmap(m_vertexBuffer, 0);

        float blendFactor[4] = { 0, 0, 0, 0 };
        m_context->OMSetBlendState(m_currentBatchAdditive ? m_blendAdditive : m_blendAlpha, blendFactor, 0xFFFFFFFF);
        m_context->PSSetShaderResources(0, 1, &m_currentBatchSRV);

        m_context->Draw(static_cast<UINT>(m_batchVertices.size()), 0);
    }

    m_batchVertices.clear();
}

void Renderer::EndFrame(bool vsync) {
    FlushBatch();
    m_swapChain->Present(vsync ? 1 : 0, 0);
}

void Renderer::DrawSprite(ID3D11ShaderResourceView* srv, float x, float y, float w, float h,
                          float angleRad, float alpha, bool additive,
                          float scaleX, float scaleY,
                          float r, float g, float b) {
    DrawSpriteUV(srv, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, angleRad, alpha, additive, scaleX, scaleY, r, g, b);
}

void Renderer::DrawSpriteUV(ID3D11ShaderResourceView* srv, float x, float y, float w, float h,
                            float u0, float v0, float u1, float v1,
                            float angleRad, float alpha, bool additive,
                            float scaleX, float scaleY,
                            float r, float g, float b) {
    if (!srv) srv = m_whitePixel.srv;

    if (m_currentBatchSRV != srv || m_currentBatchAdditive != additive || m_batchVertices.size() + 6 > MAX_BATCH_VERTICES) {
        FlushBatch();
        m_currentBatchSRV = srv;
        m_currentBatchAdditive = additive;
    }

    float hw = (w * 0.5f) * scaleX;
    float hh = (h * 0.5f) * scaleY;

    float cosA = cosf(angleRad);
    float sinA = sinf(angleRad);

    // Quad local corners: TopLeft, TopRight, BottomRight, BottomLeft
    float lx[4] = { -hw,  hw,  hw, -hw };
    float ly[4] = { -hh, -hh,  hh,  hh };
    float u[4]  = { u0,   u1,  u1,  u0 };
    float v[4]  = { v0,   v0,  v1,  v1 };

    Vertex2D vtx[4];
    for (int i = 0; i < 4; ++i) {
        vtx[i].x = x + (lx[i] * cosA - ly[i] * sinA);
        vtx[i].y = y + (lx[i] * sinA + ly[i] * cosA);
        vtx[i].u = u[i];
        vtx[i].v = v[i];
        vtx[i].r = r;
        vtx[i].g = g;
        vtx[i].b = b;
        vtx[i].a = alpha;
    }

    // Tri 1: 0, 1, 2
    m_batchVertices.push_back(vtx[0]);
    m_batchVertices.push_back(vtx[1]);
    m_batchVertices.push_back(vtx[2]);

    // Tri 2: 0, 2, 3
    m_batchVertices.push_back(vtx[0]);
    m_batchVertices.push_back(vtx[2]);
    m_batchVertices.push_back(vtx[3]);
}

void Renderer::DrawRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    DrawSprite(m_whitePixel.srv, x + w * 0.5f, y + h * 0.5f, w, h, 0.0f, a, false, 1.0f, 1.0f, r, g, b);
}

void Renderer::DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float thickness) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;

    float angle = atan2f(dy, dx);
    float midX = (x1 + x2) * 0.5f;
    float midY = (y1 + y2) * 0.5f;

    DrawSprite(m_whitePixel.srv, midX, midY, len, thickness, angle, a, false, 1.0f, 1.0f, r, g, b);
}

bool Renderer::InitFontAtlas() {
    int texW = 512;
    int texH = 256;
    m_fontAtlasW = static_cast<float>(texW);
    m_fontAtlasH = static_cast<float>(texH);

    HDC hdcMem = CreateCompatibleDC(nullptr);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = texW;
    bmi.bmiHeader.biHeight = -texH; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hbm = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    if (!hbm || !pBits) {
        DeleteDC(hdcMem);
        return false;
    }
    SelectObject(hdcMem, hbm);

    memset(pBits, 0, texW * texH * 4);

    HFONT hFont = CreateFontW(
        22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Tahoma"
    );
    SelectObject(hdcMem, hFont);

    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, RGB(255, 255, 255));

    int curX = 2;
    int curY = 2;
    int maxHeightInRow = 0;

    for (int c = 32; c <= 126; ++c) {
        wchar_t ch = static_cast<wchar_t>(c);
        SIZE sz;
        GetTextExtentPoint32W(hdcMem, &ch, 1, &sz);
        if (sz.cy > maxHeightInRow) maxHeightInRow = sz.cy;

        if (curX + sz.cx + 4 >= texW) {
            curX = 2;
            curY += maxHeightInRow + 4;
            maxHeightInRow = sz.cy;
        }

        RECT rc = { curX, curY, curX + sz.cx + 2, curY + sz.cy + 2 };
        DrawTextW(hdcMem, &ch, 1, &rc, DT_LEFT | DT_TOP | DT_NOCLIP);

        FontGlyph& g = m_glyphs[c];
        g.u0 = static_cast<float>(curX) / m_fontAtlasW;
        g.v0 = static_cast<float>(curY) / m_fontAtlasH;
        g.u1 = static_cast<float>(curX + sz.cx) / m_fontAtlasW;
        g.v1 = static_cast<float>(curY + sz.cy) / m_fontAtlasH;
        g.width = static_cast<float>(sz.cx);
        g.height = static_cast<float>(sz.cy);

        curX += sz.cx + 4;
    }

    uint8_t* pixels = reinterpret_cast<uint8_t*>(pBits);
    for (int i = 0; i < texW * texH; ++i) {
        uint8_t r = pixels[i * 4 + 2];
        uint8_t g = pixels[i * 4 + 1];
        uint8_t b = pixels[i * 4 + 0];
        uint8_t intensity = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
        pixels[i * 4 + 0] = 255;
        pixels[i * 4 + 1] = 255;
        pixels[i * 4 + 2] = 255;
        pixels[i * 4 + 3] = intensity;
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = texW;
    desc.Height = texH;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pBits;
    initData.SysMemPitch = texW * 4;

    if (SUCCEEDED(m_device->CreateTexture2D(&desc, &initData, &m_fontTexture.texture))) {
        m_device->CreateShaderResourceView(m_fontTexture.texture, nullptr, &m_fontTexture.srv);
        m_fontTexture.width = texW;
        m_fontTexture.height = texH;
    }

    DeleteObject(hFont);
    DeleteObject(hbm);
    DeleteDC(hdcMem);
    return m_fontTexture.srv != nullptr;
}

bool Renderer::SaveFrameBitmap(const std::wstring& path) {
    if (!m_context || !m_renderTargetView) return false;
    FlushBatch();
    ID3D11Resource* resource = nullptr;
    m_renderTargetView->GetResource(&resource);
    ID3D11Texture2D* source = nullptr;
    HRESULT hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&source));
    resource->Release();
    if (FAILED(hr)) return false;
    D3D11_TEXTURE2D_DESC desc{};
    source->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    ID3D11Texture2D* staging = nullptr;
    hr = m_device->CreateTexture2D(&desc, nullptr, &staging);
    if (FAILED(hr)) { source->Release(); return false; }
    m_context->CopyResource(staging, source);
    source->Release();
    D3D11_MAPPED_SUBRESOURCE mapped{};
    hr = m_context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) { staging->Release(); return false; }
    BITMAPFILEHEADER header{};
    BITMAPINFOHEADER info{};
    header.bfType = 0x4D42;
    header.bfOffBits = sizeof(header) + sizeof(info);
    header.bfSize = header.bfOffBits + desc.Width * desc.Height * 4;
    info.biSize = sizeof(info);
    info.biWidth = desc.Width; info.biHeight = -static_cast<LONG>(desc.Height);
    info.biPlanes = 1; info.biBitCount = 32; info.biCompression = BI_RGB;
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<char*>(&header), sizeof(header));
    file.write(reinterpret_cast<char*>(&info), sizeof(info));
    std::vector<uint8_t> row(desc.Width * 4);
    for (UINT y = 0; y < desc.Height; ++y) {
        auto pixels = static_cast<const uint8_t*>(mapped.pData) + y * mapped.RowPitch;
        for (UINT x = 0; x < desc.Width; ++x) {
            row[x*4] = pixels[x*4+2]; row[x*4+1] = pixels[x*4+1];
            row[x*4+2] = pixels[x*4]; row[x*4+3] = 255;
        }
        file.write(reinterpret_cast<const char*>(row.data()), row.size());
    }
    m_context->Unmap(staging, 0); staging->Release();
    return file.good();
}

void Renderer::DrawSongTitle(const std::wstring& text, float x, float y) {
    // A bounded, single-entry UTF-16 texture cache. Windows font fallback covers
    // Vietnamese/CJK titles; text is clipped to the original 160-pixel label.
    if (!m_device || text.empty()) return;
    if (text != m_cachedSongTitle || !m_songTitleTexture.srv) {
        FlushBatch();
        m_songTitleTexture.Release();
        constexpr int width = 160, height = 22;
        HDC dc = CreateCompatibleDC(nullptr);
        if (!dc) return;
        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        void* bits = nullptr;
        HBITMAP bitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        HFONT font = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Gulim");
        if (!bitmap || !bits || !font) {
            if (bitmap) DeleteObject(bitmap);
            if (font) DeleteObject(font);
            DeleteDC(dc);
            return;
        }
        auto oldBitmap = SelectObject(dc, bitmap);
        auto oldFont = SelectObject(dc, font);
        memset(bits, 0, width * height * 4);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(255, 255, 255));
        RECT rect{0, 0, width, height};
        DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &rect,
                  DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        auto pixels = static_cast<uint8_t*>(bits);
        for (int i = 0; i < width * height; ++i) {
            uint8_t alpha = (std::max)({pixels[i*4], pixels[i*4+1], pixels[i*4+2]});
            pixels[i*4] = pixels[i*4+1] = pixels[i*4+2] = 255;
            pixels[i*4+3] = alpha;
        }
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width; desc.Height = height;
        desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = bits; data.SysMemPitch = width * 4;
        if (SUCCEEDED(m_device->CreateTexture2D(&desc, &data, &m_songTitleTexture.texture))) {
            m_device->CreateShaderResourceView(m_songTitleTexture.texture, nullptr, &m_songTitleTexture.srv);
        }
        SelectObject(dc, oldFont); SelectObject(dc, oldBitmap);
        DeleteObject(font); DeleteObject(bitmap); DeleteDC(dc);
        m_cachedSongTitle = text;
    }
    if (m_songTitleTexture.srv) {
        DrawSpriteTL(m_songTitleTexture.srv, x+1, y+1, 160, 22, 0.85f, false, 0, 0, 0);
        DrawSpriteTL(m_songTitleTexture.srv, x, y, 160, 22);
    }
}

void Renderer::DrawString(const std::string& text, float x, float y, float size,
                          float r, float g, float b, float a, bool dropShadow) {
    if (!m_fontTexture.srv || text.empty()) return;

    float baseHeight = 22.0f;
    float scale = size / baseHeight;

    auto DrawPass = [&](float px, float py, float cr, float cg, float cb, float ca) {
        float cx = px;
        for (char ch : text) {
            uint8_t uc = static_cast<uint8_t>(ch);
            if (uc < 32 || uc > 126) uc = 32;
            const FontGlyph& glyph = m_glyphs[uc];
            float gw = glyph.width * scale;
            float gh = glyph.height * scale;
            if (uc != 32) {
                DrawSpriteTL_UV(m_fontTexture.srv, cx, py, gw, gh,
                                glyph.u0, glyph.v0, glyph.u1, glyph.v1,
                                ca, false, cr, cg, cb);
            }
            cx += gw + (2.0f * scale);
        }
    };

    if (dropShadow) {
        DrawPass(x + 1.5f, y + 1.5f, 0.0f, 0.0f, 0.0f, a * 0.85f);
    }
    DrawPass(x, y, r, g, b, a);
}

} // namespace BeatUp

#include "SpritePlacement.h"
#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <vector>
#include <algorithm>
#include "DDSLoader.h"

namespace BeatUp {

struct Vertex2D {
    float x, y;
    float u, v;
    float r, g, b, a;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Init(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight);
    void Shutdown();
    void Resize(uint32_t newWidth, uint32_t newHeight);

    void BeginFrame(float r = 0.05f, float g = 0.05f, float b = 0.08f, float a = 1.0f);
    void EndFrame(bool vsync = false);
    bool SaveFrameBitmap(const std::wstring& path);

    // Draw sprite centered at (x, y) with dimensions (w, h)
    void DrawSprite(ID3D11ShaderResourceView* srv, float x, float y, float w, float h,
                    float angleRad = 0.0f, float alpha = 1.0f, bool additive = false,
                    float scaleX = 1.0f, float scaleY = 1.0f,
                    float r = 1.0f, float g = 1.0f, float b = 1.0f);

    // Draw sprite with custom UV coordinates
    void DrawSpriteUV(ID3D11ShaderResourceView* srv, float x, float y, float w, float h,
                      float u0, float v0, float u1, float v1,
                      float angleRad = 0.0f, float alpha = 1.0f, bool additive = false,
                      float scaleX = 1.0f, float scaleY = 1.0f,
                      float r = 1.0f, float g = 1.0f, float b = 1.0f);

    // Optical glyph placement: alpha padding never changes glyph size or center.
    void DrawContent(const TextureResource& tex,float cx,float cy,float maxWidth,float maxHeight,float alpha=1,float scale=1) {
        if(!tex.srv || !tex.width || !tex.height)return;
        auto fit=FitContent(tex.content,maxWidth,maxHeight);if(fit.width<=0 || fit.height<=0)return;
        DrawSpriteUV(tex.srv,cx,cy,fit.width,fit.height,
            float(tex.content.left)/tex.width,float(tex.content.top)/tex.height,
            float(tex.content.right)/tex.width,float(tex.content.bottom)/tex.height,0,alpha,false,scale,scale);
    }

    // Draw sprite using top-left coordinates
    void DrawSpriteTL(ID3D11ShaderResourceView* srv, float x, float y, float w, float h,
                      float alpha = 1.0f, bool additive = false,
                      float r = 1.0f, float g = 1.0f, float b = 1.0f) {
        DrawSprite(srv, x + w * 0.5f, y + h * 0.5f, w, h, 0.0f, alpha, additive, 1.0f, 1.0f, r, g, b);
    }

    // Draw sprite using top-left coordinates with custom UV coordinates
    void DrawSpriteTL_UV(ID3D11ShaderResourceView* srv, float x, float y, float w, float h,
                         float u0, float v0, float u1, float v1,
                         float alpha = 1.0f, bool additive = false,
                         float r = 1.0f, float g = 1.0f, float b = 1.0f) {
        DrawSpriteUV(srv, x + w * 0.5f, y + h * 0.5f, w, h, u0, v0, u1, v1, 0.0f, alpha, additive, 1.0f, 1.0f, r, g, b);
    }

    // Draw a rectangle
    void DrawRect(float x, float y, float w, float h, float r, float g, float b, float a);

    // Draw line between 2 points
    void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float thickness = 2.0f);

    // Draw text with built-in font
    void DrawString(const std::string& text, float x, float y, float size = 16.0f,
                    float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f,
                    bool dropShadow = true);
    void DrawSongTitle(const std::wstring& text, float x, float y);

    ID3D11Device* GetDevice() const { return m_device; }
    uint32_t GetVirtualWidth() const { return m_virtualWidth; }
    uint32_t GetVirtualHeight() const { return m_virtualHeight; }

private:
    bool CreateDeviceAndSwapChain(HWND hWnd);
    bool CreateRenderTarget();
    bool CreateShadersAndBuffers();
    bool CreateStates();
    bool InitFontAtlas();
    void FlushBatch();

    HWND m_hWnd = nullptr;
    uint32_t m_windowWidth = 1024;
    uint32_t m_windowHeight = 768;
    const uint32_t m_virtualWidth = 1024;
    const uint32_t m_virtualHeight = 768;

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;

    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11SamplerState* m_samplerLinear = nullptr;
    ID3D11BlendState* m_blendAlpha = nullptr;
    ID3D11BlendState* m_blendAdditive = nullptr;
    ID3D11RasterizerState* m_rasterizerState = nullptr;

    TextureResource m_whitePixel;

    // Font Atlas
    struct FontGlyph {
        float u0, v0, u1, v1;
        float width;
        float height;
    };
    TextureResource m_fontTexture;
    TextureResource m_songTitleTexture;
    std::wstring m_cachedSongTitle;
    FontGlyph m_glyphs[128]{};
    float m_fontAtlasW = 512.0f;
    float m_fontAtlasH = 256.0f;

    static const size_t MAX_BATCH_VERTICES = 4096;
    std::vector<Vertex2D> m_batchVertices;
    ID3D11ShaderResourceView* m_currentBatchSRV = nullptr;
    bool m_currentBatchAdditive = false;
};

} // namespace BeatUp

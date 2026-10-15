#include "AssetBundle.h"
#include "DDSLoader.h"
#include <fstream>
#include <vector>
#include <algorithm>

namespace BeatUp {

#pragma pack(push, 1)
struct DDS_PIXELFORMAT {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwABitMask;
};

struct DDS_HEADER {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwHeight;
    DWORD dwWidth;
    DWORD dwPitchOrLinearSize;
    DWORD dwDepth;
    DWORD dwMipMapCount;
    DWORD dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    DWORD dwCaps;
    DWORD dwCaps2;
    DWORD dwCaps3;
    DWORD dwCaps4;
    DWORD dwReserved2;
};
#pragma pack(pop)

#define DDS_MAGIC 0x20534444 // "DDS "
#define DDPF_ALPHAPIXELS 0x00000001
#define DDPF_ALPHA       0x00000002
#define DDPF_FOURCC      0x00000004
#define DDPF_RGB         0x00000040

#define FOURCC_DXT1 0x31545844
#define FOURCC_DXT3 0x33545844
#define FOURCC_DXT5 0x35545844

bool DDSLoader::LoadFromFile(ID3D11Device* device, const std::wstring& filename, TextureResource& outTex) {
    auto buffer=ReadAsset(filename);
    if(!LoadFromMemory(device, buffer.data(), buffer.size(), outTex))return false;
    outTex.content=AssetContentBounds(filename,outTex.width,outTex.height);
    return true;
}

bool DDSLoader::LoadFromMemory(ID3D11Device* device, const uint8_t* data, size_t size, TextureResource& outTex) {
    if (!device || !data || size < sizeof(DWORD) + sizeof(DDS_HEADER)) return false;

    DWORD magic = *reinterpret_cast<const DWORD*>(data);
    if (magic != DDS_MAGIC) return false;

    const DDS_HEADER* header = reinterpret_cast<const DDS_HEADER*>(data + sizeof(DWORD));
    if (header->dwSize != sizeof(DDS_HEADER)) return false;

    uint32_t width = header->dwWidth;
    uint32_t height = header->dwHeight;
    if (width == 0 || height == 0) return false;

    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    UINT rowPitch = 0;
    std::vector<uint8_t> expanded;

    if (header->ddspf.dwFlags & DDPF_FOURCC) {
        if (header->ddspf.dwFourCC == FOURCC_DXT1) {
            format = DXGI_FORMAT_BC1_UNORM;
            rowPitch = std::max<UINT>(1, ((width + 3) / 4)) * 8;
        } else if (header->ddspf.dwFourCC == FOURCC_DXT3) {
            format = DXGI_FORMAT_BC2_UNORM;
            rowPitch = std::max<UINT>(1, ((width + 3) / 4)) * 16;
        } else if (header->ddspf.dwFourCC == FOURCC_DXT5) {
            format = DXGI_FORMAT_BC3_UNORM;
            rowPitch = std::max<UINT>(1, ((width + 3) / 4)) * 16;
        }
    } else if (header->ddspf.dwFlags & DDPF_RGB) {
        if (header->ddspf.dwRGBBitCount == 32) {
            if (header->ddspf.dwRBitMask == 0x00FF0000 && header->ddspf.dwBBitMask == 0x000000FF) {
                format = DXGI_FORMAT_B8G8R8A8_UNORM;
            } else if (header->ddspf.dwRBitMask == 0x000000FF && header->ddspf.dwBBitMask == 0x00FF0000) {
                format = DXGI_FORMAT_R8G8B8A8_UNORM;
            } else {
                format = DXGI_FORMAT_B8G8R8A8_UNORM; // default 32-bit fallback for Audition textures
            }
            rowPitch = width * 4;
        } else if(header->ddspf.dwRGBBitCount==16 || header->ddspf.dwRGBBitCount==24) {
            if(width>16384 || height>16384) return false;
            unsigned bytesPerPixel=header->ddspf.dwRGBBitCount/8;
            size_t pitch=static_cast<size_t>(width)*bytesPerPixel;
            if((header->dwFlags&8) && header->dwPitchOrLinearSize>=pitch) pitch=header->dwPitchOrLinearSize;
            if(pitch>size || height>(size-128)/pitch) return false;
            expanded.resize(static_cast<size_t>(width)*height*4);
            auto channel=[](uint32_t pixel,DWORD mask,uint8_t missing) -> uint8_t {
                if(!mask) return missing;
                unsigned shift=0;while(!(mask&1)) {mask>>=1;++shift;}
                return static_cast<uint8_t>(((pixel>>shift)&mask)*255/mask);
            };
            for(uint32_t y=0;y<height;++y) for(uint32_t x=0;x<width;++x) {
                const auto* source=data+128+y*pitch+x*bytesPerPixel;uint32_t pixel=source[0]|(source[1]<<8);
                if(bytesPerPixel==3) pixel|=static_cast<uint32_t>(source[2])<<16;
                auto* output=expanded.data()+(static_cast<size_t>(y)*width+x)*4;
                output[0]=channel(pixel,header->ddspf.dwBBitMask,0);output[1]=channel(pixel,header->ddspf.dwGBitMask,0);
                output[2]=channel(pixel,header->ddspf.dwRBitMask,0);output[3]=channel(pixel,header->ddspf.dwABitMask,255);
            }
            format=DXGI_FORMAT_B8G8R8A8_UNORM;rowPitch=width*4;
        }
    }

    if (format == DXGI_FORMAT_UNKNOWN) return false;

    const uint8_t* pPixels = data + sizeof(DWORD) + sizeof(DDS_HEADER);
    size_t pixelDataSize = size - (sizeof(DWORD) + sizeof(DDS_HEADER));
    if(!expanded.empty()) {pPixels=expanded.data();pixelDataSize=expanded.size();}

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pPixels;
    initData.SysMemPitch = rowPitch;
    initData.SysMemSlicePitch = static_cast<UINT>(pixelDataSize);

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &initData, &texture);
    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    hr = device->CreateShaderResourceView(texture, &srvDesc, &srv);
    if (FAILED(hr)) {
        texture->Release();
        return false;
    }

    outTex.texture = texture;
    outTex.srv = srv;
    outTex.width = width;
    outTex.height = height;
    return true;
}

} // namespace BeatUp

#pragma once
#include <windows.h>
#include <d3d11.h>
#include <string>

namespace BeatUp {

struct TextureResource {
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11Texture2D* texture = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    RECT content{};

    void Release() {
        if (srv) { srv->Release(); srv = nullptr; }
        if (texture) { texture->Release(); texture = nullptr; }
    }
};

class DDSLoader {
public:
    static bool LoadFromFile(ID3D11Device* device, const std::wstring& filename, TextureResource& outTex);
    static bool LoadFromMemory(ID3D11Device* device, const uint8_t* data, size_t size, TextureResource& outTex);
};

} // namespace BeatUp

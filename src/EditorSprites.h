#pragma once
#include <windows.h>
#include <sstream>
#include "AssetBundle.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdint>
#pragma comment(lib,"msimg32.lib")
namespace BeatUp {
// GDI editor preview uses the same supplied uncompressed DDS arrow art as D3D.
class EditorSprites {
    struct Sprite {HBITMAP bitmap=nullptr;int width=0,height=0;};
    std::array<Sprite,10> sprites_{};
    static bool Load(const std::filesystem::path& path,Sprite& sprite) {
        auto bytes=ReadAsset(path);std::istringstream f(std::string(bytes.begin(),bytes.end()),std::ios::binary);std::array<uint32_t,32> header{};
        if(!f.read(reinterpret_cast<char*>(header.data()),128) || header[0]!=0x20534444 || header[22]!=32 || header[21]!=0 || header[23]!=0x00ff0000 || header[26]!=0xff000000) return false;
        int h=header[3],w=header[4];if(w<=0 || h<=0 || w>2048 || h>2048) return false;
        std::vector<unsigned char> pixels(w*h*4);if(!f.read(reinterpret_cast<char*>(pixels.data()),pixels.size())) return false;
        for(size_t i=0;i<pixels.size();i+=4) for(int c=0;c<3;++c) pixels[i+c]=static_cast<unsigned char>(pixels[i+c]*pixels[i+3]/255);
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
        void* data=nullptr;sprite.bitmap=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&data,nullptr,0);if(!sprite.bitmap)return false;
        memcpy(data,pixels.data(),pixels.size());sprite.width=w;sprite.height=h;return true;
    }
public:
    EditorSprites() {
        wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);auto dir=std::filesystem::path(exe).parent_path();
        for(int lane:{9,6,3,7,4,1,5}) {
            auto name=lane==5?L"space.dds":L"a"+std::to_wstring(lane)+L"1.dds";
            for(auto base:{dir/L"assets/hud_clean",dir/L"../assets/hud_clean",dir/L"../../assets/hud_clean",std::filesystem::path(L"assets/hud_clean")}) if(Load(base/name,sprites_[lane])) break;
        }
    }
    ~EditorSprites() {for(auto& s:sprites_) if(s.bitmap)DeleteObject(s.bitmap);}
    bool Draw(HDC dc,int lane,int x,int y,int size) {
        auto& s=sprites_[lane];if(!s.bitmap)return false;
        int height=lane==5?size/2:size;auto memory=CreateCompatibleDC(dc);auto old=SelectObject(memory,s.bitmap);
        BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};AlphaBlend(dc,x,y,size,height,memory,0,0,s.width,s.height,blend);
        SelectObject(memory,old);DeleteDC(memory);return true;
    }
};
}

#pragma once
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <compressapi.h>
#include <cstdint>
#pragma comment(lib,"cabinet.lib")
namespace BeatUp {
inline std::vector<unsigned char> ReadAsset(const std::filesystem::path& path) {
    auto key=path.filename().u8string();
    static const std::vector<unsigned char> bundle=[] {
        std::vector<unsigned char> output;
        auto r=FindResourceW(nullptr,MAKEINTRESOURCEW(201),RT_RCDATA);if(!r)return output;
        size_t size=SizeofResource(nullptr,r);auto data=static_cast<const unsigned char*>(LockResource(LoadResource(nullptr,r)));
        uint64_t length=0;if(!data || size<8)return output;memcpy(&length,data,8);
        if(!length || length>64*1024*1024)return output;
        DECOMPRESSOR_HANDLE handle=nullptr;if(!CreateDecompressor(COMPRESS_ALGORITHM_XPRESS_HUFF,nullptr,&handle))return output;
        output.resize(static_cast<size_t>(length));SIZE_T written=0;
        bool ok=Decompress(handle,data+8,size-8,output.data(),output.size(),&written)!=FALSE;CloseDecompressor(handle);
        if(!ok || written!=length)output.clear();return output;
    }();
    if(!bundle.empty()) {
        size_t size=bundle.size();auto data=bundle.data();
        size_t pos=0;
        while(data && pos+8<=size) {
            uint32_t n=0,length=0;memcpy(&n,data+pos,4);memcpy(&length,data+pos+4,4);pos+=8;
            if(n>size-pos || length>size-pos-n)break;
            if(key==std::string(reinterpret_cast<const char*>(data+pos),n))return {data+pos+n,data+pos+n+length};
            pos+=n+length;
        }
    }
    std::ifstream file(path,std::ios::binary);return {(std::istreambuf_iterator<char>(file)),{}};
}
inline RECT AssetContentBounds(const std::filesystem::path& path,unsigned width,unsigned height) {
    static const auto layout=ReadAsset(L"hud_layout.bin");auto key=path.filename().u8string();size_t pos=0;
    while(pos+4<=layout.size()) {
        uint32_t n=0;memcpy(&n,layout.data()+pos,4);pos+=4;
        if(n>layout.size()-pos || layout.size()-pos-n<16)break;
        if(key==std::string(reinterpret_cast<const char*>(layout.data()+pos),n)) {
            uint32_t b[4]{};memcpy(b,layout.data()+pos+n,16);
            if(b[0]<=b[2] && b[1]<=b[3] && b[2]<=width && b[3]<=height)return {LONG(b[0]),LONG(b[1]),LONG(b[2]),LONG(b[3])};
        }
        pos+=n+16;
    }
    return {0,0,LONG(width),LONG(height)};
}

}

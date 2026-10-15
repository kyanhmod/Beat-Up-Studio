#pragma once
#include <windows.h>
#include <string>
#include "AudioEngine.h"
namespace BeatUp {
class AudioMixer {
public:
    void Load(AudioEngine* audio,const std::wstring& path);
    void Show(HWND owner);
    void Persist();
private:
    HWND window_=nullptr,music_=nullptr,effects_=nullptr,musicLabel_=nullptr,effectLabel_=nullptr;
    AudioEngine* audio_=nullptr;
    std::wstring path_;
    void Apply();
    static LRESULT CALLBACK Proc(HWND,UINT,WPARAM,LPARAM);
};
}

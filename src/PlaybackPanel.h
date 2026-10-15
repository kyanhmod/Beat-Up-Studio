#pragma once
#include <windows.h>
#include <functional>
#include "BeatUpGame.h"
namespace BeatUp {
class PlaybackPanel {
public:
    void Init(HWND owner,BeatUpGame* game,AudioEngine* audio,std::function<void(int)> action);
    void Show();
    void Toggle();
    void ShowSettings();
    bool SaveSettings();
    bool SettingsVisible() const {return settingsVisible_;}
    bool Visible() const {return window_ && IsWindowVisible(window_);}
    HWND Handle() const {return window_;}
    void Embed();
    void Layout();
    int Height() const {return 64;}
    void SetMode(int mode);
    void SetVisible(bool visible);
    void Ready(bool chartLoaded,const std::wstring& chartPath=L"",bool resetTiming=true);
    bool ApplyTiming();
    bool Play();
    void Pause();
    void Restart();
    void Seek(double timeMs);
    void Tick(double deltaMs);
    bool Playing() const {return playing_;}
    double Position() const {return playing_?audio_->GetTimeMs():position_;}
    bool HasChart() const {return hasChart_;}
    bool Started() const {return started_;}
    bool Accelerator(const MSG& msg);
private:
    HWND window_=nullptr,owner_=nullptr,bpm_=nullptr,offset_=nullptr,slider_=nullptr,status_=nullptr;
    HWND musicVolume_=nullptr,effectVolume_=nullptr;
    std::wstring chartName_;
    BeatUpGame* game_=nullptr;AudioEngine* audio_=nullptr;
    std::function<void(int)> action_;
    bool hasChart_=false,playing_=false,paused_=false,scrubbing_=false,resumeScrub_=false;
    double position_=0;
    bool settingsVisible_=false,timingEdited_=false;
    int mode_=1030;
    bool started_=false;
    DWORD lastPaint_=0;
    double fpsTime_=0;int fpsFrames_=0,fps_=0;
    void Refresh();
    static LRESULT CALLBACK Proc(HWND,UINT,WPARAM,LPARAM);
};
}

#include "AudioMixer.h"
#include <commctrl.h>
#include <algorithm>
#include "StudioTheme.h"
namespace BeatUp {
void AudioMixer::Load(AudioEngine* audio,const std::wstring& path) {
    audio_=audio;path_=path;
    audio_->SetMusicVolume((std::min)(100u,GetPrivateProfileIntW(L"Audio",L"Music",100,path.c_str()))/100.0f);
    audio_->SetEffectVolume((std::min)(100u,GetPrivateProfileIntW(L"Audio",L"Effects",100,path.c_str()))/100.0f);
}
void AudioMixer::Apply() {
    int music=static_cast<int>(SendMessageW(music_,TBM_GETPOS,0,0));
    int effects=static_cast<int>(SendMessageW(effects_,TBM_GETPOS,0,0));
    audio_->SetMusicVolume(music/100.0f);audio_->SetEffectVolume(effects/100.0f);
    auto a=L"Music: "+std::to_wstring(music)+L"%",b=L"Effects: "+std::to_wstring(effects)+L"%";
    SetWindowTextW(musicLabel_,a.c_str());SetWindowTextW(effectLabel_,b.c_str());
    Persist();
}
void AudioMixer::Persist() {
    if(!audio_ || path_.empty()) return;
    WritePrivateProfileStringW(L"Audio",L"Music",std::to_wstring(static_cast<int>(audio_->GetMusicVolume()*100+.5f)).c_str(),path_.c_str());
    WritePrivateProfileStringW(L"Audio",L"Effects",std::to_wstring(static_cast<int>(audio_->GetEffectVolume()*100+.5f)).c_str(),path_.c_str());
    if(window_) {
        int music=static_cast<int>(audio_->GetMusicVolume()*100+.5f),effects=static_cast<int>(audio_->GetEffectVolume()*100+.5f);
        Theme::Apply(window_);
    SendMessageW(music_,TBM_SETPOS,TRUE,music);SendMessageW(effects_,TBM_SETPOS,TRUE,effects);
        SetWindowTextW(musicLabel_,(L"Music: "+std::to_wstring(music)+L"%").c_str());
        SetWindowTextW(effectLabel_,(L"Effects: "+std::to_wstring(effects)+L"%").c_str());
    }
}
void AudioMixer::Show(HWND owner) {
    if(!audio_) return;
    if(!window_) {
        INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_BAR_CLASSES};InitCommonControlsEx(&controls);
        WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=Proc;
        wc.lpszClassName=L"BeatUpAudioMixer";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=Theme::Brush();
        RegisterClassW(&wc);
        window_=CreateWindowW(wc.lpszClassName,L"Music & effect volume",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
            CW_USEDEFAULT,CW_USEDEFAULT,460,240,owner,nullptr,wc.hInstance,this);
        musicLabel_=CreateWindowW(L"STATIC",L"Music",WS_CHILD|WS_VISIBLE,20,20,400,23,window_,nullptr,wc.hInstance,nullptr);
        music_=CreateWindowW(TRACKBAR_CLASSW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBS_AUTOTICKS,18,48,400,36,window_,reinterpret_cast<HMENU>(1),wc.hInstance,nullptr);
        effectLabel_=CreateWindowW(L"STATIC",L"Effects",WS_CHILD|WS_VISIBLE,20,98,400,23,window_,nullptr,wc.hInstance,nullptr);
        effects_=CreateWindowW(TRACKBAR_CLASSW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBS_AUTOTICKS,18,126,400,36,window_,reinterpret_cast<HMENU>(2),wc.hInstance,nullptr);
        for(auto control:{music_,effects_}) {SendMessageW(control,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));SendMessageW(control,TBM_SETTICFREQ,10,0);}
    }
    SendMessageW(music_,TBM_SETPOS,TRUE,static_cast<int>(audio_->GetMusicVolume()*100+.5f));
    SendMessageW(effects_,TBM_SETPOS,TRUE,static_cast<int>(audio_->GetEffectVolume()*100+.5f));
    Apply();ShowWindow(window_,SW_SHOW);SetForegroundWindow(window_);
}
LRESULT CALLBACK AudioMixer::Proc(HWND window,UINT msg,WPARAM w,LPARAM l) {
    auto self=reinterpret_cast<AudioMixer*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if(msg==WM_NCCREATE) {self=static_cast<AudioMixer*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);self->window_=window;SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    if(msg==WM_CTLCOLORSTATIC || msg==WM_CTLCOLORBTN) return Theme::Color(w);
    if(self && msg==WM_HSCROLL) {self->Apply();return 0;}
    if(msg==WM_CLOSE) {ShowWindow(window,SW_HIDE);return 0;}
    return DefWindowProcW(window,msg,w,l);
}
}

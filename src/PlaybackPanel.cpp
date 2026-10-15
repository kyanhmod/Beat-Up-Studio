#include "PlaybackPanel.h"
#include <commctrl.h>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include "StudioTheme.h"
namespace BeatUp {
namespace {
void Label(HWND parent,const wchar_t* text,int x,int y,int width) {CreateWindowW(L"STATIC",text,WS_CHILD|WS_VISIBLE,x,y,width,23,parent,nullptr,GetModuleHandleW(nullptr),nullptr);}
}
void PlaybackPanel::Init(HWND owner,BeatUpGame* game,AudioEngine* audio,std::function<void(int)> action) {
    owner_=owner;game_=game;audio_=audio;action_=std::move(action);
    game_->SetTransportControlled(true);
    INITCOMMONCONTROLSEX init{sizeof(init),ICC_BAR_CLASSES};InitCommonControlsEx(&init);
    WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=Proc;wc.lpszClassName=L"BeatUpPlaybackPanel";
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=Theme::Brush();RegisterClassW(&wc);
    window_=CreateWindowExW(WS_EX_TOOLWINDOW,wc.lpszClassName,L"Beat-Up Transport",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
        CW_USEDEFAULT,CW_USEDEFAULT,730,125,owner,nullptr,wc.hInstance,this);
    auto button=[&](const wchar_t* name,int x,int y,int width,int height,int id) {
        auto h=CreateWindowW(L"BUTTON",name,WS_CHILD|WS_VISIBLE|WS_TABSTOP,x,y,width,height,window_,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),wc.hInstance,nullptr);
        SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
    };
    button(L"Play",8,8,68,32,1010);button(L"Restart",80,8,68,32,1005);
    button(L"Settings",152,8,68,32,1015);
    button(L"|<",224,8,30,32,1016);button(L"<",258,8,30,32,1017);
    button(L">",566,8,30,32,1018);button(L">|",600,8,30,32,1019);
    button(L"Save",314,54,86,32,1013);
    auto label=[&](const wchar_t* name,int x,int y,int width,int id) {CreateWindowW(L"STATIC",name,WS_CHILD,x,y,width,24,window_,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),wc.hInstance,nullptr);};
    label(L"Main BPM",12,24,110,2010);label(L"Offset (tick)",12,63,110,2011);
    bpm_=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"150",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,140,18,130,30,window_,reinterpret_cast<HMENU>(2001),wc.hInstance,nullptr);
    offset_=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"0.7",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,140,57,130,30,window_,reinterpret_cast<HMENU>(2002),wc.hInstance,nullptr);
    slider_=CreateWindowW(TRACKBAR_CLASSW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP,292,8,270,32,window_,reinterpret_cast<HMENU>(2003),wc.hInstance,nullptr);
    SendMessageW(slider_,TBM_SETRANGEMAX,FALSE,100000);
    musicVolume_=CreateWindowW(TRACKBAR_CLASSW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBS_VERT|TBS_REVERSED,640,2,28,48,window_,reinterpret_cast<HMENU>(2004),wc.hInstance,nullptr);
    effectVolume_=CreateWindowW(TRACKBAR_CLASSW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBS_VERT|TBS_REVERSED,674,2,28,48,window_,reinterpret_cast<HMENU>(2005),wc.hInstance,nullptr);
    label(L"Music  FX",639,52,75,2012);ShowWindow(GetDlgItem(window_,2012),SW_SHOW);
    for(auto slider:{musicVolume_,effectVolume_}) SendMessageW(slider,TBM_SETRANGEMAX,FALSE,100);
    status_=CreateWindowW(L"STATIC",L"No music loaded",WS_CHILD|WS_VISIBLE,8,50,620,40,window_,nullptr,wc.hInstance,nullptr);
    label(L"Effects",350,12,44,2014);ShowWindow(GetDlgItem(window_,2014),SW_SHOW);
    label(L"Enter BPM and native tick offset, then Save.",12,52,600,2016);
    SetWindowLongPtrW(status_,GWL_STYLE,GetWindowLongPtrW(status_,GWL_STYLE)|SS_ENDELLIPSIS);
    ShowWindow(GetDlgItem(window_,1013),SW_HIDE);
    auto tips=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,window_,nullptr,wc.hInstance,nullptr);
    for(auto tip:{std::pair<int,const wchar_t*>{1010,L"Play / Pause (F9)"},{1005,L"Restart (R)"},{1015,L"BPM and offset"},{1016,L"Previous Space"},{1017,L"Previous beat"},{1018,L"Next beat"},{1019,L"Next Space"},{2004,L"Music volume"},{2005,L"Effect volume"}}) {
        TOOLINFOW info{sizeof(info)};info.uFlags=TTF_IDISHWND|TTF_SUBCLASS;info.hwnd=window_;info.uId=reinterpret_cast<UINT_PTR>(GetDlgItem(window_,tip.first));info.lpszText=const_cast<wchar_t*>(tip.second);SendMessageW(tips,TTM_ADDTOOLW,0,reinterpret_cast<LPARAM>(&info));
    }
    Theme::Apply(window_);Ready(false);Show();
}
void PlaybackPanel::Show() {ShowWindow(window_,SW_SHOW);SetForegroundWindow(owner_?owner_:window_);SetFocus(window_);}
void PlaybackPanel::Embed() {
    if(!owner_ || !window_) return;
    SetWindowLongPtrW(window_,GWL_STYLE,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN);
    SetWindowLongPtrW(window_,GWL_EXSTYLE,0);SetParent(window_,owner_);
    SetWindowPos(window_,nullptr,0,0,0,0,SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
    Layout();
}
void PlaybackPanel::Layout() {
    if(!window_ || !(GetWindowLongPtrW(window_,GWL_STYLE)&WS_CHILD)) return;
    RECT r{};GetClientRect(owner_,&r);int width=(std::min)(560,static_cast<int>(r.right)),height=Height();
    MoveWindow(window_,r.right-width,0,width,height,TRUE);
    auto move=[&](int id,int x,int y,int w,int h){auto c=GetDlgItem(window_,id);if(c) MoveWindow(c,x,y,w,h,TRUE);};
    move(1010,8,8,34,34);move(1005,48,8,34,34);move(1015,88,8,34,34);
    int seekWidth=(std::max)(32,width-320),next=184+seekWidth;
    move(1016,132,12,20,26);move(1017,156,12,20,26);move(2003,180,12,seekWidth,28);
    move(1018,next,12,20,26);move(1019,next+24,12,20,26);
    for(auto control:{musicVolume_,effectVolume_}) SetWindowLongPtrW(control,GWL_STYLE,GetWindowLongPtrW(control,GWL_STYLE)&~(TBS_VERT|TBS_REVERSED));
    move(2012,width-84,13,12,20);SetWindowTextW(GetDlgItem(window_,2012),L"M");
    move(2004,width-72,8,64,26);move(2014,width-86,39,18,20);SetWindowTextW(GetDlgItem(window_,2014),L"FX");move(2005,width-72,34,64,26);
    MoveWindow(status_,110,42,(std::max)(1,width-206),20,TRUE);
    move(2010,12,16,42,24);SetWindowTextW(GetDlgItem(window_,2010),L"BPM");move(2001,60,10,100,30);
    move(2011,186,16,102,24);move(2002,288,10,100,30);move(1013,414,10,100,30);move(2016,12,46,width-24,18);
    if(width<530) {move(2001,54,10,80,30);move(2011,146,16,92,24);move(2002,238,10,90,30);move(1013,width-78,10,68,30);}
    SetWindowPos(window_,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
}
void PlaybackPanel::SetMode(int mode) {
    if(mode_==mode) return;mode_=mode;

}
void PlaybackPanel::SetVisible(bool visible) {if(window_) ShowWindow(window_,visible?SW_SHOW:SW_HIDE);if(action_) action_(1041);}
void PlaybackPanel::Toggle() {SetVisible(!Visible());if(Visible()) {Layout();SetFocus(owner_);}}
void PlaybackPanel::ShowSettings() {
    Pause();settingsVisible_=true;
    for(int id:{1010,1005,1015,1016,1017,1018,1019,2003,2004,2005,2012,2014}) ShowWindow(GetDlgItem(window_,id),SW_HIDE);
    ShowWindow(status_,SW_HIDE);
    for(int id:{2001,2002,2010,2011,1013,2016}) ShowWindow(GetDlgItem(window_,id),SW_SHOW);
    SetVisible(true);Layout();SetFocus(bpm_);
}
bool PlaybackPanel::SaveSettings() {
    if(!ApplyTiming()) return false;
    settingsVisible_=false;
    for(int id:{2001,2002,2010,2011,1013,2016}) ShowWindow(GetDlgItem(window_,id),SW_HIDE);
    for(int id:{1010,1005,1015,1016,1017,1018,1019,2003,2004,2005,2012,2014}) ShowWindow(GetDlgItem(window_,id),SW_SHOW);
    ShowWindow(status_,SW_SHOW);SetVisible(true);Layout();Refresh();SetFocus(owner_);action_(1013);action_(1041);return true;
}
void PlaybackPanel::Ready(bool chartLoaded,const std::wstring& chartPath,bool resetTiming) {
    chartName_=std::filesystem::path(chartPath).filename().wstring();
    hasChart_=chartLoaded;playing_=paused_=started_=false;position_=0;audio_->SeekMusic(0,false);
    std::wostringstream bpm,offset;bpm<<std::setprecision(12)<<game_->GetBpm();offset<<std::setprecision(12)<<game_->GetNativeOffset();
    if(resetTiming) {SetWindowTextW(bpm_,bpm.str().c_str());SetWindowTextW(offset_,offset.str().c_str());timingEdited_=false;}Refresh();
}
bool PlaybackPanel::ApplyTiming() {
    if(!timingEdited_) return true;
    wchar_t b[80]{},o[80]{};GetWindowTextW(bpm_,b,80);GetWindowTextW(offset_,o,80);wchar_t *be=nullptr,*oe=nullptr;
    double bpm=wcstod(b,&be),offset=wcstod(o,&oe);
    if(be==b || oe==o || *be || *oe || !std::isfinite(bpm) || !std::isfinite(offset) || bpm<=30 || bpm>=400) {MessageBoxW(window_,L"Enter a valid BPM (30-400) and numeric offset in ticks.",L"Timing",MB_OK);return false;}
    if(bpm!=game_->GetBpm() || offset!=game_->GetNativeOffset()) {game_->ConfigureTiming(bpm,offset);game_->Seek(position_);audio_->SeekMusic(position_,false);paused_=playing_=false;}
    timingEdited_=false;return true;
}
bool PlaybackPanel::Play() {
    if(!audio_->IsMusicLoaded()) {MessageBoxW(window_,L"Choose an OGG/WAV music file first.",L"Play",MB_OK);return false;}
    if(settingsVisible_) {SetFocus(bpm_);return false;}
    if(!ApplyTiming()) return false;
    if(playing_) return true;
    if(paused_) audio_->ResumeMusic();
    else {game_->Seek(position_);if(position_==0) audio_->PlayMusic();else audio_->SeekMusic(position_,true);}
    playing_=started_=true;paused_=false;Refresh();SetForegroundWindow(owner_);SetFocus(owner_);return true;
}
void PlaybackPanel::Pause() {
    if(playing_) {audio_->PauseMusic();position_=audio_->GetTimeMs();playing_=false;paused_=true;game_->ReleaseKeys();}
    Refresh();
}
void PlaybackPanel::Restart() {if(!audio_->IsMusicLoaded()) return;Pause();game_->Restart();Seek(0);paused_=false;Play();}
void PlaybackPanel::Seek(double timeMs) {
    if(!audio_->IsMusicLoaded() && !hasChart_) return;
    bool running=playing_;position_=std::clamp(timeMs,0.0,(std::max)(0.0,game_->GetDurationMs()-1));
    game_->Seek(position_);audio_->SeekMusic(position_,running);playing_=running;paused_=!running;Refresh();
}
void PlaybackPanel::Tick(double deltaMs) {
    fpsTime_+=deltaMs;++fpsFrames_;if(fpsTime_>=500) {fps_=static_cast<int>(fpsFrames_*1000/fpsTime_);fpsTime_=0;fpsFrames_=0;}
    if(playing_) {position_=audio_->GetTimeMs();game_->Update(position_,deltaMs);if(position_>=game_->GetDurationMs()-.1) Pause();}
    if(GetTickCount()-lastPaint_>60) {Refresh();lastPaint_=GetTickCount();}
}
void PlaybackPanel::Refresh() {
    if(!window_) return;
    double duration=game_->GetDurationMs();
    SetWindowTextW(GetDlgItem(window_,1010),playing_?L"Pause":paused_?L"Resume":L"Play");
    EnableWindow(GetDlgItem(window_,1010),audio_->IsMusicLoaded());
    EnableWindow(GetDlgItem(window_,1005),audio_->IsMusicLoaded());
    EnableWindow(GetDlgItem(window_,1011),playing_);EnableWindow(GetDlgItem(window_,1004),hasChart_);EnableWindow(slider_,audio_->IsMusicLoaded() || hasChart_);
    if(!scrubbing_) SendMessageW(slider_,TBM_SETPOS,TRUE,duration>0?static_cast<LPARAM>(Position()/duration*100000):0);
    SendMessageW(musicVolume_,TBM_SETPOS,TRUE,static_cast<int>(audio_->GetMusicVolume()*100+.5f));
    SendMessageW(effectVolume_,TBM_SETPOS,TRUE,static_cast<int>(audio_->GetEffectVolume()*100+.5f));
    std::wostringstream text;text<<static_cast<int>(Position()/60000)<<L":";
    int seconds=static_cast<int>(Position()/1000)%60;if(seconds<10) text<<L"0";text<<seconds<<L" / "<<static_cast<int>(duration/60000)<<L":";
    seconds=static_cast<int>(duration/1000)%60;if(seconds<10) text<<L"0";text<<seconds;
    text<<L"   Combo: "<<game_->GetCombo()<<L"/"<<game_->GetAllCombo()<<L"   FPS: "<<fps_;
    SetWindowTextW(status_,text.str().c_str());
}
bool PlaybackPanel::Accelerator(const MSG& message) {
    if(message.message!=WM_KEYDOWN || (message.hwnd!=window_ && !IsChild(window_,message.hwnd))) return false;
    if((message.wParam==VK_F9 || message.wParam==VK_PAUSE || message.wParam==VK_SPACE) && (message.lParam&(1LL<<30))) return true;
    if(message.wParam==VK_F6) {action_(1004);return true;}
    if(message.wParam==VK_F9 || message.wParam==VK_PAUSE) {action_(1014);return true;}
    if(message.wParam==VK_SPACE && message.hwnd!=bpm_ && message.hwnd!=offset_) {action_(1014);return true;}
    return false;
}
LRESULT CALLBACK PlaybackPanel::Proc(HWND window,UINT msg,WPARAM w,LPARAM l) {
    auto self=reinterpret_cast<PlaybackPanel*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if(msg==WM_NCCREATE) {self=static_cast<PlaybackPanel*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);self->window_=window;SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    if(msg==WM_DRAWITEM && Theme::Draw(l,self?self->mode_:0)) return TRUE;
    if(msg==WM_CTLCOLORSTATIC || msg==WM_CTLCOLOREDIT || msg==WM_CTLCOLORBTN) return Theme::Color(w);
    if(self && msg==WM_COMMAND) {
        int id=LOWORD(w);
        if((id==2001 || id==2002) && HIWORD(w)==EN_CHANGE) {self->timingEdited_=true;return 0;}
        if(id==1010) {if(self->mode_==1032) self->action_(1014);else if(self->playing_) self->Pause();else self->Play();}else if(id==1011) self->Pause();else if(id==1005) self->Restart();else if(id==1013) self->SaveSettings();
        else if(id==1015) self->ShowSettings();
        else if(id>=1016 && id<=1019) {
            double next=self->Position(),beat=60000.0/self->game_->GetBpm();
            if(id==1017 || id==1018) next+=(id==1017?-beat:beat);
            else {
                next=id==1016?0:self->game_->GetDurationMs()-1;
                for(const auto& n:self->game_->GetNotes()) if(n.lane==5) {
                    if(id==1016 && n.timeMs<self->Position()-.5) next=(std::max)(next,n.timeMs);
                    if(id==1019 && n.timeMs>self->Position()+.5) next=(std::min)(next,n.timeMs);
                }
            }
            self->Seek(next);
        } else if(id<2000) self->action_(id);return 0;
    }
    if(self && (msg==WM_HSCROLL || msg==WM_VSCROLL) && (reinterpret_cast<HWND>(l)==self->musicVolume_ || reinterpret_cast<HWND>(l)==self->effectVolume_)) {
        auto control=reinterpret_cast<HWND>(l);float volume=SendMessageW(control,TBM_GETPOS,0,0)/100.0f;
        if(control==self->musicVolume_) self->audio_->SetMusicVolume(volume);else self->audio_->SetEffectVolume(volume);
        self->action_(1007);return 0;
    }
    if(self && msg==WM_HSCROLL && reinterpret_cast<HWND>(l)==self->slider_) {
        int kind=LOWORD(w);
        if(kind==TB_THUMBTRACK && !self->scrubbing_) {self->resumeScrub_=self->playing_;self->scrubbing_=true;self->Pause();}
        if(kind!=TB_ENDTRACK) self->Seek(SendMessageW(self->slider_,TBM_GETPOS,0,0)/100000.0*self->game_->GetDurationMs());
        if(kind==TB_ENDTRACK && self->scrubbing_) {self->scrubbing_=false;if(self->resumeScrub_) self->Play();}return 0;
    }
    if(msg==WM_CLOSE) {ShowWindow(window,SW_HIDE);return 0;}
    return DefWindowProcW(window,msg,w,l);
}
}


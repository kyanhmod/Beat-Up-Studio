#include "ChartEditor.h"
#include <commdlg.h>
#include <windowsx.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <set>
#include "KeyBindings.h"
#include "StudioTheme.h"
#include "EditorSprites.h"

namespace BeatUp {
namespace {
constexpr int Left=112;
int GridTop(HWND window) {RECT r{};GetClientRect(window,&r);return r.right<1010?152:112;}
constexpr int Lanes[]={9,6,3,7,4,1,5,0};
constexpr const wchar_t* Symbols[]={L"↗",L"→",L"↘",L"↖",L"←",L"↙",L"SPACE",L"F"};
void Text(HDC dc,int x,int y,const std::wstring& s,COLORREF color=RGB(225,229,239)) {
    SetTextColor(dc,color); TextOutW(dc,x,y,s.c_str(),static_cast<int>(s.size()));
}
void Box(HDC dc,RECT rect,COLORREF color) {
    auto brush=CreateSolidBrush(color); FillRect(dc,&rect,brush); DeleteObject(brush);
}
std::string Utf8(const std::wstring& text) {
    int size=WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
    std::string out(size,0);
    WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),out.data(),size,nullptr,nullptr); return out;
}
}
int ChartEditor::VisibleRows() const {
    RECT rect{}; GetClientRect(window_,&rect); return (std::max)(1,static_cast<int>((rect.right-Left+zoom_-1)/zoom_));
}
void ChartEditor::Refresh() {
    if(!window_) return;
    auto title=L"Beat-Up Editor — "+std::filesystem::path(source_).filename().wstring()+((document_.Dirty()||metadataDirty_)?L" *":L"");
    SetWindowTextW(window_,title.c_str());
    InvalidateRect(window_,nullptr,FALSE);
    SCROLLINFO scroll{sizeof(scroll),SIF_RANGE|SIF_PAGE|SIF_POS};
    scroll.nMin=0;scroll.nMax=(std::max)(document_.LastTick()+64,top_+VisibleRows()+16);
    scroll.nPage=VisibleRows();scroll.nPos=top_;ShowScrollBar(window_,SB_HORZ,FALSE);
}
bool ChartEditor::Open(const std::wstring& path) {
    if(!document_.Load(path)) {
        MessageBoxW(window_,L"Cannot open this chart for editing. This editor currently supports native Audition Location/Type/Key SLK charts.",L"Open chart",MB_OK|MB_ICONWARNING); return false;
    }
    loaded_=true;source_=std::filesystem::absolute(path).wstring();
    auto out=std::filesystem::path(source_);
    if(out.stem().wstring().size()>=7 && out.stem().wstring().substr(out.stem().wstring().size()-7)==L".edited") output_=source_;
    else {out.replace_extension(L".edited.slk"); output_=out.wstring();}
    outputOwned_=output_==source_;
    info_=ReadSongInfo(source_,150);metadataDirty_=false; SyncTiming();cursor_=anchor_=0;
    auto events=document_.Events();
    if(!events.empty()) {cursor_=anchor_=events.front().tick; for(auto e:events) cursor_=(std::min)(cursor_,e.tick); anchor_=cursor_;}
    top_=(std::max)(0,cursor_-4); status_=L"Ready. Original chart is preserved; Ctrl+S writes an .edited.slk copy.";
    Refresh(); return true;
}
bool ChartEditor::Save() {
    if(output_.empty()) {
        Command(119);return !output_.empty() && outputOwned_ && !document_.Dirty() && !metadataDirty_;
    }
    if(!ApplyTiming()) return false;
    if(!outputOwned_ && std::filesystem::exists(output_) &&
       MessageBoxW(window_,L"An edited copy already exists. Replace it with these changes?",L"Save edited copy",MB_YESNO|MB_ICONQUESTION)!=IDYES) return false;
    // Package metadata/audio reference before committing the chart. Playback uses
    // the same explicit native phase conversion as the source song.
    auto ini=std::filesystem::path(output_); ini.replace_extension(L".song.ini");
    auto iniTemp=ini; iniTemp+=L".tmp";
    {
        std::ofstream file(iniTemp,std::ios::binary|std::ios::trunc);
        if(!file) {status_=L"Cannot write song information."; Refresh(); return false;}
        file<<std::setprecision(15)<<"[song]\ntitle="<<Utf8(info_.title)<<"\nartist="<<Utf8(info_.artist)
            <<"\nbpm="<<info_.bpm<<"\noffset_ms="<<info_.offsetMs<<'\n';
        if(info_.hasAuditionOffset) file<<"audition_offset="<<info_.auditionOffset<<'\n';
        auto audio=info_.audioPath;
        if(audio.empty()) {
            auto candidate=std::filesystem::path(source_);candidate.replace_extension(L".ogg");
            if(!std::filesystem::exists(candidate)) candidate.replace_extension(L".wav");
            if(std::filesystem::exists(candidate)) audio=candidate.wstring();
        }
        if(!audio.empty()) file<<"audio="<<Utf8(std::filesystem::absolute(audio).wstring())<<'\n';
        file.flush(); if(!file) {status_=L"Cannot write song information."; Refresh(); return false;}
    }
    if(!MoveFileExW(iniTemp.c_str(),ini.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH) || !document_.Save(output_)) {
        status_=L"Save failed. Check the output folder and file permissions."; Refresh(); return false;
    }
    outputOwned_=true;
    metadataDirty_=false;
    if(savedCallback_) savedCallback_(output_);
    status_=L"Saved: "+std::filesystem::path(output_).filename().wstring(); Refresh(); return true;
}
bool ChartEditor::ConfirmClose() {
    if(!document_.Dirty() && !metadataDirty_) return true;
    int answer=MessageBoxW(window_,L"Save your chart changes to the edited copy?",L"Unsaved changes",MB_YESNOCANCEL|MB_ICONQUESTION);
    if(answer==IDNO) {
        auto path=document_.Path();
        if(path.empty()) document_.New();else if(!document_.Load(path)) return false;
        if(!path.empty()) info_=ReadSongInfo(path,150);metadataDirty_=false;SyncTiming();return true;
    }
    return answer==IDYES && Save();
}
void ChartEditor::Show(HWND owner,const std::wstring& chart,std::function<bool(const std::wstring&,double)> preview,std::function<void()> mixer,const std::wstring& sessionAudio,std::function<void(int)> transport,double sessionBpm,double sessionOffset) {
    owner_=owner; preview_=std::move(preview);mixer_=std::move(mixer);transport_=std::move(transport);
    if(Visible()) {SetForegroundWindow(window_);return;}
    if(!window_) {
        WNDCLASSW wc{}; wc.hInstance=GetModuleHandleW(nullptr); wc.lpfnWndProc=Proc;
        wc.lpszClassName=L"BeatUpChartEditor"; wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
        RegisterClassW(&wc);
        window_=CreateWindowW(wc.lpszClassName,L"Beat-Up Editor",(embedded_?WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS:WS_OVERLAPPEDWINDOW),
            CW_USEDEFAULT,CW_USEDEFAULT,1240,830,owner,nullptr,wc.hInstance,this);
        auto button=[&](const wchar_t* text,int x,int width,int id) {
            auto h=CreateWindowW(L"BUTTON",text,WS_CHILD|WS_VISIBLE|WS_TABSTOP, x,10,width,30,window_,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),wc.hInstance,nullptr);
            SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
        };
        button(L"Open SLK",12,90,100);button(L"Choose music",108,102,104);
        button(L"Save copy",216,90,101);button(L"Play [F5]",312,90,102);button(L"Volume",408,80,103);
        button(L"Undo",508,65,105);button(L"Redo",577,65,106);button(L"Copy",650,65,107);button(L"Paste",719,65,108);
        button(L"Cut",788,65,109);button(L"Delete",857,65,110);button(L"Finish",930,72,111);
        button(L"Save as…",1008,85,119);
        auto makeEdit=[&](int x,int id) {
            return CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,
                x,50,74,25,window_,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),wc.hInstance,nullptr);
        };
        bpmEdit_=makeEdit(58,201);offsetEdit_=makeEdit(218,202);
        auto apply=CreateWindowW(L"BUTTON",L"Apply timing",WS_CHILD|WS_VISIBLE|WS_TABSTOP,305,48,106,29,window_,reinterpret_cast<HMENU>(112),wc.hInstance,nullptr);
        SendMessageW(apply,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
        for(auto item:{std::pair<int,const wchar_t*>{121,L"Pause / F9"},{122,L"Resume"},{123,L"Restart"},{124,L"Transport"}}) {
            auto h=CreateWindowW(L"BUTTON",item.second,WS_CHILD|WS_VISIBLE|WS_TABSTOP,690+(item.first-121)*104,48,98,29,window_,reinterpret_cast<HMENU>(static_cast<INT_PTR>(item.first)),wc.hInstance,nullptr);
            SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
        }
        auto tools=CreateMenu(),edit=CreatePopupMenu(),file=CreatePopupMenu();
        AppendMenuW(file,MF_STRING,100,L"Open SLK…\tCtrl+O");AppendMenuW(file,MF_STRING,104,L"Choose music…");
        AppendMenuW(file,MF_STRING,101,L"Save edited copy\tCtrl+S");AppendMenuW(file,MF_STRING,119,L"Save as…");
        AppendMenuW(edit,MF_STRING,113,L"Mirror left/right (selection)");
        AppendMenuW(edit,MF_STRING,114,L"Shift selected events earlier one tick");
        AppendMenuW(edit,MF_STRING,115,L"Shift selected events later one tick");
        AppendMenuW(edit,MF_STRING,116,L"Randomize selected directions");
        AppendMenuW(edit,MF_STRING,117,L"Generate Space every 16 ticks (selection)");
        AppendMenuW(edit,MF_STRING,118,L"Validate chart");
        AppendMenuW(tools,MF_POPUP,reinterpret_cast<UINT_PTR>(file),L"File");
        AppendMenuW(edit,MF_SEPARATOR,0,nullptr);
        AppendMenuW(edit,MF_STRING,105,L"Undo\tCtrl+Z");AppendMenuW(edit,MF_STRING,106,L"Redo\tCtrl+Y");
        AppendMenuW(edit,MF_STRING,107,L"Copy\tCtrl+C");AppendMenuW(edit,MF_STRING,109,L"Cut\tCtrl+X");AppendMenuW(edit,MF_STRING,108,L"Paste\tCtrl+V");
        AppendMenuW(edit,MF_STRING,110,L"Delete\tDel");AppendMenuW(edit,MF_STRING,111,L"Toggle Finish\tF");
        AppendMenuW(tools,MF_POPUP,reinterpret_cast<UINT_PTR>(edit),L"Edit / Tools");
        auto mode=CreatePopupMenu();AppendMenuW(mode,MF_STRING|MF_CHECKED,1032,L"Editor / Classic\tF6");
        AppendMenuW(mode,MF_STRING,1031,L"Autoplay");AppendMenuW(mode,MF_STRING,1030,L"Game");
        AppendMenuW(tools,MF_POPUP,reinterpret_cast<UINT_PTR>(mode),L"Mode");
        auto player=CreatePopupMenu();AppendMenuW(player,MF_STRING,1012,L"Toggle Menu On / Off\tF10");
        AppendMenuW(player,MF_STRING,1014,L"Play / Pause\tF9");AppendMenuW(player,MF_STRING,1005,L"Restart");
        AppendMenuW(tools,MF_POPUP,reinterpret_cast<UINT_PTR>(player),L"Music Player");
        editorMenu_=tools;if(!embedded_) SetMenu(window_,tools);
    }
    if(!loaded_ && chart.empty()) {
        document_.New();loaded_=true;source_.clear();output_.clear();outputOwned_=false;
        info_={};info_.bpm=sessionBpm>30?sessionBpm:150;info_.auditionOffset=sessionOffset;info_.hasAuditionOffset=true;
        info_.audioPath=sessionAudio;info_.title=sessionAudio.empty()?L"Untitled":std::filesystem::path(sessionAudio).stem().wstring();
        cursor_=anchor_=top_=0;metadataDirty_=false;SyncTiming();
    }
    if(!chart.empty() && (!loaded_ || (!chart.empty() && source_!=std::filesystem::absolute(chart).wstring() && !document_.Dirty() && !metadataDirty_))) {
        if(!Open(chart)) return;
    }
    if(!sessionAudio.empty() && (chart.empty() || source_==std::filesystem::absolute(chart).wstring())) {
        auto current=std::filesystem::path(info_.audioPath);
        if(current.empty()) {
            current=source_;current.replace_extension(L".ogg");
            if(!std::filesystem::exists(current)) current.replace_extension(L".wav");
        }
        if(std::filesystem::absolute(current).lexically_normal()!=std::filesystem::absolute(sessionAudio).lexically_normal()) {
            info_.audioPath=sessionAudio;metadataDirty_=true;status_=L"Using the music selected in the player. Save to retain this choice.";
        }
    }
    if(sessionBpm>30 && sessionBpm<400 && (chart.empty() || source_==std::filesystem::absolute(chart).wstring()) &&
       (info_.bpm!=sessionBpm || info_.NativeTickShift()+1!=sessionOffset)) {
        info_.bpm=sessionBpm;info_.auditionOffset=sessionOffset;info_.hasAuditionOffset=true;
        metadataDirty_=true;SyncTiming();status_=L"Using the BPM and offset entered in the transport. Save to retain these settings.";
    }
    if(embedded_) {
        ownerMenu_=GetMenu(owner_);SetMenu(owner_,editorMenu_);DrawMenuBar(owner_);
        Layout();
    }
    for(int id:{100,104,103,112,121,122,123,124,201,202,119,110,111}) ShowWindow(GetDlgItem(window_,id),SW_HIDE);
    ShowWindow(GetDlgItem(window_,101),embedded_?SW_HIDE:SW_SHOW);
    int x=8;for(int id:{101,105,106,107,109,108,102}) {if(id==101 && embedded_) continue;int width=id==102?78:64;MoveWindow(GetDlgItem(window_,id),x,10,width,30,TRUE);x+=width+6;}
    Theme::Apply(window_);
    ShowWindow(window_,SW_SHOW);SetWindowPos(window_,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
    Layout();SetForegroundWindow(embedded_?owner_:window_);SetFocus(window_);Refresh();
}
bool ChartEditor::HideForPlayback() {
    if(!ApplyTiming()) return false;
    selecting_=false;if(GetCapture()==window_) ReleaseCapture();
    ShowWindow(window_,SW_HIDE);
    if(embedded_) {SetMenu(owner_,ownerMenu_);DrawMenuBar(owner_);}
    SetForegroundWindow(owner_);SetFocus(owner_);return true;
}
int ChartEditor::LaneHeight() const {RECT r{};GetClientRect(window_,&r);return (std::max)(8,static_cast<int>((r.bottom-GridTop(window_)-145)/8));}
void ChartEditor::SetTiming(double bpm,double offset) {
    info_.bpm=bpm;info_.auditionOffset=offset;info_.hasAuditionOffset=true;metadataDirty_=true;SyncTiming();Refresh();
}
void ChartEditor::Layout() {
    if(!embedded_ || !window_) return;
    RECT r=bounds_;if(r.right<=0) {GetClientRect(owner_,&r);r.top=52;r.bottom-=96;}
    MoveWindow(window_,r.left,r.top,r.right-r.left,(std::max)(1L,r.bottom-r.top),TRUE);
    RECT client{};GetClientRect(window_,&client);int x=8,y=client.right<1010?74:10;
    for(int id:{101,105,106,107,109,108,102}) {if(id==101 && embedded_) continue;int width=id==102?78:64;MoveWindow(GetDlgItem(window_,id),x,y,width,30,TRUE);x+=width+6;}

}
void ChartEditor::Navigate(int tick,bool extend) {
    cursor_=(std::max)(0,(std::min)(10000000,tick));
    if(!extend) anchor_=cursor_;
    if(cursor_<top_) top_=cursor_;
    if(cursor_>=top_+VisibleRows()) top_=cursor_-VisibleRows()+1;
    Refresh();
}
void ChartEditor::SyncTiming() {
    syncing_=true;
    std::wostringstream bpm,offset;bpm<<info_.bpm;offset<<(info_.hasAuditionOffset?info_.auditionOffset:1.0);
    if(bpmEdit_) SetWindowTextW(bpmEdit_,bpm.str().c_str());
    if(offsetEdit_) SetWindowTextW(offsetEdit_,offset.str().c_str());
    syncing_=false;
}
bool ChartEditor::ApplyTiming() {
    wchar_t a[80]{},b[80]{};GetWindowTextW(bpmEdit_,a,80);GetWindowTextW(offsetEdit_,b,80);
    wchar_t *endA=nullptr,*endB=nullptr;
    double bpm=wcstod(a,&endA),offset=wcstod(b,&endB);
    if(endA==a || endB==b || *endA || *endB || !std::isfinite(bpm) || !std::isfinite(offset) || bpm<30 || bpm>400) {
        MessageBoxW(window_,L"Enter BPM from 30 to 400 and a numeric offset in ticks (for example 0.7).",L"Song timing",MB_OK|MB_ICONWARNING);return false;
    }
    if(info_.bpm!=bpm || offset!=(info_.hasAuditionOffset?info_.auditionOffset:1.0)) {
        info_.bpm=bpm;info_.auditionOffset=offset;info_.hasAuditionOffset=true;metadataDirty_=true;
    }
    return true;
}
void ChartEditor::Command(int command) {
    if(embedded_ && transport_ && (command==100 || command==104)) {transport_(command==100?1001:1002);return;}
    if((command==100 || command==104) && transport_) transport_(1011);
    int first=(std::min)(anchor_,cursor_),last=(std::max)(anchor_,cursor_);
    if(command==100 && ConfirmClose()) {
        wchar_t file[32768]{};OPENFILENAMEW dialog{sizeof(dialog)};
        dialog.hwndOwner=window_;dialog.lpstrFilter=L"Audition chart (*.slk)\0*.slk\0\0";dialog.lpstrFile=file;dialog.nMaxFile=32768;dialog.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
        if(GetOpenFileNameW(&dialog)) Open(file);
    } else if(command==101) Save();
    else if(command==102) {
        if(livePreview_ && ApplyTiming()) {
            double position=(std::max)(0.0,(cursor_+info_.NativeTickShift())*15000.0/info_.bpm+info_.offsetMs-60000.0/info_.bpm);
            if(livePreview_(Snapshot(),info_,position)) {previewPlaying_=true;status_=L"Previewing current notes. F9 pauses/resumes.";}
        } else if(!livePreview_ && Save()) {
            double position=(std::max)(0.0,(cursor_+info_.NativeTickShift())*15000.0/info_.bpm+info_.offsetMs-60000.0/info_.bpm);
            if(preview_(output_,position)) {previewPlaying_=true;status_=L"Playing from cursor with one beat of lead-in. F9 pauses; transport can seek.";SetForegroundWindow(window_);}
        }
    }
    else if(command==103) {if(mixer_) mixer_();}
    else if(command==104) {
        wchar_t file[32768]{};OPENFILENAMEW dialog{sizeof(dialog)};
        dialog.hwndOwner=window_;dialog.lpstrFilter=L"Music (*.ogg;*.wav)\0*.ogg;*.wav\0\0";dialog.lpstrFile=file;dialog.nMaxFile=32768;dialog.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
        if(GetOpenFileNameW(&dialog)) {info_.audioPath=file;metadataDirty_=true;status_=L"Music: "+std::filesystem::path(file).filename().wstring();}
    } else if(command==105) document_.Undo();
    else if(command==106) document_.Redo();
    else if(command==107) {document_.Copy(first,last);status_=L"Selection copied.";}
    else if(command==108) document_.Paste(cursor_);
    else if(command==109) document_.Cut(first,last);
    else if(command==110) document_.Erase(first,last);
    else if(command==111) document_.ToggleFinish(cursor_);
    else if(command==112) {if(ApplyTiming()) status_=L"Timing applied. Save or Play to use it in gameplay.";}
    else if(command==113) document_.Mirror(first,last);
    else if(command==114 || command==115) {
        int delta=command==114?-1:1;
        if(document_.Shift(first,last,delta)) {cursor_=(std::max)(0,cursor_+delta);anchor_=(std::max)(0,anchor_+delta);}
        else status_=L"Cannot shift events before tick zero.";
    } else if(command==116) document_.Randomize(first,last);
    else if(command==117) document_.GenerateSpaces(first,last);
    else if(command==118) MessageBoxW(window_,document_.Validate().c_str(),L"Chart validation",MB_OK);
    else if(command==119) {
        wchar_t file[32768]{};OPENFILENAMEW dialog{sizeof(dialog)};
        dialog.hwndOwner=window_;dialog.lpstrFilter=L"Audition chart (*.slk)\0*.slk\0\0";dialog.lpstrDefExt=L"slk";dialog.lpstrFile=file;dialog.nMaxFile=32768;dialog.Flags=OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR;
        if(GetSaveFileNameW(&dialog)) {
            auto previous=output_;bool owned=outputOwned_;output_=file;outputOwned_=true;
            if(!Save()) {output_=previous;outputOwned_=owned;}
        }
    }
    if(transport_ && command==121) transport_(1011);
    if(transport_ && command==122) transport_(1010);
    if(transport_ && command==123) transport_(1005);
    if(transport_ && command==124) transport_(1012);
    Refresh();
    if(command!=103 && command!=124 && IsWindowVisible(window_)) SetFocus(window_);
    if(command==122 || command==123) SetForegroundWindow(window_);
}
std::vector<ChartNote> ChartEditor::Snapshot() const {
    std::vector<ChartNote> result;
    for(const auto& e:document_.Events()) {ChartNote n;n.tick=e.tick;n.lane=e.lane;n.finish=e.finish;n.type=e.lane==5?1:e.lane==0?2:0;n.timeMs=(e.tick+info_.NativeTickShift())*15000.0/info_.bpm+info_.offsetMs;result.push_back(n);}
    std::stable_sort(result.begin(),result.end(),[](const ChartNote& a,const ChartNote& b){return a.timeMs<b.timeMs;});return result;
}
int ChartEditor::AllCombo() const {
    auto events=document_.Events();return static_cast<int>(std::count_if(events.begin(),events.end(),[](const EditorEvent& e){return e.lane!=0;}));
}
void ChartEditor::Paint(HDC dc) {
    const int Height=LaneHeight();
    RECT client{};GetClientRect(window_,&client);
    Box(dc,client,RGB(17,20,29)); SetBkMode(dc,TRANSPARENT);
    auto font=CreateFontW(16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    auto old=SelectObject(dc,font);
    std::wostringstream details;details<<info_.title<<L"   ·   "<<AllCombo()<<L" notes   ·   "<<info_.bpm<<L" BPM   ·   Offset "<<info_.NativeTickShift()+1<<L"   ·   Snap 1/"<<16/step_;
    Text(dc,12,GridTop(window_)-38,details.str(),RGB(128,220,247));
    auto pen=CreatePen(PS_SOLID,1,RGB(44,50,67)); auto oldPen=SelectObject(dc,pen);
    int first=(std::min)(anchor_,cursor_),last=(std::max)(anchor_,cursor_);
    int count=VisibleRows();
    const wchar_t* labels[]={L"9   ↗",L"6   →",L"3   ↘",L"7   ↖",L"4   ←",L"1   ↙",L"0/5  SPACE",L"FINISH"};
    for(int lane=0;lane<8;++lane) {
        int y=GridTop(window_)+lane*Height;
        Box(dc,{0,y,Left+count*zoom_,y+Height},lane<3?RGB(22,32,46):lane<6?RGB(28,28,44):lane==6?RGB(42,34,24):RGB(37,28,30));
        Text(dc,12,y+(Height-16)/2,labels[lane],lane>=6?RGB(255,200,111):RGB(207,221,237));
        MoveToEx(dc,0,y,nullptr);LineTo(dc,Left+count*zoom_,y);
    }
    for(int c=0;c<count;++c) {
        int tick=top_+c,x=Left+c*zoom_;
        if(tick>=first && tick<=last) Box(dc,{x,GridTop(window_),x+zoom_,GridTop(window_)+8*Height},RGB(42,70,90));
        if(tick%step_==0) {MoveToEx(dc,x,GridTop(window_),nullptr);LineTo(dc,x,GridTop(window_)+8*Height);}
        if(tick%4==0) {
            Text(dc,x+3,GridTop(window_)-24,std::to_wstring(tick/16)+L":"+std::to_wstring(tick/4%4),tick%16==0?RGB(255,205,108):RGB(153,173,194));
            if(tick%16==0) {auto p=CreatePen(PS_SOLID,2,RGB(100,118,144));auto prev=SelectObject(dc,p);MoveToEx(dc,x,GridTop(window_),nullptr);LineTo(dc,x,GridTop(window_)+8*Height);SelectObject(dc,prev);DeleteObject(p);}
        }
    }
    std::set<int> finishes;
    auto symbolFont=CreateFontW(27,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI Symbol");
    for(const auto& e:document_.Events()) {
        if(e.finish) finishes.insert(e.tick);
        if(e.tick<top_ || e.tick>=top_+count || e.lane==0) continue;
        for(int lane=0;lane<7;++lane) if(Lanes[lane]==e.lane) {
            int x=Left+(e.tick-top_)*zoom_,y=GridTop(window_)+lane*Height;
            static EditorSprites sprites;
            int size=(std::min)(Height-4,(std::max)(32,zoom_));
            if(!sprites.Draw(dc,e.lane,x+(zoom_-size)/2,y+(Height-(e.lane==5?size/2:size))/2,size)) {
                if(lane<6) SelectObject(dc,symbolFont);
                Text(dc,x+(zoom_-23)/2,y+8,lane<6?Symbols[lane]:L"S");SelectObject(dc,font);
            }
        }
    }
    for(int tick:finishes) if(tick>=top_ && tick<top_+count) {
        int x=Left+(tick-top_)*zoom_,y=GridTop(window_)+7*Height;
        Box(dc,{x+2,y+6,x+zoom_-2,y+Height-6},RGB(180,80,63));Text(dc,x+zoom_/2-5,y+(Height-16)/2,L"F");
    }
    if(playhead_>=top_ && playhead_<top_+count) {
        int x=Left+static_cast<int>((playhead_-top_)*zoom_);
        auto p=CreatePen(PS_SOLID,3,RGB(110,255,183));auto prev=SelectObject(dc,p);
        MoveToEx(dc,x,GridTop(window_)-4,nullptr);LineTo(dc,x,GridTop(window_)+8*Height);SelectObject(dc,prev);DeleteObject(p);
        Text(dc,x+5,GridTop(window_)+8*Height+6,previewPlaying_?L"PLAY":L"PAUSE",RGB(110,255,183));
    }
    DeleteObject(symbolFont);
    int overview=GridTop(window_)+8*Height+42;
    Text(dc,12,overview-23,L"Song overview — click to jump",RGB(154,174,195));
    Box(dc,{Left,overview,Left+count*zoom_,overview+32},RGB(28,37,49));
    int total=(std::max)(1,document_.LastTick()+16);
    for(const auto& e:document_.Events()) {int x=Left+static_cast<int>(static_cast<double>(e.tick)/total*count*zoom_);Box(dc,{x,overview+5,x+2,overview+27},e.finish?RGB(242,145,99):RGB(59,128,153));}
    int start=Left+static_cast<int>(static_cast<double>(top_)/total*count*zoom_);
    int end=Left+static_cast<int>(static_cast<double>(top_+count)/total*count*zoom_);
    auto viewPen=CreatePen(PS_SOLID,2,RGB(110,217,238));auto prevPen=SelectObject(dc,viewPen);auto prevBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc,start,overview,(std::min)(Left+count*zoom_,end),overview+32);
    SelectObject(dc,prevBrush);SelectObject(dc,prevPen);DeleteObject(viewPen);
    Text(dc,12,overview+50,L"Selection: "+std::to_wstring(first)+L" – "+std::to_wstring(last)+L" ticks. Tools menu: mirror, shift, randomize, Space generation, validation.");
    Text(dc,12,client.bottom-27,status_,RGB(171,193,211));
    SelectObject(dc,oldPen);DeleteObject(pen);SelectObject(dc,old);DeleteObject(font);
}
LRESULT CALLBACK ChartEditor::Proc(HWND window,UINT msg,WPARAM w,LPARAM l) {
    auto self=reinterpret_cast<ChartEditor*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if(msg==WM_NCCREATE) {
        self=static_cast<ChartEditor*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
        self->window_=window; SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));
    }
    return self?self->Message(msg,w,l):DefWindowProcW(window,msg,w,l);
}
LRESULT ChartEditor::Message(UINT msg,WPARAM w,LPARAM l) {
    const int Height=LaneHeight();
    if(msg==WM_KEYDOWN || msg==WM_KEYUP) w=NormalizeKeypadKey(w,l);
    switch(msg) {
    case WM_DRAWITEM: if(Theme::Draw(l)) return TRUE;break;
    case WM_CTLCOLORSTATIC: return Theme::Color(w);
    case WM_CTLCOLOREDIT: {
        auto dc=reinterpret_cast<HDC>(w);SetTextColor(dc,RGB(235,242,250));SetBkColor(dc,RGB(35,46,63));
        SetDCBrushColor(dc,RGB(35,46,63));return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
    }
    case WM_GETMINMAXINFO: {
        auto bounds=reinterpret_cast<MINMAXINFO*>(l);
        bounds->ptMinTrackSize={1180,770};return 0;
    }
    case WM_PRINTCLIENT: Paint(reinterpret_cast<HDC>(w));return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps; auto dc=BeginPaint(window_,&ps);
        RECT r{};GetClientRect(window_,&r);auto memory=CreateCompatibleDC(dc);
        auto bitmap=CreateCompatibleBitmap(dc,(std::max)(1L,r.right),(std::max)(1L,r.bottom));auto old=SelectObject(memory,bitmap);
        Paint(memory);BitBlt(dc,0,0,r.right,r.bottom,memory,0,0,SRCCOPY);
        SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);EndPaint(window_,&ps);return 0;
    }
    case WM_ERASEBKGND:return 1;
    case WM_COMMAND:
        if(LOWORD(w)>=1000 && embedded_ && owner_) return SendMessageW(owner_,WM_COMMAND,w,l);
        if(LOWORD(w)>=100 && LOWORD(w)<=124) Command(LOWORD(w));
        else if((LOWORD(w)==201 || LOWORD(w)==202) && HIWORD(w)==EN_CHANGE && !syncing_) {metadataDirty_=true;Refresh();}
        return 0;
    case WM_HSCROLL: {
        SCROLLINFO scroll{sizeof(scroll),SIF_ALL};GetScrollInfo(window_,SB_HORZ,&scroll);
        int value=top_;
        switch(LOWORD(w)) {
        case SB_LINELEFT:value-=step_;break;case SB_LINERIGHT:value+=step_;break;
        case SB_PAGELEFT:value-=VisibleRows();break;case SB_PAGERIGHT:value+=VisibleRows();break;
        case SB_THUMBTRACK:case SB_THUMBPOSITION:value=scroll.nTrackPos;break;
        }
        top_=(std::max)(0,value);Refresh();return 0;
    }
    case WM_SIZE:Refresh();return 0;
    case WM_MOUSEWHEEL:
        if(GET_KEYSTATE_WPARAM(w)&MK_CONTROL) zoom_=(std::max)(24,(std::min)(64,zoom_+GET_WHEEL_DELTA_WPARAM(w)/WHEEL_DELTA*8));
        else top_=(std::max)(0,(std::min)(10000000,top_-GET_WHEEL_DELTA_WPARAM(w)/WHEEL_DELTA*4));
        Refresh();return 0;
    case WM_LBUTTONDOWN: {
        int x=GET_X_LPARAM(l),y=GET_Y_LPARAM(l);
        SetFocus(window_);
        int overview=GridTop(window_)+8*Height+42;
        if(y>=overview && y<overview+32 && x>=Left) {
            int tick=static_cast<int>(static_cast<double>(x-Left)/(VisibleRows()*zoom_)*(document_.LastTick()+16));
            Navigate(tick,false);return 0;
        }
        if(y<GridTop(window_) || y>=GridTop(window_)+8*Height || x<Left || x>=Left+VisibleRows()*zoom_) return 0;
        int tick=top_+(x-Left)/zoom_; tick=tick/step_*step_;
        selecting_=true;dragged_=false;extendDrag_=(w&MK_SHIFT)!=0;
        dragStart_={x,y};dragTick_=tick;dragLane_=Lanes[(y-GridTop(window_))/Height];
        SetCapture(window_);Navigate(tick,extendDrag_);return 0;
    }
    case WM_MOUSEMOVE:
        if(selecting_) {
            int x=GET_X_LPARAM(l),y=GET_Y_LPARAM(l);
            if(abs(x-dragStart_.x)>=GetSystemMetrics(SM_CXDRAG) || abs(y-dragStart_.y)>=GetSystemMetrics(SM_CYDRAG)) dragged_=true;
            if(dragged_) {
                if(x<Left) top_=(std::max)(0,top_-step_);
                else if(x>=Left+VisibleRows()*zoom_) top_=(std::min)(10000000,top_+step_);
                int column=std::clamp((x-Left)/zoom_,0,VisibleRows()-1);
                Navigate((top_+column)/step_*step_,true);
            }
        }
        return 0;
    case WM_LBUTTONUP:
        if(selecting_) {
            selecting_=false;ReleaseCapture();
            if(!dragged_ && !extendDrag_) document_.Toggle(dragTick_,dragLane_);
            Refresh();
        }
        return 0;
    case WM_CAPTURECHANGED: selecting_=false;return 0;
    case WM_KEYDOWN: {
        if((w==VK_F5 || w==VK_F9 || w==VK_SPACE) && (l&(1LL<<30))) return 0;
        bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0,shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;
        int first=(std::min)(anchor_,cursor_),last=(std::max)(anchor_,cursor_);
        if(ctrl) {
            if(w=='S') Command(101);else if(w=='Z') Command(105);else if(w=='Y') Command(106);
            else if(w=='C') Command(107);else if(w=='X') Command(109);else if(w=='V') Command(108);else if(w=='O') {if(embedded_ && transport_) transport_(1042);else Command(100);}
            else if(w=='A') {anchor_=0;Navigate(document_.LastTick(),true);}
        } else if(w==VK_UP || w==VK_LEFT) Navigate(cursor_-step_,shift);
        else if(w==VK_DOWN || w==VK_RIGHT) Navigate(cursor_+step_,shift);
        else if(w==VK_PRIOR) Navigate(cursor_-VisibleRows(),shift);
        else if(w==VK_NEXT) Navigate(cursor_+VisibleRows(),shift);
        else if(w==VK_HOME) Navigate(0,shift);
        else if(w==VK_END) Navigate(document_.LastTick(),shift);
        else if(w==VK_DELETE) document_.Erase(first,last);
        else if(w==VK_F5) {
            Command(102);
        } else if(w==VK_F6 || w==VK_ESCAPE) {
            if(embedded_ && owner_) SendMessageW(owner_,WM_KEYDOWN,VK_F6,0);
            else SendMessageW(window_,WM_CLOSE,0,0);return 0;
        }
        else if(w==VK_F10 && transport_) transport_(1012);
        else if(w==VK_F7) step_=step_==1?2:step_==2?4:1;
        else if(w==VK_SPACE) {
            if(transport_) transport_(1014);
            else Command(102);
        }
        else if((w==VK_F9 || w==VK_PAUSE) && transport_) transport_(1014);
        else if(w=='F') document_.ToggleFinish(cursor_);
        else {
            int lane=0;
            if(w==VK_NUMPAD0 || w==VK_NUMPAD5) lane=5;
            else if(w>=VK_NUMPAD0 && w<=VK_NUMPAD9) lane=static_cast<int>(w-VK_NUMPAD0);
            else if(w>='0' && w<='9') lane=w=='0'?5:static_cast<int>(w-'0');
            if(lane) document_.Toggle(cursor_,lane);
        }
        Refresh();return 0;
    }
    case WM_CLOSE:
        if(ConfirmClose()) {
            selecting_=false;if(GetCapture()==window_) ReleaseCapture();
            ShowWindow(window_,SW_HIDE);
            if(embedded_) {SetMenu(owner_,ownerMenu_);DrawMenuBar(owner_);}
            SetForegroundWindow(owner_);SetFocus(owner_);
        }
        return 0;
    }
    return DefWindowProcW(window_,msg,w,l);
}
bool ChartEditor::Accelerator(const MSG& message) {
    if(!Visible() || message.message!=WM_KEYDOWN || message.hwnd==window_ || !IsChild(window_,message.hwnd)) return false;
    auto key=message.wParam;bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0;
    bool edit=message.hwnd==bpmEdit_ || message.hwnd==offsetEdit_;
    if(key==VK_F5 || key==VK_F6 || key==VK_F7 || key==VK_F9 || key==VK_PAUSE || key==VK_F10 || (ctrl && (key=='O' || key=='S')) || !edit) {
        SendMessageW(window_,WM_KEYDOWN,key,message.lParam);return true;
    }
    return false;
}
void ChartEditor::SetPlaybackTime(double ms,bool playing) {
    if(!Visible() || GetTickCount()-playbackPaint_<30) return;
    playbackPaint_=GetTickCount();previewPlaying_=playing;
    if(playing || playhead_>=0) {
        double previous=playhead_;
        playhead_=(ms-info_.offsetMs)*info_.bpm/15000.0-info_.NativeTickShift();
        if((playing || std::abs(previous-playhead_)>.5) && (playhead_<top_ || playhead_>=top_+VisibleRows())) top_=(std::max)(0,static_cast<int>(playhead_)-4);
        Refresh();
    }
}
}

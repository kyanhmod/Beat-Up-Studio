#include "ChartDocument.h"
#include "ChartEditor.h"
#include "SlkParser.h"
#include "SongInfo.h"
#include "AudioMixer.h"
#include <commctrl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cmath>
namespace fs=std::filesystem;
void Check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
std::string Bytes(const fs::path& p) {std::ifstream f(p,std::ios::binary); return {(std::istreambuf_iterator<char>(f)),{}};}
void Key(HWND window,int key,bool ctrl=false) {
    BYTE before[256]{},state[256]{}; GetKeyboardState(before);
    if(ctrl) state[VK_CONTROL]=0x80;
    SetKeyboardState(state);SendMessageW(window,WM_KEYDOWN,key,0);SetKeyboardState(before);
}
void Capture(HWND window,const fs::path& path) {
    RECT rect{}; GetClientRect(window,&rect);
    auto screen=GetDC(window),dc=CreateCompatibleDC(screen);
    auto bitmap=CreateCompatibleBitmap(screen,rect.right,rect.bottom);
    auto old=SelectObject(dc,bitmap);
    SendMessageW(window,WM_PRINTCLIENT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT);
    for(HWND child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)) {
        RECT bounds{};GetWindowRect(child,&bounds);MapWindowPoints(HWND_DESKTOP,window,reinterpret_cast<POINT*>(&bounds),2);
        int state=SaveDC(dc);SetViewportOrgEx(dc,bounds.left,bounds.top,nullptr);
        IntersectClipRect(dc,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top);
        SendMessageW(child,WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT|PRF_NONCLIENT|PRF_ERASEBKGND);
        RestoreDC(dc,state);
    }
    SelectObject(dc,old);
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=rect.right;
    info.bmiHeader.biHeight=-rect.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    std::vector<char> pixels(rect.right*rect.bottom*4);
    Check(GetDIBits(dc,bitmap,0,rect.bottom,pixels.data(),&info,DIB_RGB_COLORS)!=0,"Capture editor pixels");
    BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);
    header.bfSize=header.bfOffBits+static_cast<DWORD>(pixels.size());
    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<char*>(&header),sizeof(header));
    out.write(reinterpret_cast<char*>(&info.bmiHeader),sizeof(info.bmiHeader));out.write(pixels.data(),pixels.size());
    DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(window,screen);
}
int main() {
    try {
        fs::create_directories("tests/runtime");
        const std::wstring song=L"Sai Gon Night (Gucci Remix).slk";
        auto original=Bytes(song);
        auto info=BeatUp::ReadSongInfo(song,150);
        Check(info.bpm==135 && info.hasAuditionOffset && std::abs(info.auditionOffset-.7)<1e-9,"Test song BPM/offset");
        std::vector<BeatUp::ChartNote> notes;
        Check(BeatUp::SlkParser::LoadChart(song,notes,135),"Korean native headers load");
        Check(notes.front().tick==144 && std::abs(notes.front().timeMs-16000)<1e-6,"Native song first tick at BPM135");
        BeatUp::ChartDocument doc;
        Check(doc.Load(song),"Open actual user chart");auto count=doc.Events().size();
        Check(count==notes.size(),"Editor and player agree on event count");
        Check(doc.Save(L"tests/runtime/roundtrip.slk"),"Save unchanged chart");
        Check(Bytes("tests/runtime/roundtrip.slk")==original,"Unchanged save is byte exact including Korean bytes");
        Check(doc.Toggle(100,7) && doc.Dirty(),"Add note marks document dirty");
        Check(doc.Undo() && !doc.Dirty() && doc.Events().size()==count,"Undo restores saved state");
        Check(doc.Redo() && doc.Events().size()==count+1,"Redo restores note");
        doc.Copy(100,100);doc.Paste(104);
        Check(doc.Events().size()==count+2,"Copy/paste selected tick");
        doc.Paste(104);Check(doc.Events().size()==count+2,"Paste avoids identical duplicate events");
        doc.Cut(100,100);Check(doc.Events().size()==count+1,"Cut removes range");
        doc.Paste(108);Check(doc.Events().size()==count+2,"Cut contents paste at cursor");
        Check(doc.Save(L"tests/runtime/edited_roundtrip.slk"),"Save edited chart");
        Check(BeatUp::SlkParser::LoadChart(L"tests/runtime/edited_roundtrip.slk",notes,135) && notes.size()==count+2,"Edited chart plays after reopening");
        Check(notes.front().tick==104,"Saved edits ordered by tick");
        Check(Bytes(song)==original,"Original user chart untouched");
        {
            std::ofstream f("tests/runtime/preserve.slk",std::ios::binary);
            f<<"ID;PTEST\r\nC;X4;Y1;K\"Location\"\r\nC;X5;K\"Type\"\r\nC;X6;K\"Key\"\r\n"
               "C;X4;Y3;K16\r\nC;X5;K\"n,s,f\"\r\nC;X6;K\"7\"\r\nC;X9;K\"custom;quoted\"\r\nE\r\n";
        }
        Check(doc.Load(L"tests/runtime/preserve.slk"),"Combined flags fixture");
        doc.Mirror(16,16);Check(doc.Events()[0].lane==9 && doc.Events()[0].finish,"Mirror preserves finish and Space");
        doc.Undo();
        Check(doc.Shift(16,16,-1) && doc.Events()[0].tick==15,"Shift retains tick units");doc.Undo();
        Check(!doc.Shift(16,16,-17),"Cannot shift before zero");
        doc.Randomize(16,16);Check(doc.Events().size()==2 && doc.Events()[0].finish,"Randomization preserves event count/finish");doc.Undo();
        doc.GenerateSpaces(32,64);Check(doc.Events().size()==5,"Generate Space every sixteen ticks");doc.Undo();
        doc.ToggleFinish(16);Check(!doc.Events()[0].finish,"Finish toggle removes flag while retaining notes");
        doc.ToggleFinish(16);Check(doc.Events()[0].finish && doc.Events()[1].finish,"Finish toggle combines with existing events");
        doc.Toggle(16,7);
        Check(doc.Events().size()==1 && doc.Events()[0].lane==5 && doc.Events()[0].finish,"Removing direction preserves Space/finish");
        Check(doc.Save(L"tests/runtime/preserve_edited.slk"),"Save combined flags");
        Check(Bytes("tests/runtime/preserve_edited.slk").find("K\"custom;quoted\"")!=std::string::npos,"Unknown column/quoted delimiter preserved");
        doc.Toggle(16,5);Check(doc.Events().size()==1 && doc.Events()[0].lane==0,"Removing Space retains finish event");
        doc.Erase(16,16);Check(doc.Events().empty(),"Erase final event");
        Check(doc.Save(L"tests/runtime/empty.slk") && doc.Load(L"tests/runtime/empty.slk"),"Empty chart can save/reopen in editor");
        doc.Toggle(32,9);Check(doc.Events().size()==1,"Empty chart can be edited again");
        Check(!doc.Load(L"tests/runtime/missing.slk") && doc.Events().size()==1,"Failed open preserves current document");
        Check(!doc.Save(L"tests/runtime/missing_folder/file.slk") && doc.Dirty(),"Failed save keeps unsaved state");

        BeatUp::ChartDocument fresh;fresh.New();
        Check(fresh.Events().empty() && !fresh.Dirty(),"New chart starts empty without a file");
        Check(fresh.Save(L"tests/runtime/new_empty.slk"),"New empty document saves valid SLK headers");
        Check(fresh.Load(L"tests/runtime/new_empty.slk"),"New empty document reopens");
        fresh.Toggle(16,9);Check(fresh.Save(L"tests/runtime/new_drawn.slk"),"New drawn chart saves");
        Check(BeatUp::SlkParser::LoadChart(L"tests/runtime/new_drawn.slk",notes,135) && notes.size()==1 && notes.front().lane==9,"New chart plays after drawing");
        BeatUp::ChartEditor freshEditor;
        bool liveCalled=false;
        freshEditor.SetLivePreview([&](const std::vector<BeatUp::ChartNote>& snapshot,const BeatUp::SongInfo& info,double){liveCalled=snapshot.size()==1 && info.bpm==135 && info.auditionOffset==.7;return true;});
        freshEditor.Show(nullptr,L"",[](const std::wstring&,double){return false;},{},fs::absolute(song).replace_extension(L".ogg").wstring(),{},135,.7);
        auto freshWindow=freshEditor.Handle();
        Check(freshEditor.AllCombo()==0,"Editor available with OGG and no SLK");
        Key(freshWindow,VK_NUMPAD9);Check(freshEditor.AllCombo()==1,"ALL COMBO increments when drawing");
        Key(freshWindow,VK_F5);Check(liveCalled,"New notes preview without requiring a SLK save dialog");
        DestroyWindow(freshWindow);
        fs::copy_file(song,"tests/runtime/editor_ui.slk",fs::copy_options::overwrite_existing);
        fs::remove("tests/runtime/editor_ui.edited.slk");
        fs::remove("tests/runtime/editor_ui.edited.song.ini");
        { std::ofstream f("tests/runtime/editor_ui.song.ini");f<<"title=Sai Gon Night (Gucci Remix)\nbpm=135\naudition_offset=0.7\naudio=../../Sai Gon Night (Gucci Remix).ogg\n"; }
        BeatUp::ChartEditor editor;std::wstring preview;double previewPosition=-1;std::vector<int> transportActions;
        editor.Show(nullptr,L"tests/runtime/editor_ui.slk",[&](const std::wstring& p,double time){preview=p;previewPosition=time;return true;},{},L"",[&](int action){transportActions.push_back(action);});
        HWND window=FindWindowW(L"BeatUpChartEditor",nullptr);Check(window!=nullptr,"Editor window created");
        ShowWindow(window,SW_HIDE);
        Capture(window,"tests/runtime/editor_sai_gon.bmp");
        wchar_t bpmText[80]{},offsetText[80]{};
        GetWindowTextW(GetDlgItem(window,201),bpmText,80);GetWindowTextW(GetDlgItem(window,202),offsetText,80);
        Check(std::wstring(bpmText)==L"135" && std::wstring(offsetText)==L"0.7","Timing fields show loaded values");
        SetWindowTextW(GetDlgItem(window,201),L"137.5");SetWindowTextW(GetDlgItem(window,202),L"0.8");
        SendMessageW(window,WM_COMMAND,112,0);SendMessageW(window,WM_COMMAND,101,0);
        auto temporary=BeatUp::ReadSongInfo(L"tests/runtime/editor_ui.edited.slk",150);
        Check(temporary.bpm==137.5 && temporary.auditionOffset==.8,"Timing controls save BPM and native offset");
        SetWindowTextW(GetDlgItem(window,201),L"135");SetWindowTextW(GetDlgItem(window,202),L"0.7");
        SendMessageW(window,WM_COMMAND,112,0);
        Key(window,VK_HOME);Key(window,VK_NUMPAD7); // Place a direction at tick zero.
        Key(window,'Z',true);Key(window,'Y',true); // Exercise UI undo/redo routing.
        SendMessageW(window,WM_COMMAND,111,0); // Toolbar Finish combines with the note at tick zero.
        ShowWindow(window,SW_SHOWNA);
        Key(window,VK_F5);
        Check(!preview.empty(),"F5 saves and sends preview to gameplay");
        Check(IsWindowVisible(window) && previewPosition==0,"Editor stays open during cursor preview");
        for(int i=0;i<100;++i) Key(window,VK_RIGHT);
        SetWindowTextW(GetDlgItem(window,201),L"137.5");SetWindowTextW(GetDlgItem(window,202),L"0.8");Key(window,VK_F5);
        Check(std::abs(previewPosition-((100-.2)*15000/137.5-60000/137.5))<1e-6,"Cursor preview uses pending timing fields after applying them");
        SetWindowTextW(GetDlgItem(window,201),L"135");SetWindowTextW(GetDlgItem(window,202),L"0.7");Key(window,VK_HOME);Key(window,VK_F5);
        Sleep(35);editor.SetPlaybackTime(16000,true);Capture(window,"tests/runtime/editor_playhead.bmp");
        for(int id:{121,122,123,124}) SendMessageW(window,WM_COMMAND,id,0);
        Check(transportActions==std::vector<int>({1011,1010,1005,1012}),"Editor Pause Resume Restart Transport commands route correctly");
        ShowWindow(window,SW_HIDE);
        Check(BeatUp::SlkParser::LoadChart(preview,notes,135) && notes.front().tick==0 && notes.front().lane==7 && notes.front().finish,"UI edit and Finish survive save/reopen");
        auto savedInfo=BeatUp::ReadSongInfo(preview,150);
        Check(savedInfo.bpm==135 && savedInfo.hasAuditionOffset && std::abs(savedInfo.auditionOffset-.7)<1e-9,"Preview metadata preserves BPM/offset");
        Check(fs::exists(savedInfo.audioPath),"Edited preview retains original OGG reference");
        DestroyWindow(window);
        fs::copy_file(song,"tests/runtime/session_timing.slk",fs::copy_options::overwrite_existing);
        fs::copy_file("tests/runtime/editor_ui.song.ini","tests/runtime/session_timing.song.ini",fs::copy_options::overwrite_existing);
        fs::remove("tests/runtime/session_timing.edited.slk");fs::remove("tests/runtime/session_timing.edited.song.ini");
        BeatUp::ChartEditor sessionEditor;
        sessionEditor.Show(nullptr,L"tests/runtime/session_timing.slk",[](const std::wstring&,double){return true;},{},fs::absolute("songs/v.wav").wstring(),{},136.5,.65);
        auto sessionWindow=FindWindowW(L"BeatUpChartEditor",nullptr);ShowWindow(sessionWindow,SW_HIDE);
        GetWindowTextW(GetDlgItem(sessionWindow,201),bpmText,80);GetWindowTextW(GetDlgItem(sessionWindow,202),offsetText,80);
        Check(std::wstring(bpmText)==L"136.5" && std::wstring(offsetText)==L"0.65","Editor inherits transport BPM and offset");
        Key(sessionWindow,'S',true);
        auto inherited=BeatUp::ReadSongInfo(L"tests/runtime/session_timing.edited.slk",150);
        Check(inherited.bpm==136.5 && inherited.auditionOffset==.65 && fs::path(inherited.audioPath)==fs::absolute("songs/v.wav"),"Saving retains session timing and selected music");
        DestroyWindow(sessionWindow);
        // A drag selects a range without toggling its first note; a click still toggles.
        fs::copy_file(song,"tests/runtime/drag_ui.slk",fs::copy_options::overwrite_existing);
        fs::remove("tests/runtime/drag_ui.edited.slk");fs::remove("tests/runtime/drag_ui.edited.song.ini");
        auto owner=CreateWindowW(L"STATIC",L"Embedded editor test",WS_OVERLAPPEDWINDOW,0,0,1240,830,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        auto ownerMenu=CreateMenu();AppendMenuW(ownerMenu,MF_STRING,1,L"Player");SetMenu(owner,ownerMenu);ShowWindow(owner,SW_SHOWNA);
        BeatUp::ChartEditor dragEditor;dragEditor.SetEmbedded(true);
        dragEditor.Show(owner,L"tests/runtime/drag_ui.slk",[](const std::wstring&,double){return true;});
        auto dragWindow=dragEditor.Handle();
        Check(GetParent(dragWindow)==owner && (GetWindowLongPtrW(dragWindow,GWL_STYLE)&WS_CHILD),"Editor occupies the main window");
        Check(GetMenu(owner)!=ownerMenu,"Embedded editor installs editing menus");
        Key(dragWindow,VK_HOME);
        SendMessageW(dragWindow,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(120,180));
        SendMessageW(dragWindow,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(216,180));
        SendMessageW(dragWindow,WM_LBUTTONUP,0,MAKELPARAM(216,180));
        Key(dragWindow,'S',true);
        Check(Bytes("tests/runtime/drag_ui.edited.slk")==original,"Dragging selects without changing chart notes");
        // Fill the selected span, then delete it: exactly ticks zero through three.
        for(int j=0;j<5;++j) {Key(dragWindow,VK_NUMPAD7);Key(dragWindow,VK_RIGHT);}
        Key(dragWindow,VK_HOME);
        SendMessageW(dragWindow,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(120,180));
        SendMessageW(dragWindow,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(216,180));
        SendMessageW(dragWindow,WM_LBUTTONUP,0,MAKELPARAM(216,180));
        Key(dragWindow,VK_DELETE);Key(dragWindow,'S',true);
        Check(BeatUp::SlkParser::LoadChart(L"tests/runtime/drag_ui.edited.slk",notes,135) && notes.front().tick==4,"Drag-selected span is used by Delete");
        SendMessageW(dragWindow,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(120,180));
        SendMessageW(dragWindow,WM_LBUTTONUP,0,MAKELPARAM(120,180));Key(dragWindow,'S',true);
        Check(BeatUp::SlkParser::LoadChart(L"tests/runtime/drag_ui.edited.slk",notes,135) && notes.front().tick==0,"Single click still adds a note");
        Capture(dragWindow,"tests/runtime/editor_embedded_drag.bmp");
        SendMessageW(dragWindow,WM_CLOSE,0,0);
        Check(!IsWindowVisible(dragWindow) && GetMenu(owner)==ownerMenu,"Returning to player restores its menu");
        DestroyWindow(owner);
        BeatUp::AudioEngine audio;Check(audio.Init(),"Mixer audio initialization");
        fs::remove("tests/runtime/mixer.ini");
        BeatUp::AudioMixer mixer;mixer.Load(&audio,fs::absolute("tests/runtime/mixer.ini").wstring());mixer.Show(nullptr);
        auto mixerWindow=FindWindowW(L"BeatUpAudioMixer",nullptr);Check(mixerWindow!=nullptr,"Mixer window created");ShowWindow(mixerWindow,SW_HIDE);
        SendMessageW(GetDlgItem(mixerWindow,1),TBM_SETPOS,TRUE,25);
        SendMessageW(GetDlgItem(mixerWindow,2),TBM_SETPOS,TRUE,15);
        SendMessageW(mixerWindow,WM_HSCROLL,TB_THUMBPOSITION,0);
        Check(audio.GetMusicVolume()==.25f && audio.GetEffectVolume()==.15f,"Mixer sliders set independent gains");
        audio.SetMusicVolume(1);audio.SetEffectVolume(1);
        BeatUp::AudioMixer reload;reload.Load(&audio,fs::absolute("tests/runtime/mixer.ini").wstring());
        Check(audio.GetMusicVolume()==.25f && audio.GetEffectVolume()==.15f,"Mixer settings persist across reload");
        audio.SetMusicVolume(.45f);audio.SetEffectVolume(.3f);mixer.Persist();
        Check(SendMessageW(GetDlgItem(mixerWindow,1),TBM_GETPOS,0,0)==45 && SendMessageW(GetDlgItem(mixerWindow,2),TBM_GETPOS,0,0)==30,"External transport gain changes synchronize mixer controls");
        DestroyWindow(mixerWindow);
        std::cout<<"PASS: actual Sai Gon chart, byte-exact roundtrip, edits, undo/redo, selection, flags, unknown cells, safe save, editor UI and preview\n";
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}

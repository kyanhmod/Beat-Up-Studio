#include "AssetBundle.h"
// Exercise the real application menu and keyboard handlers, not substitute UI callbacks.
#include "../src/main.cpp"
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <commctrl.h>
static void Check(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
static void Pump() {MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
// Deterministic render readback plus native control printing. Desktop capture is
// unavailable in this test environment; this verifies layout without claiming it.
static void PrintChildren(HWND parent,HWND root,HDC dc) {
    auto first=GetWindow(parent,GW_CHILD);
    for(HWND child=first?GetWindow(first,GW_HWNDLAST):nullptr;child;child=GetWindow(child,GW_HWNDPREV)) {
        if(!IsWindowVisible(child) || child==g_renderWindow) continue;
        RECT bounds{};GetWindowRect(child,&bounds);MapWindowPoints(nullptr,root,reinterpret_cast<POINT*>(&bounds),2);
        int saved=SaveDC(dc);SetViewportOrgEx(dc,bounds.left,bounds.top,nullptr);
        IntersectClipRect(dc,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top);
        SendMessageW(child,WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT|PRF_NONCLIENT|PRF_ERASEBKGND);
        RestoreDC(dc,saved);PrintChildren(child,root,dc);
    }
}
static void Capture(HWND window,const char* path) {
    Pump();g_shell.SetMode(g_editor.Visible()?1032:g_mode);g_transport.SetMode(g_editor.Visible()?1032:g_mode);UpdateWindow(window);
    g_renderer.BeginFrame(0,0,0,1);if(!g_editor.Visible()) g_game.Render(g_transport.Started());
    Check(g_renderer.SaveFrameBitmap(L"tests/runtime/session_underlay.bmp"),"Gameplay render readback");g_renderer.EndFrame(true);
    auto bitmap=static_cast<HBITMAP>(LoadImageW(nullptr,L"tests/runtime/session_underlay.bmp",IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION));
    Check(bitmap!=nullptr,"Load gameplay readback for native UI composition");
    auto screen=GetDC(window),dc=CreateCompatibleDC(screen);RECT root{};GetClientRect(window,&root);
    auto canvas=CreateCompatibleBitmap(screen,root.right,root.bottom);auto source=CreateCompatibleDC(screen);auto sourceOld=SelectObject(source,bitmap),old=SelectObject(dc,canvas);
    FillRect(dc,&root,reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    RECT target{};GetWindowRect(g_renderWindow,&target);MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&target),2);
    BitBlt(dc,target.left,target.top,target.right-target.left,target.bottom-target.top,source,0,0,SRCCOPY);
    SelectObject(source,sourceOld);DeleteDC(source);DeleteObject(bitmap);PrintChildren(window,window,dc);SelectObject(dc,old);bitmap=canvas;
    int width=root.right,height=root.bottom;
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    std::vector<char> pixels(width*height*4);GetDIBits(dc,bitmap,0,height,pixels.data(),&info,DIB_RGB_COLORS);
    BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+static_cast<DWORD>(pixels.size());
    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<char*>(&header),sizeof(header));out.write(reinterpret_cast<char*>(&info.bmiHeader),sizeof(info.bmiHeader));out.write(pixels.data(),pixels.size());
    DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(window,screen);
}
int main() {
    try {
        WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=WndProc;wc.lpszClassName=L"BeatUpSessionFlowTests";RegisterClassW(&wc);
        auto owner=CreateWindowW(wc.lpszClassName,L"Beat-Up session flow test",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,0,0,1240,900,nullptr,CreateMainMenu(),wc.hInstance,nullptr);
        ShowWindow(owner,SW_SHOWNA);
        g_renderWindow=CreateRenderSurface(owner,1224,841);
        Check(g_renderer.Init(g_renderWindow,1224,841),"Renderer initialization");Check(g_audio.Init(),"Audio initialization");
        TextureResource promptTexture;Check(DDSLoader::LoadFromFile(g_renderer.GetDevice(),L"assets/hud_clean/press chance key.dds",promptTexture) && promptTexture.width==256 && promptTexture.height==32,"Original A4R4G4B4 Chance DDS loads");promptTexture.Release();
        Check(DDSLoader::LoadFromFile(g_renderer.GetDevice(),L"assets/hud_clean/bg.dds",promptTexture) && promptTexture.width==1024 && promptTexture.height==768,"Original RGB24 bg.dds with mipmaps loads");promptTexture.Release();
        g_game.SetTransportControlled(true);Check(g_game.Init(&g_renderer,&g_audio,L"assets/hud_clean"),"Game initialization");
        g_transport.Init(owner,&g_game,&g_audio,[owner](int action){SendMessageW(owner,WM_COMMAND,action,0);});g_transport.Embed();g_shell.Init(owner);g_uiReady=true;LayoutWorkspace(owner);
        Check(OpenSessionFile(owner,L"Sai Gon Night (Gucci Remix).ogg",true),"Open OGG without any SLK");
        Check(g_transport.SettingsVisible() && g_game.GetAllCombo()==0 && !g_transport.Playing(),"OGG opens settings and starts with zero combo");
        auto scratch=CreateCompatibleDC(nullptr);auto one=CreateBitmap(1,1,1,32,nullptr);auto oldPixel=SelectObject(scratch,one);
        SetPixel(scratch,0,0,RGB(255,255,255));SendMessageW(owner,WM_ERASEBKGND,reinterpret_cast<WPARAM>(scratch),0);
        Check(GetPixel(scratch,0,0)==RGB(0,0,0),"Uncovered area beside toggle repaints black");
        SelectObject(scratch,oldPixel);DeleteObject(one);DeleteDC(scratch);
        auto bar=g_transport.Handle();SetWindowTextW(GetDlgItem(bar,2001),L"135");SetWindowTextW(GetDlgItem(bar,2002),L"0.7");
        g_renderer.BeginFrame(0,0,0,1);g_game.Render(false);g_renderer.EndFrame(true);Capture(owner,"tests/runtime/session_settings.bmp");
        SendMessageW(bar,WM_COMMAND,1013,0);
        Check(!g_transport.SettingsVisible() && g_game.GetBpm()==135 && std::abs(g_game.GetNativeOffset()-.7)<1e-6,"Save commits song timing and exposes toggle");
        RECT surfaceBefore{},surfaceHidden{},surfaceShown{},clientBounds{};GetWindowRect(g_renderWindow,&surfaceBefore);GetClientRect(owner,&clientBounds);
        Check(surfaceBefore.right-surfaceBefore.left==clientBounds.right && surfaceBefore.bottom-surfaceBefore.top==clientBounds.bottom,"Toggle overlays the complete player surface");
        SendMessageW(owner,WM_COMMAND,1012,0);GetWindowRect(g_renderWindow,&surfaceHidden);Check(EqualRect(&surfaceBefore,&surfaceHidden),"Hiding toggle does not change player scale");Check(!g_transport.Visible(),"Toggle menu hides embedded bar");
        SendMessageW(owner,WM_KEYDOWN,VK_F10,0);GetWindowRect(g_renderWindow,&surfaceShown);Check(EqualRect(&surfaceBefore,&surfaceShown),"Showing toggle does not change player scale");Check(g_transport.Visible(),"F10 toggles embedded bar back on");
        SendMessageW(owner,WM_COMMAND,1010,0);Sleep(25);g_transport.Tick(25);Check(g_transport.Playing() && g_transport.Position()>0,"Music plays with an empty chart");
        Check(GetFocus()==owner,"Play returns keyboard focus to gameplay");
        SendMessageW(owner,WM_KEYDOWN,VK_F9,0);double hold=g_transport.Position();Sleep(25);g_transport.Tick(25);
        Check(!g_transport.Playing() && std::abs(g_transport.Position()-hold)<.1,"Pause holds music position");
        RECT targetClient{0,0,1024,768};AdjustWindowRectEx(&targetClient,static_cast<DWORD>(GetWindowLongPtrW(owner,GWL_STYLE)),TRUE,0);
        SetWindowPos(owner,nullptr,0,0,targetClient.right-targetClient.left,targetClient.bottom-targetClient.top,SWP_NOMOVE|SWP_NOZORDER);LayoutWorkspace(owner);
        RECT beforeEditor{},beforePlayerArea{},barBounds{};GetWindowRect(owner,&beforeEditor);GetWindowRect(g_renderWindow,&beforePlayerArea);GetWindowRect(bar,&barBounds);
        Check(barBounds.right-barBounds.left==560 && barBounds.bottom-barBounds.top==64,"Toggle is one compact fixed-width cluster");
        SendMessageW(owner,WM_COMMAND,1032,0);
        RECT inEditor{};GetWindowRect(owner,&inEditor);Check(EqualRect(&beforeEditor,&inEditor),"Opening Editor never enlarges the main window");
        Check(g_editor.Visible() && GetParent(g_editor.Handle())==owner && g_editor.AllCombo()==0,"Mode Editor works with no SLK in the same window");
        auto editor=g_editor.Handle();SendMessageW(editor,WM_KEYDOWN,VK_NUMPAD9,0);Check(g_editor.AllCombo()==1,"Drawing increments ALL COMBO");
        SendMessageW(editor,WM_KEYDOWN,VK_F5,0);Check(g_transport.Playing() && g_game.GetAllCombo()==1,"Unsaved notes preview in the shared session");
        auto noteCountBeforeSpace=g_editor.AllCombo();SendMessageW(editor,WM_KEYDOWN,VK_SPACE,0);
        Check(!g_transport.Playing() && g_editor.AllCombo()==noteCountBeforeSpace,"Editor Space pauses without adding a Space note");
        double spaceHold=g_transport.Position();Sleep(20);SendMessageW(editor,WM_KEYDOWN,VK_SPACE,0);
        Check(g_transport.Playing() && g_transport.Position()>=spaceHold && g_editor.AllCombo()==noteCountBeforeSpace,"Editor Space resumes current preview without adding notes");
        Capture(owner,"tests/runtime/session_editor.bmp");
        SendMessageW(owner,WM_COMMAND,1030,0);
        RECT afterEditor{},afterPlayerArea{};GetWindowRect(owner,&afterEditor);GetWindowRect(g_renderWindow,&afterPlayerArea);
        Check(EqualRect(&beforeEditor,&afterEditor) && EqualRect(&beforePlayerArea,&afterPlayerArea),"Returning to Game preserves both outer size and player bounds");
        Check(!g_editor.Visible() && !g_game.IsAutoPlay() && g_game.GetAllCombo()==1,"Mode Game retains unsaved chart without a save dialog");
        SendMessageW(owner,WM_COMMAND,1032,0);Check(g_editor.AllCombo()==1,"Returning to Editor retains unsaved notes");
        SendMessageW(g_editor.Handle(),WM_COMMAND,105,0);SendMessageW(owner,WM_COMMAND,1030,0);
        Check(OpenSessionFile(owner,L"Sai Gon Night (Gucci Remix).slk",false),"Import SLK into selected song");
        Check(g_game.GetAllCombo()>1500 && g_game.GetBpm()==135 && std::abs(g_game.GetNativeOffset()-.7)<1e-6,"Import populates ALL COMBO and retains song timing");
        auto chartCount=g_game.GetAllCombo();
        Check(OpenSessionFile(owner,L"Sai Gon Night (Gucci Remix).ogg",true) && g_game.GetAllCombo()==chartCount,"Choosing OGG after SLK retains imported notes");
        g_transport.SaveSettings();
        g_game.ConfigureTiming(138.123456789123,.7123456789123);g_transport.Ready(true,g_chartPath);
        g_game.SetAutoPlay(true);g_game.Update(60000,0);
        int preservedCombo=g_game.GetCombo(),preservedScore=g_game.GetScore();auto preservedCounts=g_game.GetJudgeCounts();auto preservedRating=g_game.GetLastRating();
        Check(preservedCombo>0,"Seek regression begins with a real scored combo");
        g_transport.Play();auto seek=GetDlgItem(bar,2003);
        for(int pos:{70000,12000,50000}) {
            SendMessageW(seek,TBM_SETPOS,TRUE,pos);SendMessageW(bar,WM_HSCROLL,TB_THUMBTRACK,reinterpret_cast<LPARAM>(seek));
            SendMessageW(bar,WM_HSCROLL,TB_ENDTRACK,reinterpret_cast<LPARAM>(seek));
            Check(g_game.GetCombo()==preservedCombo && g_game.GetScore()==preservedScore && g_game.GetJudgeCounts()==preservedCounts && g_game.GetLastRating()==preservedRating,"Dragging timeline and resuming retains combo, score, counts and visible judgement");
        }
        Check(g_game.GetBpm()==138.123456789123,"Scrub resume never reparses rounded display BPM");
        g_transport.Pause();g_game.ConfigureTiming(135,.7);g_transport.Ready(true,g_chartPath);
        SendMessageW(owner,WM_COMMAND,1032,0);Check(g_editor.Visible() && g_editor.AllCombo()==g_game.GetAllCombo(),"Imported chart opens in Editor mode");
        Check((GetWindowLongPtrW(g_editor.Handle(),GWL_STYLE)&WS_HSCROLL)==0,"Editor has no redundant horizontal scrollbar");
        Capture(owner,"tests/runtime/session_editor_loaded.bmp");SendMessageW(owner,WM_COMMAND,1030,0);
        g_renderer.BeginFrame(0,0,0,1);g_game.Render(false);g_renderer.EndFrame(true);Capture(owner,"tests/runtime/session_player.bmp");
        g_game.SetBackgroundImage(true);Capture(owner,"tests/runtime/player_bg_overlay.bmp");
        SendMessageW(owner,WM_COMMAND,1012,0);Capture(owner,"tests/runtime/player_bg_hidden.bmp");
        SendMessageW(owner,WM_COMMAND,1012,0);g_game.SetBackgroundImage(false);
        Check(OpenSessionFile(owner,L"Sai Gon Night (Gucci Remix).ogg",true,true) && g_game.GetAllCombo()==0,"Opening a new music session clears old chart data");
        g_transport.SaveSettings();
        SetWindowPos(owner,nullptr,0,0,1450,950,SWP_NOMOVE|SWP_NOZORDER);LayoutWorkspace(owner);
        Capture(owner,"tests/runtime/studio_4_3.bmp");
        RECT expandedBar{};GetWindowRect(bar,&expandedBar);Check(expandedBar.right-expandedBar.left==560,"Resizing app does not stretch toggle controls");
        std::vector<ChartNote> chanceChart;
        for(int i=0;i<30;++i) {ChartNote n;n.tick=i;n.lane=7;chanceChart.push_back(n);}
        for(int i=320;i<480;i+=4) {ChartNote n;n.tick=i;n.lane=9;chanceChart.push_back(n);}
        ChartNote finish;finish.tick=480;finish.finish=true;chanceChart.push_back(finish);
        g_game.SetSessionChart(chanceChart,120,1);g_game.SetAutoPlay(true);g_game.Update(29*125,0);g_game.SetAutoPlay(false);
        g_game.Seek(288*125);Check(g_game.ChanceAvailable(288*125),"Native Del prompt opens twelve measures before Finish above 5000 score");
        g_game.Update(288*125,0);g_game.Update(288*125+500,500);
        g_renderer.BeginFrame(0,0,0,1);g_game.Render(false);g_renderer.EndFrame(true);Capture(owner,"tests/runtime/studio_chance.bmp");
        g_game.OnKeyDown(VK_DELETE,288*125+500);Check(g_game.ChanceActive() && !g_game.ChanceAvailable(288*125+500),"Delete activates Chance and dismisses prompt");
        auto pink=std::find_if(g_game.GetNotes().begin(),g_game.GetNotes().end(),[](const ChartNote& n){return n.chance;});
        Check(pink!=g_game.GetNotes().end(),"Chance generates pink notes only in its Finish section");
        // Clear prior normal arrows by following the ordered chart, then press the first pink arrow.
        double hitTime=pink->timeMs;
        for(const auto& n:g_game.GetNotes()) if(n.lane==9 && n.timeMs<=hitTime) {
            g_game.OnKeyDown(VK_NUMPAD9,n.timeMs);g_game.OnKeyUp(VK_NUMPAD9);
        }
        Check(g_game.GetLastRating()==JUDGE_BAD,"Pressing a pink Chance note is BAD, as in native judge flag");
        g_game.Restart();Check(!g_game.ChanceActive(),"Restart clears Chance activation");
        Check(OpenSessionFiles(owner,{L"Sai Gon Night (Gucci Remix).slk",L"Sai Gon Night (Gucci Remix).ogg"}),"One operation opens an OGG/SLK pair regardless of selection order");
        Check(g_game.GetAllCombo()>1500 && g_audio.IsMusicLoaded(),"Paired import retains both music and notes");
        Check(!BeatUp::ReadAsset(L"missing/assets/a91.dds").empty(),"DDS comes from embedded bundle without an assets directory");
        Check(!BeatUp::ReadAsset(L"missing/SOUND/READY.ogg").empty(),"Sound comes from embedded bundle without a SOUND directory");
        Check(LoadIconW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(101))!=nullptr,"Application icon is embedded");
        Check(g_audio.LoadEffects(L"missing/SOUND")==6,"All six effects decode directly from embedded data");
        g_transport.Tick(16);g_transport.Tick(500);g_transport.Ready(true,g_chartPath,false);
        bool telemetry=false;
        for(HWND label=GetWindow(bar,GW_CHILD);label;label=GetWindow(label,GW_HWNDNEXT)) {
            wchar_t caption[512]{};GetWindowTextW(label,caption,512);std::wstring value=caption;
            if(value.find(L"Combo:")!=std::wstring::npos && value.find(L"FPS:")!=std::wstring::npos)telemetry=true;
        }
        Check(telemetry,"Compact toggle displays combo and FPS");
        DestroyWindow(owner);std::cout<<"PASS: real app OGG/settings/save, empty music playback, toggle, editor mode, drawing, live preview, SLK import, return to Game\n";
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
    return 0;
}

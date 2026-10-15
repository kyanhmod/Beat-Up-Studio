#define main SessionReferenceMain
#include "session_flow_tests.cpp"
#undef main
int main() {
 try {
    std::filesystem::create_directories("tests/runtime");
    const wchar_t* wave=L"tests/runtime/transport_generated.wav";
    {std::ofstream f(wave,std::ios::binary);uint32_t bytes=44100*2*30,total=bytes+36,fmt=16,rate=44100,byteRate=88200;uint16_t one=1,align=2,bits=16;
     f.write("RIFF",4);f.write((char*)&total,4);f.write("WAVEfmt ",8);f.write((char*)&fmt,4);f.write((char*)&one,2);f.write((char*)&one,2);f.write((char*)&rate,4);f.write((char*)&byteRate,4);f.write((char*)&align,2);f.write((char*)&bits,2);f.write("data",4);f.write((char*)&bytes,4);std::vector<char> zero(bytes);f.write(zero.data(),zero.size());}
    WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=WndProc;wc.lpszClassName=L"BeatUpTransportSkinTest";RegisterClassW(&wc);
    auto owner=CreateWindowW(wc.lpszClassName,L"Transport skin test",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,0,0,1050,850,nullptr,CreateMainMenu(),wc.hInstance,nullptr);ShowWindow(owner,SW_SHOWNA);
    g_renderWindow=CreateRenderSurface(owner,1024,768);Check(g_renderer.Init(g_renderWindow,1024,768),"Renderer");Check(g_audio.Init(),"Audio");g_game.SetTransportControlled(true);Check(g_game.Init(&g_renderer,&g_audio,L"missing"),"Embedded skin");
    g_transport.Init(owner,&g_game,&g_audio,[owner](int id){SendMessageW(owner,WM_COMMAND,id,0);});g_transport.Embed();g_shell.Init(owner);g_uiReady=true;LayoutWorkspace(owner);
    Check(g_game.NewSong(wave),"Generated audio");
    std::vector<ChartNote> notes;for(int tick=16;tick<450;++tick)for(int lane:{7,4,1,9,6,3}) {ChartNote n;n.tick=tick;n.lane=lane;notes.push_back(n);}
    g_game.SetSessionChart(notes,240,1);g_game.SetAutoPlay(true);g_transport.Ready(true);g_game.Update(12000,0);
    Check(g_game.GetCombo()>1000,"Seed four-digit combo");int before=g_game.GetCombo();Check(g_transport.Play(),"Start");
    auto bar=g_transport.Handle(),slider=GetDlgItem(bar,2003);
    for(int pos:{70000,12000,50000}) {
      SendMessageW(slider,TBM_SETPOS,TRUE,pos);SendMessageW(bar,WM_HSCROLL,TB_THUMBTRACK,(LPARAM)slider);
      SendMessageW(bar,WM_HSCROLL,TB_THUMBPOSITION,(LPARAM)slider);SendMessageW(bar,WM_HSCROLL,TB_ENDTRACK,(LPARAM)slider);
      for(int i=0;i<40;++i) {Sleep(8);g_transport.Tick(8);Pump();}
      std::cout<<"combo after resume: "<<g_game.GetCombo()<<" position "<<g_transport.Position()<<'\n';
      Check(g_game.GetCombo()>=before,"Combo survives real audio ticks after slider release");before=g_game.GetCombo();
    }
    g_transport.Pause();Capture(owner,"tests/runtime/default_player.bmp");
    TextureResource skin;
    for(const wchar_t* name:{L"crazy.dds",L"chance_active.dds",L"perfect.dds"}) {
        Check(DDSLoader::LoadFromFile(g_renderer.GetDevice(),std::wstring(L"missing/")+name,skin),"Embedded original HUD texture loads");skin.Release();
    }
    Check(DDSLoader::LoadFromFile(g_renderer.GetDevice(),L"missing/song_info_base.dds",skin),"Original song info loads");
    Check(skin.width==256 && skin.height==64 && skin.content.right==256,"Original song-info canvas preserved");skin.Release();
    Check(DDSLoader::LoadFromFile(g_renderer.GetDevice(),L"missing/countdown_1.dds",skin),"Original countdown loads");
    auto fit=FitContent(skin.content,96,64);Check(fit.height==64 && fit.width==32,"Original countdown aspect preserved");skin.Release();
    auto savedCombo=g_game.GetCombo();g_game.SetSessionChart(notes,240,1);Check(g_game.GetCombo()==savedCombo,"Unchanged editor document does not restart scored chart");
    g_game.SetAutoPlay(false);g_game.Seek(10000);g_game.Update(10300,300);Check(g_game.GetCombo()==savedCombo,"In-flight manual notes do not break combo immediately after seek");
    for(HWND focus:{owner,slider,GetDlgItem(bar,2004)}) {
        SetFocus(focus);bool visible=g_transport.Visible();MSG key{};key.hwnd=focus;key.message=WM_SYSKEYDOWN;key.wParam=VK_F10;
        Check(GlobalShortcut(owner,key) && g_transport.Visible()!=visible,"F10 system key toggles from every focus");
        key.lParam=1LL<<30;GlobalShortcut(owner,key);Check(g_transport.Visible()!=visible,"F10 repeat does not double toggle");
        key.message=WM_SYSKEYUP;GlobalShortcut(owner,key);Check(g_transport.Visible()!=visible,"F10 release does not activate menu");
    }
    // Native Chance window, with two opposing icons after Delete.
    std::vector<ChartNote> chance;
    for(int i=0;i<30;++i){ChartNote n;n.tick=i;n.lane=7;chance.push_back(n);}
    for(int i=320;i<480;i+=4){ChartNote n;n.tick=i;n.lane=9;chance.push_back(n);}
    ChartNote finish;finish.tick=480;finish.finish=true;chance.push_back(finish);
    g_game.SetSessionChart(chance,120,1);g_game.SetAutoPlay(true);g_game.Update(29*125,0);g_game.SetAutoPlay(false);g_game.Seek(288*125);
    g_game.Update(288*125,0);g_game.Update(288*125+500,500);
    Check(g_game.ActivateChance(288*125+500),"Delete Chance activation");
    g_game.Update(288*125+625,125);Capture(owner,"tests/runtime/default_prompt_exit.bmp");
    g_game.Update(288*125+1000,375);Capture(owner,"tests/runtime/default_chance.bmp");
    std::vector<ChartNote> introNotes;ChartNote first;first.tick=64;first.lane=7;introNotes.push_back(first);g_game.SetSessionChart(introNotes,120,1);
    for(int count:{3,2,1}) {double t=(64-count*4)*125.0;g_game.Seek(t);g_game.Update(t,0);std::string path="tests/runtime/default_count_"+std::to_string(count)+".bmp";Capture(owner,path.c_str());}


    for(int threshold:{100,400}) {
        std::vector<ChartNote> milestoneNotes;
        for(int i=1;i<=threshold;++i){ChartNote n;n.tick=i;n.lane=7;milestoneNotes.push_back(n);}
        g_game.SetSessionChart(milestoneNotes,120,1);g_game.SetAutoPlay(true);
        g_game.Update(threshold*125,0);g_game.Update(threshold*125+125,125);
        Check(g_game.GetCombo()==threshold,"Milestone capture reaches its exact combo");
        std::string image="tests/runtime/milestone_"+std::to_string(threshold)+".bmp";Capture(owner,image.c_str());
    }

    RECT compactBounds=InitialWindowRect(RECT{0,0,800,560},16,59);
    SetWindowPos(owner,nullptr,compactBounds.left,compactBounds.top,compactBounds.right-compactBounds.left,compactBounds.bottom-compactBounds.top,SWP_NOZORDER);LayoutWorkspace(owner);
    SendMessageW(owner,WM_COMMAND,1032,0);Check(g_editor.Visible(),"Editor opens in compact window");
    RECT grid{};GetClientRect(g_editor.Handle(),&grid);Check(grid.right<1010,"Compact editor uses narrow layout");
    g_transport.SetVisible(true);LayoutWorkspace(owner);Capture(owner,"tests/runtime/compact_editor.bmp");SendMessageW(owner,WM_COMMAND,1030,0);Capture(owner,"tests/runtime/compact_player.bmp");
    DestroyWindow(owner);g_renderer.Shutdown();g_audio.Shutdown();std::filesystem::remove(wave);
    std::cout<<"PASS: four-digit combo through backward/forward live scrubbing and embedded original HUD\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}

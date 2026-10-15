#include <windows.h>
#include <shellapi.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <commdlg.h>
#include "Renderer.h"
#include "AudioEngine.h"
#include "BeatUpGame.h"
#include "KeyBindings.h"
#include "ChartEditor.h"
#include "AudioMixer.h"
#include "PlaybackPanel.h"
#include "StudioShell.h"
#include "WindowSizing.h"

using namespace BeatUp;

static Renderer g_renderer;
static AudioEngine g_audio;
static BeatUpGame g_game;
static ChartEditor g_editor;
static AudioMixer g_mixer;
static PlaybackPanel g_transport;
static std::wstring g_chartPath;
static std::wstring g_selectedAudio;
static bool g_running = true;
static bool g_vsync = false;
static bool g_fullscreen = false;
static RECT g_windowedRect = {};
static HMENU g_mainMenu=nullptr;
static bool g_editorPreview=false,g_savedAutoPlay=false;
static HWND g_renderWindow=nullptr;
static int g_mode=1030;
static StudioShell g_shell;
static bool g_uiReady=false;
static bool g_restoreEditorAfterSetup=false;
static void LayoutWorkspace(HWND owner) {
    if(!g_uiReady) return;
    RECT r=g_shell.Layout(g_transport.Visible()?g_transport.Height():0);
    g_editor.SetBounds(r);g_transport.Layout();
    if(g_renderWindow) MoveWindow(g_renderWindow,r.left,r.top,r.right-r.left,r.bottom-r.top,FALSE);
    g_renderer.Resize(r.right-r.left,r.bottom-r.top);
}
static LRESULT CALLBACK RenderSurfaceProc(HWND window,UINT msg,WPARAM w,LPARAM l) {
    if(msg==WM_ERASEBKGND) return 1;
    if(msg==WM_PAINT) {PAINTSTRUCT ps;BeginPaint(window,&ps);EndPaint(window,&ps);return 0;}
    return DefWindowProcW(window,msg,w,l);
}
static HWND CreateRenderSurface(HWND owner,int width,int height) {
    WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=RenderSurfaceProc;wc.lpszClassName=L"BeatUpRenderSurface";RegisterClassW(&wc);
    return CreateWindowW(wc.lpszClassName,L"",WS_CHILD|WS_VISIBLE|WS_DISABLED|WS_CLIPSIBLINGS,0,0,width,height,owner,nullptr,wc.hInstance,nullptr);
}

static bool OpenSessionFile(HWND owner,const std::wstring& path,bool music,bool newSong=false) {
    bool wasEditor=g_editor.Visible();
    if(g_editor.Visible()) {SendMessageW(g_editor.Handle(),WM_CLOSE,0,0);if(g_editor.Visible()) return false;}
    else if(!g_editor.CanClose()) return false;
    double bpm=g_game.GetBpm(),offset=g_game.GetNativeOffset();
    bool keepChart=music && !newSong && (!g_chartPath.empty() || g_game.GetAllCombo()>0);
    bool loaded=music?(keepChart?g_game.LoadAudio(path):g_game.NewSong(path)):g_game.LoadSong(path,g_selectedAudio);
    if(!loaded) return false;
    if(music) {
        g_selectedAudio=path;
        if(!keepChart) {g_chartPath.clear();g_editor.ResetSession();g_game.SetAutoPlay(true);g_mode=1031;}
        g_transport.Ready(keepChart,g_chartPath);g_transport.ShowSettings();
    } else {
        g_chartPath=path;
        if(!g_selectedAudio.empty()) g_game.ConfigureTiming(bpm,offset);
        g_transport.Ready(true,g_chartPath);
    }
    CheckMenuRadioItem(g_mainMenu,1030,1032,g_mode,MF_BYCOMMAND);
    if(wasEditor) {if(g_transport.SettingsVisible()) g_restoreEditorAfterSetup=true;else SendMessageW(owner,WM_KEYDOWN,VK_F6,0);}
    return true;
}
static bool OpenSessionFiles(HWND owner,const std::vector<std::wstring>& files) {
    std::wstring music,chart;
    for(const auto& file:files) {
        auto ext=std::filesystem::path(file).extension().wstring();for(auto& c:ext)c=towlower(c);
        if(ext==L".slk") chart=file;else if(ext==L".ogg" || ext==L".wav") music=file;
    }
    if(music.empty() && chart.empty()) return false;
    if(!music.empty() && !OpenSessionFile(owner,music,true,!chart.empty())) return false;
    if(!chart.empty() && !OpenSessionFile(owner,chart,false)) return false;
    return true;
}
static void SyncEditorToPlayer() {
    auto position=g_transport.Position();
    g_game.SetSessionChart(g_editor.Snapshot(),g_editor.Info().bpm,g_editor.Info().NativeTickShift()+1);
    g_transport.Ready(true,g_chartPath);g_transport.Seek(position);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Numeric-pad directions keep their identity with NumLock off. Extended
    // navigation keys are distinct; do not turn their arrow keys into notes.
    if (msg == WM_KEYDOWN || msg == WM_KEYUP)
        wParam = NormalizeKeypadKey(wParam,lParam);
    switch (msg) {
        case WM_ERASEBKGND: {
            RECT r{};GetClientRect(hWnd,&r);
            FillRect(reinterpret_cast<HDC>(wParam),&r,reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            return 1;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;auto dc=BeginPaint(hWnd,&ps);
            FillRect(dc,&ps.rcPaint,reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            EndPaint(hWnd,&ps);return 0;
        }
        case WM_COMMAND: {
            int command=LOWORD(wParam);
            if(g_editor.Visible() && command>=100 && command<=124) {SendMessageW(g_editor.Handle(),WM_COMMAND,wParam,lParam);return 0;}
            if(command==1001 || command==1002 || command==1042 || command==1043) {
                wchar_t path[32768]{};OPENFILENAMEW dialog{sizeof(dialog)};
                dialog.hwndOwner=hWnd;dialog.lpstrFile=path;dialog.nMaxFile=32768;
                dialog.lpstrFilter=command==1042?L"Song files (*.ogg;*.wav;*.slk)\0*.ogg;*.wav;*.slk\0\0":command==1001?L"Audition chart (*.slk)\0*.slk\0\0":L"Music (*.ogg;*.wav)\0*.ogg;*.wav\0\0";
                dialog.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER|(command==1042?OFN_ALLOWMULTISELECT:0);
                g_transport.Pause();
                if(GetOpenFileNameW(&dialog)) {
                    std::vector<std::wstring> files;
                    const wchar_t* next=path+wcslen(path)+1;
                    if(command==1042 && *next) {for(;*next;next+=wcslen(next)+1) files.push_back((std::filesystem::path(path)/next).wstring());}
                    else files.push_back(path);
                    bool loaded=command==1043?OpenSessionFile(hWnd,path,true,true):OpenSessionFiles(hWnd,files);
                    if(!loaded) MessageBoxW(hWnd,command==1001?L"Cannot open this chart. The previous chart is retained.":L"Cannot decode this music file. The previous music is retained.",L"Open",MB_OK|MB_ICONWARNING);
                }
                return 0;
            }
            if(command==1020 || command==1021) {
                g_game.SetBackgroundImage(command==1021);
                CheckMenuRadioItem(g_mainMenu,1020,1021,command,MF_BYCOMMAND);
                return 0;
            }
            if(command==1003) {g_mixer.Show(hWnd);return 0;}
            if(command==1004) {SendMessageW(hWnd,WM_KEYDOWN,VK_F6,0);return 0;}
            if(command==1005) {g_transport.Restart();return 0;}
            if(command==1006) {g_game.ToggleAutoPlay();return 0;}
            if(command==1007) {g_mixer.Persist();return 0;}
            if(command==1010) {g_transport.Play();return 0;}
            if(command==1011) {g_transport.Pause();return 0;}
            if(command==1012) {g_transport.Toggle();CheckMenuItem(GetMenu(hWnd),1012,MF_BYCOMMAND|(g_transport.Visible()?MF_CHECKED:MF_UNCHECKED));return 0;}
            if(command==1013) {if(g_restoreEditorAfterSetup) {g_restoreEditorAfterSetup=false;SendMessageW(hWnd,WM_KEYDOWN,VK_F6,0);}if(g_editor.Visible()) g_editor.SetTiming(g_game.GetBpm(),g_game.GetNativeOffset());LayoutWorkspace(hWnd);SetFocus(g_editor.Visible()?g_editor.Handle():hWnd);return 0;}
            if(command==1041) {LayoutWorkspace(hWnd);return 0;}
            if(command==1040) {if(!g_editor.Visible()) SendMessageW(hWnd,WM_KEYDOWN,VK_F6,0);if(g_editor.Visible()) SendMessageW(g_editor.Handle(),WM_COMMAND,101,0);return 0;}
            if(command==1014) {
                if(g_transport.Playing()) g_transport.Pause();
                else if(g_editor.Visible()) {
                    auto notes=g_editor.Snapshot();const auto& current=g_game.GetNotes();
                    bool changed=notes.size()!=current.size() || !std::equal(notes.begin(),notes.end(),current.begin(),[](const ChartNote& a,const ChartNote& b){return a.tick==b.tick && a.lane==b.lane && a.finish==b.finish;});
                    if(!g_transport.Started() || changed) SendMessageW(g_editor.Handle(),WM_COMMAND,102,0);else g_transport.Play();
                } else g_transport.Play();
                if(g_editor.Visible()) SetFocus(g_editor.Handle());return 0;
            }
            if(command==1015) {g_transport.ShowSettings();return 0;}
            if(command==1032) {SendMessageW(hWnd,WM_KEYDOWN,VK_F6,0);return 0;}
            if(command==1030 || command==1031) {
                g_transport.Pause();
                if(g_editor.Visible()) {if(!g_editor.HideForPlayback()) return 0;SyncEditorToPlayer();}
                g_mode=command;g_game.SetAutoPlay(command==1031);g_editorPreview=false;
                CheckMenuRadioItem(g_mainMenu,1030,1032,command,MF_BYCOMMAND);return 0;
            }
            break;
        }
        case WM_GETMINMAXINFO: {
            auto info=reinterpret_cast<MINMAXINFO*>(lParam);auto work=WorkArea(hWnd);
            info->ptMinTrackSize={(std::min)(640L,work.right-work.left),(std::min)(480L,work.bottom-work.top)};return 0;
        }
        case WM_DISPLAYCHANGE:
        case WM_EXITSIZEMOVE:
            if(!g_fullscreen) FitWindowToWorkArea(hWnd);
            return 0;
        case WM_SETTINGCHANGE:
            if(wParam==SPI_SETWORKAREA && !g_fullscreen) FitWindowToWorkArea(hWnd);
            break;
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                if(g_uiReady) LayoutWorkspace(hWnd);
                else g_renderer.Resize(width,height);
            }
            return 0;

        case WM_KEYDOWN:
            if (lParam & (1LL << 30)) return 0;
            if(wParam=='O' && (GetKeyState(VK_CONTROL)&0x8000)) {SendMessageW(hWnd,WM_COMMAND,1042,0);return 0;}

            if(wParam==VK_PAUSE || wParam==VK_F9) {SendMessageW(hWnd,WM_COMMAND,1014,0);return 0;}
            if(wParam==VK_F10) {SendMessageW(hWnd,WM_COMMAND,1012,0);return 0;}
            if(wParam==VK_SPACE) {SendMessageW(hWnd,WM_COMMAND,1014,0);return 0;}
            if((wParam==VK_NUMPAD0 || wParam==VK_NUMPAD5) && !g_transport.Playing() && !g_editor.Visible()) {g_transport.Play();return 0;}
            if(wParam==VK_F6) {
                if(g_editor.Visible()) {
                    if(!g_editor.HideForPlayback()) return 0;
                    if(!g_editor.Visible()) {SyncEditorToPlayer();g_game.SetAutoPlay(g_mode==1031);g_editorPreview=false;}
                    return 0;
                }
                if(g_transport.SettingsVisible() && !g_transport.SaveSettings()) return 0;
                g_game.ReleaseKeys();g_transport.Pause();
                if(!g_transport.ApplyTiming()) return 0;
                g_editor.SetEmbedded(true);
                g_editor.SetSavedCallback([](const std::wstring& path){g_chartPath=path;});
                g_editor.SetLivePreview([](const std::vector<ChartNote>& notes,const SongInfo& info,double ms){
                    g_game.SetSessionChart(notes,info.bpm,info.NativeTickShift()+1);g_game.SetAutoPlay(true);
                    g_transport.Ready(true,g_chartPath);g_transport.Seek(ms);return g_transport.Play();
                });
                g_editor.Show(hWnd,g_chartPath,[](const std::wstring& path,double timeMs) {
                    if(g_game.LoadSong(path,g_selectedAudio)) {g_chartPath=path;g_selectedAudio=g_game.GetAudioPath();g_transport.Ready(true,g_chartPath);g_transport.Seek(timeMs);return g_transport.Play();}
                    else MessageBoxW(nullptr,L"Saved chart has no playable notes or cannot be read.",L"Preview",MB_OK|MB_ICONWARNING);
                    return false;
                },[hWnd] {g_mixer.Show(hWnd);},g_game.GetAudioPath(),[hWnd](int action){SendMessageW(hWnd,WM_COMMAND,action,0);},g_game.GetBpm(),g_game.GetNativeOffset());
                LayoutWorkspace(hWnd);return 0;
            }
            if(g_editor.Visible()) return 0;
            if (wParam == VK_ESCAPE) {
                SendMessageW(hWnd,WM_CLOSE,0,0);
                return 0;
            }
            if (wParam == VK_F11) {
                g_fullscreen = !g_fullscreen;
                if (g_fullscreen) {
                    GetWindowRect(hWnd, &g_windowedRect);
                    SetMenu(hWnd,nullptr);
                    SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                    MONITORINFO screen{sizeof(screen)};GetMonitorInfoW(MonitorFromWindow(hWnd,MONITOR_DEFAULTTONEAREST),&screen);
                    SetWindowPos(hWnd, HWND_TOP, screen.rcMonitor.left, screen.rcMonitor.top, screen.rcMonitor.right-screen.rcMonitor.left, screen.rcMonitor.bottom-screen.rcMonitor.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                } else {
                    SetMenu(hWnd,g_mainMenu);
                    SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
                    SetWindowPos(hWnd, HWND_TOP, g_windowedRect.left, g_windowedRect.top,
                                 g_windowedRect.right - g_windowedRect.left,
                                 g_windowedRect.bottom - g_windowedRect.top,
                                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                }
                return 0;
            }
            if (wParam == VK_F1) { g_game.SwitchKeyScheme(); return 0; }
            if (wParam == VK_F2) {SendMessageW(hWnd,WM_COMMAND,g_game.IsAutoPlay()?1030:1031,0);return 0;}
            if (wParam == VK_F3) { g_game.ToggleKeyAssist(); return 0; }
            if (wParam == VK_F4) { g_vsync = !g_vsync; return 0; }
            if (wParam == VK_F5) { SendMessageW(hWnd,WM_COMMAND,g_game.BackgroundImageEnabled()?1020:1021,0); return 0; }
            if (wParam == VK_TAB) { g_game.SwitchKeyScheme(); return 0; }
            if (wParam == 'R') { g_transport.Restart(); return 0; }

            if (wParam == VK_OEM_PLUS || wParam == VK_ADD || wParam == VK_UP) {
                g_game.SetSpeed(std::min<float>(3.0f, g_game.GetSpeed() + 0.1f));
                return 0;
            }
            if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT || wParam == VK_DOWN) {
                g_game.SetSpeed(std::max<float>(0.5f, g_game.GetSpeed() - 0.1f));
                return 0;
            }
            if(wParam==VK_LEFT || wParam==VK_RIGHT) {g_transport.Seek(g_transport.Position()+(wParam==VK_LEFT?-1:1)*60000.0/g_game.GetBpm());return 0;}

            if(g_transport.Playing()) {
                // Timestamp the queued event, removing time spent waiting for
                // rendering/Present before this message was dispatched.
                DWORD ageMs = GetTickCount() - static_cast<DWORD>(GetMessageTime());
                double eventTime = g_audio.GetTimeMs() - (ageMs < 1000 ? ageMs : 0);
                g_game.OnKeyDown(wParam, (std::max)(0.0, eventTime));
            }
            return 0;

        case WM_KEYUP:
            g_game.OnKeyUp(wParam);
            return 0;

        case WM_KILLFOCUS:
            g_game.ReleaseKeys();
            return 0;

        case WM_DROPFILES: {
            HDROP hDrop = reinterpret_cast<HDROP>(wParam);
            std::vector<std::wstring> files;
            UINT count=DragQueryFileW(hDrop,0xffffffff,nullptr,0);
            for(UINT i=0;i<count;++i) {UINT n=DragQueryFileW(hDrop,i,nullptr,0);std::wstring path(n+1,L'\0');DragQueryFileW(hDrop,i,path.data(),n+1);path.resize(n);files.push_back(path);}
            g_transport.Pause();
            if(!OpenSessionFiles(hWnd,files)) MessageBoxW(hWnd,L"Cannot open the selected song files.",L"Open song",MB_OK|MB_ICONWARNING);
            DragFinish(hDrop);
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_CLOSE:
            if(g_editor.CanClose()) DestroyWindow(hWnd);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static HMENU CreateMainMenu() {
    auto menu=CreateMenu(),fileMenu=CreatePopupMenu(),playMenu=CreatePopupMenu(),backgroundMenu=CreatePopupMenu(),modeMenu=CreatePopupMenu(),settingMenu=CreatePopupMenu();
    g_mainMenu=menu;
    AppendMenuW(fileMenu,MF_STRING,1042,L"Open song files…\tCtrl+O");
    AppendMenuW(fileMenu,MF_STRING,1043,L"New song…");
    AppendMenuW(fileMenu,MF_STRING,1001,L"Import SLK…");AppendMenuW(fileMenu,MF_STRING,1002,L"Choose music…");
    AppendMenuW(fileMenu,MF_STRING,1004,L"Chart editor\tF6");
    AppendMenuW(playMenu,MF_STRING,1014,L"Play / Pause\tF9");
    AppendMenuW(playMenu,MF_STRING,1005,L"Restart\tR");
    AppendMenuW(playMenu,MF_STRING|MF_CHECKED,1012,L"Toggle Menu On / Off\tF10");
    AppendMenuW(modeMenu,MF_STRING,1032,L"Editor / Classic\tF6");
    AppendMenuW(modeMenu,MF_STRING,1031,L"Autoplay");AppendMenuW(modeMenu,MF_STRING|MF_CHECKED,1030,L"Game");
    CheckMenuRadioItem(modeMenu,1030,1032,1030,MF_BYCOMMAND);
    AppendMenuW(settingMenu,MF_STRING,1015,L"Default Setting (BPM / Offset)");
    AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(fileMenu),L"File");
    AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(playMenu),L"Music Player");
    AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(modeMenu),L"Mode");
    AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(settingMenu),L"Setting");
    AppendMenuW(backgroundMenu,MF_STRING|MF_CHECKED,1020,L"Black");
    AppendMenuW(backgroundMenu,MF_STRING,1021,L"bg.dds");
    CheckMenuRadioItem(backgroundMenu,1020,1021,1020,MF_BYCOMMAND);
    AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(backgroundMenu),L"Background [F5]");

    return menu;
}

static bool GlobalShortcut(HWND owner,const MSG& msg) {
    if(msg.wParam!=VK_F10 || (msg.message!=WM_KEYDOWN && msg.message!=WM_SYSKEYDOWN && msg.message!=WM_KEYUP && msg.message!=WM_SYSKEYUP)) return false;
    if((msg.message==WM_KEYDOWN || msg.message==WM_SYSKEYDOWN) && !(msg.lParam&(1LL<<30))) SendMessageW(owner,WM_COMMAND,1012,0);
    return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int) {
    // Determine asset directory
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring dir = exePath;
    size_t lastSlash = dir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) dir = dir.substr(0, lastSlash);

    std::wstring assetDir = dir + L"/assets/hud_clean";
    DWORD attr = GetFileAttributesW(assetDir.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        assetDir = dir + L"/../assets/hud_clean";
    }

    // Register Window Class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hIcon=LoadIconW(hInstance,MAKEINTRESOURCEW(101));
    wc.hIconSm=wc.hIcon;
    wc.lpfnWndProc = WndProc;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"AuditionBeatUpSimulatorClass";
    RegisterClassEx(&wc);

    RECT frame{0,0,1024,768};
    AdjustWindowRectEx(&frame,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,TRUE,WS_EX_ACCEPTFILES);
    auto initial=InitialWindowRect(WorkArea(),frame.right-frame.left-1024,frame.bottom-frame.top-768);
    int winW=initial.right-initial.left,winH=initial.bottom-initial.top;
    int posX=initial.left,posY=initial.top;

    auto menu=CreateMainMenu();
    HWND hWnd = CreateWindowEx(
        WS_EX_ACCEPTFILES,
        wc.lpszClassName,
        L"Beat-Up Studio",
        WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        posX, posY, winW, winH,
        nullptr, menu, hInstance, nullptr
    );

    if (!hWnd) return 1;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // Initialize Subsystems
    RECT initialClient{};GetClientRect(hWnd,&initialClient);
    g_renderWindow=CreateRenderSurface(hWnd,initialClient.right,initialClient.bottom);
    if (!g_renderer.Init(g_renderWindow, initialClient.right, initialClient.bottom)) {
        MessageBoxW(hWnd, L"Failed to initialize Direct3D 11 Renderer!", L"Error", MB_ICONERROR);
        return 1;
    }

    if (!g_audio.Init()) {
        MessageBoxW(hWnd, L"Failed to initialize XAudio2 Audio Engine!", L"Error", MB_ICONERROR);
    }
    g_mixer.Load(&g_audio,dir+L"/BeatUp.settings.ini");

    g_game.SetTransportControlled(true);
    if (!g_game.Init(&g_renderer, &g_audio, assetDir)) {
        MessageBoxW(hWnd, L"Failed to initialize Beat-Up Game (check asset directory)!", L"Error", MB_ICONERROR);
    }

    int effects=g_audio.LoadEffects(dir+L"/SOUND");
    if(effects==0) g_audio.LoadEffects(dir+L"/../assets/sound");
    g_transport.Init(hWnd,&g_game,&g_audio,[hWnd](int action){SendMessageW(hWnd,WM_COMMAND,action,0);});
    // No implicit song and no automatic playback. An explicit command-line
    // chart selects a document only, still awaiting Play.
    std::wstring defaultSong;
    int argc=0; auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    for(int i=1;argv && i<argc;++i) if(std::filesystem::path(argv[i]).extension()==L".slk") defaultSong=argv[i];
    if(argv) LocalFree(argv);
    if (GetFileAttributesW(defaultSong.c_str()) != INVALID_FILE_ATTRIBUTES) {
        if(g_game.LoadSong(defaultSong)) {g_chartPath=std::filesystem::absolute(defaultSong).wstring();g_transport.Ready(true,g_chartPath);}
    }

    if (lpCmdLine && (wcsstr(lpCmdLine, L"--auto") || wcsstr(lpCmdLine, L"-auto"))) {
        g_game.ToggleAutoPlay();
    }

    g_transport.Embed();g_shell.Init(hWnd);g_uiReady=true;LayoutWorkspace(hWnd);

    // Precision game loop
    LARGE_INTEGER freq, lastTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);

    MSG msg = {};

    while (g_running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_running = false;
                break;
            }
            if(GlobalShortcut(hWnd,msg)) continue;
            if(g_editor.Accelerator(msg)) continue;
            if(g_transport.Accelerator(msg)) continue;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!g_running) break;

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double deltaMs = static_cast<double>(now.QuadPart - lastTime.QuadPart) * 1000.0 / freq.QuadPart;
        lastTime = now;

        // Current song time from audio clock
        if(g_editor.Visible() && g_transport.Playing() && !g_editorPreview) {
            g_savedAutoPlay=g_game.IsAutoPlay();g_game.SetAutoPlay(true);g_editorPreview=true;
        }
        static bool editorWasVisible=false;
        if(editorWasVisible && !g_editor.Visible()) {
            g_transport.Pause();SyncEditorToPlayer();g_game.SetAutoPlay(g_mode==1031);g_editorPreview=false;
        }
        editorWasVisible=g_editor.Visible();
        g_shell.SetMode(g_editor.Visible()?1032:g_mode);g_transport.SetMode(g_editor.Visible()?1032:g_mode);
        static bool lastBarVisible=true;static int lastBarHeight=0;
        if(lastBarVisible!=g_transport.Visible() || lastBarHeight!=g_transport.Height()) {LayoutWorkspace(hWnd);lastBarVisible=g_transport.Visible();lastBarHeight=g_transport.Height();}
        g_transport.Tick(deltaMs);
        g_editor.SetPlaybackTime(g_transport.Position(),g_transport.Playing());

        // GDI editor fills the same client area; do not present over its child controls.
        if(!g_editor.Visible()) {
            g_renderer.BeginFrame(0.0f, 0.0f, 0.0f, 1.0f);
            g_game.Render(g_transport.Started());
            
            if(g_transport.Started() && !g_transport.Playing()) g_renderer.DrawString("PAUSED - F9 / Pause to resume",330,270,22);
            g_renderer.EndFrame(g_vsync || !g_transport.Playing() || g_editor.Visible());
        }

        else Sleep(8);

    }

    g_renderer.Shutdown();
    g_audio.Shutdown();
    return 0;
}

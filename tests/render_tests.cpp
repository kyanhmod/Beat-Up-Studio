#include "Renderer.h"
#include "BeatUpGame.h"
#include "PlaybackPanel.h"
#include <commctrl.h>
#include <cmath>
#include <iostream>
#include <fstream>

int main() {
    WNDCLASSW cls{};
    cls.lpfnWndProc = DefWindowProcW;
    cls.hInstance = GetModuleHandleW(nullptr);
    cls.lpszClassName = L"BeatUpHiddenRenderTest";
    RegisterClassW(&cls);
    HWND window = CreateWindowW(cls.lpszClassName, L"Render test", WS_OVERLAPPEDWINDOW,
        0, 0, 1040, 807, nullptr, nullptr, cls.hInstance, nullptr);
    bool ok = false;
    {
        BeatUp::Renderer renderer;
        BeatUp::BeatUpGame game;
        if (window && renderer.Init(window, 1024, 768) && game.Init(&renderer, nullptr, L"assets/hud_clean") &&
            game.LoadSong(L"songs/v.slk")) {
            game.Update(100000, 0);
            renderer.BeginFrame();
            game.Render();
            ok = renderer.SaveFrameBitmap(L"tests/runtime/game.bmp");
            renderer.EndFrame();
            // Deterministic chart: simultaneous left/right/Space at 1 second.
            {
                std::ofstream f("tests/runtime/alignment.slk");
                f << "C;X1;Y1;K\"TIME\"\nC;X2;K\"LANE\"\nC;X3;K\"TYPE\"\n"
                     "C;X1;Y2;K1000\nC;X2;K7\nC;X3;K0\n"
                     "C;X1;Y3;K1000\nC;X2;K9\nC;X3;K0\n"
                     "C;X1;Y4;K1000\nC;X2;K5\nC;X3;K1\n";
            }
            ok = game.LoadSong(L"tests/runtime/alignment.slk") && ok;
            game.Update(500, 0);
            renderer.BeginFrame(); game.Render();
            ok = renderer.SaveFrameBitmap(L"tests/runtime/approach.bmp") && ok;
            renderer.EndFrame();
            game.Update(1000, 0);
            renderer.BeginFrame(); game.Render();
            ok = renderer.SaveFrameBitmap(L"tests/runtime/at_target.bmp") && ok;
            renderer.EndFrame();
            game.OnKeyDown('Q', 1000);
            game.OnKeyDown('P', 1000);
            game.OnKeyDown(VK_SPACE, 1000);
            ok = game.GetCombo() == 3 && game.GetScore() == 2800 &&
                 game.GetJudgeCounts()[BeatUp::JUDGE_PERFECT] == 3 && ok;
            renderer.BeginFrame(); game.Render();
            ok = renderer.SaveFrameBitmap(L"tests/runtime/hit.bmp") && ok;
            renderer.EndFrame();
            game.OnKeyUp('Q'); game.OnKeyUp('P'); game.OnKeyUp(VK_SPACE);
            game.Update(1100,100);
            renderer.BeginFrame(); game.Render();
            ok = renderer.SaveFrameBitmap(L"tests/runtime/hit_100ms.bmp") && ok;
            renderer.EndFrame();
            game.Update(1250,150);
            renderer.BeginFrame(); game.Render();
            ok = renderer.SaveFrameBitmap(L"tests/runtime/hit_250ms.bmp") && ok;
            renderer.EndFrame();
            game.Restart();
            game.Update(200,0);
            renderer.BeginFrame(); game.Render();
            ok = renderer.SaveFrameBitmap(L"tests/runtime/spawn_mask.bmp") && ok;
            renderer.EndFrame();
            game.Restart();
            game.Update(1100,0);
            game.OnKeyDown('Q',1100);
            ok = game.GetCombo() == 0 && game.GetJudgeCounts()[BeatUp::JUDGE_MISS] == 1 && ok;
            renderer.BeginFrame(); game.Render();
            ok = renderer.SaveFrameBitmap(L"tests/runtime/fade.bmp") && ok;
            renderer.EndFrame();
            game.Update(1320,220);
            ok = game.GetJudgeCounts()[BeatUp::JUDGE_MISS] == 3 && ok;
            game.Restart();
            game.ToggleAutoPlay();
            game.Update(1000, 0);
            ok = game.GetCombo() == 3 && game.GetJudgeCounts()[BeatUp::JUDGE_PERFECT] == 3 && ok;
            game.ToggleAutoPlay();
            {
                std::ofstream f("tests/runtime/order.slk");
                f << "C;X1;Y1;K\"TIME\"\nC;X2;K\"LANE\"\n"
                     "C;X1;Y2;K1000\nC;X2;K7\nC;X1;Y3;K1050\nC;X2;K7\n";
            }
            ok = game.LoadSong(L"tests/runtime/order.slk") && ok;
            game.OnKeyDown('Q',1050);
            ok = game.GetLastRating() == BeatUp::JUDGE_COOL && ok;
            game.OnKeyDown('Q',1050); // Repeated key-down must not consume the second note.
            ok = game.GetCombo() == 1 && ok;
            game.ReleaseKeys();
            game.OnKeyDown('Q',1050);
            ok = game.GetCombo() == 2 && game.GetLastRating() == BeatUp::JUDGE_PERFECT && ok;
            renderer.BeginFrame(0.1f, 0.1f, 0.1f);
            renderer.DrawSongTitle(L"Tên bài hát 한국어 (150 bpm)", 30, 30);
            ok = renderer.SaveFrameBitmap(L"tests/runtime/unicode.bmp") && ok;
            renderer.EndFrame();
            renderer.Resize(1280,720);
            renderer.BeginFrame(0.01f,0.01f,0.01f);
            game.Render();
            ok = renderer.SaveFrameBitmap(L"tests/runtime/widescreen.bmp") && ok;
            renderer.EndFrame();
            renderer.Resize(1024,768);
            {
                std::ofstream f("tests/runtime/gauge.slk");
                f << "C;X1;Y1;K\"TIME\"\nC;X2;K\"LANE\"\n";
                for (int i=0;i<401;++i)
                    f << "C;X1;Y" << i+2 << ";K" << 1000+i*100 << "\nC;X2;K7\n";
            }
            ok=game.LoadSong(L"tests/runtime/gauge.slk") && ok;
            game.ToggleAutoPlay();
            const int thresholds[] = {9,10,20,40,60,80,99,100,149,150,200,250,300,350,399,400};
            int snapshot=0;
            for (int combo=1;combo<=400;++combo) {
                game.Update(1000+(combo-1)*100,100);
                ok=game.GetCombo()==combo && ok;
                if (snapshot<16 && combo==thresholds[snapshot]) {
                    renderer.BeginFrame(); game.Render();
                    ok=renderer.SaveFrameBitmap(L"tests/runtime/combo_"+std::to_wstring(combo)+L".bmp") && ok;
                    renderer.EndFrame(); ++snapshot;
                }
            }
            ok=game.GetSpaceGauge().flame==2 && game.GetSpaceGauge().letters[5]==2 && ok;
            game.ToggleAutoPlay();
            game.Update(41000,100);
            renderer.BeginFrame(); game.Render();
            ok=renderer.SaveFrameBitmap(L"tests/runtime/combo_400_banner.bmp") && ok;
            renderer.EndFrame();
            game.Update(41400,400);
            ok=game.GetCombo()==0 && game.GetSpaceGauge().flame==0 && game.GetSpaceGauge().letters[0]==0 && ok;
            renderer.BeginFrame(); game.Render();
            ok=renderer.SaveFrameBitmap(L"tests/runtime/combo_after_miss.bmp") && ok;
            renderer.EndFrame();
            game.Restart();
            ok=game.GetCombo()==0 && game.GetSpaceGauge().flame==0 && ok;
            for (WPARAM key : {WPARAM(VK_NUMPAD0),WPARAM(VK_NUMPAD5),WPARAM(VK_SPACE)}) {
                ok=game.LoadSong(L"tests/runtime/alignment.slk") && ok;
                game.Update(1000,0);
                game.OnKeyDown(key,1000);
                game.OnKeyDown(key,1000);
                ok=game.GetCombo()==1 && game.GetScore()==2000 &&
                   game.GetJudgeCounts()[BeatUp::JUDGE_PERFECT]==1 && ok;
                game.OnKeyUp(key);
            }
            {
                std::ofstream f("tests/runtime/native_offset.slk");
                f << "C;X4;Y1;K\"Location\"\nC;X5;K\"Type\"\nC;X6;K\"Key\"\n"
                     "C;X4;Y3;K10\nC;X5;K\"s\"\nC;X6;K\"5\"\n";
                std::ofstream ini("tests/runtime/native_offset.song.ini");
                ini << "bpm=150\naudition_offset=0.7\n";
            }
            ok=game.LoadSong(L"tests/runtime/native_offset.slk") && ok;
            for (int attempt=0;attempt<2;++attempt) {
                game.Restart(); game.Update(970,0); game.OnKeyDown(VK_NUMPAD0,970);
                ok=game.GetScore()==2000 && game.GetLastRating()==BeatUp::JUDGE_PERFECT && ok;
                game.OnKeyUp(VK_NUMPAD0);
            }
            game.SetBpm(120); game.Restart(); game.Update(1212.5,0);
            game.OnKeyDown(VK_NUMPAD5,1212.5);
            ok=game.GetScore()==2000 && game.GetLastRating()==BeatUp::JUDGE_PERFECT && ok;
            ok=game.LoadSong(L"Sai Gon Night (Gucci Remix).slk") && game.GetBpm()==135 && ok;
            const double firstSpace=(144.0+0.7-1)*15000.0/135;
            game.Update(firstSpace,0);game.OnKeyDown(VK_NUMPAD0,firstSpace);
            ok=game.GetScore()==2000 && game.GetLastRating()==BeatUp::JUDGE_PERFECT && ok;
            {
                ok=game.LoadSong(L"Sai Gon Night (Gucci Remix).slk") && ok;
                const double first=game.GetFirstNoteMs(),tick=15000.0/135;
                const double times[]={first-30*tick,first-24*tick,first-14*tick,first-10*tick,first-6*tick};
                const wchar_t* names[]={L"ready",L"start",L"count3",L"count2",L"count1"};
                for(int i=0;i<5;++i) {game.Update(times[i],0);renderer.BeginFrame();game.Render();ok=renderer.SaveFrameBitmap(std::wstring(L"tests/runtime/")+names[i]+L".bmp") && ok;renderer.EndFrame();}
                ok=game.LoadSong(L"tests/runtime/gauge.slk") && ok;game.ToggleAutoPlay();
                for(int i=0;i<99;++i) game.Update(1000+i*100,100);
                game.ToggleAutoPlay();game.Update(11260,460);
                ok=game.GetCombo()==80 && game.GetSpaceGauge().letters[4]==0 && ok;
                game.Update(11360,100);ok=game.GetCombo()==60 && ok;
                auto score=game.GetScore();auto counts=game.GetJudgeCounts();
                game.Seek(18000);ok=game.GetCombo()==60 && game.GetScore()==score && game.GetJudgeCounts()==counts && ok;
                game.Seek(500);ok=game.GetCombo()==60 && game.GetScore()==score && game.GetJudgeCounts()==counts && ok;
                game.Restart();ok=game.GetCombo()==0 && game.GetScore()==0 && ok;
            }
            {
                std::ofstream f("tests/runtime/finish.slk");
                f<<"C;X4;Y1;K\"Location\"\nC;X5;K\"Type\"\nC;X6;K\"Key\"\n"
                   "C;X4;Y3;K10\nC;X5;K\"f\"\nC;X6;K\"\"\n";
                std::ofstream ini("tests/runtime/finish.song.ini");ini<<"bpm=150\n";
            }
            ok=game.LoadSong(L"tests/runtime/finish.slk") && ok;game.Update(1000,0);
            renderer.BeginFrame();game.Render();ok=renderer.SaveFrameBitmap(L"tests/runtime/finish_banner.bmp") && ok;renderer.EndFrame();
            game.Update(1100,100);renderer.BeginFrame();game.Render();ok=renderer.SaveFrameBitmap(L"tests/runtime/finish_blink.bmp") && ok;renderer.EndFrame();
            ok=game.GetCombo()==0 && game.GetJudgeCounts()[BeatUp::JUDGE_MISS]==0 && ok;
            {
                BeatUp::AudioEngine music;BeatUp::BeatUpGame musicGame;musicGame.SetTransportControlled(true);
                ok=music.Init() && musicGame.Init(&renderer,&music,L"assets/hud_clean") && ok;
                ok=musicGame.LoadSong(L"Sai Gon Night (Gucci Remix).slk") && ok;
                auto originalAudio=musicGame.GetAudioPath();auto duration=music.GetMusicDurationMs();
                ok=!musicGame.LoadAudio(L"tests/runtime/missing.ogg") && musicGame.GetAudioPath()==originalAudio && music.GetMusicDurationMs()==duration && ok;
                ok=musicGame.LoadAudio(L"songs/v.wav") && musicGame.GetAudioPath()==L"songs/v.wav" && musicGame.GetBpm()==135 && ok;
                BeatUp::PlaybackPanel panel;panel.Init(window,&musicGame,&music,[](int){});
                auto transport=FindWindowW(L"BeatUpPlaybackPanel",nullptr);ShowWindow(transport,SW_HIDE);
                ok=!panel.HasChart() && !panel.Playing() && ok;
                ok=IsWindowEnabled(GetDlgItem(transport,1010)) && ok;
                panel.Ready(true);Sleep(25);ok=!panel.Playing() && music.GetTimeMs()==0 && ok;
                auto musicGain=GetDlgItem(transport,2004),effectGain=GetDlgItem(transport,2005);
                SendMessageW(musicGain,TBM_SETPOS,TRUE,35);SendMessageW(transport,WM_HSCROLL,TB_THUMBPOSITION,reinterpret_cast<LPARAM>(musicGain));
                SendMessageW(effectGain,TBM_SETPOS,TRUE,20);SendMessageW(transport,WM_HSCROLL,TB_THUMBPOSITION,reinterpret_cast<LPARAM>(effectGain));
                ok=music.GetMusicVolume()==.35f && music.GetEffectVolume()==.2f && ok;
                panel.Seek(10000);ok=std::abs(panel.Position()-10000)<1 && !panel.Playing() && ok;
                ok=panel.Play() && ok;Sleep(35);panel.Tick(35);ok=panel.Position()>10010 && ok;
                SendMessageW(transport,WM_COMMAND,1010,0);ok=!panel.Playing() && ok;
                double hold=panel.Position();Sleep(30);panel.Tick(30);ok=std::abs(panel.Position()-hold)<.1 && ok;
                SendMessageW(transport,WM_COMMAND,1010,0);ok=panel.Playing() && ok;
                panel.Pause();panel.Seek(5000);ok=panel.Play() && ok;Sleep(35);panel.Tick(35);ok=panel.Position()>5010 && panel.Position()<5200 && ok;
                panel.Restart();Sleep(20);panel.Tick(20);ok=panel.Position()<200 && panel.Playing() && ok;
                auto seek=GetDlgItem(transport,2003);SendMessageW(seek,TBM_SETPOS,TRUE,25000);
                SendMessageW(transport,WM_HSCROLL,TB_THUMBTRACK,reinterpret_cast<LPARAM>(seek));
                ok=!panel.Playing() && std::abs(panel.Position()-music.GetMusicDurationMs()*.25)<1 && ok;
                SendMessageW(transport,WM_HSCROLL,TB_ENDTRACK,reinterpret_cast<LPARAM>(seek));Sleep(25);panel.Tick(25);
                ok=panel.Playing() && panel.Position()>music.GetMusicDurationMs()*.25 && ok;
                panel.Pause();DestroyWindow(transport);
            }
        }
    }
    if (window) DestroyWindow(window);
    std::cout << (ok ? "PASS: render, collision alignment, manual judgement, input order, key repeat, autoplay\n"
                    : "FAIL: render/gameplay regression\n");
    return ok ? 0 : 1;
}

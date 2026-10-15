#include "AudioEngine.h"
#include "SongInfo.h"
#include "SlkParser.h"
#include "BeatUpRules.h"
#include "SpaceGauge.h"
#include "KeyBindings.h"
#include "FinishVisual.h"
#include "NativeSequence.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <stdexcept>

void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
int main() {
    try {
        namespace fs = std::filesystem;
        int level=BeatUp::ComboLevel(99);
        Check(BeatUp::ComboMiss(level)==80 && BeatUp::ComboMiss(level)==60 && BeatUp::ComboMiss(level)==40,"Native miss drops gauge tiers");
        level=BeatUp::ComboLevel(10);Check(BeatUp::ComboMiss(level)==10 && level==0 && BeatUp::ComboMiss(level)==0,"Native 10-combo consecutive miss state");
        level=BeatUp::ComboLevel(100);Check(BeatUp::ComboMiss(level)==0 && level==0,"Native full orange gauge miss resets");
        Check(BeatUp::NativeIntro(0,10000,150).stage==0,"No READY during long song lead-in");
        Check(BeatUp::NativeIntro(5200,10000,150).stage==1 && BeatUp::NativeIntro(6800,10000,150).stage==2,"READY and START each retain four beats before countdown");
        Check(BeatUp::NativeIntro(8500,10000,150).count==3 && BeatUp::NativeIntro(8900,10000,150).count==2 && BeatUp::NativeIntro(9300,10000,150).count==1,"Native 3 2 1 tick schedule");
        Check(BeatUp::NativeIntro(8450,10000,150).count==0,"Countdown follows native integer ticks rather than continuous interpolation");
        Check(BeatUp::NativeIntro(10000,10000,150).stage==0,"Native intro finishes at first event");
        Check(!BeatUp::GetFinishVisual(899,1000,150).visible,"Finish not visible before entry");
        Check(BeatUp::GetFinishVisual(900,1000,150).visible && BeatUp::GetFinishVisual(900,1000,150).x==8,"Finish enters one tick before f");
        Check(BeatUp::GetFinishVisual(1000,1000,150).visible && !BeatUp::GetFinishVisual(1100,1000,150).visible && BeatUp::GetFinishVisual(1200,1000,150).visible,"Finish blink schedule");
        Check(!BeatUp::GetFinishVisual(1700,1000,150).visible,"Finish expires after eight ticks");
        Check(BeatUp::NormalizeKeypadKey(VK_INSERT,0x52LL<<16)==VK_NUMPAD0,"NumLock off keypad 0");
        Check(BeatUp::NormalizeKeypadKey(VK_CLEAR,0x4cLL<<16)==VK_NUMPAD5,"NumLock off keypad 5");
        Check(BeatUp::NormalizeKeypadKey(VK_NUMPAD0,0x52LL<<16)==VK_NUMPAD0,"NumLock on keypad 0");
        Check(BeatUp::NormalizeKeypadKey(VK_INSERT,(0x52LL<<16)|(1LL<<24))==VK_INSERT,"Extended Insert unchanged");
        const int thresholds1[] = {10,20,40,60,80,100};
        const int thresholds2[] = {150,200,250,300,350,400};
        for (int i=0;i<6;++i) {
            Check(BeatUp::GetSpaceGaugeVisual(thresholds1[i]-1).letters[i]==0, "Orange cell not lit early");
            Check(BeatUp::GetSpaceGaugeVisual(thresholds1[i]).letters[i]==1, "Orange cell exact threshold");
            Check(BeatUp::GetSpaceGaugeVisual(thresholds2[i]-1).letters[i]==1, "Blue cell not lit early");
            Check(BeatUp::GetSpaceGaugeVisual(thresholds2[i]).letters[i]==2, "Blue cell exact threshold");
        }
        Check(BeatUp::GetSpaceGaugeVisual(99).flame==0 && BeatUp::GetSpaceGaugeVisual(100).flame==1 &&
              BeatUp::GetSpaceGaugeVisual(399).flame==1 && BeatUp::GetSpaceGaugeVisual(400).flame==2,
              "Space flames at 100 and 400");
        Check(BeatUp::GetSpaceGaugeVisual(99).digitBank==0 && BeatUp::GetSpaceGaugeVisual(100).digitBank==1 &&
              BeatUp::GetSpaceGaugeVisual(200).digitBank==2,"Combo digit banks");
        BeatUp::SpaceMilestone milestone;
        milestone.Observe(99,1000); Check(milestone.Level(1000,150)==0,"No premature milestone");
        milestone.Observe(100,1100); Check(milestone.Level(1100,150)==1,"100 combo banner");
        milestone.Observe(101,1200); Check(milestone.Age(1200)==100,"Banner does not restart every hit");
        Check(milestone.Level(1900,150)==0,"Milestone expires after eight ticks");
        milestone.Observe(400,2000); Check(milestone.Level(2100,150)==2,"400 combo banner");
        milestone.Observe(0,2200); Check(milestone.Level(2200,150)==0,"Miss clears banner");
        milestone.Observe(100,2300); Check(milestone.Level(2300,150)==1,"Milestone retriggers on a new chain");
        using namespace BeatUp::Rules;
        Check(std::abs(DirectionSpeed(150) - 0.405) < 1e-9, "Native directional travel speed at 150 BPM");
        Check(std::abs(SpaceSpeed(150) - 0.1675) < 1e-9, "Native Space travel speed at 150 BPM");
        Check(NoteX(1000,1000,150,1,true) == 319 && NoteX(1000,1000,150,1,false) == 705,
              "Hit timestamp aligns arrow centers with receptors");
        Check(JudgeDirection(5)==0 && JudgeDirection(6)==1 && JudgeDirection(15)==1 &&
              JudgeDirection(16)==2 && JudgeDirection(27)==2 && JudgeDirection(28)==3 &&
              JudgeDirection(40)==3 && JudgeDirection(41)==4 && JudgeDirection(56)==-1 &&
              JudgeDirection(-56)==4, "OldVTC directional pixel boundaries");
        Check(JudgeSpace(3)==0 && JudgeSpace(4)==1 && JudgeSpace(-8)==1 && JudgeSpace(-9)==4 &&
              JudgeSpace(20)==2 && JudgeSpace(32)==3 && JudgeSpace(45)==4 && JudgeSpace(46)==-1,
              "Space has separate asymmetric boundaries");
        Check(!Expired(1000,1047,150,1,5) && Expired(1000,1054,150,1,5), "Space expiry is distinct from directions");
        Check(!Expired(1000,1100,150,1,7) && Expired(1000,1320,150,1,7), "Directional miss follows fade lifecycle");
        fs::create_directories("tests/runtime");
        auto chart = fs::absolute(L"tests/runtime/Tên bài hát.slk");
        auto ini = chart; ini.replace_extension(L".song.ini");
        {
            std::ofstream f(ini, std::ios::binary);
            f << u8"[song]\ntitle=Tên bài hát 한국어\nartist=Nghệ sĩ\nbpm=123.5\n"
                 "offset_ms=-42.5\naudio=music.ogg\n";
        }
        auto info = BeatUp::ReadSongInfo(chart.wstring(), 150);
        Check(info.title == L"Tên bài hát 한국어", "Unicode metadata");
        Check(info.artist == L"Nghệ sĩ", "Unicode artist");
        Check(info.bpm == 123.5 && info.offsetMs == -42.5, "BPM and offset");
        Check(fs::path(info.audioPath) == chart.parent_path() / L"music.ogg", "Relative audio path");
        {
            std::ofstream f(ini); f << "bpm=nan\noffset_ms=inf\nbpm=90garbage\n";
        }
        info = BeatUp::ReadSongInfo(chart.wstring(), 120);
        Check(info.bpm == 120 && info.offsetMs == 0, "Invalid metadata must preserve defaults");
        Check(info.title == chart.stem().wstring(), "Unicode filename fallback");
        Check(!info.hasAuditionOffset && info.ChartShiftMs()==0,"Missing native offset preserves accepted timing");
        {
            std::ofstream f(ini); f << "bpm=150\naudition_offset=0.7\noffset_ms=5\n";
        }
        info=BeatUp::ReadSongInfo(chart.wstring(),120);
        Check(info.hasAuditionOffset && std::abs(info.ChartShiftMs()+25)<1e-8,"Native clock phase and fine offset combine once");
        info.bpm=120;
        Check(std::abs(info.ChartShiftMs()+32.5)<1e-8,"Native offset scales with tick duration");

        BeatUp::AudioEngine audio;
        Check(audio.Init(), "XAudio initialization");
        Check(audio.LoadEffects(L"bin/SOUND")==6,"All six real SOUND effects decode and resample");
        audio.SetMusicVolume(.25f);audio.SetEffectVolume(.1f);
        Check(audio.GetMusicVolume()==.25f && audio.GetEffectVolume()==.1f,"Separate music/effect gains");
        audio.SetEffectVolume(-1);audio.SetMusicVolume(2);
        Check(audio.GetMusicVolume()==1 && audio.GetEffectVolume()==0,"Mixer values clamp");
        Check(audio.LoadMusic(L"songs/v.wav"), "Load existing WAV");
        double expected = (fs::file_size("songs/v.wav") - 44) / 176.4;
        Check(std::abs(audio.GetMusicDurationMs() - expected) < 1, "WAV duration from PCM");
        audio.PlayMusic(); Sleep(90);
        double before = audio.GetTimeMs();
        Check(before > 20 && before < 1000, "Playback clock advances");
        audio.SetMusicVolume(.5f);audio.SetEffectVolume(.3f);
        Check(audio.GetTimeMs()>=before,"Volume changes preserve running audio clock");
        int advancing = 0;
        double previous = audio.GetTimeMs();
        for (int i = 0; i < 30; ++i) {
            Sleep(1);
            double next = audio.GetTimeMs();
            Check(next >= previous, "Interpolated playback clock cannot move backward");
            if (next > previous) ++advancing;
            previous = next;
        }
        Check(advancing >= 15, "Playback advances between audio processing updates");
        audio.PlayMusic(); Sleep(30);
        Check(audio.GetMusicVolume()==.5f && audio.GetEffectVolume()==.3f,"Mixer levels survive restart");
        double after = audio.GetTimeMs();
        Check(after < before && after < 150, "Restart clock resets rather than accumulates");
        audio.SeekMusic(5000,false);double held=audio.GetTimeMs();Sleep(30);
        Check(std::abs(held-5000)<1 && std::abs(audio.GetTimeMs()-held)<.1,"Seek holds music clock while paused");
        audio.ResumeMusic();Sleep(40);Check(audio.GetTimeMs()>held+10,"Resume advances from seek position");
        audio.PauseMusic();held=audio.GetTimeMs();Sleep(30);Check(std::abs(audio.GetTimeMs()-held)<.1,"Pause freezes clock");
        audio.SeekMusic(1000,true);Sleep(30);Check(audio.GetTimeMs()>1000 && audio.GetTimeMs()<1200,"Backward seek resets monotonic origin");
        audio.PlayMusic();Sleep(20);Check(audio.GetTimeMs()<150,"Ordinary restart clears seek origin");
        {
            std::ofstream f("tests/runtime/broken.wav", std::ios::binary);
            const char broken[] = "RIFF\0\0\0\0WAVEdata\xff\xff\xff\x7f";
            f.write(broken, sizeof(broken));
        }
        Check(!audio.LoadMusic(L"tests/runtime/broken.wav"), "Truncated WAV rejected");
        Check(audio.IsMusicLoaded(), "Failed audio load preserves previous track");
        Check(audio.LoadMusic(L"Dj Taka - V -conclusion- 150 BPM.ogg"), "Load existing OGG");
        Check(audio.GetMusicDurationMs() > 100000 && audio.GetMusicDurationMs() < 300000, "OGG decoded duration");
        std::cout << "OGG duration: " << audio.GetMusicDurationMs() << " ms\n";
        Check(audio.LoadMusic(L"Sai Gon Night (Gucci Remix).ogg"),"Decode user editor test song");
        Check(audio.GetMusicDurationMs()>60000,"Sai Gon music duration available");
        std::cout << "Sai Gon Night duration: " << audio.GetMusicDurationMs() << " ms\n";
        audio.UnloadMusic();
        Check(!audio.IsMusicLoaded() && audio.GetMusicDurationMs() == 0, "Unload clears song state");
        audio.ResetClock(); Sleep(20);
        Check(audio.GetTimeMs() >= 10 && audio.GetTimeMs() < 1000, "Silent chart clock resets");
        std::vector<BeatUp::ChartNote> notes;
        Check(BeatUp::SlkParser::LoadChart(L"songs/demo_chart.slk", notes, 120), "Demo chart loads");
        Check(notes.size() == 14 && notes.front().timeMs == 1000, "Demo preserves first row and absolute time");
        Check(BeatUp::SlkParser::LoadChart(L"songs/v.slk", notes, 120), "Native chart loads");
        auto first = notes.front().timeMs;
        Check(first == 12000, "Native location is a sixteenth-note tick");
        Check(BeatUp::SlkParser::LoadChart(L"songs/v.slk", notes, 240), "Native chart loads at second BPM");
        Check(std::abs(notes.front().timeMs * 2 - first) < 0.001, "BPM scales chart timing");
        {
            std::ofstream f("tests/runtime/broken.slk");
            f << "C;Xoops;Y1;K1\n";
        }
        auto count = notes.size();
        Check(!BeatUp::SlkParser::LoadChart(L"tests/runtime/broken.slk", notes), "Malformed chart rejected without throwing");
        Check(notes.size() == count, "Failed chart load preserves previous notes");
        {
            std::ofstream f("tests/runtime/combined.slk");
            f << "C;X4;Y1;K\"Location\"\nC;X5;K\"Type\"\nC;X6;K\"Key\"\n"
                 "C;Y3;X4;K16\nC;X5;K\"n,s,f\"\nC;X6;K\"7\"\n"
                 "C;Y4;X4;K32\nC;X5;K\"f\"\nC;X6;K\"\"\n";
        }
        Check(BeatUp::SlkParser::LoadChart(L"tests/runtime/combined.slk", notes, 120), "Combined native type flags load");
        Check(notes.size() == 3 && notes[0].lane == 7 && notes[1].lane == 5 && notes[2].lane == 0,
              "Normal/Space/finish event remain distinct");
        Check(notes[0].finish && notes[1].finish && notes[2].finish, "Finish flags preserved");
        std::cout << "PASS: metadata, WAV/OGG, restart clock, malformed audio, silent clock, chart timing\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n'; return 1;
    }
}

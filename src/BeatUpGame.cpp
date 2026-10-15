#include "BeatUpGame.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include "SongInfo.h"
#include "BeatUpRules.h"
#include "FinishVisual.h"
#include "NativeSequence.h"

namespace BeatUp {

BeatUpGame::BeatUpGame() {
    SetupLanes();
}

BeatUpGame::~BeatUpGame() {
    m_texChancePrompt.Release();
    m_texBackground.Release();
    m_texTrackL3.Release();
    m_texTrackL1.Release();
    m_texTrackL2.Release();
    m_texTrackR2.Release();
    m_texTrackR1.Release();
    m_texTrackR3.Release();

    m_texSpaceBar.Release();
    m_texSpaceNote.Release();
    m_texSpaceHit.Release();
    m_texChanceBtn.Release();m_texChanceActive.Release();
    m_texFinish.Release();
    m_texReady.Release();m_texStart.Release();for(auto& tex:m_texCountdown) tex.Release();
    for (auto& tex : m_texGaugeFlames) tex.Release();
    for (auto& tex : m_texGaugeLetterGlows) tex.Release();
    for (auto& tex : m_texGaugeLetters) tex.Release();
    for (auto& tex : m_texMilestoneBanners) tex.Release();
    for (auto& tex : m_texGaugeUp) tex.Release();

    m_texSongBase.Release();
    m_texSongFeather.Release();
    m_texSongBlank.Release();
    m_texSongFull.Release();
    m_texSongBar.Release();

    m_texHitEffect.Release();

    for (int l = 0; l < 10; ++l) {
        m_texWaves[l].Release();
        for (int f = 0; f < 4; ++f) {
            m_texArrowsNormal[l][f].Release();
            m_texArrowsChance[l][f].Release();
        }
    }
    for (int i = 0; i < 6; ++i) m_texJudges[i].Release();
    for (int i = 0; i < 10; ++i) {
        m_texComboDigits[i].Release();
        m_texComboDigitsYellow[i].Release();
        m_texComboDigitsPurple[i].Release();
    }
}

void BeatUpGame::SetupLanes() {
    // Centers of the native-sized 64x64 arrows in the supplied legacy skin.
    m_lanes[7] = {7, -32, 319, 455, true};
    m_lanes[4] = {4, -32, 319, 519, true};
    m_lanes[1] = {1, -32, 319, 583, true};
    m_lanes[9] = {9, 1056, 705, 455, false};
    m_lanes[6] = {6, 1056, 705, 519, false};
    m_lanes[3] = {3, 1056, 705, 583, false};
    m_lanes[5] = {5, 0, 512, 656, true};
}

bool BeatUpGame::Init(Renderer* renderer, AudioEngine* audio, const std::wstring& assetDir) {
    m_renderer = renderer;
    m_audio = audio;
    m_assetDir = assetDir;

    auto LoadTex = [this](const std::wstring& name, TextureResource& res) {
        std::wstring fullPath = m_assetDir + L"/" + name;
        return DDSLoader::LoadFromFile(m_renderer->GetDevice(), fullPath, res);
    };

    // 1. Stage Background
    LoadTex(L"bg.dds", m_texBackground);
    LoadTex(L"press chance key.dds",m_texChancePrompt);

    // 2. Dual-Box Track Panels
    LoadTex(L"track_l3.dds", m_texTrackL3); // Far left cap
    LoadTex(L"track_l1.dds", m_texTrackL1); // Left lanes
    LoadTex(L"track_l2.dds", m_texTrackL2); // Left receptor target boxes

    LoadTex(L"track_r2.dds", m_texTrackR2); // Right receptor target boxes
    LoadTex(L"track_r1.dds", m_texTrackR1); // Right lanes
    LoadTex(L"track_r3.dds", m_texTrackR3); // Far right cap

    // 3. Bottom Beat-Up Bar & Chance
    LoadTex(L"spacebar.dds", m_texSpaceBar);
    LoadTex(L"space.dds",    m_texSpaceNote);
    LoadTex(L"spower.dds",   m_texSpaceHit);
    LoadTex(L"crazy.dds",    m_texChanceBtn);
    LoadTex(L"chance_active.dds",m_texChanceActive);
    LoadTex(L"finishmove.dds",m_texFinish);
    LoadTex(L"getready.dds",m_texReady);LoadTex(L"start.dds",m_texStart);
    for(int i=0;i<3;++i) LoadTex(L"countdown_"+std::to_wstring(i+1)+L".dds",m_texCountdown[i]);
    LoadTex(L"gauge_flame_orange.dds", m_texGaugeFlames[0]);
    LoadTex(L"gauge_flame_blue.dds", m_texGaugeFlames[1]);
    LoadTex(L"gauge_letter_orange.dds", m_texGaugeLetterGlows[0]);
    LoadTex(L"gauge_letter_blue.dds", m_texGaugeLetterGlows[1]);
    const wchar_t* letterNames[] = {L"b",L"e",L"a",L"t",L"u",L"p"};
    for (int i=0;i<6;++i) LoadTex(std::wstring(L"gauge_letter_")+letterNames[i]+L".dds",m_texGaugeLetters[i]);
    LoadTex(L"beatup.dds",m_texMilestoneBanners[0]);
    LoadTex(L"beatup_1.dds",m_texMilestoneBanners[1]);
    LoadTex(L"up.dds",m_texGaugeUp[0]);
    LoadTex(L"gauge_up_blue.dds",m_texGaugeUp[1]);

    // 4. Song Info Bar (Bottom-Left)
    LoadTex(L"song_info_base.dds",    m_texSongBase);
    LoadTex(L"song_info_feather.dds", m_texSongFeather);
    LoadTex(L"song_info_blank.dds",   m_texSongBlank);
    LoadTex(L"song_info_full.dds",    m_texSongFull);
    LoadTex(L"song_info_bar.dds",     m_texSongBar);

    // 5. Hit spark
    LoadTex(L"hit_effect.dds", m_texHitEffect);

    // 6. 4-frame animated arrow sprites & hit wave pulses
    const int laneIds[] = { 1, 3, 4, 6, 7, 9 };
    for (int l : laneIds) {
        for (int f = 1; f <= 4; ++f) {
            std::wstring normName = L"a" + std::to_wstring(l) + std::to_wstring(f) + L".dds";
            std::wstring chncName = L"c" + std::to_wstring(l) + std::to_wstring(f) + L".dds";
            LoadTex(normName, m_texArrowsNormal[l][f - 1]);
            LoadTex(chncName, m_texArrowsChance[l][f - 1]);
        }
        std::wstring waveName = L"wave_" + std::to_wstring(l) + L".dds";
        LoadTex(waveName, m_texWaves[l]);
    }

    // 7. Judgements
    LoadTex(L"perfect.dds", m_texJudges[JUDGE_PERFECT]);
    LoadTex(L"great.dds",   m_texJudges[JUDGE_GREAT]);
    LoadTex(L"cool.dds",    m_texJudges[JUDGE_COOL]);
    LoadTex(L"bad.dds",     m_texJudges[JUDGE_BAD]);
    LoadTex(L"miss.dds",    m_texJudges[JUDGE_MISS]);

    // 8. Combo digits
    for (int i = 0; i < 10; ++i) {
        std::wstring digitName = L"combo_" + std::to_wstring(i) + L".dds";
        LoadTex(digitName, m_texComboDigits[i]);
        LoadTex(L"combo_yellow_"+std::to_wstring(i)+L".dds", m_texComboDigitsYellow[i]);
        LoadTex(L"combo_purple_"+std::to_wstring(i)+L".dds", m_texComboDigitsPurple[i]);
    }

    Restart();
    return true;
}

void BeatUpGame::SetBpm(double bpm) {
    if (bpm > 30.0 && bpm < 400.0) {
        m_bpm = bpm;
        if (m_usingCustomChart) {
            for (auto& note : m_notes) if (note.tick >= 0.0)
                note.timeMs = (note.tick+m_chartTickShift) * 60000.0 / (bpm * 4.0) + m_chartOffsetMs;
            if (!m_audio || !m_audio->IsMusicLoaded())
                m_songTotalDurationMs = (std::max)(0.0, (m_notes.empty()?0.0:m_notes.back().timeMs) + 150.0);
            Restart();
        }
    }
}

void BeatUpGame::SwitchKeyScheme() {
    ReleaseKeys();
    m_useNumpadScheme = !m_useNumpadScheme;
}

void BeatUpGame::Restart() {
    m_currentTimeMs = 0.0;
    m_lastBeatIndex = -1;
    m_score = 0;
    m_combo = 0;
    m_manualSeekGraceUntil=-1;
    m_comboLevel=0;m_introStage=0;m_chanceFinish=-1;m_chancePromptSlide={};
    m_maxCombo = 0;
    m_lastRating = JUDGE_NONE;
    m_judgeTimer = 0.0;
    m_judgeCounts.fill(0);
    m_activeWaves.clear();
    m_activeSparks.clear();
    m_keyPulseMs.fill(0.0);
    m_spaceMilestone.Reset();
    for (int i = 0; i < 10; ++i) m_keyHeld[i] = false;

    for (auto& note : m_notes) {
        note.hit = false;
        note.missed = false;
        note.fading = false;note.chance=false;
    }

    if (m_audio) {
        m_audio->StopEffects();
        m_audio->StopMusic();
        m_audio->ResetClock();
        if(m_autoStart) m_audio->PlayMusic();
    }
}

bool BeatUpGame::LoadSong(const std::wstring& slkPath, const std::wstring& wavPath) {
    std::vector<ChartNote> parsedNotes;
    const SongInfo info = ReadSongInfo(slkPath, m_bpm);

    if (SlkParser::LoadChart(slkPath, parsedNotes, info.bpm,true)) {
        for (auto& note : parsedNotes) note.timeMs += note.tick >= 0.0 ? info.ChartShiftMs() : info.offsetMs;
        m_notes = std::move(parsedNotes);
        m_bpm = info.bpm;
        m_usingCustomChart = true;
        m_chartOffsetMs = info.offsetMs;
        m_chartTickShift = info.NativeTickShift();

        m_currentSongWav.clear();
        if (m_audio) {
            m_audio->UnloadMusic();
            std::vector<std::wstring> candidates;
            if (!wavPath.empty()) candidates.push_back(wavPath);
            else if (!info.audioPath.empty()) candidates.push_back(info.audioPath);
            else {
                std::filesystem::path audio(slkPath);
                candidates.push_back(audio.replace_extension(L".ogg").wstring());
                candidates.push_back(audio.replace_extension(L".wav").wstring());
            }
            for (const auto& candidate : candidates) {
                if (m_audio->LoadMusic(candidate)) { m_currentSongWav = candidate; break; }
            }
        }
        m_songTitle = info.artist.empty() ? info.title : info.artist + L" - " + info.title;
        m_songTotalDurationMs = m_audio ? m_audio->GetMusicDurationMs() : 0.0;
        if (m_songTotalDurationMs <= 0.0) m_songTotalDurationMs = (std::max)(0.0, (m_notes.empty()?0.0:m_notes.back().timeMs) + 150.0);

        Restart();
        return true;
    }
    return false;
}

bool BeatUpGame::ChanceAvailable(double now) const {
    if(m_score<=5000 || m_chanceFinish>=0) return false;
    double tick=15000.0/m_bpm;
    int measure=static_cast<int>(std::floor((now-m_chartOffsetMs)/tick-m_chartTickShift))/16;
    for(const auto& n:m_notes) if(n.finish && n.tick>=0) {
        int finish=static_cast<int>(n.tick)/16;
        if(measure>=finish-12 && measure<finish-10) return true;
    }
    return false;
}
bool BeatUpGame::ActivateChance(double now) {
    if(!ChanceAvailable(now)) return false;
    double tick=15000.0/m_bpm;
    int measure=static_cast<int>(std::floor((now-m_chartOffsetMs)/tick-m_chartTickShift))/16;
    for(const auto& n:m_notes) if(n.finish && n.tick>=0 && measure>=static_cast<int>(n.tick)/16-12 && measure<static_cast<int>(n.tick)/16-10) {m_chanceFinish=n.tick;break;}
    for(size_t i=0;i<m_notes.size();++i) {
        auto& n=m_notes[i];
        if(n.lane!=0 && n.lane!=5 && n.tick>=m_chanceFinish-160 && n.tick<m_chanceFinish && n.timeMs>now)
            n.chance=((static_cast<unsigned>(i)*2654435761u)>>31)!=0;
    }
    return true;
}
void BeatUpGame::SetSessionChart(std::vector<ChartNote> notes,double bpm,double offset) {
    // Re-entering player with an unchanged document is not a new chart session.
    if(m_usingCustomChart && bpm==m_bpm && offset==m_chartTickShift+1 && notes.size()==m_notes.size() &&
       std::equal(notes.begin(),notes.end(),m_notes.begin(),[](const ChartNote& a,const ChartNote& b){return a.tick==b.tick && a.lane==b.lane && a.finish==b.finish && a.type==b.type;})) return;
    m_notes=std::move(notes);m_usingCustomChart=true;m_chartOffsetMs=0;
    m_chartTickShift=offset-1;SetBpm(bpm);
}
int BeatUpGame::GetAllCombo() const {
    return static_cast<int>(std::count_if(m_notes.begin(),m_notes.end(),[](const ChartNote& n){return n.lane!=0;}));
}
bool BeatUpGame::NewSong(const std::wstring& path) {
    if(!LoadAudio(path)) return false;
    m_notes.clear();m_usingCustomChart=true;m_chartOffsetMs=0;m_chartTickShift=-0.3;
    m_songTitle=std::filesystem::path(path).stem().wstring();Restart();return true;
}
bool BeatUpGame::LoadAudio(const std::wstring& path) {
    if(!m_audio || !m_audio->LoadMusic(path)) return false;
    m_currentSongWav=path;m_songTotalDurationMs=m_audio->GetMusicDurationMs();Restart();return true;
}
double BeatUpGame::GetFirstNoteMs() const {
    for(const auto& note:m_notes) if(note.lane) return note.timeMs;
    return -1;
}
void BeatUpGame::Seek(double timeMs,bool resetStats) {
    if(resetStats) Restart();
    m_currentTimeMs=(std::max)(0.0,timeMs);
    // Notes already travelling when manually scrubbing cannot be read for their
    // full approach. They remain hittable, but skipping them is not a new miss.
    m_manualSeekGraceUntil=timeMs>0 && !m_autoPlay ? m_currentTimeMs+(351.0/324.0)*8*15000.0/(m_bpm*m_speed) : -1;
    m_lastBeatIndex=static_cast<int>(m_currentTimeMs*m_bpm/60000.0);
    m_chanceFinish=-1;m_chancePromptSlide={};
    m_introStage=NativeIntro(m_currentTimeMs,GetFirstNoteMs(),m_bpm).stage;
    ReleaseKeys();m_activeWaves.clear();m_activeSparks.clear();m_keyPulseMs.fill(0);
    m_spaceMilestone.Reset();
    for(auto& note:m_notes) {
        note.hit=note.timeMs<m_currentTimeMs;note.missed=false;note.fading=false;note.chance=false;
    }
}
void BeatUpGame::ConfigureTiming(double bpm,double nativeOffset) {
    if(!std::isfinite(bpm) || !std::isfinite(nativeOffset) || bpm<=30 || bpm>=400) return;
    m_chartTickShift=nativeOffset-1.0;SetBpm(bpm);
}

void BeatUpGame::Update(double currentTimeMs, double deltaMs) {
    m_currentTimeMs = currentTimeMs;
    if(m_chanceFinish>=0 && (currentTimeMs-m_chartOffsetMs)/(15000.0/m_bpm)-m_chartTickShift>=m_chanceFinish) m_chanceFinish=-1;
    m_chancePromptSlide.Update(ChanceAvailable(currentTimeMs),deltaMs);
    auto intro=NativeIntro(currentTimeMs,GetFirstNoteMs(),m_bpm);
    if(intro.stage!=m_introStage) {
        m_introStage=intro.stage;
        if(m_audio && intro.stage==2) m_audio->PlaySound(SOUND_READY);
        if(m_audio && intro.stage==3) m_audio->PlaySound(SOUND_START);
    }
    for (auto& note : m_notes) {
        if (note.lane == 0 && note.timeMs <= currentTimeMs) note.hit = true;
    }
    m_approachTimeMs = 1000.0 / m_speed;
    for (auto& pulse : m_keyPulseMs) pulse = (std::max)(0.0, pulse - deltaMs);

    // Metronome pulse to song BPM
    double beatMs = 60000.0 / m_bpm;
    int currentBeat = static_cast<int>(m_currentTimeMs / beatMs);
    if (currentBeat > m_lastBeatIndex && m_currentTimeMs >= 0.0) {
        m_lastBeatIndex = currentBeat;
        m_pulseScale = 1.15f; // Receptor pulse

        // Play SFX click only if no background music is loaded
        if (m_audio && !m_audio->IsMusicLoaded()) {
            if (currentBeat % 4 == 0) m_audio->PlaySound(SOUND_KICK, 0.5f);
            else m_audio->PlaySound(SOUND_HIHAT, 0.35f);
        }
    }

    m_pulseScale += (1.0f - m_pulseScale) * static_cast<float>(1.0 - std::exp(-deltaMs / 60.0));

    // Update wave effects
    for (auto it = m_activeWaves.begin(); it != m_activeWaves.end(); ) {
        it->timer += deltaMs;
        if (it->timer >= it->maxTime) it = m_activeWaves.erase(it);
        else ++it;
    }

    // Update sparks
    for (auto it = m_activeSparks.begin(); it != m_activeSparks.end(); ) {
        it->timer += deltaMs;
        if (it->timer >= it->maxTime) it = m_activeSparks.erase(it);
        else ++it;
    }

    // Update judge banner display timer
    if (m_judgeTimer > 0.0) {
        m_judgeTimer -= deltaMs;
        if (m_judgeTimer < 0.0) m_judgeTimer = 0.0;
    }

    // Auto-Play
    if (m_autoPlay) {
        for (auto& note : m_notes) {
            if (!note.hit && !note.missed && !note.fading && note.timeMs <= m_currentTimeMs) {
                note.hit = true;
                if(!note.chance) TriggerJudge(JUDGE_PERFECT, 0.0, note.lane);
            }
        }
    }

    // Native direction rendering retires pending input 27 pixels past the
    // receptor, freezes its position, then fades over another 100 pixels.
    for (auto& note : m_notes) {
        if (note.lane && note.lane != 5 && !note.hit && !note.missed &&
            Rules::DirectionDistance(note.timeMs, m_currentTimeMs, m_bpm, m_speed,
                m_lanes.at(note.lane).isLeft) < -27) note.fading = true;
        if (note.lane && !note.hit && !note.missed && Rules::Expired(note.timeMs, m_currentTimeMs, m_bpm, m_speed, note.lane)) {
            if(note.chance || note.timeMs<m_manualSeekGraceUntil) {note.hit=true;continue;}
            note.missed = true;
            TriggerJudge(JUDGE_MISS, m_currentTimeMs - note.timeMs, note.lane);
        }
    }
    m_spaceMilestone.Observe(m_combo,m_currentTimeMs);
}

void BeatUpGame::OnKeyDown(WPARAM vk, double eventTimeMs) {
    if(vk==VK_DELETE) {ActivateChance(eventTimeMs>=0?eventTimeMs:m_currentTimeMs);return;}
    int lane = 0;

    if (m_useNumpadScheme) {
        switch (vk) {
            case VK_NUMPAD7: lane = 7; break;
            case VK_NUMPAD4: lane = 4; break;
            case VK_NUMPAD1: lane = 1; break;
            case VK_NUMPAD9: lane = 9; break;
            case VK_NUMPAD6: lane = 6; break;
            case VK_NUMPAD3: lane = 3; break;
            case VK_NUMPAD0: case VK_NUMPAD5: case VK_SPACE: lane = 5; break;
        }
    }

    if (lane == 0) {
        switch (vk) {
            case 'Q': lane = 7; break;
            case 'A': lane = 4; break;
            case 'Z': lane = 1; break;
            case 'P': case 'I': lane = 9; break;
            case 'L': case 'K': lane = 6; break;
            case 'M': case VK_OEM_COMMA: lane = 3; break;
            case VK_NUMPAD0: case VK_NUMPAD5: case VK_SPACE: lane = 5; break;
        }
    }

    if (lane != 0 && !m_keyHeld[lane]) {
        m_keyHeld[lane] = true;
        m_keyPulseMs[lane] = 30000.0 / m_bpm;
        if (!m_autoPlay) ProcessInput(lane, eventTimeMs);
    }
}

void BeatUpGame::OnKeyUp(WPARAM vk) {
    int lane = 0;
    if (m_useNumpadScheme) {
        switch (vk) {
            case VK_NUMPAD7: lane = 7; break;
            case VK_NUMPAD4: lane = 4; break;
            case VK_NUMPAD1: lane = 1; break;
            case VK_NUMPAD9: lane = 9; break;
            case VK_NUMPAD6: lane = 6; break;
            case VK_NUMPAD3: lane = 3; break;
            case VK_NUMPAD0: case VK_NUMPAD5: case VK_SPACE: lane = 5; break;
        }
    }
    if (lane == 0) {
        switch (vk) {
            case 'Q': lane = 7; break;
            case 'A': lane = 4; break;
            case 'Z': lane = 1; break;
            case 'P': case 'I': lane = 9; break;
            case 'L': case 'K': lane = 6; break;
            case 'M': case VK_OEM_COMMA: lane = 3; break;
            case VK_NUMPAD0: case VK_NUMPAD5: case VK_SPACE: lane = 5; break;
        }
    }
    if (lane != 0) m_keyHeld[lane] = false;
}

void BeatUpGame::ReleaseKeys() {
    for (auto& held : m_keyHeld) held = false;
    m_keyPulseMs.fill(0.0);
}

void BeatUpGame::ProcessInput(int laneId, double eventTimeMs) {
    const double inputTimeMs = eventTimeMs >= 0.0 ? eventTimeMs
        : m_audio ? m_audio->GetTimeMs() : m_currentTimeMs;
    // The native key path walks chart order, selecting the first pending note of
    // this direction. Choosing the nearest note can incorrectly skip a late note.
    for (auto& note : m_notes) {
        if (note.hit || note.missed || note.fading || note.lane != laneId) continue;
        int distance = laneId == 5 ? Rules::SpaceDistance(note.timeMs, inputTimeMs, m_bpm, m_speed)
            : Rules::DirectionDistance(note.timeMs, inputTimeMs, m_bpm, m_speed, m_lanes.at(laneId).isLeft);
        int result = laneId == 5 ? Rules::JudgeSpace(distance) : Rules::JudgeDirection(distance);
        if (result < 0) return;
        auto rating = note.chance ? JUDGE_BAD : static_cast<JudgeRating>(result + 1);
        note.hit = rating != JUDGE_MISS;
        note.missed = rating == JUDGE_MISS;
        TriggerJudge(rating, inputTimeMs - note.timeMs, laneId);
        return;
    }
}

void BeatUpGame::TriggerJudge(JudgeRating rating, double offsetMs, int laneId) {
    m_lastRating = rating;
    m_lastJudgeLane = laneId;
    m_lastOffsetMs = offsetMs;
    m_judgeTimer = 60000.0 / m_bpm;
    m_judgeCounts[rating]++;

    if (rating == JUDGE_PERFECT) {
        m_score += laneId == 5 ? 2000 : 400;
        m_combo++;
        if (m_audio) m_audio->PlaySound(laneId==5?SOUND_SPACE:SOUND_HIT, 1.0f);
    } else if (rating == JUDGE_GREAT) {
        m_score += laneId == 5 ? 1500 : 200;
        m_combo++;
        if (m_audio) m_audio->PlaySound(SOUND_GREAT, 0.85f);
    } else if (rating == JUDGE_COOL) {
        m_score += laneId == 5 ? 1000 : 100;
        m_combo++;
        if (m_audio) m_audio->PlaySound(SOUND_HIT, 0.7f);
    } else if (rating == JUDGE_BAD) {
        m_score += laneId == 5 ? 500 : 20;
        if (m_audio) m_audio->PlaySound(SOUND_HIT, 0.4f);
    } else if (rating == JUDGE_MISS) {
        m_combo = ComboMiss(m_comboLevel);
        if (m_audio) m_audio->PlaySound(SOUND_MISS, 0.7f);
    }

    if(rating==JUDGE_PERFECT || rating==JUDGE_GREAT || rating==JUDGE_COOL) m_comboLevel=ComboLevel(m_combo);
    if (m_combo > m_maxCombo) m_maxCombo = m_combo;

    // Trigger visual hit response
    if (rating <= JUDGE_BAD && laneId > 0 && laneId < 10) {
        float sparkX = 0.0f;
        float sparkY = 0.0f;

        if (laneId == 5) {
            sparkX = 512.0f;
            sparkY = 656.0f;
        } else {
            auto it = m_lanes.find(laneId);
            if (it != m_lanes.end()) {
                sparkX = it->second.targetX;
                sparkY = it->second.y;
            }
        }

        if (sparkX > 0.0f) {
            LaneWaveAnim wave;
            wave.laneId = laneId;
            wave.x = sparkX;
            wave.y = sparkY;
            wave.timer = 0.0;
            wave.maxTime = 60000.0 / m_bpm / 2.0;
            m_activeWaves.push_back(wave);

            SparkAnim spark;
            spark.laneId = laneId;
            spark.rating = rating;
            // Native hit arrows freeze at their judged position, not snapped
            // to the receptor, then fade across two sixteenth-note ticks.
            spark.x = laneId == 5 ? sparkX : Rules::NoteX(0, offsetMs, m_bpm, m_speed,
                m_lanes.at(laneId).isLeft);
            spark.y = sparkY;
            spark.timer = 0.0;
            spark.maxTime = 30000.0 / m_bpm;
            m_activeSparks.push_back(spark);
        }
    }
}

void BeatUpGame::Render(bool showSequence) {
    if (!m_renderer) return;

    // -------------------------------------------------------------
    // Layer 1: Optional original bg.dds; black comes from the frame clear.
    // -------------------------------------------------------------
    if (m_showReferenceBackground && m_texBackground.srv) {
        m_renderer->DrawSpriteTL(m_texBackground.srv,0,0,1024,768);
    }

    // -------------------------------------------------------------
    // Layer 2: Legacy skin track geometry (OldVTC + supplied reference images).
    //   TrackL3: Far left cap (X=0, Y=419, width 128, height 256)
    // -------------------------------------------------------------
    if (m_texTrackL1.srv) m_renderer->DrawSpriteTL(m_texTrackL1.srv, 27.0f, 421.0f, 256.0f, 256.0f);
    if (m_texTrackL2.srv) m_renderer->DrawSpriteTL(m_texTrackL2.srv, 283.0f, 421.0f, 128.0f, 256.0f);

    // -------------------------------------------------------------
    // Layer 3: Right track box
    // -------------------------------------------------------------
    if (m_texTrackR2.srv) m_renderer->DrawSpriteTL(m_texTrackR2.srv, 613.0f, 421.0f, 128.0f, 256.0f);
    if (m_texTrackR1.srv) m_renderer->DrawSpriteTL(m_texTrackR1.srv, 741.0f, 421.0f, 256.0f, 256.0f);

    // -------------------------------------------------------------
    // Layer 4: Bottom Beat-Up Bar & Chance Button
    //   Chance [C]: Centered at X=287, Y=653, size 64x64
    //   SpaceBar: native origin (357,623), note center (512,656)
    // -------------------------------------------------------------
    if (m_texChanceBtn.srv) {
        m_renderer->DrawSprite(m_texChanceBtn.srv, 287.0f, 653.0f, 64.0f, 64.0f);
    }
    if(m_chancePromptSlide.Visible() && m_texChancePrompt.srv)
        m_renderer->DrawSpriteTL(m_texChancePrompt.srv,m_chancePromptSlide.x,395,256,32);
    if(ChanceActive() && m_texChanceActive.srv) m_renderer->DrawSprite(m_texChanceActive.srv,737.0f,653.0f,64.0f,64.0f);
    const auto gauge = GetSpaceGauge();
    // Cdiu CheckComboAdd/ShowY/ShowB: flames behind the original bar.
    if (gauge.flame && m_texGaugeFlames[gauge.flame-1].srv) {
        float x = gauge.flame == 2 ? 254.0f : 330.0f;
        float y = gauge.flame == 2 ? 591.0f : 594.0f;
        m_renderer->DrawSpriteTL(m_texGaugeFlames[gauge.flame-1].srv,x,y,512,128);
    }
    if (m_texSpaceBar.srv) {
        m_renderer->DrawSpriteTL(m_texSpaceBar.srv, 357.0f, 623.0f, 512.0f, 64.0f);
    }
    // Each B/E/A/T/U/P cell has its own orange-to-blue progression.
    for (int i=0;i<6;++i) {
        int color = gauge.letters[i];
        if (!color) continue;
        float x=390.0f+i*48.0f;
        if (m_texGaugeLetterGlows[color-1].srv)
            m_renderer->DrawSprite(m_texGaugeLetterGlows[color-1].srv,x+8,655,64,32);
        if (m_texGaugeLetters[i].srv)
            m_renderer->DrawSpriteTL(m_texGaugeLetters[i].srv,x,647,16,16);
    }

    // Notes use the same integer pixel coordinates as the judgement model.
    for (const auto& note : m_notes) {
        if (note.hit || note.missed || !note.lane) continue;
        if (note.lane == 5) {
            int distance = Rules::SpaceDistance(note.timeMs, m_currentTimeMs, m_bpm, m_speed);
            if (distance > 134 || distance < -8) continue;
            if (m_texSpaceNote.srv) {
                m_renderer->DrawSprite(m_texSpaceNote.srv, 512.0f - distance, 656, 64, 32);
                if (distance != 0) m_renderer->DrawSprite(m_texSpaceNote.srv, 512.0f + distance, 656, 64, 32);
            }
            continue;
        }
        const auto& lane = m_lanes.at(note.lane);
        float x = Rules::NoteX(note.timeMs, m_currentTimeMs, m_bpm, m_speed, lane.isLeft);
        int distance = Rules::DirectionDistance(note.timeMs, m_currentTimeMs, m_bpm, m_speed, lane.isLeft);
        float alpha = 1.0f;
        if (note.fading) {
            x = lane.targetX + (lane.isLeft ? 28.0f : -28.0f);
            alpha = std::clamp((127.0f + distance) / 100.0f, 0.0f, 1.0f);
        }
        if (x < -32 || x > 1056) continue;
        // Native frame selection follows travel distance, not wall-clock time.
        int frame = Rules::ArrowFrame(x, lane.isLeft);
        auto srv = (note.type == 1 || note.chance) ? m_texArrowsChance[note.lane][frame].srv
                                  : m_texArrowsNormal[note.lane][frame].srv;
        if (srv) m_renderer->DrawSprite(srv, x, lane.y, 64, 64, 0, alpha);
    }

    // A finite key-down pulse, independent of successful judgement. Holding a
    // key must not leave a permanent starburst over the receptor.
    for (int laneId : {7,4,1,9,6,3}) {
        if (m_keyPulseMs[laneId] <= 0.0 || !m_texWaves[laneId].srv) continue;
        const auto& lane = m_lanes.at(laneId);
        float alpha = static_cast<float>(m_keyPulseMs[laneId] / (30000.0 / m_bpm));
        m_renderer->DrawSprite(m_texWaves[laneId].srv, lane.targetX, lane.y,
                              64, 64, 0, alpha * 0.8f, true);
    }

    // Active wave pulses on hit
    for (const auto& wave : m_activeWaves) {
        float progress = static_cast<float>(wave.timer / wave.maxTime);
        float alpha = 1.0f - progress;
        float scale = 1.0f + progress * 0.75f;
        if (wave.laneId > 0 && wave.laneId < 10 && m_texWaves[wave.laneId].srv) {
            m_renderer->DrawSprite(m_texWaves[wave.laneId].srv, wave.x, wave.y, 64.0f, 64.0f, 0.0f, alpha, true, scale, scale);
        }
    }

    // Native hit-state branch (0xA0D0D0..0xA0D2B0): retain the arrow,
    // fade it over two ticks, and shrink a glow for Perfect/Great only.
    for (const auto& spark : m_activeSparks) {
        float progress = static_cast<float>(spark.timer / spark.maxTime);
        float alpha = 1.0f - progress;
        if (spark.laneId != 5) {
            auto arrow = m_texArrowsNormal[spark.laneId][0].srv;
            if (arrow) m_renderer->DrawSprite(arrow, spark.x, spark.y, 64, 64, 0, alpha);
        } else if (m_texSpaceHit.srv && spark.rating <= JUDGE_GREAT) {
            m_renderer->DrawSprite(m_texSpaceHit.srv, 512, 656,
                static_cast<float>(m_texSpaceHit.width), static_cast<float>(m_texSpaceHit.height),
                0, alpha, true, 1.0f-progress*0.5f, 1.0f-progress*0.5f);
        }
        if (spark.rating <= JUDGE_GREAT && m_texHitEffect.srv && spark.laneId != 5) {
            float scale = 1.5f - progress;
            m_renderer->DrawSprite(m_texHitEffect.srv, spark.x, spark.y, 256, 256,
                                  0, alpha * 0.7f, true, scale, scale);
        }
    }

    // OldVTC draws the outer track masks AFTER arrows (0xA1193D -> 0xA11A1C).
    // This is the spawn aperture: arrows emerge from behind the end caps.
    if (m_texTrackL3.srv) m_renderer->DrawSpriteTL(m_texTrackL3.srv, 0.0f, 419.0f, 128.0f, 256.0f);
    if (m_texTrackR3.srv) m_renderer->DrawSpriteTL(m_texTrackR3.srv, 897.0f, 419.0f, 128.0f, 256.0f);

    // -------------------------------------------------------------
    // Layer 7: Judgement & Combo
    //   Judgement: center (513,194), native texture dimensions; combo y=242
    // -------------------------------------------------------------
    if (m_judgeTimer > 0.0 && m_lastRating != JUDGE_NONE) {
        ID3D11ShaderResourceView* jTex = m_texJudges[m_lastRating].srv;
        if (jTex) {
            float alpha = std::min<float>(1.0f, static_cast<float>(m_judgeTimer / 100.0));
            double elapsed = 60000.0 / m_bpm - m_judgeTimer;
            float scale = static_cast<float>((std::max)(1.0, 1.8 - elapsed / (15000.0 / m_bpm)));
            const auto& tex=m_texJudges[m_lastRating];
            m_renderer->DrawSprite(tex.srv,513,194,static_cast<float>(tex.width),static_cast<float>(tex.height),0,alpha,false,scale,scale);
        }
    }

    // Combo Counter
    if (m_combo >= 2 && m_judgeTimer > 0.0) {
        std::string comboStr = std::to_string(m_combo);
        // OldVTC A0C0BB..A0C14B: centered sprite anchors, fixed 32px advance.
        // The texture descriptor supplies each full canvas; no alpha crop/fit.
        double elapsed=60000.0/m_bpm-m_judgeTimer;
        float comboScale=static_cast<float>((std::max)(0.8,1.4-elapsed/(15000.0/m_bpm)));
        auto* bank=m_combo>=400?m_texComboDigitsPurple:m_combo>=100?m_texComboDigitsYellow:m_texComboDigits;
        float x=529-static_cast<float>(comboStr.size())*16;
        for(char c:comboStr) {
            const auto& tex=bank[c-'0'];
            if(tex.srv)m_renderer->DrawSprite(tex.srv,x,242,static_cast<float>(tex.width),static_cast<float>(tex.height),0,1,false,comboScale,comboScale);
            x+=32;
        }
    }

    if (gauge.flame && m_texGaugeUp[gauge.flame-1].srv)
        m_renderer->DrawSpriteTL(m_texGaugeUp[gauge.flame-1].srv,558,260,static_cast<float>(m_texGaugeUp[gauge.flame-1].width),static_cast<float>(m_texGaugeUp[gauge.flame-1].height));
    // Cdiu CheckMoreCombo: entry, alternating flashes, exit over eight ticks.
    int milestone=m_spaceMilestone.Level(m_currentTimeMs,m_bpm);
    if (milestone && m_texMilestoneBanners[milestone-1].srv) {
        double ticks=m_spaceMilestone.Age(m_currentTimeMs)/(15000.0/m_bpm);
        int tick=static_cast<int>(ticks);
        float fraction=static_cast<float>(ticks-tick);
        bool visible=tick==0 || tick==1 || tick==3 || tick==5 || tick==7;
        float x=tick==0 ? 514+(1-fraction)*500 : tick==7 ? 514-fraction*1000 : 514;
        if (visible) {
            const auto& tex=m_texMilestoneBanners[milestone-1];
            m_renderer->DrawSprite(tex.srv,x+8,244,static_cast<float>(tex.width),static_cast<float>(tex.height));
        }
    }

    // Preserve f/n,f/s,f timeline flags; one banner per finish tick.
    if(m_texFinish.srv) {
        double finish=-1e30;
        for(const auto& note:m_notes) if(note.finish && note.timeMs<=m_currentTimeMs+15000.0/m_bpm)
            finish=(std::max)(finish,note.timeMs);
        auto visual=GetFinishVisual(m_currentTimeMs,finish,m_bpm);
        if(visual.visible) m_renderer->DrawSprite(m_texFinish.srv,visual.x,244,512,128);
    }

    auto intro=NativeIntro(m_currentTimeMs,GetFirstNoteMs(),m_bpm);
    // Native 0x8E6CC8: ready/start top-left 382,382; count top-left 369,367.
    if(showSequence && intro.stage==1 && m_texReady.srv) m_renderer->DrawSpriteTL(m_texReady.srv,382,382,static_cast<float>(m_texReady.width),static_cast<float>(m_texReady.height));
    if(showSequence && intro.stage==2 && m_texStart.srv) m_renderer->DrawSpriteTL(m_texStart.srv,382,382,static_cast<float>(m_texStart.width),static_cast<float>(m_texStart.height));
    if(showSequence && intro.count && m_texCountdown[intro.count-1].srv) {
        const auto& tex=m_texCountdown[intro.count-1];
        m_renderer->DrawContent(tex,401,399,96,64);
    }

    // OldVTC song-info composition, traced at 0x008EB820..0x008EB93C.
    // The default branch uses x=-16, y=700; another game mode adds an x offset.
    const float infoX = -16.0f;
    const float infoY = 700.0f;
    float progress = m_songTotalDurationMs > 0.0
        ? static_cast<float>(std::clamp(m_currentTimeMs / m_songTotalDurationMs, 0.0, 1.0)) : 0.0f;
    if (m_texSongBlank.srv) m_renderer->DrawSpriteTL(m_texSongBlank.srv, infoX, infoY, static_cast<float>(m_texSongBlank.width),static_cast<float>(m_texSongBlank.height));
    // Old helper draws 105 * (progress + 0.6) texels, preserving UV scale.
    if (m_texSongFull.srv) {
        float width = 105.0f * (progress + 0.6f);
        m_renderer->DrawSpriteTL_UV(m_texSongFull.srv, infoX, infoY, width, static_cast<float>(m_texSongFull.height),
                                    0, 0, width / m_texSongFull.width, 1);
    }
    if (m_texSongBase.srv) m_renderer->DrawSpriteTL(m_texSongBase.srv, infoX, infoY, static_cast<float>(m_texSongBase.width),static_cast<float>(m_texSongBase.height));
    if (m_texSongBar.srv) m_renderer->DrawSpriteTL(m_texSongBar.srv, 31.0f + progress * 105.0f, 731, static_cast<float>(m_texSongBar.width),static_cast<float>(m_texSongBar.height));
    std::wostringstream songLabel;
    songLabel << m_songTitle << L" (" << std::fixed << std::setprecision(0) << m_bpm << L" bpm)";
    m_renderer->DrawSongTitle(songLabel.str(), 33, 711);
    if (m_texSongFeather.srv) m_renderer->DrawSpriteTL(m_texSongFeather.srv, infoX, infoY, static_cast<float>(m_texSongFeather.width),static_cast<float>(m_texSongFeather.height));
    int remSec = static_cast<int>(std::ceil((std::max)(0.0, m_songTotalDurationMs - m_currentTimeMs) / 1000.0));
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", remSec / 60, remSec % 60);
    m_renderer->DrawString(timeStr, 160, 739, 13, 1, 1, 1, 1, true);

    // -------------------------------------------------------------
    // Layer 9: Key Assists & Top Controls Bar
    // -------------------------------------------------------------
    std::string schemeStr = m_useNumpadScheme ? "Numpad (741/963/0/5)" : "Laptop (QAZ/PLM/Space)";
    std::string speedStr = std::to_string(m_speed).substr(0, 3) + "x";
    std::string hudTop = "F1: " + schemeStr + " | F2: Auto (" + (m_autoPlay ? "ON" : "OFF") + ") | F3: Assist | F6: Editor | +/-: Speed (" + speedStr + ") | R: Restart";


    if (m_showKeyAssist) {
        const int ids[] = {7,4,1,9,6,3};
        const char* num[] = {"7","4","1","9","6","3"};
        const char* alternate[] = {"Q","A","Z","P","L","M"};
        for (int i = 0; i < 6; ++i) {
            const auto& lane = m_lanes.at(ids[i]);
            m_renderer->DrawString(m_useNumpadScheme ? num[i] : alternate[i],
                                  lane.targetX-4, lane.y-28, 10, 0.8f, 0.85f, 0.9f, 0.6f);
        }
    }
}

} // namespace BeatUp

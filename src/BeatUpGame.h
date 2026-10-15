#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <array>
#include <map>
#include "Renderer.h"
#include "AudioEngine.h"
#include "SlkParser.h"
#include "SpaceGauge.h"
#include "IntroPresentation.h"

namespace BeatUp {

enum JudgeRating {
    JUDGE_NONE = 0,
    JUDGE_PERFECT,
    JUDGE_GREAT,
    JUDGE_COOL,
    JUDGE_BAD,
    JUDGE_MISS
};

struct ActiveLane {
    int id;              // 7, 4, 1, 9, 6, 3, 5
    float startX;
    float targetX;
    float y;
    bool isLeft;
};

class BeatUpGame {
public:
    BeatUpGame();
    ~BeatUpGame();

    bool Init(Renderer* renderer, AudioEngine* audio, const std::wstring& assetDir);
    void Update(double currentTimeMs, double deltaMs);
    void Render(bool showSequence=true);

    // Input handlers
    void OnKeyDown(WPARAM vk, double eventTimeMs = -1.0);
    void OnKeyUp(WPARAM vk);
    void ReleaseKeys();
    void ToggleReferenceBackground() { m_showReferenceBackground = !m_showReferenceBackground; }
    void SetBackgroundImage(bool enabled) { m_showReferenceBackground = enabled; }
    bool BackgroundImageEnabled() const { return m_showReferenceBackground; }

    // Controls
    void SetBpm(double bpm);
    double GetBpm() const { return m_bpm; }
    void SetSpeed(float speed) { m_speed = speed; }
    float GetSpeed() const { return m_speed; }
    void ToggleAutoPlay() { m_autoPlay = !m_autoPlay; }
    void SetAutoPlay(bool enabled) {m_autoPlay=enabled;}
    bool IsAutoPlay() const { return m_autoPlay; }
    void ToggleKeyAssist() { m_showKeyAssist = !m_showKeyAssist; }
    void SwitchKeyScheme();

    void Restart();
    void SetTransportControlled(bool controlled) {m_autoStart=!controlled;}
    void Seek(double timeMs,bool resetStats=false);
    void ConfigureTiming(double bpm,double nativeOffset);
    double GetDurationMs() const {return m_songTotalDurationMs;}
    double GetNativeOffset() const {return m_chartTickShift+1.0;}
    double GetFirstNoteMs() const;
    bool LoadSong(const std::wstring& slkPath, const std::wstring& wavPath = L"");
    bool NewSong(const std::wstring& path);
    int GetAllCombo() const;
    bool ChanceAvailable(double now) const;
    bool ChanceActive() const {return m_chanceFinish>=0;}
    bool ActivateChance(double now);
    const std::vector<ChartNote>& GetNotes() const {return m_notes;}
    void SetSessionChart(std::vector<ChartNote> notes,double bpm,double offset);
    bool LoadAudio(const std::wstring& path);
    const std::wstring& GetAudioPath() const {return m_currentSongWav;}

    // Stats
    int GetScore() const { return m_score; }
    int GetCombo() const { return m_combo; }
    int GetMaxCombo() const { return m_maxCombo; }
    const std::array<int, 6>& GetJudgeCounts() const { return m_judgeCounts; }
    double GetLastOffsetMs() const { return m_lastOffsetMs; }
    JudgeRating GetLastRating() const { return m_lastRating; }
    SpaceGaugeVisual GetSpaceGauge() const {
        auto visual=GetSpaceGaugeVisual(m_combo);
        if(m_combo<100) for(int i=0;i<6;++i) visual.letters[i]=i<m_comboLevel?1:0;
        return visual;
    }

private:
    void SetupLanes();
    void ProcessInput(int laneId, double eventTimeMs);
    void TriggerJudge(JudgeRating rating, double offsetMs, int laneId);

    Renderer* m_renderer = nullptr;
    AudioEngine* m_audio = nullptr;
    std::wstring m_assetDir;

    // Authentic Stage & Track Textures
    TextureResource m_texChancePrompt;
    TextureResource m_texBackground; // bg.dds, original 1024x768 background
    TextureResource m_texTrackL3;    // track_l3.dds (outer cap)
    TextureResource m_texTrackL1;    // track_l1.dds (lane body)
    TextureResource m_texTrackL2;    // track_l2.dds (receptor inner cap)
    TextureResource m_texTrackR2;    // track_r2.dds (receptor inner cap)
    TextureResource m_texTrackR1;    // track_r1.dds (lane body)
    TextureResource m_texTrackR3;    // track_r3.dds (outer cap)

    // Bottom Beat-Up Space Bar
    TextureResource m_texSpaceBar;   // spacebar.dds
    TextureResource m_texSpaceNote;  // space.dds
    TextureResource m_texSpaceHit;   // spower.dds
    TextureResource m_texChanceActive;
    TextureResource m_texChanceBtn;  // crazy.dds
    TextureResource m_texFinish;
    TextureResource m_texReady,m_texStart,m_texCountdown[3];
    TextureResource m_texGaugeFlames[2];
    TextureResource m_texGaugeLetterGlows[2];
    TextureResource m_texGaugeLetters[6];
    TextureResource m_texMilestoneBanners[2];
    TextureResource m_texGaugeUp[2];
    SpaceMilestone m_spaceMilestone;

    // Song Info Widget
    TextureResource m_texSongBase;    // song_info_base.dds
    TextureResource m_texSongFeather; // song_info_feather.dds
    TextureResource m_texSongBlank;   // song_info_blank.dds
    TextureResource m_texSongFull;    // song_info_full.dds
    TextureResource m_texSongBar;     // song_info_bar.dds
    std::wstring m_songTitle = L"No Music";
    double m_songTotalDurationMs = 0.0;

    // Per-lane 4-frame animation arrows
    // Array index: lane (0..9), frame (0..3)
    TextureResource m_texArrowsNormal[10][4]; // a71..a74, a41..a44, etc.
    TextureResource m_texArrowsChance[10][4]; // c71..c74, c41..c44, etc.

    // Per-lane wave hit pulse & hit sparks
    TextureResource m_texWaves[10]; // wave_1, wave_3, wave_4, wave_6, wave_7, wave_9
    TextureResource m_texHitEffect; // hit_effect.dds

    // Judgements & Combo
    TextureResource m_texJudges[6]; // None, Perfect, Great, Cool, Bad, Miss
    TextureResource m_texComboDigits[10];
    TextureResource m_texComboDigitsYellow[10];
    TextureResource m_texComboDigitsPurple[10];

    // Game state
    double m_currentTimeMs = 0.0;
    double m_bpm = 150.0;
    float m_speed = 1.0f;
    double m_approachTimeMs = 1000.0;

    std::vector<ChartNote> m_notes;
    bool m_usingCustomChart = false;
    double m_chartOffsetMs = 0.0;
    double m_chartTickShift = 0.0;
    std::wstring m_currentSongWav;

    // Metronome & Audio sync
    int m_lastBeatIndex = -1;

    // Judgement & scoring
    int m_score = 0;
    double m_manualSeekGraceUntil=-1;
    int m_combo = 0;
    int m_comboLevel=0;
    bool m_autoStart=true;
    int m_introStage=0;
    double m_chanceFinish=-1;
    ChancePromptSlide m_chancePromptSlide;
    int m_maxCombo = 0;
    std::array<int, 6> m_judgeCounts{};
    JudgeRating m_lastRating = JUDGE_NONE;
    double m_judgeTimer = 0.0;
    double m_lastOffsetMs = 0.0;

    // Visual feedback animations
    float m_pulseScale = 1.0f;

    struct LaneWaveAnim {
        int laneId;
        float x, y;
        double timer;
        double maxTime;
    };
    std::vector<LaneWaveAnim> m_activeWaves;

    struct SparkAnim {
        int laneId;
        JudgeRating rating;
        float x, y;
        double timer;
        double maxTime;
    };
    std::vector<SparkAnim> m_activeSparks;
    std::array<double, 10> m_keyPulseMs{};

    // Settings
    bool m_autoPlay = false;
    bool m_showKeyAssist = false;
    bool m_showReferenceBackground = false;
    int m_lastJudgeLane = 0;
    bool m_useNumpadScheme = true; // true = Numpad, false = Laptop (QAZ/PLM)

    // Lanes map (7, 4, 1 on left; 9, 6, 3 on right; 5 in center)
    std::map<int, ActiveLane> m_lanes;

    // Key states
    bool m_keyHeld[10]{};
};

} // namespace BeatUp

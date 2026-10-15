#pragma once
#include <windows.h>
#include <xaudio2.h>
#include <vector>
#include <string>
#include <cstdint>

namespace BeatUp {

enum SoundType {
    SOUND_KICK = 0,
    SOUND_HIHAT,
    SOUND_HIT,
    SOUND_MISS,
    SOUND_SPACE,SOUND_GREAT,SOUND_READY,SOUND_START,
    SOUND_COUNT
};

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool Init();
    void Shutdown();

    // Sound effects
    void PlaySound(SoundType type, float volume = 0.8f);
    int LoadEffects(const std::wstring& folder);
    void StopEffects();
    void SetMusicVolume(float volume);
    void SetEffectVolume(float volume);
    float GetMusicVolume() const {return m_musicVolume;}
    float GetEffectVolume() const {return m_effectVolume;}

    // Music playback (WAV streaming / sample-accurate clock)
    bool LoadMusic(const std::wstring& wavPath);
    void PlayMusic();
    void StopMusic();
    void SeekMusic(double timeMs,bool play=false);
    void PauseMusic();
    void ResumeMusic();
    void UnloadMusic();
    double GetMusicTimeMs() const;
    double GetMusicDurationMs() const;
    bool IsMusicLoaded() const { return m_musicLoaded; }

    // Fallback QPC clock
    void ResetClock();
    double GetTimeMs() const;

private:
    void GenerateSynthesizedSounds();

    IXAudio2* m_xaudio = nullptr;
    IXAudio2MasteringVoice* m_masterVoice = nullptr;

    // SFX
    struct SoundBuffer {
        std::vector<int16_t> pcm;
        WAVEFORMATEX wfx;
        XAUDIO2_BUFFER buffer;
    };
    SoundBuffer m_sounds[SOUND_COUNT];
    std::vector<IXAudio2SourceVoice*> m_effectVoices;
    std::vector<float> m_effectGains;
    float m_musicVolume=1.0f,m_effectVolume=1.0f;

    // Music voice & data
    IXAudio2SourceVoice* m_musicVoice = nullptr;
    std::vector<uint8_t> m_musicData;
    WAVEFORMATEX m_musicWfx{};
    bool m_musicLoaded = false;
    bool m_musicPlaying = false;
    bool m_paused=false;
    double m_clockBaseMs=0,m_frozenTimeMs=0;
    mutable UINT64 m_clockSamples = 0;
    mutable LARGE_INTEGER m_sampleQpc{};
    mutable double m_lastMusicMs = 0.0;
    mutable bool m_musicClockInitialized = false;

    LARGE_INTEGER m_qpcFrequency{};
    LARGE_INTEGER m_qpcStart{};
};

} // namespace BeatUp

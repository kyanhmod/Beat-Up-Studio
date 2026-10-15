#include "AssetBundle.h"
#include "AudioEngine.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <limits>
#include <filesystem>
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#undef STB_VORBIS_HEADER_ONLY

namespace BeatUp {

AudioEngine::AudioEngine() {
    QueryPerformanceFrequency(&m_qpcFrequency);
    ResetClock();
}

AudioEngine::~AudioEngine() {
    Shutdown();
}

bool AudioEngine::Init() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    hr = XAudio2Create(&m_xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) return false;

    hr = m_xaudio->CreateMasteringVoice(&m_masterVoice);
    if (FAILED(hr)) return false;

    GenerateSynthesizedSounds();
    // Keep allocations and voice creation out of the key-press path.
    for (int i = 0; i < 32; ++i) {
        IXAudio2SourceVoice* voice = nullptr;
        if (SUCCEEDED(m_xaudio->CreateSourceVoice(&voice, &m_sounds[SOUND_HIT].wfx))) {
            m_effectVoices.push_back(voice);
            m_effectGains.push_back(1.0f);
        }
    }
    ResetClock();
    return true;
}

void AudioEngine::Shutdown() {
    for (auto voice : m_effectVoices) voice->DestroyVoice();
    m_effectVoices.clear();
    m_effectGains.clear();
    StopMusic();
    if (m_musicVoice) {
        m_musicVoice->DestroyVoice();
        m_musicVoice = nullptr;
    }
    if (m_masterVoice) {
        m_masterVoice->DestroyVoice();
        m_masterVoice = nullptr;
    }
    if (m_xaudio) {
        m_xaudio->Release();
        m_xaudio = nullptr;
    }
    CoUninitialize();
}

void AudioEngine::ResetClock() {
    QueryPerformanceCounter(&m_qpcStart);
    m_musicClockInitialized = false;
    m_lastMusicMs = 0.0;
    m_clockBaseMs=0;m_frozenTimeMs=0;m_paused=false;
}

double AudioEngine::GetTimeMs() const {
    if ((m_musicPlaying || m_paused) && m_musicVoice) {
        return GetMusicTimeMs();
    }
    if(m_paused) return m_frozenTimeMs;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return m_clockBaseMs+static_cast<double>(now.QuadPart - m_qpcStart.QuadPart) * 1000.0 / static_cast<double>(m_qpcFrequency.QuadPart);
}

bool AudioEngine::LoadMusic(const std::wstring& wavPath) {
    if (!m_xaudio) return false;
    std::ifstream file(wavPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize fileSize = file.tellg();
    if (fileSize < 4 || fileSize > (std::numeric_limits<int>::max)()) return false;

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) return false;

    WAVEFORMATEX format{};
    std::vector<uint8_t> pcm;
    if (memcmp(buffer.data(), "OggS", 4) == 0) {
        int channels = 0, sampleRate = 0;
        short* decoded = nullptr;
        int frames = stb_vorbis_decode_memory(buffer.data(), static_cast<int>(buffer.size()),
                                             &channels, &sampleRate, &decoded);
        if (frames <= 0 || !decoded || channels < 1 || channels > 2 || sampleRate <= 0) {
            free(decoded);
            return false;
        }
        size_t bytes = static_cast<size_t>(frames) * channels * sizeof(short);
        if (bytes > XAUDIO2_MAX_BUFFER_BYTES) { free(decoded); return false; }
        pcm.assign(reinterpret_cast<uint8_t*>(decoded), reinterpret_cast<uint8_t*>(decoded) + bytes);
        free(decoded);
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = static_cast<WORD>(channels);
        format.nSamplesPerSec = sampleRate;
        format.wBitsPerSample = 16;
        format.nBlockAlign = format.nChannels * 2;
        format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    } else {
    if (buffer.size() < 12 || memcmp(buffer.data(), "RIFF", 4) != 0 || memcmp(buffer.data() + 8, "WAVE", 4) != 0) return false;

    size_t offset = 12;
    bool foundFmt = false;
    bool foundData = false;

    while (offset + 8 <= buffer.size()) {
        char chunkId[5] = {};
        memcpy(chunkId, buffer.data() + offset, 4);
        uint32_t chunkSize = 0;
        memcpy(&chunkSize, buffer.data() + offset + 4, sizeof(chunkSize));
        offset += 8;
        if (chunkSize > buffer.size() - offset) return false;

        if (strcmp(chunkId, "fmt ") == 0) {
            if (chunkSize < 16) return false;
            size_t copySize = std::min<size_t>(chunkSize, sizeof(WAVEFORMATEX));
            memcpy(&format, buffer.data() + offset, copySize);
            foundFmt = true;
        } else if (strcmp(chunkId, "data") == 0) {
            pcm.assign(buffer.data() + offset, buffer.data() + offset + chunkSize);
            foundData = true;
        }

        offset += chunkSize;
        if (chunkSize % 2 != 0) offset++; // Word alignment
    }

    if (!foundFmt || !foundData || pcm.empty()) return false;
    if (format.wFormatTag != WAVE_FORMAT_PCM && format.wFormatTag != WAVE_FORMAT_IEEE_FLOAT) return false;
    format.cbSize = 0;
    }
    if (format.nChannels < 1 || format.nChannels > 2 || !format.nSamplesPerSec ||
        !format.nBlockAlign || format.nBlockAlign != format.nChannels * format.wBitsPerSample / 8 ||
        format.nAvgBytesPerSec != format.nSamplesPerSec * format.nBlockAlign ||
        pcm.size() % format.nBlockAlign || pcm.size() > XAUDIO2_MAX_BUFFER_BYTES) return false;

    IXAudio2SourceVoice* newVoice = nullptr;
    HRESULT hr = m_xaudio->CreateSourceVoice(&newVoice, &format);
    if (FAILED(hr)) return false;
    UnloadMusic();
    m_musicVoice = newVoice;
    m_musicData = std::move(pcm);
    m_musicWfx = format;
    m_musicLoaded = true;
    m_musicPlaying = false;
    return true;
}

void AudioEngine::PlayMusic() {
    if (!m_musicLoaded || !m_musicVoice) return;

    // Recreate the voice: SamplesPlayed otherwise survives stop/flush/restart.
    m_musicVoice->DestroyVoice();
    m_musicVoice = nullptr;
    m_musicPlaying = false;
    if (FAILED(m_xaudio->CreateSourceVoice(&m_musicVoice, &m_musicWfx))) return;
    m_musicVoice->SetVolume(m_musicVolume);

    XAUDIO2_BUFFER xbuf = {};
    xbuf.AudioBytes = static_cast<UINT32>(m_musicData.size());
    xbuf.pAudioData = m_musicData.data();
    xbuf.Flags = XAUDIO2_END_OF_STREAM;

    if (FAILED(m_musicVoice->SubmitSourceBuffer(&xbuf)) || FAILED(m_musicVoice->Start(0))) return;
    m_musicPlaying = true;
    ResetClock();
}

void AudioEngine::StopMusic() {
    if (m_musicVoice) {
        m_musicVoice->Stop(0);
        m_musicVoice->FlushSourceBuffers();
    }
    m_musicPlaying = false;
    m_paused=false;
}
void AudioEngine::PauseMusic() {
    if(m_paused) return;
    m_frozenTimeMs=GetTimeMs();
    if(m_musicVoice) m_musicVoice->Stop(0);
    for(auto voice:m_effectVoices) voice->Stop(0);
    m_musicPlaying=false;m_paused=true;
}
void AudioEngine::ResumeMusic() {
    if(!m_paused) return;
    if(m_musicVoice && m_musicLoaded) {m_musicVoice->Start(0);m_musicPlaying=true;m_musicClockInitialized=false;}
    else {QueryPerformanceCounter(&m_qpcStart);m_clockBaseMs=m_frozenTimeMs;}
    m_paused=false;
    for(auto voice:m_effectVoices) voice->Start(0);
}
void AudioEngine::SeekMusic(double timeMs,bool play) {
    if(!std::isfinite(timeMs)) return;
    timeMs=(std::max)(0.0,timeMs);
    if(!m_musicLoaded || !m_musicVoice) {
        ResetClock();m_clockBaseMs=m_frozenTimeMs=timeMs;m_paused=!play;return;
    }
    const auto frames=m_musicData.size()/m_musicWfx.nBlockAlign;
    const UINT32 begin=static_cast<UINT32>((std::min)(static_cast<double>(frames-1),timeMs*m_musicWfx.nSamplesPerSec/1000.0));
    m_musicVoice->DestroyVoice();m_musicVoice=nullptr;m_musicPlaying=false;ResetClock();
    if(FAILED(m_xaudio->CreateSourceVoice(&m_musicVoice,&m_musicWfx))) return;
    m_musicVoice->SetVolume(m_musicVolume);
    XAUDIO2_BUFFER buffer{};buffer.AudioBytes=static_cast<UINT32>(m_musicData.size());
    buffer.pAudioData=m_musicData.data();buffer.PlayBegin=begin;buffer.Flags=XAUDIO2_END_OF_STREAM;
    if(FAILED(m_musicVoice->SubmitSourceBuffer(&buffer))) return;
    m_clockBaseMs=begin*1000.0/m_musicWfx.nSamplesPerSec;m_lastMusicMs=m_frozenTimeMs=m_clockBaseMs;
    m_paused=!play;
    if(play) {m_musicVoice->Start(0);m_musicPlaying=true;}
}

double AudioEngine::GetMusicTimeMs() const {
    if (!m_musicVoice || !m_musicLoaded || m_musicWfx.nSamplesPerSec == 0) return 0.0;

    XAUDIO2_VOICE_STATE state{};
    m_musicVoice->GetState(&state);
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    if (!m_musicClockInitialized || state.SamplesPlayed != m_clockSamples) {
        m_clockSamples = state.SamplesPlayed;
        m_sampleQpc = now;
        m_musicClockInitialized = true;
    }
    double samplesMs = m_clockBaseMs+static_cast<double>(state.SamplesPlayed) * 1000.0 / m_musicWfx.nSamplesPerSec;
    // SamplesPlayed changes on audio processing passes, not every display frame.
    // Interpolate between observations, keeping the sample clock authoritative.
    double sinceSampleMs = static_cast<double>(now.QuadPart - m_sampleQpc.QuadPart) * 1000.0 / m_qpcFrequency.QuadPart;
    double interpolated = samplesMs + (m_musicPlaying ? std::clamp(sinceSampleMs, 0.0, 20.0) : 0.0);
    m_lastMusicMs = (std::min)(GetMusicDurationMs(), (std::max)(m_lastMusicMs, interpolated));
    return m_lastMusicMs;
}

void AudioEngine::UnloadMusic() {
    StopMusic();
    if (m_musicVoice) { m_musicVoice->DestroyVoice(); m_musicVoice = nullptr; }
    m_musicData.clear();
    m_musicWfx = {};
    m_musicLoaded = false;
    ResetClock();
}

double AudioEngine::GetMusicDurationMs() const {
    return m_musicLoaded && m_musicWfx.nAvgBytesPerSec
        ? static_cast<double>(m_musicData.size()) * 1000.0 / m_musicWfx.nAvgBytesPerSec : 0.0;
}

void AudioEngine::GenerateSynthesizedSounds() {
    const int sampleRate = 44100;
    const double pi = 3.14159265358979323846;

    for (int s = 0; s < SOUND_COUNT; ++s) {
        m_sounds[s].wfx.wFormatTag = WAVE_FORMAT_PCM;
        m_sounds[s].wfx.nChannels = 1;
        m_sounds[s].wfx.nSamplesPerSec = sampleRate;
        m_sounds[s].wfx.wBitsPerSample = 16;
        m_sounds[s].wfx.nBlockAlign = 2;
        m_sounds[s].wfx.nAvgBytesPerSec = sampleRate * 2;
        m_sounds[s].wfx.cbSize = 0;
    }

    // 1. Kick Sound (75Hz -> 35Hz drop)
    {
        double duration = 0.12;
        int numSamples = static_cast<int>(duration * sampleRate);
        m_sounds[SOUND_KICK].pcm.resize(numSamples);
        double phase = 0.0;
        for (int i = 0; i < numSamples; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            double freq = 75.0 - (40.0 * (t / duration));
            phase += 2.0 * pi * freq / sampleRate;
            double env = exp(-28.0 * t);
            double sample = sin(phase) * env;
            m_sounds[SOUND_KICK].pcm[i] = static_cast<int16_t>(sample * 30000.0);
        }
    }

    // 2. Hi-Hat Sound (Crisp noise tick)
    {
        double duration = 0.04;
        int numSamples = static_cast<int>(duration * sampleRate);
        m_sounds[SOUND_HIHAT].pcm.resize(numSamples);
        for (int i = 0; i < numSamples; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            double noise = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0;
            double env = exp(-110.0 * t);
            m_sounds[SOUND_HIHAT].pcm[i] = static_cast<int16_t>(noise * env * 22000.0);
        }
    }

    // 3. Hit Sound (Sharp Audition-style percussion click)
    {
        double duration = 0.07;
        int numSamples = static_cast<int>(duration * sampleRate);
        m_sounds[SOUND_HIT].pcm.resize(numSamples);
        double phase1 = 0.0, phase2 = 0.0;
        for (int i = 0; i < numSamples; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            phase1 += 2.0 * pi * 950.0 / sampleRate;
            phase2 += 2.0 * pi * 1900.0 / sampleRate;
            double noise = ((static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0) * exp(-120.0 * t);
            double tones = (sin(phase1) * 0.7 + sin(phase2) * 0.3) * exp(-45.0 * t);
            double sample = (tones * 0.75 + noise * 0.25);
            m_sounds[SOUND_HIT].pcm[i] = static_cast<int16_t>(sample * 31000.0);
        }
    }

    // 4. Miss Sound (Dull low thud)
    {
        double duration = 0.08;
        int numSamples = static_cast<int>(duration * sampleRate);
        m_sounds[SOUND_MISS].pcm.resize(numSamples);
        double phase = 0.0;
        for (int i = 0; i < numSamples; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            phase += 2.0 * pi * 110.0 / sampleRate;
            double env = exp(-35.0 * t);
            double sample = (sin(phase) > 0 ? 0.6 : -0.6) * env;
            m_sounds[SOUND_MISS].pcm[i] = static_cast<int16_t>(sample * 18000.0);
        }
    }

    for (int s = 0; s < SOUND_COUNT; ++s) {
        m_sounds[s].buffer = {};
        m_sounds[s].buffer.AudioBytes = static_cast<UINT32>(m_sounds[s].pcm.size() * sizeof(int16_t));
        m_sounds[s].buffer.pAudioData = reinterpret_cast<const BYTE*>(m_sounds[s].pcm.data());
        m_sounds[s].buffer.Flags = XAUDIO2_END_OF_STREAM;
    }
}

void AudioEngine::PlaySound(SoundType type, float volume) {
    if (!m_xaudio || type < 0 || type >= SOUND_COUNT || m_sounds[type].pcm.empty()) return;
    for (size_t i=0;i<m_effectVoices.size();++i) {
        auto voice=m_effectVoices[i];
        XAUDIO2_VOICE_STATE state{};
        voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        if (state.BuffersQueued) continue;
        m_effectGains[i]=std::clamp(volume,0.0f,1.0f);
        voice->SetVolume(m_effectGains[i]*m_effectVolume);
        if (SUCCEEDED(voice->SubmitSourceBuffer(&m_sounds[type].buffer))) voice->Start(0);
        return;
    }
}
int AudioEngine::LoadEffects(const std::wstring& folder) {
    StopEffects();
    int loaded=0;
    for(auto item:{std::pair<SoundType,const wchar_t*>{SOUND_HIT,L"BEAT.ogg"},
        {SOUND_SPACE,L"SPACE.ogg"},{SOUND_GREAT,L"GREAT.ogg"},{SOUND_MISS,L"MISS.ogg"},
        {SOUND_READY,L"READY.ogg"},{SOUND_START,L"START.ogg"}}) {
        auto bytes=ReadAsset(std::filesystem::path(folder)/item.second);
        if(bytes.empty()) continue;
        if(bytes.size()>(std::numeric_limits<int>::max)()) continue;
        int channels=0,rate=0;short* pcm=nullptr;
        int samples=stb_vorbis_decode_memory(bytes.data(),static_cast<int>(bytes.size()),&channels,&rate,&pcm);
        if(samples<=0 || channels<1 || rate<1) {free(pcm);continue;}
        auto& sound=m_sounds[item.first];
        sound.wfx=m_sounds[SOUND_HIT].wfx;
        size_t count=static_cast<size_t>(static_cast<double>(samples)*44100/rate);
        std::vector<int16_t> mono(count);
        for(size_t i=0;i<count;++i) {
            double pos=i*static_cast<double>(rate)/44100;int a=static_cast<int>(pos),b=(std::min)(samples-1,a+1);double sample=0;
            for(int c=0;c<channels;++c) sample+=pcm[a*channels+c]*(1-(pos-a))+pcm[b*channels+c]*(pos-a);
            mono[i]=static_cast<int16_t>(std::clamp(sample/channels,-32768.0,32767.0));
        }
        free(pcm);sound.pcm=std::move(mono);sound.buffer={};sound.buffer.AudioBytes=static_cast<UINT32>(sound.pcm.size()*sizeof(int16_t));
        sound.buffer.pAudioData=reinterpret_cast<const BYTE*>(sound.pcm.data());sound.buffer.Flags=XAUDIO2_END_OF_STREAM;++loaded;
    }
    return loaded;
}
void AudioEngine::StopEffects() {
    for(auto voice:m_effectVoices) {voice->Stop(0);voice->FlushSourceBuffers();}
}

void AudioEngine::SetMusicVolume(float volume) {
    if(!std::isfinite(volume)) return;
    m_musicVolume=std::clamp(volume,0.0f,1.0f);
    if(m_musicVoice) m_musicVoice->SetVolume(m_musicVolume);
}
void AudioEngine::SetEffectVolume(float volume) {
    if(!std::isfinite(volume)) return;
    m_effectVolume=std::clamp(volume,0.0f,1.0f);
    for(size_t i=0;i<m_effectVoices.size();++i) m_effectVoices[i]->SetVolume(m_effectGains[i]*m_effectVolume);
}

} // namespace BeatUp

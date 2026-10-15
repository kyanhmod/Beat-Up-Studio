#pragma once
#include <string>

namespace BeatUp {
struct SongInfo {
    std::wstring title;
    std::wstring artist;
    std::wstring audioPath;
    double bpm = 150.0;
    double offsetMs = 0.0;
    double auditionOffset = 1.0;
    bool hasAuditionOffset = false;
    // Native clock advances elapsed by (1-offset)*tickMs. For a chart event,
    // invert that phase shift; retain offset_ms as an additional fine adjustment.
    double NativeTickShift() const { return hasAuditionOffset ? auditionOffset-1.0 : 0.0; }
    double ChartShiftMs() const { return offsetMs + NativeTickShift()*15000.0/bpm; }
};
// Optional UTF-8 <chart stem>.song.ini; paths are relative to the chart.
SongInfo ReadSongInfo(const std::wstring& chartPath, double fallbackBpm);
}

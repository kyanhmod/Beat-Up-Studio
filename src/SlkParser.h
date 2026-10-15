#pragma once
#include <string>
#include <vector>

namespace BeatUp {

struct ChartNote {
    double timeMs = 0.0;
    double tick = -1.0; // Audition sixteenth-note position; -1 for absolute-ms demo charts.
    int lane = 0;       // 7 (UpLeft), 4 (MidLeft), 1 (DownLeft), 9 (UpRight), 6 (MidRight), 3 (DownRight), 5 (Space)
    int type = 0;       // 0: Normal blue, 1: Chance orange, 2: Space Finish
    bool chance=false; // Runtime Chance flag; never exported into SLK.
    bool hit = false;
    bool missed = false;
    bool fading = false; // Native direction state 6: no longer accepts input, fades before Miss.
    bool finish = false; // Preserve SLK f flag; native finish animation is not implemented yet.
};

class SlkParser {
public:
    static bool LoadChart(const std::wstring& filePath, std::vector<ChartNote>& outNotes, double bpm = 150.0, bool allowEmpty = false);
};

} // namespace BeatUp

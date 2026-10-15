#pragma once
#include <cmath>
#include <algorithm>

namespace BeatUp::Rules {
// OldVTC: 0x964664..0x96468F gives tickMs = (240000/BPM)/16.
// 0xA125E0..0xA12613: 324 pixels / 8 ticks; Space is 134 / 8 ticks.
inline double DirectionSpeed(double bpm, double speed = 1.0) { return 324.0 * bpm / 120000.0 * speed; }
inline double SpaceSpeed(double bpm, double speed = 1.0) { return 134.0 * bpm / 120000.0 * speed; }
inline int SpaceDistance(double noteMs, double nowMs, double bpm, double speed = 1.0) {
    return static_cast<int>((noteMs - nowMs) * SpaceSpeed(bpm, speed));
}
inline float NoteX(double noteMs, double nowMs, double bpm, double speed, bool left) {
    // Anchors fit the supplied 6-lane skin; OldVTC's 6-lane input branch uses 319/705.
    double delta = (noteMs - nowMs) * DirectionSpeed(bpm, speed);
    return static_cast<float>(static_cast<int>(left ? 319.0 - delta : 705.0 + delta));
}
inline int DirectionDistance(double noteMs, double nowMs, double bpm, double speed, bool left) {
    int x = static_cast<int>(NoteX(noteMs, nowMs, bpm, speed, left));
    return left ? 319 - x : x - 705; // positive = early
}
inline int ArrowFrame(float x, bool left) {
    int distance = (std::max)(0, static_cast<int>(left ? 404.0f-x : x-620.0f));
    int phase = distance % 202;
    // Native distance-based four-frame cycle, rounded before wrapping.
    return 3 - (static_cast<int>(phase / 202.0 * 4.0 + 0.5) % 4);
}
// Native judgement indices: Perfect, Great, Cool, Bad, Miss; -1 = too early.
// 0xA13295 gates early directional presses; 0xA0BA20 uses pixel thresholds.
inline int JudgeDirection(int distance) {
    if (distance > 55) return -1;
    constexpr int limits[] = {5, 15, 27, 40, 55};
    int absolute = std::abs(distance);
    for (int i = 0; i < 5; ++i) if (absolute <= limits[i]) return i;
    return 4;
}
// 0xA0BA70 and 0xA0EAC0: Space uses its own asymmetric late boundary.
inline int JudgeSpace(int distance) {
    if (distance > 55) return -1;
    if (distance < -8) return 4;
    constexpr int limits[] = {3, 8, 20, 32, 45};
    for (int i = 0; i < 5; ++i) if (std::abs(distance) <= limits[i]) return i;
    return -1;
}
inline bool Expired(double noteMs, double nowMs, double bpm, double speed, int lane) {
    return lane == 5 ? SpaceDistance(noteMs, nowMs, bpm, speed) < -8
        : DirectionDistance(noteMs, nowMs, bpm, speed, lane == 7 || lane == 4 || lane == 1) <= -127;
}
}

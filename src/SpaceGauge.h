#pragma once
#include <array>
#include <algorithm>

namespace BeatUp {
struct SpaceGaugeVisual {
    // 0 = unlit, 1 = orange, 2 = blue. Display only: no input/scoring changes.
    std::array<int,6> letters{};
    int flame = 0;
    int digitBank = 0;
};
inline SpaceGaugeVisual GetSpaceGaugeVisual(int combo) {
    constexpr int orange[] = {10,20,40,60,80,100};
    constexpr int blue[] = {150,200,250,300,350,400};
    SpaceGaugeVisual result;
    for (int i=0;i<6;++i) result.letters[i] = combo >= blue[i] ? 2 : combo >= orange[i] ? 1 : 0;
    result.flame = combo >= 400 ? 2 : combo >= 100 ? 1 : 0;
    result.digitBank = combo >= 200 ? 2 : combo >= 100 ? 1 : 0;
    return result;
}

class SpaceMilestone {
public:
    void Reset() { m_previousCombo=0; m_level=0; m_startedMs=0; }
    void Observe(int combo, double nowMs) {
        int crossed = combo >= 400 && m_previousCombo < 400 ? 2
                    : combo >= 100 && m_previousCombo < 100 ? 1 : 0;
        if (crossed) { m_level=crossed; m_startedMs=nowMs; }
        if (combo == 0) m_level=0;
        m_previousCombo=combo;
    }
    int Level(double nowMs, double bpm) const {
        return m_level && nowMs >= m_startedMs && nowMs-m_startedMs < 120000.0/bpm ? m_level : 0;
    }
    double Age(double nowMs) const { return (std::max)(0.0,nowMs-m_startedMs); }
private:
    int m_previousCombo=0, m_level=0;
    double m_startedMs=0;
};
}

#pragma once
#include <algorithm>
#include <cmath>
namespace BeatUp {
inline int ComboLevel(int combo) {
    int level=0;for(int value:{10,20,40,60,80,100}) if(combo>=value) ++level;return level;
}
inline int ComboMiss(int& level) {
    constexpr int thresholds[]={10,20,40,60,80,100};
    if(level<=0 || level>=6) {level=0;return 0;}
    return thresholds[--level];
}
// Audition Old 8E6320..8E6369 and 8E6CC8..8E6DD0.
// Restore native phase durations; READY is limited to the preceding four beats.
struct IntroVisual {int stage=0;int count=0;};
inline IntroVisual NativeIntro(double now,double first,double bpm) {
    if(now<0 || first<0 || now>=first) return {};
    // Native phase comparisons and count progress use the integer tick clock.
    double remaining=std::ceil((first-now)/(15000.0/bpm)-1e-9);
    if(remaining>48) return {};
    if(remaining>32) return {1,0};
    if(remaining>16) return {2,0};
    int count=static_cast<int>(remaining/4);
    return {3,count>=1 && count<=3?count:0};
}
}

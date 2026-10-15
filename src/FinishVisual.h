#pragma once
#include <cmath>
namespace BeatUp {
struct FinishVisual {bool visible=false; float x=522;};
// Cdiu cma4.RenderFinish / ShowBS: starts one tick before the f event.
inline FinishVisual GetFinishVisual(double now,double finishMs,double bpm) {
    const double age=(now-finishMs)/(15000.0/bpm)+1.0;
    if(age<0 || age>=8) return {};
    int tick=static_cast<int>(age);double fraction=age-tick;
    return {tick==0 || tick==1 || tick==3 || tick==5 || tick==7,
        static_cast<float>(tick==0?8+514*fraction:tick==7?522+514*fraction:522)};
}
}

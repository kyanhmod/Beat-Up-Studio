#pragma once
#include <algorithm>
namespace BeatUp {
inline float CountdownWidth(unsigned width,unsigned height) {
    return height ? 64.0f*width/height : 64.0f;
}
struct ChancePromptSlide {
    float x=-256;
    bool entering=false;
    void Update(bool available,double deltaMs) {
        if(available && !entering && x<=-256) x=-210;
        entering=available;
        float step=static_cast<float>((std::max)(0.0,deltaMs))*(available?210.0f:256.0f)/500.0f;
        x=available?(std::min)(0.0f,x+step):(std::max)(-256.0f,x-step);
    }
    bool Visible() const {return x>-256;}
};
}

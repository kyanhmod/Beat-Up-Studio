#pragma once
#include <windows.h>
#include <algorithm>
namespace BeatUp {
struct ContentSize {float width=0,height=0;};
inline ContentSize FitContent(RECT bounds,float maxWidth,float maxHeight) {
    float w=static_cast<float>(bounds.right-bounds.left),h=static_cast<float>(bounds.bottom-bounds.top);
    if(w<=0 || h<=0 || maxWidth<=0 || maxHeight<=0)return {};
    float scale=(std::min)(maxWidth/w,maxHeight/h);return {w*scale,h*scale};
}
}

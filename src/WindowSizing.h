#pragma once
#include <windows.h>
#include <algorithm>
namespace BeatUp {
// Use the existing default Windows DPI virtualization consistently for GDI and D3D.
inline RECT WorkArea(HWND window=nullptr) {
    MONITORINFO info{sizeof(info)};
    HMONITOR monitor=window?MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST):MonitorFromPoint(POINT{},MONITOR_DEFAULTTOPRIMARY);
    if(GetMonitorInfoW(monitor,&info))return info.rcWork;
    RECT result{};SystemParametersInfoW(SPI_GETWORKAREA,0,&result,0);return result;
}
inline RECT InitialWindowRect(RECT work,int borderWidth,int borderHeight) {
    int availableW=(std::max)(4,static_cast<int>(work.right-work.left)-32-borderWidth);
    int availableH=(std::max)(3,static_cast<int>(work.bottom-work.top)-32-borderHeight);
    int unit=(std::max)(1,(std::min)({256,availableW/4,availableH/3}));
    int width=(std::min)(static_cast<int>(work.right-work.left),unit*4+borderWidth);
    int height=(std::min)(static_cast<int>(work.bottom-work.top),unit*3+borderHeight);
    int x=work.left+(work.right-work.left-width)/2,y=work.top+(work.bottom-work.top-height)/2;
    return {x,y,x+width,y+height};
}
inline void FitWindowToWorkArea(HWND window) {
    if(IsIconic(window) || IsZoomed(window))return;
    RECT work=WorkArea(window),r{};GetWindowRect(window,&r);
    int width=(std::min)(r.right-r.left,work.right-work.left),height=(std::min)(r.bottom-r.top,work.bottom-work.top);
    int x=std::clamp(static_cast<int>(r.left),static_cast<int>(work.left),static_cast<int>(work.right)-width);
    int y=std::clamp(static_cast<int>(r.top),static_cast<int>(work.top),static_cast<int>(work.bottom)-height);
    if(x!=r.left || y!=r.top || width!=r.right-r.left || height!=r.bottom-r.top)SetWindowPos(window,nullptr,x,y,width,height,SWP_NOACTIVATE|SWP_NOZORDER);
}
}

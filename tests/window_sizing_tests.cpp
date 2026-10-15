#include "WindowSizing.h"
#include <cassert>
#include <iostream>
int main() {
    for(RECT work: {RECT{0,0,1920,1040},RECT{0,0,1024,728},RECT{0,0,800,560},RECT{0,0,640,440},RECT{0,0,1280,680},RECT{-1280,40,0,760}}) {
        auto r=BeatUp::InitialWindowRect(work,16,59);
        assert(r.left>=work.left && r.top>=work.top && r.right<=work.right && r.bottom<=work.bottom);
        int w=r.right-r.left-16,h=r.bottom-r.top-59;
        assert(w*3==h*4 && w<=1024 && h<=768);
    }
    std::cout<<"PASS: startup bounds fit small/large/scaled work areas, taskbars and negative monitor origins; client remains 4:3\n";
}

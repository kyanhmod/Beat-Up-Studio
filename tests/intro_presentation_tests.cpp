#include "IntroPresentation.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    using namespace BeatUp;
    assert(CountdownWidth(324,64)==324 && CountdownWidth(32,64)==32);
    ChancePromptSlide p;p.Update(true,0);assert(p.x==-210);
    p.Update(true,500);assert(p.x==0 && p.Visible());
    p.Update(false,0);assert(p.x==0 && p.Visible());
    p.Update(false,250);assert(p.x==-128 && p.Visible());
    p.Update(false,250);assert(p.x==-256 && !p.Visible());
    p.Update(true,200);float x=p.x;p.Update(false,0);assert(p.x==x);
    p.Update(false,500);assert(!p.Visible());
    std::cout<<"PASS: countdown source aspect; prompt enters, retreats continuously and fully leaves screen\n";
}

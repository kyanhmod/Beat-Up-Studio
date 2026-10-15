#include "StudioShell.h"
#include <algorithm>
namespace BeatUp {
void StudioShell::Init(HWND owner) {owner_=owner;}
RECT StudioShell::Layout(int transportHeight) {
    RECT r{};GetClientRect(owner_,&r);
    return {0,0,r.right,(std::max)(1L,r.bottom)};
}
void StudioShell::SetMode(int mode) {mode_=mode;}
}

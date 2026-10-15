#pragma once
#include <windows.h>
namespace BeatUp {
// Stable workspace below one compact transport; file operations stay in menus.
class StudioShell {
public:
    void Init(HWND owner);
    RECT Layout(int transportHeight);
    void SetMode(int mode);
private:
    HWND owner_=nullptr;
    int mode_=1030;
};
}

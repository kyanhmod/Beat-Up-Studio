#pragma once
#include <windows.h>

namespace BeatUp {
inline WPARAM NormalizeKeypadKey(WPARAM key, LPARAM flags) {
    if (flags & (1LL<<24)) return key; // dedicated navigation cluster
    switch ((flags>>16)&0xff) {
        case 0x47:return VK_NUMPAD7;
        case 0x4b:return VK_NUMPAD4;
        case 0x4f:return VK_NUMPAD1;
        case 0x49:return VK_NUMPAD9;
        case 0x4d:return VK_NUMPAD6;
        case 0x51:return VK_NUMPAD3;
        case 0x52:return VK_NUMPAD0;
        case 0x4c:return VK_NUMPAD5;
        default:return key;
    }
}
}

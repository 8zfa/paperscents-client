#include "settings.h"
#include <Windows.h>

std::string KeybindSetting::KeyToString(int key)
{
    switch (key)
    {
    case 0: return "None";
    case VK_LBUTTON: return "LMB";
    case VK_RBUTTON: return "RMB";
    case VK_MBUTTON: return "M3";
    case VK_XBUTTON1: return "M4";
    case VK_XBUTTON2: return "M5";
    case VK_BACK: return "Back";
    case VK_TAB: return "Tab";
    case VK_RETURN: return "Enter";
    case VK_SHIFT: return "Shift";
    case VK_LSHIFT: return "LShift";
    case VK_RSHIFT: return "RShift";
    case VK_CONTROL: return "Ctrl";
    case VK_LCONTROL: return "LCtrl";
    case VK_RCONTROL: return "RCtrl";
    case VK_MENU: return "Alt";
    case VK_LMENU: return "LAlt";
    case VK_RMENU: return "RAlt";
    case VK_ESCAPE: return "Esc";
    case VK_SPACE: return "Space";
    case VK_PRIOR: return "PgUp";
    case VK_NEXT: return "PgDn";
    case VK_END: return "End";
    case VK_HOME: return "Home";
    case VK_INSERT: return "Ins";
    case VK_DELETE: return "Del";
    case VK_UP: return "Up";
    case VK_DOWN: return "Down";
    case VK_LEFT: return "Left";
    case VK_RIGHT: return "Right";
    case VK_CAPITAL: return "Caps";
    case VK_SNAPSHOT: return "PrtSc";
    case VK_SCROLL: return "Scroll";
    case VK_NUMLOCK: return "NumLk";
    case VK_DIVIDE: return "Num/";
    case VK_MULTIPLY: return "Num*";
    case VK_SUBTRACT: return "Num-";
    case VK_ADD: return "Num+";
    case VK_DECIMAL: return "Num.";
    case VK_F1: return "F1";
    case VK_F2: return "F2";
    case VK_F3: return "F3";
    case VK_F4: return "F4";
    case VK_F5: return "F5";
    case VK_F6: return "F6";
    case VK_F7: return "F7";
    case VK_F8: return "F8";
    case VK_F9: return "F9";
    case VK_F10: return "F10";
    case VK_F11: return "F11";
    case VK_F12: return "F12";
    default:
        if (key >= '0' && key <= '9') return std::string(1, (char)key);
        if (key >= 'A' && key <= 'Z') return std::string(1, (char)key);
        return "Key:" + std::to_string(key);
    }
}

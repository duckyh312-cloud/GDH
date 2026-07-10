#pragma once
#ifdef GEODE_IS_WINDOWS
#include <Windows.h>

namespace GDH { namespace WindowUtils {
    HWND FindMainHWND();

    struct WindowStateBackup {
        HWND hwnd;
        LONG Style;
        LONG ExStyle;
        RECT ClientRect;
        POINT WindowPos;
    };

    void setWindowBorderless(bool apply = false, int width = 1920, int height = 1080);
} }
#endif
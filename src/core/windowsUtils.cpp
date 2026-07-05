
#include <Geode/Geode.hpp>

#include "windowsUtils.hpp"

#ifdef GEODE_IS_WINDOWS

HWND hwnd_window = NULL;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);

    DWORD currentPid = GetCurrentProcessId();
    if (windowPid == currentPid && hwnd != GetConsoleWindow()) {
        if (!IsWindowVisible(hwnd)) {
            return TRUE;
        }

        if (GetParent(hwnd) != NULL) {
            return TRUE;
        }

        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        if (exStyle & WS_EX_TOOLWINDOW) {
            return TRUE;
        }

        hwnd_window = hwnd;
        return FALSE;
    }
    return TRUE;
}

HWND GDH::WindowUtils::FindMainHWND() {
    hwnd_window = NULL;
    EnumWindows(EnumWindowsProc, 0);
    return hwnd_window;
}

#endif

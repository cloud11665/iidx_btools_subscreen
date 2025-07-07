#include "utils.hpp"


void FindLargestMonitor(
    HMONITOR* hmon,
    RECT* rc
)
{
    struct { RECT rc{}; long area{}; HMONITOR mon{}; } best;
    EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR hm, HDC, LPRECT, LPARAM p)->BOOL {
            MONITORINFO mi{ sizeof(mi) }; GetMonitorInfo(hm, &mi);
            long w = mi.rcMonitor.right - mi.rcMonitor.left;
            long h = mi.rcMonitor.bottom - mi.rcMonitor.top;
            long a = w * h;
            auto* b = reinterpret_cast<decltype(best)*>(p);
            if (a > b->area) {
                b->area = a;
                b->rc = mi.rcMonitor;
                b->mon = hm;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&best));
    *rc = best.rc;
    *hmon = best.mon;
}

bool GetFirstOtherMonitor(
    HWND hwnd,
    HMONITOR* outHmonitor,
    RECT* outRect
)
{
    if (!hwnd || !outHmonitor || !outRect)
        return false;

    // Get window rect (in screen coordinates)
    RECT wndRect;
    if (!GetWindowRect(hwnd, &wndRect))
        return false;

    // Get the monitor the window is currently on
    HMONITOR currMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);

    struct MonitorInfo {
        HMONITOR hMonitor;
        RECT rcMonitor;
    };

    // Gather all monitors
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(
        nullptr, nullptr,
        [](HMONITOR hMon, HDC, LPRECT lprc, LPARAM lp) -> BOOL {
            auto* v = reinterpret_cast<std::vector<MonitorInfo>*>(lp);
            v->push_back(MonitorInfo{ hMon, *lprc });
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&monitors)
    );

    // Find the first monitor that is NOT the one the window is on
    for (const auto& mon : monitors) {
        if (mon.hMonitor != currMonitor) {
            *outHmonitor = mon.hMonitor;
            *outRect = mon.rcMonitor;
            return true;
        }
    }

    // No such monitor found (maybe only one monitor)
    return false;
}

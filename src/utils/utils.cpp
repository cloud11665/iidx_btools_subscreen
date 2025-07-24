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


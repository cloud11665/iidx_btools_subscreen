#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winuser.h>
#include <Shlwapi.h>
#include <shellscalingapi.h>
#include <d3d11.h>
#include <tchar.h>

#include <atomic>
#include <optional>
#include <vector>
#include <array>
#include <mutex>
#include <algorithm>
#include <utility>

void FindLargestMonitor(HMONITOR* hmon, RECT* rc);
bool GetFirstOtherMonitor(HWND hwnd, HMONITOR* outHmonitor, RECT* outRect);

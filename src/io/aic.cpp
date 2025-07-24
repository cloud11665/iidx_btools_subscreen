#define NOMINMAX
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>
#include <algorithm>

#include "globals.hpp"
#include "io/aic.hpp"

constexpr WORD VID = 0xCAFF;
constexpr WORD PID = 0x400E;
constexpr int  CARDIO_MI = 0;      // interface 00 = card reader


bool aic::icontains(const std::string& hay, const char* needle)
{
    auto it = std::search(hay.begin(), hay.end(),
        needle, needle + std::strlen(needle),
        [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    return it != hay.end();
}

bool aic::isDesiredCardIO(const std::string & path)
{
    char pat[64];
    std::snprintf(pat, sizeof(pat),
        "vid_%04x&pid_%04x&mi_%02x",
        VID, PID, CARDIO_MI);
    return icontains(path, pat);
}

// Enumerate every CardIO device path found right now
std::vector<std::string> aic::listCardIOPaths()
{
    std::vector<std::string> paths;
    GUID hidGuid;
    ::HidD_GetHidGuid(&hidGuid);

    HDEVINFO h = ::SetupDiGetClassDevs(&hidGuid, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (h == INVALID_HANDLE_VALUE) return paths;

    SP_DEVICE_INTERFACE_DATA ifd{ sizeof(ifd) };
    for (DWORD idx = 0; ::SetupDiEnumDeviceInterfaces(h, nullptr, &hidGuid, idx, &ifd); ++idx)
    {
        DWORD need = 0;
        ::SetupDiGetDeviceInterfaceDetail(h, &ifd, nullptr, 0, &need, nullptr);
        std::vector<char> buf(need);
        auto det = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(buf.data());
        det->cbSize = sizeof(*det);

        if (::SetupDiGetDeviceInterfaceDetail(h, &ifd, det, need, nullptr, nullptr))
        {
            std::string path = det->DevicePath;
            if (isDesiredCardIO(path)) paths.push_back(std::move(path));
        }
    }
    ::SetupDiDestroyDeviceInfoList(h);
    return paths;
}

// Try to open (or reopen) CARDIO by index; returns INVALID_HANDLE_VALUE on failure
HANDLE aic::openCardIOHandle(size_t deviceIndex, std::string& outPath)
{
    std::lock_guard<std::mutex> lock(g_aic_mutex);

    g_aic_cardio_paths = listCardIOPaths();

    if (deviceIndex >= g_aic_cardio_paths.size()) return INVALID_HANDLE_VALUE;

    outPath = g_aic_cardio_paths[deviceIndex];
    return ::CreateFile(outPath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
}

// ---------------------------------------------------------------- worker
void aic::readerThread(size_t deviceIndex)
{
    constexpr DWORD DISC_ERRS[] = {
        ERROR_DEVICE_NOT_CONNECTED, ERROR_INVALID_HANDLE,
        ERROR_BAD_COMMAND, ERROR_NO_DATA
    };

    std::string devPath;
    while (true)
    {
        // ---------- wait until we can open the device ----------
        HANDLE hDev = INVALID_HANDLE_VALUE;
        while (!hDev || hDev == INVALID_HANDLE_VALUE)
        {
            hDev = openCardIOHandle(deviceIndex, devPath);
            if (!hDev || hDev == INVALID_HANDLE_VALUE)
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << "[Device " << deviceIndex << "] opened: " << devPath << '\n';

        // ---------- read loop ----------
        std::vector<uint8_t> buf(100);
        OVERLAPPED ov{};
        DWORD got = 0;
        while (true)
        {
            bool ok = ::ReadFile(hDev, buf.data(), (DWORD)buf.size(), &got, &ov);
            if (!ok && ::GetLastError() == ERROR_IO_PENDING)
                ok = (::WaitForSingleObjectEx(hDev, INFINITE, TRUE) == WAIT_OBJECT_0) &&
                ::GetOverlappedResult(hDev, &ov, &got, FALSE);

            if (ok)         // got a full report
            {
                // byte 0    - card type
                // bytes 1-9 - card data
                if (got != 9) continue;
                std::lock_guard<std::mutex> lock(g_eamio_card_mutex);

                card_data_t card_data;
                std::copy_n(&buf[1], 8, card_data.begin());
                int reader_no = deviceIndex;
                if (g_aic_flip_readers)
                    reader_no = 1 - reader_no;

                if (reader_no == 0)
                {
                    g_eamio_card_p1 = card_data;
                }
                else
                {
                    g_eamio_card_p2 = card_data;
                }

                
                //std::printf("Dev %zu [%u B]: ", deviceIndex, got);
                //for (DWORD i = 0; i < got; ++i) std::printf("%02X", buf[i]);
                //std::putchar('\n');
            }
            else            // some error happened
            {
                DWORD e = ::GetLastError();
                bool disc = false;
                for (DWORD de : DISC_ERRS) if (e == de) { disc = true; break; }
 
                //if (disc)
                //    std::cerr << "[Device " << deviceIndex << "] disconnected – waiting…\n";
                //else
                //    std::cerr << "[Device " << deviceIndex << "] read error " << e << '\n';

                ::CloseHandle(hDev);
                break;      // break to outer "re‑open" loop
            }
        }
    }
}

void aic::init()
{
    g_aic_cardio_paths = listCardIOPaths();
    for (size_t i = 0; i < g_aic_cardio_paths.size(); ++i)
        g_aic_threads.emplace_back(readerThread, i);
}

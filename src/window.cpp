#include <cassert>
#include <algorithm>
#include <span>
#include <fstream>
#include <string>
#include <windows.h>
#include <ranges>

#include <d3d11.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "globals.hpp"
#include "window.hpp"
#include "defer.hpp"
#include "exceptions.hpp"
#include "widgets/widgets.hpp"
#include "resources.hpp"


static void CreateRT()
{
    ID3D11Texture2D* back = nullptr;
    HRESULT hr = g_dx11_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    if (FAILED(hr) || !back)
        throw d3d_error("GetBuffer(0) failed in CreateRT", hr);

    hr = g_dx11_dev->CreateRenderTargetView(back, nullptr, &g_dx11_RTV);
    back->Release();
    if (FAILED(hr) || !g_dx11_RTV)
        throw d3d_error("CreateRenderTargetView failed in CreateRT", hr);
}

static void CleanupRT()
{
    if (g_dx11_RTV)
    {
        g_dx11_RTV->Release();
        g_dx11_RTV = nullptr;
    }
}

static bool CreateDevice(HWND wnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.SampleDesc.Count = 1;
    sd.OutputWindow = wnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL lvlReq[]{ D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL got;
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, lvlReq, 1, D3D11_SDK_VERSION,
        &sd, &g_dx11_swap, &g_dx11_dev, &got, &g_dx11_ctx)))
        return false;

    CreateRT();
    return true;
}

static void CleanupDevice()
{
    CleanupRT();
    if (g_dx11_swap) g_dx11_swap->Release();
    if (g_dx11_ctx) g_dx11_ctx->Release();
    if (g_dx11_dev) g_dx11_dev->Release();
    g_dx11_swap = nullptr;
    g_dx11_ctx = nullptr;
    g_dx11_dev = nullptr;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l))
        return 0;

    switch (m)
    {
    case WM_SIZE:
    {
        if (w != SIZE_MINIMIZED && g_dx11_swap) {
            CleanupRT();
            g_dx11_swap->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            CreateRT();
        }
        return 0;
    }
    case WM_DESTROY:
    {
        g_running = false;
        return 0;
    }
    }
    return DefWindowProc(h, m, w, l);
}

static HWND FindIIDXWindow()
{
    static const char* bm2dx_class_prefix[] = {
        "beatmaniaIIDX",
        "beatmania IIDX",
        "C02"
    };

    HWND iidx_window = nullptr;
    ::EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        char className[256] = { 0 };

        bool found = false;
        if (::GetClassName(hwnd, className, sizeof(className)))
        {
            for (int i = 0; i < _countof(bm2dx_class_prefix); i++)
            {
                int length = int(strlen(bm2dx_class_prefix[i]));
                if (!strncmp(bm2dx_class_prefix[i], className, length))
                {
                    found = true;
                    // Optionally: copy className to g_iidx_name for later debug
                    strncpy_s(g_iidx_name, className, sizeof(g_iidx_name) - 1);
                    break;
                }
            }

            if (found) {
                *reinterpret_cast<HWND*>(lParam) = hwnd;
                return FALSE;
            }
        }
        return TRUE;
        }, reinterpret_cast<LPARAM>(&iidx_window));

    return iidx_window;
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
    if (!::GetWindowRect(hwnd, &wndRect))
        return false;

    // Get the monitor the window is currently on
    HMONITOR currMonitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);

    struct MonitorInfo {
        HMONITOR hMonitor;
        RECT rcMonitor;
    };

    // Gather all monitors
    std::vector<MonitorInfo> monitors;
    ::EnumDisplayMonitors(
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


auto window::find_iidx() -> void
{
    for (int i = 0; i < 10; i++)
    {
        g_iidx_hwnd = FindIIDXWindow();
        if (g_iidx_hwnd)
        {
            break;
        }
        Sleep(1000);
    }
}

auto window::create_device() -> void
{
    if (!GetFirstOtherMonitor(g_iidx_hwnd, &g_hmon, &g_rect))
        throw std::runtime_error("Could not find monitor");

    ImGui_ImplWin32_EnableDpiAwareness();
    g_dpi_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(g_hmon);

    g_wndclass = {
        sizeof(g_wndclass), CS_CLASSDC, WndProc, 0L, 0L,
        ::GetModuleHandle(nullptr),
        nullptr, nullptr, nullptr, nullptr,
        L"BM2DX_SUBMON_TOUCH", nullptr
    };

    if (!::RegisterClassExW(&g_wndclass))
        throw win32_error("RegisterClassEx failed");

    g_hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        g_wndclass.lpszClassName, L"", WS_POPUP,
        g_rect.left,
        g_rect.top,
        g_rect.right - g_rect.left,
        g_rect.bottom - g_rect.top,
        nullptr, nullptr, g_wndclass.hInstance, nullptr
    );
    if (!g_hwnd)
        throw win32_error("CreateWindowEx failed");

    if (!::SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA))
        throw win32_error("SetLayeredWindowAttributes failed");

    ::ShowWindow(g_hwnd, SW_SHOW); // Don't check result!

    if (!::SetWindowPos(g_hwnd, HWND_TOPMOST,
        g_rect.left, g_rect.top,
        g_rect.right - g_rect.left,
        g_rect.bottom - g_rect.top,
        SWP_FRAMECHANGED))
        throw win32_error("SetWindowPos failed");

    if (!::UpdateWindow(g_hwnd))
        throw win32_error("UpdateWindow failed");

    if (!::GetWindowRect(g_iidx_hwnd, &g_iidx_rect))
        throw win32_error("GetWindowRect failed");

    if (!CreateDevice(g_hwnd))
        throw std::runtime_error("CreateDevice failed");
}

auto window::init_imgui() -> void
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    ImFont* font = io.Fonts->AddFontFromMemoryTTF(
        reinterpret_cast<void*>(assets::iidx_font.data().data()),
        assets::iidx_font.data().size(),
        20.0f,
        &font_cfg
    );
    io.FontDefault = font;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(g_dpi_scale);
    style.FontScaleDpi = g_dpi_scale;

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dx11_dev, g_dx11_ctx);
}

auto window::cleanup() -> void
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    ::DestroyWindow(g_hwnd);
    ::UnregisterClassW(g_wndclass.lpszClassName, g_wndclass.hInstance);
}
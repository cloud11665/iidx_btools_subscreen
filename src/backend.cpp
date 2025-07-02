// ------------------------------------------------------------
//  Sub-monitor overlay that mirrors the system cursor
// ------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <iostream>
#include <atomic>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#pragma comment(lib,"d3d11.lib")

// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------
static std::atomic<long> g_mouseX{ 0 }, g_mouseY{ 0 };
static std::atomic<bool> g_running{ true };

static HWND                 g_overlayWnd = nullptr;
static HHOOK                g_mouseHook = nullptr;

static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static IDXGISwapChain* g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// ------------------------------------------------------------
//  Global mouse hook – fires for real mice *and* the first finger
// ------------------------------------------------------------
static LRESULT CALLBACK LLMouseProc(int code, WPARAM wp, LPARAM lp)
{
    if (code == HC_ACTION)
    {
        auto* ms = reinterpret_cast<PMSLLHOOKSTRUCT>(lp);
        g_mouseX = ms->pt.x;
        g_mouseY = ms->pt.y;
    }
    return CallNextHookEx(nullptr, code, wp, lp);
}

DWORD WINAPI InputThread(LPVOID)
{
    // global, system-wide (thread id = 0)
    g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, LLMouseProc,
        GetModuleHandle(nullptr), 0);

    // just keep the thread alive until shutdown
    MSG m;
    while (g_running && GetMessage(&m, nullptr, 0, 0))
        ;

    if (g_mouseHook)
        UnhookWindowsHookEx(g_mouseHook);
    return 0;
}

// ------------------------------------------------------------
//  D3D11 helpers
// ------------------------------------------------------------
static bool CreateDeviceAndSwap(HWND wnd, UINT w, UINT h)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.Width = w;
    sd.BufferDesc.Height = h;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferCount = 2;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = wnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL lvl;
    const D3D_FEATURE_LEVEL req[]{ D3D_FEATURE_LEVEL_11_0 };
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, req, 1, D3D11_SDK_VERSION,
        &sd, &g_swap, &g_dev, &lvl, &g_ctx)))
        return false;

    ID3D11Texture2D* bb{};
    g_swap->GetBuffer(0, IID_PPV_ARGS(&bb));
    g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv);
    bb->Release();
    return true;
}

static void Resize(UINT w, UINT h)
{
    if (!g_swap) return;
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    g_swap->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);

    ID3D11Texture2D* bb{};
    g_swap->GetBuffer(0, IID_PPV_ARGS(&bb));
    g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv);
    bb->Release();
}

// ------------------------------------------------------------
//  Overlay window
// ------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    ImGui_ImplWin32_WndProcHandler(h, m, w, l);

    switch (m)
    {
    case WM_SIZE:
        if (w != SIZE_MINIMIZED) Resize(LOWORD(l), HIWORD(l));
        return 0;
    case WM_DESTROY:
        g_running = false;
        return 0;
    }
    return DefWindowProc(h, m, w, l);
}

DWORD WINAPI OverlayThread(LPVOID)
{
    // choose the first non-primary monitor
    RECT rc{};
    EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR mon, HDC, LPRECT, LPARAM p)->BOOL {
            MONITORINFO mi{ sizeof(mi) };
            GetMonitorInfo(mon, &mi);
            if (!(mi.dwFlags & MONITORINFOF_PRIMARY))
            {
                *reinterpret_cast<RECT*>(p) = mi.rcMonitor;
                return FALSE;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&rc));

    if (rc.right == 0) {
        rc.right = GetSystemMetrics(SM_CXSCREEN);
        rc.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    WNDCLASSEX wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = _T("SubMonOverlay");
    wc.style = CS_CLASSDC;
    RegisterClassEx(&wc);

    DWORD ex = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE;
    DWORD wnd = WS_POPUP;

    g_overlayWnd = CreateWindowEx(ex, wc.lpszClassName, _T(""), wnd,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, wc.hInstance, nullptr);

    SetLayeredWindowAttributes(g_overlayWnd, 0, 255, LWA_ALPHA);
    ShowWindow(g_overlayWnd, SW_SHOW);
    SetWindowPos(g_overlayWnd, HWND_TOPMOST,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        SWP_FRAMECHANGED);

    if (!CreateDeviceAndSwap(g_overlayWnd, rc.right - rc.left, rc.bottom - rc.top))
        return 0;

    ImGui::CreateContext();
    ImGui_ImplWin32_Init(g_overlayWnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    MSG msg;
    while (g_running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg); DispatchMessage(&msg);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::Begin("Cursor", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("X : %ld", g_mouseX.load());
        ImGui::Text("Y : %ld", g_mouseY.load());
        ImGui::End();

        const float clr[4]{ 0,0,0,0 };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clr);

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_rtv)g_rtv->Release();
    if (g_swap)g_swap->Release();
    if (g_ctx) g_ctx->Release();
    if (g_dev) g_dev->Release();

    DestroyWindow(g_overlayWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return 0;
}

// ------------------------------------------------------------
//  DLL entry
// ------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID)
{
    if (r == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(h);

        AllocConsole(); FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);
        std::ios::sync_with_stdio(false);

        CreateThread(nullptr, 0, InputThread, nullptr, 0, nullptr);
        CreateThread(nullptr, 0, OverlayThread, nullptr, 0, nullptr);
    }
    return TRUE;
}

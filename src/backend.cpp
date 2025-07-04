// ---------------------------------------------------------------------------
//  dll_overlay.cpp  –  inject or LoadLibrary() to show a 1920×1080 ImGui
//                      overlay on the largest monitor
// ---------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Shlwapi.h>
#include <shellscalingapi.h>
#include <d3d11.h>
#include <tchar.h>
#include <atomic>
#include <tuple>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "Shlwapi.lib")

// ------------------------------ forward -------------------------------------
static DWORD WINAPI OverlayThread(LPVOID);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler
(HWND, UINT, WPARAM, LPARAM);

// ------------------------------ globals -------------------------------------
static std::atomic<bool> g_running{ false };
static HANDLE            g_thr = nullptr;
static HANDLE            inp_thr = nullptr;

static HWND g_iidx = nullptr;

// ----------------------- DX11 helpers (unchanged) ---------------------------
static ID3D11Device* gDev = nullptr;
static ID3D11DeviceContext* gCtx = nullptr;
static IDXGISwapChain* gSwap = nullptr;
static ID3D11RenderTargetView* gRTV = nullptr;

static std::atomic<long>  g_pxX{ 0 }, g_pxY{ 0 };     // cursor in screen pixels
static std::atomic<bool>  g_btnHeld{ false };
static HHOOK              g_mouseLL = nullptr;

static void CreateRT()
{
    ID3D11Texture2D* back{};
    gSwap->GetBuffer(0, IID_PPV_ARGS(&back));
    gDev->CreateRenderTargetView(back, nullptr, &gRTV);
    back->Release();
}
static void CleanupRT() { if (gRTV) { gRTV->Release(); gRTV = nullptr; } }

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
        &sd, &gSwap, &gDev, &got, &gCtx)))
        return false;

    CreateRT();
    return true;
}
static void CleanupDevice()
{
    CleanupRT();
    if (gSwap) gSwap->Release();
    if (gCtx) gCtx->Release();
    if (gDev) gDev->Release();
    gSwap = nullptr;
    gCtx = nullptr;
    gDev = nullptr;
}

// ------------------------- largest-monitor util -----------------------------
static std::tuple<RECT, HMONITOR> LargestMonitor()
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
    return std::make_tuple(best.rc, best.mon);
}

static LONG wpx = 0, wpy = 0;

// ----------------------------- WinProc --------------------------------------
static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    //if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return 0;

    switch (m)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            ImGui_ImplWin32_WndProcHandler(h, m, w, l);
            [[fallthrough]];
        }

        case WM_SIZE:
        {
            if (w != SIZE_MINIMIZED && gSwap) {
                CleanupRT();
                gSwap->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
                CreateRT();
            }
            return 0;
        }
        case WM_TOUCH:
        {
            UINT c = LOWORD(w);
            TOUCHINPUT* ti = new TOUCHINPUT[c];
            if (GetTouchInputInfo((HTOUCHINPUT)l, c, ti, sizeof(TOUCHINPUT)))
            {
                ImGuiIO& io = ImGui::GetIO();
                for (UINT i = 0; i < c || i < 1; ++i)
                {
                    wpx = ti[i].x;
                    wpy = ti[i].y;
                    //POINT p{ TOUCH_COORD_TO_PIXEL(ti[i].x),
                    //        TOUCH_COORD_TO_PIXEL(ti[i].y) };
                    //ScreenToClient(h, &p);
                    //io.AddMousePosEvent((float)p.x, (float)p.y);

                    //bool down = (ti[i].dwFlags & TOUCHEVENTF_DOWN) != 0;
                    //bool up = (ti[i].dwFlags & TOUCHEVENTF_UP) != 0;
                    //if (down) io.AddMouseButtonEvent(0, true), g_btnHeld = true;
                    //if (up)   io.AddMouseButtonEvent(0, false), g_btnHeld = false;

                    //ClientToScreen(h, &p);
                    //g_pxX = p.x; g_pxY = p.y;
                }
            }
            delete[] ti;
            CloseTouchInputHandle((HTOUCHINPUT)l);
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



// -----------------------------------------------------------------------------
//  Input – low-level mouse hook  (mouse + first finger synth. by Windows)
// -----------------------------------------------------------------------------
static LRESULT CALLBACK LLMouseProc(int code, WPARAM wp, LPARAM lp)
{
    if (code == HC_ACTION)
    {
        auto* ms = reinterpret_cast<PMSLLHOOKSTRUCT>(lp);
        g_pxX = ms->pt.x;  g_pxY = ms->pt.y;
        switch (wp)
        {
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
            g_btnHeld = true;  break;
        case WM_LBUTTONUP:   case WM_RBUTTONUP:   case WM_MBUTTONUP:
            g_btnHeld = false; break;
        }
    }
    return CallNextHookEx(nullptr, code, wp, lp);
}
DWORD WINAPI InputThread(LPVOID)
{
    g_mouseLL = SetWindowsHookExW(WH_MOUSE_LL, LLMouseProc,
        GetModuleHandle(nullptr), 0);
    MSG m; while (g_running && GetMessage(&m, nullptr, 0, 0));
    if (g_mouseLL) UnhookWindowsHookEx(g_mouseLL);
    return 0;
}

static HWND FindIIDXWindow()
{
    static const PWCHAR bm2dx_class_prefix[] = {
        L"beatmaniaIIDX",
        L"beatmania IIDX",
        L"C02"
    };

    HWND iidx_window = nullptr;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        WCHAR className[256] = { 0 };

        bool found = false;
        if (GetClassNameW(hwnd, className, 256))
        {
            for (int i = 0; i < _countof(bm2dx_class_prefix); i++)
            {
                if (!StrCmpNW(bm2dx_class_prefix[i], className, lstrlenW(bm2dx_class_prefix[i])))
                {
                    found = true;
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


// ----------------------------- overlay thread -------------------------------
static DWORD WINAPI OverlayThread(LPVOID)
{
    for (int i = 0; i < 10; i++)
    {
        g_iidx = FindIIDXWindow();
        if (g_iidx)
        {
            break;
        }
        Sleep(1000);
    }

    auto [rc, mon] = LargestMonitor();

    WNDCLASSEXW wc = {
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        GetModuleHandle(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        L"SubMonOverlay",
        nullptr
    };
    ::RegisterClassExW(&wc);

    HWND wnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"", WS_POPUP,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, wc.hInstance, nullptr);

    ::SetLayeredWindowAttributes(wnd, 0, 255, LWA_ALPHA);
    ::ShowWindow(wnd, SW_SHOW);
    ::SetWindowPos(wnd, HWND_TOPMOST,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        SWP_FRAMECHANGED);
    ::RegisterTouchWindow(wnd, 0);
    ::UpdateWindow(wnd);

    if (!CreateDevice(wnd)) return 0;
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();


    ImGui_ImplWin32_Init(wnd);
    ImGui_ImplDX11_Init(gDev, gCtx);


    bool lastBtn = false;
    g_running = true;
    MSG msg{};
    while (g_running && msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg); DispatchMessage(&msg);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::GetIO().DisplaySize = { 
            float(rc.right - rc.left),
            float(rc.bottom - rc.top)
        };

        //RECT src = SrcRect();
        //long sx = g_pxX.load();
        //long sy = g_pxY.load();
        //POINT local{ sx,sy }; ScreenToClient(g_overlay, &local);
        //io.AddMousePosEvent((float)local.x, (float)local.y);

        // button
        //bool curBtn = g_btnHeld.load();
        //if (curBtn != lastBtn)
        //    io.AddMouseButtonEvent(0, curBtn), lastBtn = curBtn;



        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        
        
        ImGui::Begin("debug_data");
            //RECT r; GetClientRect(wnd, &r);
            ImGui::Text("r.left  r.top    = (%d, %d)", (int)rc.left, (int)rc.top);
            ImGui::Text("r.right r.bottom = (%d, %d)", (int)rc.right, (int)rc.bottom);
            ImGui::Separator();
            ImGui::Text("raw pos (%d, %d)", (int)wpx, (int)wpy);
            ImGui::Text("g_iidx %p", g_iidx);
        ImGui::End();
        
        ImGui::Render();

        const float clr[4]{ 0.1f,0.1f,0.1f,1.f };
        gCtx->OMSetRenderTargets(1, &gRTV, nullptr);
        gCtx->ClearRenderTargetView(gRTV, clr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        gSwap->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    ::DestroyWindow(wnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

// ----------------------------- exports --------------------------------------
extern "C"
{
    __declspec(dllexport) void __cdecl StartOverlay()
    {
        if (!g_running) {
            g_thr = CreateThread(nullptr, 0, OverlayThread, nullptr, 0, nullptr);
            inp_thr = CreateThread(nullptr, 0, InputThread, nullptr, 0, nullptr);
        }
    }
    __declspec(dllexport) void __cdecl StopOverlay()
    {
        g_running = false;
        if (g_thr) {
            WaitForSingleObject(g_thr, 5000);
            CloseHandle(g_thr);
            g_thr = nullptr;

            WaitForSingleObject(inp_thr, 5000);
            CloseHandle(inp_thr);
            inp_thr = nullptr;
        }
    }
}

// ----------------------------- DLL entry ------------------------------------
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(GetModuleHandle(nullptr));
        //SetProcessDpiAwareness(PROCESS_DPI_UNAWARE);
        
        // auto-start overlay (comment if you prefer manual StartOverlay())
        StartOverlay();
    }
    return TRUE;
}

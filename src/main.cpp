#include <tchar.h>
#include <cstdio>
#include <string>

#include <windowsx.h>
#include <d3d9.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include "assets.hpp"
#include "style.hpp"
#include "widgets.hpp"
#include "textures.hpp"



// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static bool                     g_DeviceLost = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main2(int, char**)
{
    ImGui_ImplWin32_EnableDpiAwareness();
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX9 Example", WS_OVERLAPPEDWINDOW, 0, 0,
        //GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        1920, 1080,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    RegisterTouchWindow(hwnd, 0);
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(
        (void*)DF_POPMIX_W5_compressed_data,
        DF_POPMIX_W5_compressed_size,
        20.0f,
        &font_cfg
    );

    ImFont* seg16 = io.Fonts->AddFontFromMemoryCompressedTTF(
        (void*)SEG16_compressed_data,
        SEG16_compressed_size,
        200.0f,
        &font_cfg
    );

    // Set the font explicitly (optional — it’ll be the default if it’s the first font added)
    io.FontDefault = font;

    // Enable keyboard/gamepad navigation
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

    IDirect3DTexture9* texture = nullptr;
    int g_texWidth = 0;
    int g_texHeight = 0;

    const char* tex_path = "C:\\Users\\bemani\\Desktop\\iidx_btools_subscreen\\assets\\12.png";
    
    assert(LoadTextureFromFile(g_pd3dDevice, tex_path, &texture, &g_texWidth, &g_texHeight));


    // Style setup
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale);      // If you're using a DPI scale
    style.FontScaleDpi = scale;      // Same

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    static int effector_vals[5] = { 7, 7, 7, 0, 0 };

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        if (g_DeviceLost)
        {
            HRESULT hr = g_pd3dDevice->TestCooperativeLevel();
            if (hr == D3DERR_DEVICELOST) { ::Sleep(10); continue; }
            if (hr == D3DERR_DEVICENOTRESET) ResetDevice();
            g_DeviceLost = false;
        }

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth = g_ResizeWidth;
            g_d3dpp.BackBufferHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            ResetDevice();
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImVec2 displaySize = io.DisplaySize;

        //// Draw the background full-screen quad
        //ImDrawList* bg = ImGui::GetBackgroundDrawList();
        //bg->AddImage(
        //    (ImTextureID)(intptr_t)texture,       // your D3D9 texture
        //    ImVec2(0, 0),                         // top‐left
        //    ImVec2(displaySize.x, displaySize.y),// bottom‐right
        //    ImVec2(0, 0),                         // UV0
        //    ImVec2(1, 1),                         // UV1
        //    IM_COL32_WHITE                       // tint (no change)
        //);


        ImGui::ShowDemoWindow();
        ImGui::PushFont(seg16);


        ImVec2 ticker_sz = ImGui::CalcTextSize("⌓⌓⌓⌓⌓⌓⌓⌓⌓");
        ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
        ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImVec2 view_sz = main_viewport->Size;

        ImVec2 ticker_w_sz = {
            ticker_sz.x + window_padding.x * 2.f,
            ticker_sz.y + window_padding.y * 2.f
        };

        ImGui::SetNextWindowSize(ticker_w_sz);
        ImGui::SetNextWindowPos({
            (view_sz.x - ticker_w_sz.x) * 0.5f,
            0.f
        });
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.13f, 0.13f, 0.13f, 1.f));

        ImGui::Begin("16seg", nullptr, ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar);

        ImVec2 start = ImGui::GetCursorPos();
        ImGui::TextColored({ 0.15f, 0.15f, 0.15f, 1.f }, "⌓⌓⌓⌓⌓⌓⌓⌓⌓");
        ImGui::SetCursorPos(start + ImVec2{-1.f, 0.f});
        ImGui::TextColored({ 0.8f, 0.0f, 0.0f, 1.f}, "BEATMANIA");
        ImGui::PopFont();
        ImGui::End();
        ImGui::PopStyleColor(1);

        Effector(effector_vals);

        Keypad(0);
        Keypad(1);

        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA(114, 140, 153, 255);
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
        if (result == D3DERR_DEVICELOST) g_DeviceLost = true;
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    return g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) >= 0;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) { g_ResizeWidth = (UINT)LOWORD(lParam); g_ResizeHeight = (UINT)HIWORD(lParam); }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_TOUCH:
        UINT cInputs = LOWORD(wParam);
        TOUCHINPUT* pInputs = new TOUCHINPUT[cInputs];
        if (pInputs && GetTouchInputInfo((HTOUCHINPUT)lParam, cInputs, pInputs, sizeof(TOUCHINPUT)))
        {
            for (UINT i = 0; i < cInputs; ++i)
            {
                POINT pt;
                pt.x = TOUCH_COORD_TO_PIXEL(pInputs[i].x);
                pt.y = TOUCH_COORD_TO_PIXEL(pInputs[i].y);
                ScreenToClient(hWnd, &pt);

                // Simulate left mouse click
                ImGuiIO& io = ImGui::GetIO();
                io.AddMousePosEvent((float)pt.x, (float)pt.y);
                if (pInputs[i].dwFlags & TOUCHEVENTF_DOWN)
                    io.AddMouseButtonEvent(0, true);
                else if (pInputs[i].dwFlags & TOUCHEVENTF_UP)
                    io.AddMouseButtonEvent(0, false);
            }
            CloseTouchInputHandle((HTOUCHINPUT)lParam);
        }
        delete[] pInputs;
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}





//=======================================================================================






#include <cstring>
#include <iostream>

static HWND hwnd_submon = nullptr, hwnd_iidx = nullptr;
static HMONITOR hmon_submon = nullptr;
static WNDCLASSEXW wc = {
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
    L"CLS_BM2DX_BI2X_SUBMONITOR",
    nullptr
};

static const char* bm2dx_class_prefix[] = {
    "beatmaniaIIDX",
    "beatmania IIDX",
};

#define SUBMON_WIDTH  1920
#define SUBMON_HEIGHT 1080

struct CTX_FINDMONITOR {
    HMONITOR gameMonitor;
    HMONITOR targetMonitor;
};

static BOOL CALLBACK WindowEnumProcFind(HWND hwnd, LPARAM lParam) {
    auto data = reinterpret_cast<HWND*>(lParam);
    char className[256] = { 0 };

    if (GetClassNameA(hwnd, className, sizeof(className))) {
        bool is_match = false;
        if (!strcmp("C02", className)) {
            is_match = true;
            //iidx_slider_type = 1;
        }
        else {
            for (int i = 0; i < _countof(bm2dx_class_prefix) && !is_match; i++) {
                if (!strncmp(bm2dx_class_prefix[i], className, strlen(bm2dx_class_prefix[i]))) {
                    is_match = true;
                    //iidx_slider_type = 1;
                }
            }
        }

        // Add MAME emulator check if want to use on Twinkle games
        // And because only old styles(1st - 8th) uses "track volume" instead of "filter" effector
        // that conviently only runs under MAME(by far) we can set the silder type here as 0
        if (!is_match && !strcmp("MAME", className)) {
            is_match = true;
            //iidx_slider_type = 0;
        }

        if (is_match) {
            printf("Found game window '%s'\n", className);
            *data = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL CALLBACK MonitorEnumProcGet(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    auto target = reinterpret_cast<CTX_FINDMONITOR*>(dwData);
    if (hMonitor != target->gameMonitor) {
        target->targetMonitor = hMonitor;
        return FALSE;
    }

    return TRUE;
}

static BOOL CALLBACK MonitorEnumProcCount(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    auto c = reinterpret_cast<int*>(dwData);
    *c += 1;
    return TRUE;
}
#if 1
int main() {
    int init_retry = 10;
    HWND game_window = NULL;

    while (init_retry-- > 0) {
        game_window = NULL;
        EnumWindows(WindowEnumProcFind, reinterpret_cast<LPARAM>(&game_window));
        if (game_window) {
            break;
        }
        Sleep(1000);
    }

    if (!game_window) {
        printf("Cannot find game window after 10 seconds...\n");
        return -100;
    }

    RECT rect;
    if (GetWindowRect(game_window, &rect)) {
        HMONITOR hmon = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
        if (hmon) {
            CTX_FINDMONITOR find;
            find.targetMonitor = NULL;
            find.gameMonitor = hmon;

            EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProcGet, reinterpret_cast<LPARAM>(&find));

            hmon_submon = find.targetMonitor;
        }
    }

    std::cout << "hmon_submon = " << hmon_submon << std::endl;


    int nr_monitor = 0;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProcCount, reinterpret_cast<LPARAM>(&nr_monitor));

    std::cout << "nr_monitor = " << nr_monitor << std::endl;
}
#endif
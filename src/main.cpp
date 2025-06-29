#include <d3d9.h>
#include <tchar.h>
#include <cstdio>
#include <windowsx.h>
#include <string>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include "assets.hpp"
#include "style.hpp"
#include "widgets.hpp"


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

int main(int, char**)
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

    // Load your embedded font as the only font
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(
        (void*)DF_POPMIX_W5_compressed_data,
        DF_POPMIX_W5_compressed_size,
        20.0f,
        &font_cfg
    );

    //ImFont* font_big = io.Fonts->AddFontFromMemoryCompressedTTF(
    //    (void*)DF_POPMIX_W5_compressed_data,
    //    DF_POPMIX_W5_compressed_size,
    //    50.0f,
    //    &font_cfg
    //);


    // Set the font explicitly (optional — it’ll be the default if it’s the first font added)
    io.FontDefault = font;

    // Enable keyboard/gamepad navigation
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

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


        ImGui::ShowDemoWindow();

        ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
        ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
        ImVec2 w_size = {
            s.effector_width * 5 + 4 * spacing.x + 2 * window_padding.x,
            s.effector_height + window_padding.y
        };
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImVec2 view_sz = main_viewport->Size;
        ImGui::SetNextWindowPos({
            (view_sz.x - w_size.x) * 0.5f,
            (view_sz.y - w_size.y) * 0.8f
        });
        ImGui::SetNextWindowSize(w_size);
        ImGui::Begin("vefex_panel", nullptr,
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoBackground
        );
            ImGui::PushID(0);
                EffectorFader("VEFX", "エフェクトの程度", &effector_vals[0], 2);
                ImGui::SameLine();
            ImGui::PopID();

            ImGui::PushID(1);
                EffectorFader("low-EQ", "低音域", &effector_vals[1], 2);
                ImGui::SameLine();
            ImGui::PopID();

            ImGui::PushID(2);
                EffectorFader("hi-EQ", "高音域", &effector_vals[2], 2);
                ImGui::SameLine();
            ImGui::PopID();

            ImGui::PushID(3);
                EffectorFader("filter", "フィルター設定", &effector_vals[3], 0);
                ImGui::SameLine();
            ImGui::PopID();

            ImGui::PushID(4);
                EffectorFader("play volume", "ボリューム設定", &effector_vals[4], 0);
                ImGui::SameLine();
            ImGui::PopID();
        ImGui::End();

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

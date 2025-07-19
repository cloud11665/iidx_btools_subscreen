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

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "style.hpp"
#include "widgets.hpp"
#include "utils.hpp"
#include "aic.hpp"

#include "globals.hpp"

// Test ffmpeg
#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}
void encode_example() {
    const AVCodec* codec = avcodec_find_encoder_by_name("libopenh264");
    if (!codec) {
        std::cerr << "Encoder 'libopenh264' not found\n";
        return;
    }

    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        std::cerr << "Failed to allocate codec context\n";
        return;
    }

    ctx->bit_rate = 400000;
    ctx->width = 640;
    ctx->height = 480;
    ctx->time_base = {1, 25};
    ctx->framerate = {25, 1};
    ctx->gop_size = 10;
    ctx->max_b_frames = 1;
    ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        std::cerr << "Failed to open codec\n";
        avcodec_free_context(&ctx);
        return;
    }

    AVFrame* frame = av_frame_alloc();
    frame->format = ctx->pix_fmt;
    frame->width  = ctx->width;
    frame->height = ctx->height;
    av_frame_get_buffer(frame, 32);  // align

    AVPacket* pkt = av_packet_alloc();

    for (int i = 0; i < 5; i++) {
        av_frame_make_writable(frame);
        // Fill Y, U, V with dummy data
        for (int y = 0; y < ctx->height; y++) {
            memset(frame->data[0] + y * frame->linesize[0], i * 10, ctx->width);  // Y
        }
        for (int y = 0; y < ctx->height / 2; y++) {
            memset(frame->data[1] + y * frame->linesize[1], 128, ctx->width / 2); // U
            memset(frame->data[2] + y * frame->linesize[2], 128, ctx->width / 2); // V
        }

        frame->pts = i;

        int ret = avcodec_send_frame(ctx, frame);
        while (ret >= 0) {
            ret = avcodec_receive_packet(ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) {
                std::cerr << "Error during encoding\n";
                break;
            }
            std::cout << "Encoded frame " << i << ", size: " << pkt->size << " bytes\n";
            av_packet_unref(pkt);
        }
    }

    // Flush encoder
    avcodec_send_frame(ctx, nullptr);
    while (avcodec_receive_packet(ctx, pkt) == 0) {
        std::cout << "Flushed packet, size: " << pkt->size << " bytes\n";
        av_packet_unref(pkt);
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&ctx);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static void CreateRT()
{
    ID3D11Texture2D* back{};
    g_dx11_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    g_dx11_dev->CreateRenderTargetView(back, nullptr, &g_dx11_RTV);
    back->Release();
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

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
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
    static const WCHAR* bm2dx_class_prefix[] = {
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
                int length = lstrlenW(bm2dx_class_prefix[i]);
                if (!StrCmpNW(bm2dx_class_prefix[i], className, length))
                {
                    found = true;
                    ::WideCharToMultiByte(CP_UTF8, 0, className, -1, g_iidx_name, 256, NULL, NULL);
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

static LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0 || wParam != PM_REMOVE)
        return ::CallNextHookEx(g_iidx_msgHook, code, wParam, lParam);
    
    MSG* msg = reinterpret_cast<MSG*>(lParam);
    switch (msg->message) // lmao
    {
    case WM_POINTERENTER:
    case WM_POINTERDOWN:
    case WM_POINTERUPDATE:
    case WM_POINTERUP:
    case WM_POINTERLEAVE:
        break;
    default:
        return ::CallNextHookEx(g_iidx_msgHook, code, wParam, lParam);
    }

    const WORD pointerId = GET_POINTERID_WPARAM(msg->wParam);

    //g_touchpoints_mutex.lock();
    std::pair<DWORD, POINTER_INFO>* maybe_self = nullptr;
    std::pair<DWORD, POINTER_INFO>* first_free = nullptr;

    for (auto& val : g_touchpoints)
    {
        if (val.first == 0xffffffff && !first_free)
        {
            first_free = &val;
        }
        if (val.first == pointerId)
        {
            maybe_self = &val;
        }
    }

    //g_touchpoints_mutex.unlock();
    g_updates_captured++;

    POINTER_INFO* pi;

    switch (msg->message) {
    case WM_POINTERENTER:
        if (first_free)
        {
            GetPointerInfo(pointerId, &first_free->second);
            first_free->first = pointerId;
        }
        break;
    case WM_POINTERDOWN:
    case WM_POINTERUPDATE:
        if (!maybe_self)
            break;
        pi = &maybe_self->second;

        GetPointerInfo(pointerId, pi);
        if (!g_himetric_scale_x || !g_himetric_scale_y)
        {
            if (pi->ptPixelLocation.x > 32 &&
                pi->ptPixelLocation.y > 32)
            {
                float iidx_width = float(g_iidx_rect.right - g_iidx_rect.left);
                float iidx_height = float(g_iidx_rect.bottom - g_iidx_rect.top);
                float sub_width = float(g_rect.right - g_rect.left);
                float sub_height = float(g_rect.bottom - g_rect.top);

                float hi_x = float(pi->ptHimetricLocation.x);
                float px_x = float(pi->ptPixelLocation.x);
                g_himetric_scale_x = (sub_width / (hi_x / px_x * iidx_width));

                float hi_y = float(pi->ptHimetricLocation.y);
                float px_y = float(pi->ptPixelLocation.y);
                g_himetric_scale_y = (sub_height / (hi_y / px_y * iidx_height));
            }
        }
        break;
    case WM_POINTERUP:
    case WM_POINTERLEAVE:
        if (!maybe_self)
            break;
        maybe_self->first = 0xffffffff; // Mark as unused
        maybe_self->second = {};
        break;
    }
    return 0;
}

static DWORD WINAPI SubmonThread(LPVOID)
{
    for (auto& [id, v] : g_touchpoints)
    {
        id = 0xffffffff;
        v = { 0 };
    }

    Sleep(2000);

    for (int i = 0; i < 10; i++)
    {
        g_iidx_hwnd = FindIIDXWindow();
        if (g_iidx_hwnd)
        {
            break;
        }
        Sleep(1000);
    }

    aic::init();

    ::EnableMouseInPointer(TRUE);
    ::RegisterTouchWindow(g_iidx_hwnd, 0);

    DWORD processId;
    DWORD threadId = ::GetWindowThreadProcessId(g_iidx_hwnd, &processId);
    g_iidx_msgHook = ::SetWindowsHookExW(WH_GETMESSAGE, GetMsgProc, nullptr, threadId);
    HANDLE hThread = ::OpenThread(THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION, FALSE, threadId);
    if (hThread != nullptr) {
        ::SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
        ::CloseHandle(hThread);
    }

    GetFirstOtherMonitor(g_iidx_hwnd, &g_hmon, &g_rect);

    ImGui_ImplWin32_EnableDpiAwareness();
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(g_hmon);

    WNDCLASSEXW wc = { 
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        ::GetModuleHandleW(nullptr),
        nullptr, nullptr, nullptr, nullptr,
        L"BM2DX_SUBMON_TOUCH", nullptr
    };
    ::RegisterClassExW(&wc);

     g_hwnd = ::CreateWindowExW(
        WS_EX_LAYERED | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"", WS_OVERLAPPED,
        g_rect.left,
        g_rect.top,
        g_rect.right - g_rect.left,
        g_rect.bottom - g_rect.top,
        nullptr, nullptr, wc.hInstance, nullptr
     );

    ::SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA);
    ::ShowWindow(g_hwnd, SW_SHOW);
    ::SetWindowPos(g_hwnd, HWND_BOTTOM,
        g_rect.left, g_rect.top,
        g_rect.right - g_rect.left,
        g_rect.bottom - g_rect.top,
        SWP_FRAMECHANGED
    );
    ::UpdateWindow(g_hwnd);

    ::GetWindowRect(g_iidx_hwnd, &g_iidx_rect);

    if (!CreateDevice(g_hwnd)) return 0;
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;

    r_IDR_DF_POPMIX_W5.load();
    ImFont* font = io.Fonts->AddFontFromMemoryTTF(
        r_IDR_DF_POPMIX_W5.data(),
        r_IDR_DF_POPMIX_W5.size(),
        20.0f,
        &font_cfg
    );

    r_IDR_16SEGMENTS.load();
    s.font_seg16 = io.Fonts->AddFontFromMemoryTTF(
        r_IDR_16SEGMENTS.data(),
        r_IDR_16SEGMENTS.size(),
        200.0f,
        &font_cfg
    );

    io.FontDefault = font;
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    //io.ConfigFlags |= ImGuiConfigFlags_;
    
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale);      // If you're using a DPI scale
    style.FontScaleDpi = scale;      // Same

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dx11_dev, g_dx11_ctx);

    g_running = true;
    MSG msg{};
    while (g_running && msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::GetIO().DisplaySize = {
            float(g_rect.right - g_rect.left),
            float(g_rect.bottom - g_rect.top)
        };

        if (g_himetric_scale_x && g_himetric_scale_y)
        {
            static bool g_prev_pressed = false;
            bool is_pressed = false;
            ImVec2 pos;

            for (const auto& [k, v] : g_touchpoints)
            {
                if (k == 0xffffffff)
                    continue;

                pos.x = float(v.ptHimetricLocation.x) * g_himetric_scale_x.value();
                pos.y = float(v.ptHimetricLocation.y) * g_himetric_scale_y.value();
                is_pressed = true;
                break; // Only take the first
            }

            if (is_pressed)
            {
                io.AddMousePosEvent(pos.x, pos.y);
                io.AddMouseButtonEvent(0, 1);
            }

            if (!is_pressed && g_prev_pressed) {
                io.AddMouseButtonEvent(0, 0);
                io.AddMousePosEvent(-1.f, -1.f);
            }
            g_prev_pressed = is_pressed;
        }

        ImGui::NewFrame();

        ImDrawList* draw_list = ImGui::GetForegroundDrawList();

        
        //ImGui::Begin();

        //ImGui::End();


        //{
        //    std::scoped_lock<std::mutex> l(g_vefxio_ticker_mutex);
        //    Ticker16seg(g_vefxio_ticker_text);
        //}

        //{
        //    std::scoped_lock<std::mutex> l(g_vefxio_effector_mutex);
        //    Effector(g_vefxio_effector_state);
        //}


        //ImGui::ShowDemoWindow();
        
        DrawAll();
        
        ImGui::Render();

        const float clr[4]{ 0.1f,0.1f,0.1f,1.f };
        g_dx11_ctx->OMSetRenderTargets(1, &g_dx11_RTV, nullptr);
        g_dx11_ctx->ClearRenderTargetView(g_dx11_RTV, clr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_dx11_swap->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    ::DestroyWindow(g_hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}


// ----------------------------- DLL entry ------------------------------------
BOOL APIENTRY DllMain(
        HINSTANCE hinstDLL,
        DWORD     fdwReason,
        LPVOID    lpReserved
) 
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
        ::DisableThreadLibraryCalls(::GetModuleHandleA(nullptr));
        if (!g_running) {
            g_backend_h = ::CreateThread(nullptr, 0, SubmonThread, nullptr, 0, nullptr);
        }
    }
    return TRUE;
}

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

auto window::Resource::load() -> void
{
    if (this->m_data)
        throw std::logic_error("resource already loaded!");
    if (!g_hInstance)
        throw std::logic_error("g_hInstance is uninitialized!");

    HRSRC hRsrc = FindResource(g_hInstance, MAKEINTRESOURCE(this->m_tag), RT_RCDATA);
    if (!hRsrc)
        throw std::runtime_error("Failed to find resource!");

    HGLOBAL hGlob = ::LoadResource(g_hInstance, hRsrc);
    if (!hGlob)
        throw std::runtime_error("Failed to load resource!");

    this->m_data = ::LockResource(hGlob);
    this->m_size = ::SizeofResource(g_hInstance, hRsrc);
}

auto window::Resource::data() const -> std::span<uint8_t>
{
    if (this->m_data == nullptr) {
        throw std::logic_error("Resource::data() called before resource was loaded");
    }
    return { reinterpret_cast<uint8_t*>(this->m_data), static_cast<size_t>(this->m_size) };
}

window::Texture::~Texture()
{
    if (m_srv) m_srv->Release();
}

window::Texture::Texture(Texture&& other) noexcept
    : m_width(other.m_width), m_height(other.m_height), m_srv(other.m_srv)
{
    other.m_width = 0;
    other.m_height = 0;
    other.m_srv = nullptr;
}

window::Texture& window::Texture::operator=(window::Texture&& other) noexcept
{
    if (this != &other)
    {
        if (m_srv)
            m_srv->Release();

        m_width = other.m_width;
        m_height = other.m_height;
        m_srv = other.m_srv;

        other.m_width = 0;
        other.m_height = 0;
        other.m_srv = nullptr;
    }
    return *this;
}

auto window::Texture::from_buffer(std::span<uint8_t> buffer) -> Texture
{
    ID3D11ShaderResourceView* srv;
    int width, height;

    uint8_t* image_data = stbi_load_from_memory(buffer.data(), buffer.size(), &width, &height, nullptr, 4);
    if (image_data == nullptr)
        throw std::runtime_error("stbi_load_from_memory failed!");
    defer{ stbi_image_free(image_data); };

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;

    HRESULT hr;
    ID3D11Texture2D* pTexture = nullptr;
    hr = g_dx11_dev->CreateTexture2D(&desc, &subResource, &pTexture);
    if (FAILED(hr))
        throw std::runtime_error("CreateTexture2D failed!");
    defer { pTexture->Release(); };

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hr = g_dx11_dev->CreateShaderResourceView(pTexture, &srvDesc, &srv);
    if (FAILED(hr))
        throw std::runtime_error("CreateShaderResourceView failed! HRESULT = " + std::to_string(hr));
    
    return { width, height, srv };
}

auto window::Texture::from_file(std::string_view path) -> window::Texture
{
    std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("Could not open file");

    std::streamsize size = file.tellg();
    if (size < 0)
        throw std::runtime_error("File size error");
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        throw std::runtime_error("File read error");

    return Texture::from_buffer(buffer);
}


auto window::init_resources() -> void
{
    // Load resources (always safe to call load() repeatedly)
    res_iidx_font.load();
    res_icon_home.load();
    res_icon_keypad.load();
    res_keypad.load();
    res_keypad_close.load();
    res_icon_card.load();
    res_icon_settings.load();
    res_effector_bg.load();
    res_effector_0.load();
    res_effector_1.load();
    res_effector_head.load();
    res_touch_effect01.load();
    res_touch_effect_cross.load();
    res_touch_effect_cross2.load();

    // Load textures from resources
    tex_icon_home = Texture::from_buffer(res_icon_home.data());
    tex_icon_keypad = Texture::from_buffer(res_icon_keypad.data());
    tex_keypad = Texture::from_buffer(res_keypad.data());
    tex_keypad_close = Texture::from_buffer(res_keypad_close.data());
    tex_icon_card = Texture::from_buffer(res_icon_card.data());
    tex_icon_settings = Texture::from_buffer(res_icon_settings.data());
    tex_effector_bg = Texture::from_buffer(res_effector_bg.data());
    tex_effector_0 = Texture::from_buffer(res_effector_0.data());
    tex_effector_1 = Texture::from_buffer(res_effector_1.data());
    tex_effector_head = Texture::from_buffer(res_effector_head.data());
    tex_touch_effect01 = Texture::from_buffer(res_touch_effect01.data());
    tex_touch_effect_cross = Texture::from_buffer(res_touch_effect_cross.data());
    tex_touch_effect_cross2 = Texture::from_buffer(res_touch_effect_cross2.data());
}



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

LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
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
            ::GetPointerInfo(pointerId, &first_free->second);
            first_free->first = pointerId;
        }
        break;
    case WM_POINTERDOWN:
    case WM_POINTERUPDATE:
        if (!maybe_self)
            break;
        pi = &maybe_self->second;

        ::GetPointerInfo(pointerId, pi);
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

auto window::init_touch() -> void
{
    // Reset all touchpoints
    for (auto& [id, v] : g_touchpoints)
    {
        id = 0xffffffff;
        v = { 0 };
    }

    // Enable MouseInPointer (throws if it fails)
    if (!::EnableMouseInPointer(TRUE))
        throw win32_error("EnableMouseInPointer failed");

    //// RegisterTouchWindow (throws if it fails)
    //if (!::RegisterTouchWindow(g_iidx_hwnd, 0))
    //    throw win32_error("RegisterTouchWindow failed");

    DWORD processId;
    DWORD threadId = ::GetWindowThreadProcessId(g_iidx_hwnd, &processId);

    // SetWindowsHookExW (throws if it fails)
    g_iidx_msgHook = ::SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, nullptr, threadId);
    if (!g_iidx_msgHook)
        throw win32_error("SetWindowsHookExW failed");

    // Try to boost message thread priority, but only throw if OpenThread fails (priority change is non-fatal)
    HANDLE hThread = ::OpenThread(THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION, FALSE, threadId);
    if (!hThread)
        throw win32_error("OpenThread failed");
    if (!::SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST))
        throw win32_error("SetThreadPriority failed");
    ::CloseHandle(hThread);
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
        reinterpret_cast<void*>(res_iidx_font.data().data()),
        res_iidx_font.data().size(),
        20.0f,
        &font_cfg
    );
    io.FontDefault = font;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(g_dpi_scale);      // If you're using a DPI scale
    style.FontScaleDpi = g_dpi_scale;      // Same

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dx11_dev, g_dx11_ctx);
}

auto window::process_touch() -> void
{
    ImGuiIO& io = ImGui::GetIO();
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

        if (is_pressed && !g_prev_pressed) {
            g_active_touch_animations.push_back({ pos, g_frame_n });
        }

        g_prev_pressed = is_pressed;
    }
}

auto window::render_touch_animations() -> void
{
    int duration = 20;
    auto* draw_list = ImGui::GetForegroundDrawList();

    for (auto it = g_active_touch_animations.begin(); it != g_active_touch_animations.end(); )
    {
        int nf = g_frame_n - it->start_frame;
        if (nf > duration) {
            continue;
        }

        float t = nf / float(duration);

        // 1st effect: tex_touch_effect01
        {
            auto easeOutQuint = [](float t) { return 1 - (1 - t) * (1 - t) * (1 - t) * (1 - t) * (1 - t); };
            float scale = easeOutQuint(t);
            ImVec2 img_size = tex_touch_effect01->size() * scale * 0.7f;
            float alpha = 1.0f - t;

            draw_list->AddImage(
                tex_touch_effect01->srv(),
                it->pos - img_size * 0.5f,
                it->pos + img_size * 0.5f,
                ImVec2(0, 0), ImVec2(1, 1),
                IM_COL32(255, 255, 255, int(alpha * 255))
            );
        }

        // 2nd effect: tex_touch_effect_cross (uses half-duration, reverse scaling)
        {
            float t2 = nf / float(duration / 2);
            t2 = std::clamp(t2, 0.0f, 1.0f);
            auto easeOut = [](float t) { return 1 - (1 - t) * (1 - t); };
            float scale = easeOut(1.f - t2);
            ImVec2 img_size = tex_touch_effect_cross->size() * scale * 0.6f;
            float alpha = 1.0f - t2;

            draw_list->AddImage(
                tex_touch_effect_cross->srv(),
                it->pos - img_size * 0.5f,
                it->pos + img_size * 0.5f,
                ImVec2(0, 0), ImVec2(1, 1),
                IM_COL32(255, 255, 255, int(alpha * 255))
            );
        }

        // 3rd effect: tex_touch_effect_cross2 (static size, hard fade at 0.5)
        {
            float alpha = 0.f;
            if      (t < 0.3f) alpha = 0.8f;
            else if (t < 0.5f) alpha = 0.f;
            else if (t < 0.7f) alpha = 0.8f;
            else if (t < 0.9f) alpha = 0.f;
            else if (t < 1.0f) alpha = 0.8f;

            ImVec2 img_size = tex_touch_effect_cross2->size();
            draw_list->AddImage(
                tex_touch_effect_cross2->srv(),
                it->pos - img_size * 0.5f,
                it->pos + img_size * 0.5f,
                ImVec2(0, 0), ImVec2(1, 1),
                IM_COL32(255, 255, 255, int(alpha * 255))
            );
        }

        ++it;
    }

    std::vector<window::TouchAnimation> vt;
    for (auto ta : g_active_touch_animations)
    {
        if (g_frame_n - ta.start_frame < duration)
            vt.push_back(ta);
    }
    g_active_touch_animations = vt;
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
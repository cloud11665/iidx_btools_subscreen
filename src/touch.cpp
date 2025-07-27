#include <vector>

#include "imgui.h"
#include <windows.h>

#include "touch.hpp"
#include "globals.hpp"
#include "resources.hpp"
#include "exceptions.hpp"

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

auto touch::init_touch() -> void
{
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

    UINT_PTR off = CONTACTVISUALIZATION_OFF;    // = 0
    SystemParametersInfo(SPI_SETCONTACTVISUALIZATION,
        0,
        reinterpret_cast<PVOID>(off),
        SPIF_SENDCHANGE);       // broadcast WM_SETTINGCHANGE

    UINT_PTR gestOff = GESTUREVISUALIZATION_OFF;
    SystemParametersInfo(SPI_SETGESTUREVISUALIZATION,
        0,
        reinterpret_cast<PVOID>(gestOff),
        SPIF_SENDCHANGE);
}

auto touch::render_touch_animations() -> void
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
            ImVec2 img_size = assets::touch_effect01.tex()->size() * scale * 0.7f;
            float alpha = 1.0f - t;

            draw_list->AddImage(
                assets::touch_effect01.tex()->srv(),
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
            ImVec2 img_size = assets::touch_effect_cross.tex()->size() * scale * 0.6f;
            float alpha = 1.0f - t2;

            draw_list->AddImage(
                assets::touch_effect_cross.tex()->srv(),
                it->pos - img_size * 0.5f,
                it->pos + img_size * 0.5f,
                ImVec2(0, 0), ImVec2(1, 1),
                IM_COL32(255, 255, 255, int(alpha * 255))
            );
        }

        // 3rd effect: tex_touch_effect_cross2 (static size, hard fade at 0.5)
        {
            float alpha = 0.f;
            if (t < 0.3f) alpha = 0.8f;
            else if (t < 0.5f) alpha = 0.f;
            else if (t < 0.7f) alpha = 0.8f;
            else if (t < 0.9f) alpha = 0.f;
            else if (t < 1.0f) alpha = 0.8f;

            ImVec2 img_size = assets::touch_effect_cross2.tex()->size();
            draw_list->AddImage(
                assets::touch_effect_cross2.tex()->srv(),
                it->pos - img_size * 0.5f,
                it->pos + img_size * 0.5f,
                ImVec2(0, 0), ImVec2(1, 1),
                IM_COL32(255, 255, 255, int(alpha * 255))
            );
        }

        ++it;
    }

    std::vector<TouchAnimation> vt;
    for (auto ta : g_active_touch_animations)
    {
        if (g_frame_n - ta.start_frame < duration)
            vt.push_back(ta);
    }
    g_active_touch_animations = vt;
}

auto touch::process_touch() -> void
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
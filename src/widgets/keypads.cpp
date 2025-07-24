#include "imgui.h"

#include "globals.hpp"
#include "style.hpp"
#include "widgets/widgets.hpp"

auto keypads::draw() -> void
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImVec2 view_sz = main_viewport->Size;

    ImVec2 key_size = tex_keypad->size() * 1.4f;
    ImVec2 close_size = tex_keypad_close->size() * 1.4f;
    ImVec2 btsz = { 81.f, 81.f };
    float btoff = 98.f;
    ImVec2 btsz_big = { 81.f + btoff, 81.f };

    if (g_gui_keypad1_visible)
    {

        ImGui::SetNextWindowPos({
            s.keypad_padding,
            view_sz.y - key_size.y - s.keypad_padding
            });
        ImGui::SetNextWindowSize({
            key_size.x + window_padding.x * 2.f + close_size.x,
            key_size.y + window_padding.y * 2.f
            });

        ImGui::Begin("keypad0", nullptr,
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar
            | (g_debug ? 0 : ImGuiWindowFlags_NoBackground)
        );
        ImVec2 sp = ImGui::GetCursorScreenPos();
        ImVec2 base = sp + ImVec2(24.f, 24.f);

        ImGui::Image(tex_keypad->srv(), key_size);

        ImDrawList* dl = ImGui::GetWindowDrawList();

        auto keypad_button = [&](ImVec2 pos, int keycode, ImVec2 sz) {
            ImGui::PushID(keycode);
            ImGui::SetCursorScreenPos(base + pos);
            if (g_debug)
                dl->AddRect(base + pos, base + pos + sz, 0xff0000ff);

            ImGui::InvisibleButton("", sz);
            if (ImGui::IsItemHovered())
            {
                g_eamio_keypad_p1 |= (1 << keycode);
                ImVec2 off1 = { 1.f, 1.f };
                dl->AddRectFilled(base + pos - off1,
                    base + pos + sz + off1,
                    s.keypad_highlight_color,
                    2.f);
            }
            else
            {
                g_eamio_keypad_p1 &= ~(1 << keycode);
            }
            ImGui::PopID();
            };

        keypad_button(ImVec2(btoff * 0, btoff * 0), EAM_IO_KEYPAD_1, btsz);
        keypad_button(ImVec2(btoff * 1, btoff * 0), EAM_IO_KEYPAD_2, btsz);
        keypad_button(ImVec2(btoff * 2, btoff * 0), EAM_IO_KEYPAD_3, btsz);

        keypad_button(ImVec2(btoff * 0, btoff * 1), EAM_IO_KEYPAD_4, btsz);
        keypad_button(ImVec2(btoff * 1, btoff * 1), EAM_IO_KEYPAD_5, btsz);
        keypad_button(ImVec2(btoff * 2, btoff * 1), EAM_IO_KEYPAD_6, btsz);

        keypad_button(ImVec2(btoff * 0, btoff * 2), EAM_IO_KEYPAD_7, btsz);
        keypad_button(ImVec2(btoff * 1, btoff * 2), EAM_IO_KEYPAD_8, btsz);
        keypad_button(ImVec2(btoff * 2, btoff * 2), EAM_IO_KEYPAD_9, btsz);

        keypad_button(ImVec2(btoff * 0, btoff * 3), EAM_IO_KEYPAD_0, btsz);
        keypad_button(ImVec2(btoff * 1, btoff * 3), EAM_IO_KEYPAD_DECIMAL, btsz_big);

        ImVec2 bb = {
            sp.x + key_size.x,
            sp.y + key_size.y - close_size.y
        };
        ImGui::SetCursorScreenPos(bb);
        ImGui::Image(tex_keypad_close->srv(), close_size);
        if (g_debug) dl->AddRect(bb, bb + close_size, 0xff0000ff);

        ImGui::SetCursorScreenPos(bb);
        if (ImGui::InvisibleButton("p1_close", close_size))
        {
            g_gui_keypad1_visible = false;
        }
        if (ImGui::IsItemHovered())
        {
            ImVec2 off1 = { 1.f, 1.f };
            dl->AddRectFilled(bb - off1,
                bb + close_size + off1,
                s.keypad_highlight_color,
                2.f);
        }
        ImGui::End();
    }
    if (g_gui_keypad2_visible)
    {
        std::lock_guard<std::mutex> l(g_eamio_card_mutex);

        ImVec2 wsz = {
            key_size.x + window_padding.x * 2.f + close_size.x,
            key_size.y + window_padding.y * 2.f
        };
        ImGui::SetNextWindowSize(wsz);
        ImGui::SetNextWindowPos({
            view_sz.x - wsz.x - s.keypad_padding,
            view_sz.y - key_size.y - s.keypad_padding
            });

        ImGui::Begin("keypad2", nullptr,
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar
            | (g_debug ? 0 : ImGuiWindowFlags_NoBackground)
        );
        ImVec2 sp = ImGui::GetCursorScreenPos();
        ImVec2 base = sp + ImVec2(24.f, 24.f);
        base.x += close_size.x;

        ImGui::SetCursorScreenPos({ sp.x + close_size.x, sp.y });
        ImGui::Image(tex_keypad->srv(), key_size);

        ImDrawList* dl = ImGui::GetWindowDrawList();

        auto keypad_button = [&](ImVec2 pos, int keycode, ImVec2 sz) {
            ImGui::PushID(keycode);
            ImGui::SetCursorScreenPos(base + pos);
            if (g_debug)
                dl->AddRect(base + pos, base + pos + sz, 0xff0000ff);

            ImGui::InvisibleButton("", sz);
            if (ImGui::IsItemHovered())
            {
                g_eamio_keypad_p2 |= (1 << keycode);
                ImVec2 off1 = { 1.f, 1.f };
                dl->AddRectFilled(base + pos - off1,
                    base + pos + sz + off1,
                    s.keypad_highlight_color,
                    2.f);
            }
            else
            {
                g_eamio_keypad_p2 &= ~(1 << keycode);
            }
            ImGui::PopID();
            };

        keypad_button(ImVec2(btoff * 0, btoff * 0), EAM_IO_KEYPAD_1, btsz);
        keypad_button(ImVec2(btoff * 1, btoff * 0), EAM_IO_KEYPAD_2, btsz);
        keypad_button(ImVec2(btoff * 2, btoff * 0), EAM_IO_KEYPAD_3, btsz);

        keypad_button(ImVec2(btoff * 0, btoff * 1), EAM_IO_KEYPAD_4, btsz);
        keypad_button(ImVec2(btoff * 1, btoff * 1), EAM_IO_KEYPAD_5, btsz);
        keypad_button(ImVec2(btoff * 2, btoff * 1), EAM_IO_KEYPAD_6, btsz);

        keypad_button(ImVec2(btoff * 0, btoff * 2), EAM_IO_KEYPAD_7, btsz);
        keypad_button(ImVec2(btoff * 1, btoff * 2), EAM_IO_KEYPAD_8, btsz);
        keypad_button(ImVec2(btoff * 2, btoff * 2), EAM_IO_KEYPAD_9, btsz);

        keypad_button(ImVec2(btoff * 0, btoff * 3), EAM_IO_KEYPAD_0, btsz);
        keypad_button(ImVec2(btoff * 1, btoff * 3), EAM_IO_KEYPAD_DECIMAL, btsz_big);

        ImVec2 bb = {
            sp.x,
            sp.y + key_size.y - close_size.y
        };
        ImGui::SetCursorScreenPos(bb);
        ImGui::Image(tex_keypad_close->srv(), close_size);
        if (g_debug) dl->AddRect(bb, bb + close_size, 0xff0000ff);

        ImGui::SetCursorScreenPos(bb);
        if (ImGui::InvisibleButton("p2_close", close_size))
        {
            g_gui_keypad2_visible = false;
        }
        if (ImGui::IsItemHovered())
        {
            ImVec2 off1 = { 1.f, 1.f };
            dl->AddRectFilled(bb - off1,
                bb + close_size + off1,
                s.keypad_highlight_color,
                2.f);
        }
        ImGui::End();
    }
}

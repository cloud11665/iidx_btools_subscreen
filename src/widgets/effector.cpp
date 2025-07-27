#include <cstdint>
#include <fstream>
#include <string_view>
#include <filesystem>
#include <string>
#include <array>
#include <print>

#include "imgui.h"
#include "imgui_internal.h"

#include "widgets/widgets.hpp"
#include "globals.hpp"
#include "style.hpp"
#include "resources.hpp"

float scale = 1.5f;


auto draw_slider(
    const char* text_top,
    const char* text_bot,
    int type,
    float* value,
    int* value_int
) -> void
{
    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    ImVec2 window_padding = ImGui::GetStyle().WindowPadding;

    ImVec2 effector_0_sz = assets::effector_0.tex()->size() * scale;
    ImVec2 effector_bg_sz = assets::effector_bg.tex()->size() * scale;
    ImVec2 effector_head_sz = assets::effector_head.tex()->size() * scale;

    ImGui::BeginChild("slider", { s.effector_width, 0 });

        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 avail = ImGui::GetContentRegionAvail();

        {
            ImVec2 tsz = ImGui::CalcTextSize(text_top);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - tsz.x) * 0.5f);
            ImGui::TextUnformatted(text_top);
        }

        {
            ImVec2 tsz = ImGui::CalcTextSize(text_bot);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - tsz.x) * 0.5f);
            ImGui::TextUnformatted(text_bot);
        }

        ImVec2 pos = ImGui::GetCursorPos();
        pos.x -= (avail.x - effector_bg_sz.x) / 2;
        pos.y += 10;

        ImVec2 bg_pos = pos + (effector_0_sz - effector_bg_sz) / 2.f;
        ImGui::SetCursorPos(bg_pos);
        ImGui::Image(assets::effector_bg.tex()->srv(), effector_bg_sz);

        ImVec2 track_pos = pos;
        ImGui::SetCursorPos(pos);
        if (type == 0)
            ImGui::Image(assets::effector_0.tex()->srv(), effector_0_sz);
        else
            ImGui::Image(assets::effector_1.tex()->srv(), effector_0_sz);

        ImVec2 button_pos = {
            bg_pos.x + (effector_bg_sz.x - effector_head_sz.x) / 2,
            bg_pos.y + 47.f * scale
        };
        ImGui::SetCursorPos(button_pos);

        ImVec2 screen_pos = ImGui::GetCursorScreenPos();

        ImVec2 slider_size = {
            effector_head_sz.x,
            effector_0_sz.y - 41.f * scale
        };

        //ImGui::Button("##head", slider_size);
        ImGui::InvisibleButton("##head", slider_size, ImGuiButtonFlags_None);

        bool changed = false;
        const bool is_active = ImGui::IsItemActive();
        const bool is_hover = ImGui::IsItemHovered();

        const float track_thickness = 4.0f;
        const ImU32 track_col = IM_COL32(180, 180, 180, 255);

        float v_min = 0.0f;
        float v_max = 15.f;

        const float track_x = screen_pos.x + slider_size.x * 0.5f - track_thickness * 0.5f;
        const float track_top = screen_pos.y;
        const float track_bot = screen_pos.y + slider_size.y;

        if (is_active || (is_hover && ImGui::IsMouseDown(ImGuiMouseButton_Left)))
        {
            float mouse_t = (io.MousePos.y - track_top) / slider_size.y;   // 0..1
            mouse_t = ImClamp(mouse_t, 0.0f, 1.0f);
            float new_val = v_min + (v_max - v_min) * (1.0f - mouse_t); // invert (top = max)
            if (new_val != *value)
            {
                *value = new_val;
                changed = true;
            }
        }

        float t = (*value - v_min) / (v_max - v_min);   // 0..1
        float knob_y = track_top + (1.0f - t) * slider_size.y - effector_head_sz.y * 0.5f;
        knob_y = ImClamp(knob_y,
            track_top - effector_head_sz.y * 0.5f,
            track_bot - effector_head_sz.y * 0.5f);

        // Draw knob image
        ImVec2 knob_pos(screen_pos.x, knob_y);
        dl->AddImage(assets::effector_head.tex()->srv(),
            knob_pos,
            knob_pos + effector_head_sz,
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32_WHITE);

        *value_int = int(*value);

    ImGui::EndChild();

}

auto effector::draw() -> void
{
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
        (view_sz.y - w_size.y) * 0.9f
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
            draw_slider("VEFX", "エフェクトの程度", 0, &g_gui_effector_values[0], &g_vefxio_effector_state[0]);
            ImGui::SameLine();
        ImGui::PopID();

        ImGui::PushID(1);
            draw_slider("low-EQ", "低音域", 0, &g_gui_effector_values[1], &g_vefxio_effector_state[1]);
            ImGui::SameLine();
        ImGui::PopID();

        ImGui::PushID(2);
            draw_slider("hi-EQ", "高音域", 0, &g_gui_effector_values[2], &g_vefxio_effector_state[2]);
            ImGui::SameLine();
        ImGui::PopID();

        ImGui::PushID(3);
            draw_slider("filter", "フィルター設定", 1, &g_gui_effector_values[3], &g_vefxio_effector_state[3]);
            ImGui::SameLine();
        ImGui::PopID();

        ImGui::PushID(4);
            draw_slider("play volume", "ボリューム設定", 1, &g_gui_effector_values[4], &g_vefxio_effector_state[4]);
            ImGui::SameLine();
        ImGui::PopID();

    ImGui::End();
}
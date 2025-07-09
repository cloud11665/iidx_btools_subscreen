#include <cassert>
#include <string>

#include "imgui.h"
#include "imgui_internal.h"

#include "style.hpp"

void EffectorFader(const char* text_top, const char* text_bottom, int* value, int main_tick) {
    assert(0 <= *value && *value <= 14);

    ImGui::BeginChild("Effector", ImVec2(s.effector_width, s.effector_height));

        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
        ImVec2 window_padding = ImGui::GetStyle().FramePadding;

        {
            ImVec2 tsz = ImGui::CalcTextSize(text_top);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - tsz.x) * 0.5f);
            ImGui::TextUnformatted(text_top);
        }

        {
            ImVec2 tsz = ImGui::CalcTextSize(text_bottom);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - tsz.x) * 0.5f);
            ImGui::TextUnformatted(text_bottom);
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing.y);

        // — fader slider —
        ImVec2 winPos = ImGui::GetCursorScreenPos();
        avail = ImGui::GetContentRegionAvail();
        dl->AddRectFilled(winPos, winPos + avail, s.effector_background_color);
        dl->AddRect(winPos, winPos + avail, s.effector_background_frame_color, 0.f, 0, s.effector_background_frame_thickness);

        ImVec2 fader_sz = { s.effector_fader_width, s.effector_fader_height };
        ImVec2 track_sz = { s.effector_track_width, avail.y - window_padding.y * 4 };
        ImVec2 tick_small = { s.effector_tick_small_width, s.effector_tick_small_thickness };
        ImVec2 tick_big = { s.effector_tick_big_width, s.effector_tick_big_thickness };

        float slider_padding_x = (avail.x - fader_sz.x) * 0.5f;
        ImVec2 trackTL = {
            winPos.x + slider_padding_x + (fader_sz.x - track_sz.x) * 0.5f,
            winPos.y + window_padding.y * 2
        };
        ImVec2 trackBR = {
            trackTL.x + track_sz.x,
            trackTL.y + track_sz.y
        };

        // draw track
        dl->AddRectFilled(trackTL, trackBR, s.effector_track_color);

        auto draw_tick = [&](float y, ImVec2 size) {
            dl->AddLine(
                { trackBR.x, y },
                { trackBR.x + size.x, y },
                s.effector_tick_color, size.y
            );
            };

        float tick_offset_y = trackTL.y + fader_sz.y * 0.5f;
        float tick_step = (track_sz.y - fader_sz.y) * 0.25f;
        for (int i = 0; i < 5; i++) {
            ImVec2 tick_size = (i == main_tick) ? tick_big : tick_small;
            draw_tick(tick_offset_y + tick_step * float(i), tick_size);
        }

        // compute fader position
        float available = track_sz.y - fader_sz.y;
        float normValue = *value / 14.0f;
        ImVec2 faderTL = { winPos.x + slider_padding_x, trackTL.y + normValue * available };
        ImVec2 faderBR = { faderTL.x + fader_sz.x, faderTL.y + fader_sz.y };
        // invisible button for dragging
        ImGui::SetCursorScreenPos(faderTL);
        ImGui::InvisibleButton("fader", fader_sz);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            float my = ImGui::GetIO().MousePos.y - trackTL.y;
            *value = ImClamp(
                int((my / available) * 14.0f + 0.5f),
                0, 14
            );
        }

        // draw fader
        dl->AddRectFilled(faderTL, faderBR, s.effector_fader_color);

    ImGui::EndChild();
}

void Effector(int effector_vals[5]) {
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
}

int32_t Keypad(int side) {
    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImVec2 view_sz = main_viewport->Size;

    // calculate size...
    ImVec2 keypad_sz = {
        3 * s.keypad_unit_width + spacing.x * 2 + window_padding.x * 2,
        4 * s.keypad_unit_width + spacing.x * 4 + window_padding.x * 1 + s.keypad_font_size_tiny
    };
    if (side == 0) {
        ImGui::SetNextWindowPos({
            s.keypad_padding,
            view_sz.y - keypad_sz.y - s.keypad_padding
            });
    }
    else {
        ImGui::SetNextWindowPos({
            view_sz.x - keypad_sz.x - s.keypad_padding,
            view_sz.y - keypad_sz.y - s.keypad_padding
            });
    }
    ImGui::SetNextWindowSize(keypad_sz);

    // ——— Style setup ———
    // semi-transparent gray window background
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
    // white window border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushFont(nullptr, s.keypad_font_size);

    ImGui::Begin(("keypad" + std::to_string(side)).c_str(), nullptr,
        ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoScrollbar
    );


    // button styling: same semi-transparent gray, white border
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.7f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    // reuse the ImGuiCol_Border color already on the stack

    ImVec2 bt_size = { s.keypad_unit_width, s.keypad_unit_width };

    ImGui::Button("7", bt_size); ImGui::SameLine();
    ImGui::Button("8", bt_size); ImGui::SameLine();
    ImGui::Button("9", bt_size);

    ImGui::Button("4", bt_size); ImGui::SameLine();
    ImGui::Button("5", bt_size); ImGui::SameLine();
    ImGui::Button("6", bt_size);

    ImGui::Button("1", bt_size); ImGui::SameLine();
    ImGui::Button("2", bt_size); ImGui::SameLine();
    ImGui::Button("3", bt_size);

    ImGui::Button("0", bt_size); ImGui::SameLine();
    ImGui::Button("訂正", { s.keypad_unit_width * 2 + spacing.x, s.keypad_unit_width });

    ImGui::PushFont(nullptr, s.keypad_font_size_tiny);
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        const char* bot_text = "10KEY";
        ImVec2 tsz = ImGui::CalcTextSize(bot_text);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - tsz.x) * 0.5f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail.y - tsz.y) * 0.5f);
        ImGui::TextUnformatted(bot_text);
    }
    ImGui::PopFont();

    ImGui::PopStyleVar(1);        // FrameBorderSize
    ImGui::PopStyleColor(3);      // Button, Hovered, Active
    ImGui::End();

    // pop window styles
    ImGui::PopStyleColor(2);      // WindowBg, Border
    ImGui::PopStyleVar(1);        // WindowBorderSize
    ImGui::PopFont();

    return -1;
}

void Ticker16seg(const char* text)
{
    char text_data[10] = { 0 };
    memset(text_data, ' ', 9);
    strncpy(text_data, text, 9);
    ImGui::PushFont(s.font_seg16);

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
        ImGui::SetCursorPos(start + ImVec2{ -1.f, 0.f });
        ImGui::TextColored({ 0.8f, 0.0f, 0.0f, 1.f }, text_data);
    ImGui::End();
    
    ImGui::PopFont();
    ImGui::PopStyleColor(1);
}

#include "imgui.h"

#include "globals.hpp"
#include "widgets/widgets.hpp"
#include "resources.hpp"

auto concentration_button(const char* top_text) -> void
{
    ImVec2 conc_size = assets::icon_concentration_off.tex()->size() * 1.2f;

    ImVec2 cpos = ImGui::GetCursorPos();
    if (ImGui::InvisibleButton("concentration", conc_size))
    {
        g_gui_concentration = !g_gui_concentration;
        g_gui_concentration_frame = g_frame_n;
    }
    ImGui::SetCursorPos(cpos);

    {
        ImGui::PushFont(nullptr, 15);
        ImVec2 ts = ImGui::CalcTextSize(top_text);
        ImGui::SetCursorPosX(cpos.x + (conc_size.x - ts.x) / 2.f);
        ImGui::Text(top_text);
        ImGui::PopFont();
    }

    ImGui::SetCursorPos({
        cpos.x,
        cpos.y - 15.f
        });

    if (g_gui_concentration)
        ImGui::Image(
            assets::icon_concentration_on.tex()->srv(),
            conc_size
        );
    else
        ImGui::Image(
            assets::icon_concentration_off.tex()->srv(),
            conc_size
        );

    ImGui::SetCursorPos(cpos);

    {
        ImGui::PushFont(nullptr, 12);
        ImVec2 ts = ImGui::CalcTextSize("concentration");
        ImGui::SetCursorPos({
            cpos.x + (conc_size.x - ts.x) / 2.f,
            cpos.y + 55.f
            });
        ImGui::Text("concentration");
        ImGui::PopFont();
    }
    {
        ImGui::PushFont(nullptr, 12);
        ImVec2 ts = ImGui::CalcTextSize("mode");
        ImGui::SetCursorPos({
            cpos.x + (conc_size.x - ts.x) / 2.f,
            cpos.y + 67.f
            });
        ImGui::Text("mode");
        ImGui::PopFont();
    }
}

auto navbar::draw() -> void
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize({ io.DisplaySize.x, 100.f });
    ImGui::Begin("top_bar", nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    avail -= style.WindowPadding;


    float key_img_scale = avail.y / assets::icon_keypad.tex()->height();
    ImVec2 key_img_dim = assets::icon_keypad.tex()->size() * key_img_scale;

    float card_img_scale = avail.y / assets::icon_card.tex()->height();
    ImVec2 card_img_dim = assets::icon_card.tex()->size() * card_img_scale;

    float tint1 = g_gui_keypad1_visible ? 1.f : 0.5f;
    if (ImGui::ImageButton("p1_key_btn",
        assets::icon_keypad.tex()->srv(),
        key_img_dim,
        ImVec2(0, 0),
        ImVec2(1, 1),
        ImVec4(0, 0, 0, 0),
        ImVec4(tint1, tint1, tint1, 1)
    ))
    {
        g_gui_keypad1_visible = !g_gui_keypad1_visible;
    }
    ImGui::SameLine();

    ImGui::SetCursorPosX(avail.x - key_img_dim.x);

    float tint2 = g_gui_keypad2_visible ? 1.f : 0.5f;
    if (ImGui::ImageButton("p2_key_btn",
        assets::icon_keypad.tex()->srv(),
        key_img_dim,
        ImVec2(0, 0),
        ImVec2(1, 1),
        ImVec4(0, 0, 0, 0),
        ImVec4(tint2, tint2, tint2, 1)
    ))
    {
        g_gui_keypad2_visible = !g_gui_keypad2_visible;
    }
    ImGui::SameLine();

    float mid_pos = (avail.x - card_img_dim.x * 3 - 20.f * 1) * 0.5f;

    ImGui::SetCursorPosX(mid_pos);
    ImGui::Image(assets::icon_card.tex()->srv(), card_img_dim);
    ImGui::SameLine();
    ImGui::SetCursorPosX(mid_pos);
    if (ImGui::InvisibleButton("card_icon", card_img_dim))
    {
        g_gui_card_view_visible = !g_gui_card_view_visible;
    }
    ImGui::SameLine();

    ImGui::SetCursorPosX(mid_pos + card_img_dim.x + 20.f);
    ImGui::Image(assets::icon_settings.tex()->srv(), card_img_dim);
    ImGui::SameLine();
    ImGui::SetCursorPosX(mid_pos + card_img_dim.x + 20.f);
    if (ImGui::InvisibleButton("settings_icon", card_img_dim))
    {
        g_gui_settings_view_visible = !g_gui_settings_view_visible;
    }
    ImGui::SameLine();

    ImGui::SetCursorPosX(mid_pos + card_img_dim.x * 2 + 20.f * 2);
    ImGui::Image(assets::icon_coin.tex()->srv(), card_img_dim);
    ImGui::SameLine();
    ImGui::SetCursorPosX(mid_pos + card_img_dim.x * 2 + 20.f * 2);
    ImGui::InvisibleButton("coin_icon", card_img_dim);
    if (ImGui::IsItemHovered())
    {
        g_vefxio_ppad.store(1ull << 0x1D);
    }
    else
    {
        g_vefxio_ppad.store(0);
    }
    ImGui::SameLine();

    ImVec2 cpos = ImGui::GetCursorPos();
    ImVec2 spacing = style.ItemSpacing;
    ImVec2 conc_size = assets::icon_concentration_off.tex()->size() * 1.2f;

    ImGui::SetCursorPos({
        key_img_dim.x + spacing.x * 3,
        cpos.y
    });
    ImGui::PushID(0);
    concentration_button("PLAYER 01");
    ImGui::PopID();

    ImGui::SetCursorPos({
        avail.x - key_img_dim.x - spacing.x * 3 - conc_size.x,
        cpos.y
    });
    ImGui::PushID(1);
    concentration_button("PLAYER 02");
    ImGui::PopID();


    if (g_gui_concentration)
    {
        ImDrawList* fdl = ImGui::GetForegroundDrawList();
        ImGuiViewport* vp = ImGui::GetMainViewport();
        
        float t = (g_frame_n - g_gui_concentration_frame) / 45.f;
        t = std::clamp(t, 0.f, 1.f);
   
        fdl->AddRectFilled(
            { 0, 100 },
            vp->Size,
            ImColor(0, 0, 0, int(t * 255))
        );

        fdl->AddRectFilled(
            { 0, 0 },
            { vp->Size.x, 100 },
            ImColor(0, 0, 0, int(t * 128))
        );

        ImVec2 mp = ImGui::GetMousePos();
        if (mp.y > 100 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            g_gui_concentration = false;
            g_gui_concentration_frame = -1;
        }
    }

    ImGui::End();
}

#include "imgui.h"

#include "globals.hpp"
#include "widgets/widgets.hpp"


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


    float key_img_scale = avail.y / tex_icon_keypad->height();
    ImVec2 key_img_dim = tex_icon_keypad->size() * key_img_scale;

    float home_img_scale = avail.y / tex_icon_home->height();
    ImVec2 home_img_dim = tex_icon_home->size() * home_img_scale;

    float tint1 = g_gui_keypad1_visible ? 1.f : 0.5f;
    if (ImGui::ImageButton("p1_key_btn",
        tex_icon_keypad->srv(),
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
        tex_icon_keypad->srv(),
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

    ImGui::SetCursorPosX((avail.x - home_img_dim.x) * 0.5f);
    ImGui::Image(tex_icon_home->srv(), home_img_dim);
    ImGui::SameLine();
    ImGui::SetCursorPosX((avail.x - home_img_dim.x) * 0.5f);
    if (ImGui::InvisibleButton("home_icon", home_img_dim))
    {
        ImGui::OpenPopup("home_page");
    }
    if (ImGui::BeginPopup("home_page"))
    {
        ImGui::Text("dim %f %f", home_img_dim.x, home_img_dim.y);

        ImGui::EndPopup();
    }
    ImGui::End();
}

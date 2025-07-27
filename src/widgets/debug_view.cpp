#include "imgui.h"

#include "globals.hpp"
#include "style.hpp"
#include "widgets/widgets.hpp"

static bool show_demo = false;

auto debug_view::draw() -> void
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::Begin("debug_data");
    ImGui::Text("FPS: %.2f", 1.f / io.DeltaTime);
    ImGui::Text("rect {L=%d, T=%d, R=%d, B=%d}",
        (int)g_rect.left, (int)g_rect.top,
        (int)g_rect.right, (int)g_rect.bottom);

    ImGui::SeparatorText("IIDX");
    ImGui::Text("iidx_rect {L=%d, T=%d, R=%d, B=%d}",
        (int)g_iidx_rect.left, (int)g_iidx_rect.top,
        (int)g_iidx_rect.right, (int)g_iidx_rect.bottom);
    ImGui::Text("iidx_name \"%s\"", g_iidx_name);
    ImGui::Text("iidx_hwnd %p", g_iidx_hwnd);
    ImGui::Checkbox("debug", &g_debug);

    ImGui::SeparatorText("Touch input");
    ImGui::Text("touch_event_count: %d", g_touch_event_count.load());
    if (!g_himetric_scale_x || !g_himetric_scale_y)
        ImGui::Text("himetric_scale: (NULL, NULL)");
    else
        ImGui::Text("himetric_scale: (%.5f, %.5f)", g_himetric_scale_x.value(), g_himetric_scale_y.value());

    for (const auto& [k, v] : g_touchpoints)
    {
        float xposf = float(v.ptHimetricLocation.x) * g_himetric_scale_x.value_or(1.f);
        float yposf = float(v.ptHimetricLocation.y) * g_himetric_scale_y.value_or(1.f);
        int xpos = int(xposf);
        int ypos = int(yposf);
        if (k == 0xffffffff)
        {
            ImGui::Text("-");
            continue;
        }
        else
        {
            ImGui::Text("%04x -> (%d, %d) [%d, %d]",
                int(k),
                v.ptPixelLocation.x, v.ptPixelLocation.y,
                xpos, ypos
            );
        }
    }

    //for (auto tp : g_active_touch_animations)
    //{
    //    ImGui::Text("(%d, %d) %d", int(tp.pos.x), int(tp.pos.y), tp.start_frame);
    //}

    ImGui::SeparatorText("VEFXIO");
    ImGui::Text("enabled: %d", int(g_vefxio_enabled.load()));
    {
        std::lock_guard<std::mutex> l(g_vefxio_ticker_mutex);
        ImGui::Text("ticker_text: \"%s\"", g_vefxio_ticker_text);
    }
    {
        std::lock_guard<std::mutex> l(g_vefxio_effector_mutex);
        ImGui::Text("effector:");
        ImGui::SliderInt("[0]", &g_vefxio_effector_state[0], 0, 14);
        ImGui::SliderInt("[1]", &g_vefxio_effector_state[1], 0, 14);
        ImGui::SliderInt("[2]", &g_vefxio_effector_state[2], 0, 14);
        ImGui::SliderInt("[3]", &g_vefxio_effector_state[3], 0, 14);
        ImGui::SliderInt("[4]", &g_vefxio_effector_state[4], 0, 14);
    }

    ImGui::SeparatorText("AIC");
    {
        std::lock_guard<std::mutex> l(g_aic_mutex);
        ImGui::Checkbox("aic_flip_readers", &g_aic_flip_readers);

        for (int r_no = 0; r_no < (int)g_aic_cardio_paths.size(); r_no++)
        {
            ImGui::Text("[%d]: %s", (int)r_no, g_aic_cardio_paths[r_no].c_str());
        }
    }
    ImGui::NewLine();

    ImGui::SeparatorText("EAMIO");
    ImGui::Text("enabled: %d", int(g_eamio_enabled.load()));
    {
        std::lock_guard<std::mutex> lc(g_eamio_card_mutex);
        std::lock_guard<std::mutex> lk(g_eamio_keypad_mutex);

        ImGui::PushID("p1");
        ImGui::Text("keypad: %04x", (int)g_eamio_keypad_p1);
        if (g_eamio_card_p1)
            ImGui::Text("card: %02x%02x%02x%02x%02x%02x%02x%02x",
                g_eamio_card_p1.value()[0],
                g_eamio_card_p1.value()[1],
                g_eamio_card_p1.value()[2],
                g_eamio_card_p1.value()[3],
                g_eamio_card_p1.value()[4],
                g_eamio_card_p1.value()[5],
                g_eamio_card_p1.value()[6],
                g_eamio_card_p1.value()[7]);
        else
            ImGui::Text("card: NULL");
        ImGui::PopID();

        ImGui::PushID("p2");
        ImGui::Text("keypad: %04x", (int)g_eamio_keypad_p2);
        if (g_eamio_card_p2)
            ImGui::Text("card: %02x%02x%02x%02x%02x%02x%02x%02x",
                g_eamio_card_p2.value()[0],
                g_eamio_card_p2.value()[1],
                g_eamio_card_p2.value()[2],
                g_eamio_card_p2.value()[3],
                g_eamio_card_p2.value()[4],
                g_eamio_card_p2.value()[5],
                g_eamio_card_p2.value()[6],
                g_eamio_card_p2.value()[7]);
        else
            ImGui::Text("card: NULL");
        ImGui::PopID();
    }
    ImGui::Checkbox("show imgui demo", &show_demo);
    ImGui::ShowDemoWindow(&show_demo);
    ImGui::End();
}

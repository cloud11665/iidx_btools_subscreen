#include <cassert>
#include <string>
#include <mutex>


#include "imgui.h"
#include "imgui_internal.h"

#include <d3d11.h>

#include "globals.hpp"
#include "style.hpp"
#include "widgets.hpp"

//#include "textures.hpp"

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


void DrawAll()
{
#if 0
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    // top bar
    {
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


        float key_img_scale = avail.y / float(icon_10key_mini.height);
        ImVec2 key_img_dim = {
            float(icon_10key_mini.width) * key_img_scale,
            float(icon_10key_mini.height) * key_img_scale,
        };
        float home_img_scale = avail.y / float(icon_home.height);
        ImVec2 home_img_dim = {
            float(icon_home.width) * home_img_scale,
            float(icon_home.height) * home_img_scale,
        };

        float tint1 = g_gui_keypad1_visible ? 1.f : 0.5f;
        if (ImGui::ImageButton("p1_key_btn",
            icon_10key_mini.data,
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
            icon_10key_mini.data,
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
        ImGui::Image(icon_home.data, home_img_dim);
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

    // keypads
    {
        ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
        ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImVec2 view_sz = main_viewport->Size;

        ImVec2 key_size = usr_tenkey.size() * 1.4f;
        ImVec2 close_size = close_button.size() * 1.4f;
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

            ImGui::Image(usr_tenkey.data, key_size);

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
            ImGui::Image(close_button.data, close_size);
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
            ImGui::Image(usr_tenkey.data, key_size);

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
            ImGui::Image(close_button.data, close_size);
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
    
    //ImGui::ShowDemoWindow();

    Ticker();

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
    ImGui::End();
#endif
}

// Test ffmpeg

/*

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
*/

/*
    const auto bm2dx = modules::find("bm2dx.exe");

    const auto render_boot_text_p = memory::find(bm2dx.region(), "A1 ? ? ? ? 8B 4C 24 04 69 C0 ? ? ? ?");
    using render_boot_text_t = int (*) (int, int, uint32_t, const char*);
    static const render_boot_text_t render_boot_text = reinterpret_cast<render_boot_text_t>(render_boot_text_p);

    const auto print_boot_info_p = memory::find(bm2dx.region(), "7C A8 68 ? ? ? ?");

    static auto print_boot_hook = safetyhook::create_mid(print_boot_info_p,
        [](safetyhook::Context& ctx) {
            int y;
            if (g_iidx_version.value_or(0) > 14)
                y = 72 + 24 * 8;
            else
                y = 72 + 24 * 7;

            render_boot_text(32, y, 0x90FFFFFF, "------------------------------------");
            y += 24;

            render_boot_text(32, y, 0xFFFFFFFF, "SUBMON HOOK VER.   :");
            render_boot_text(240, y, 0xFF00FFFF, "v0.1");
            y += 24;

            render_boot_text(32, y, 0xffffff30, "MODULES ENABLED    :");
            std::string modules = "";
            if (g_eamio_enabled)
                modules += "EAMIO";
            if (g_eamio_enabled && g_vefxio_enabled)
                modules += ", ";
            if (g_vefxio_enabled)
                modules += "VEFXIO";
            render_boot_text(240, y, 0xffffff30, modules.c_str());
        }
    );
*/

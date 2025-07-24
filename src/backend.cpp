#include <print>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "globals.hpp"
#include "io/aic.hpp"
#include "window.hpp"
#include "widgets/widgets.hpp"
#include "exceptions.hpp"

static DWORD WINAPI SubmonThread(LPVOID)
{
    try
    {
        aic::init();
        Sleep(4000);
        window::find_iidx();
        window::init_touch();
        window::create_device();
        window::init_resources();
        window::init_imgui();
    }
    catch (const std::exception& ex)
    {
        std::println("Fatal error: {}", ex.what());
        return 1;
    }

    g_running = true;
    MSG msg{};
    while (g_running && msg.message != WM_QUIT)
    {
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        g_frame_n++;

        window::process_touch();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::GetIO().DisplaySize = {
            float(g_rect.right - g_rect.left),
            float(g_rect.bottom - g_rect.top)
        };
        ImGui::NewFrame();
        
        navbar::draw();
        ticker::draw();
        keypads::draw();
        debug_view::draw();
        ImGui::ShowDemoWindow();
        card_view::draw();

        //ImGui::Begin("animation2");
        //static bool running = false;
        //static int frame_n = 0;
        //frame_n++;
        //static int start_frame = 0;
        //if (ImGui::Button("start"))
        //{
        //    start_frame = frame_n;
        //    running = true;
        //}
        //int duration = 15;
        //if (running && frame_n - start_frame > duration)
        //{
        //    running = false;
        //}
        //if (running)
        //{
        //    int nf = frame_n - start_frame;
        //    float t = nf / float(duration);
        //    auto easeOut = [](float t) { return 1 - (1 - t) * (1 - t); };
        //    float scale = easeOut(t);
        //    ImVec2 img_size = tex_touch_effect01->size() * scale * 0.4;
        //    float alpha = 1.0f - t;
        //    ImVec2 center_point(200, 200);
        //    ImVec2 img_pos = center_point - img_size * 0.5f;
        //    ImGui::SetCursorScreenPos(ImGui::GetWindowPos() + img_pos);
        //    ImGui::ImageWithBg(tex_touch_effect01->srv(), img_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0,0,0,0), ImVec4(1, 1, 1, alpha));
        //}
        //if (running)
        //{
        //    int nf = frame_n - start_frame;
        //    float t = nf / float(duration / 2);
        //    if (t < 0.f) t = 0.f;
        //    if (t > 1.f) t = 1.f;
        //    auto easeOut = [](float t) { return 1 - (1 - t) * (1 - t); };
        //    float scale = easeOut(1.f - t);
        //    ImVec2 img_size = tex_touch_effect_cross->size() * scale * 0.6;
        //    float alpha = 1.0f - t;
        //    ImVec2 center_point(200, 200);
        //    ImVec2 img_pos = center_point - img_size * 0.5f;
        //    ImGui::SetCursorScreenPos(ImGui::GetWindowPos() + img_pos);
        //    ImGui::ImageWithBg(tex_touch_effect_cross->srv(), img_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, alpha));
        //}
        //if (running)
        //{
        //    int nf = frame_n - start_frame;
        //    float t = nf / float(duration);
        //    float alpha = t < 0.5f ? 0.8f : 0.f;
        //    ImVec2 img_size = tex_touch_effect_cross2->size();
        //    ImVec2 center_point(200, 200);
        //    ImVec2 img_pos = center_point - img_size * 0.5f;
        //    ImGui::SetCursorScreenPos(ImGui::GetWindowPos() + img_pos);
        //    ImGui::ImageWithBg(tex_touch_effect_cross2->srv(), img_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, alpha));
        //}
        //ImGui::End();

        window::render_touch_animations();


        ImGui::Render();
        if (!g_dx11_ctx || !g_dx11_RTV)
            throw std::runtime_error("DirectX device/context or RTV is null!");
        g_dx11_ctx->OMSetRenderTargets(1, &g_dx11_RTV, nullptr);
        
        const float clr[4]{ 0.1f,0.1f,0.1f,1.f };
        g_dx11_ctx->ClearRenderTargetView(g_dx11_RTV, clr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_dx11_swap->Present(1, 0);
        if (FAILED(hr)) {
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
                HRESULT remove_reason = g_dx11_dev->GetDeviceRemovedReason();
                throw d3d_error("DX11 device removed/reset during Present", remove_reason);
            }
            else {
                throw d3d_error("DX11 Present failed", hr);
            }
        }
    }

    window::cleanup();
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
        if (::AllocConsole())
        {
            FILE* fDummy;
            ::freopen_s(&fDummy, "CONOUT$", "w", stdout);
            ::freopen_s(&fDummy, "CONOUT$", "w", stderr);
            ::freopen_s(&fDummy, "CONIN$", "r", stdin);
            ::SetConsoleOutputCP(CP_UTF8);
        }
        ::DisableThreadLibraryCalls(::GetModuleHandle(nullptr));
        if (!g_running) {
            g_backend_h = ::CreateThread(nullptr, 0, SubmonThread, nullptr, 0, nullptr);
        }
    }
    return TRUE;
}

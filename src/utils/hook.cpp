#include <windows.h>

#include <cstdint>
#include <iostream>
#include <print>

#include "safetyhook.hpp"
#include "modules.hpp"
#include "memory.hpp"


DWORD WINAPI RunningFn(LPVOID)
{

    const auto bm2dx = modules::find("bm2dx.exe");

    const auto render_boot_text_p = memory::find(bm2dx.region(), "A1 ? ? ? ? 8B 4C 24 04 69 C0 ? ? ? ?");
    using render_boot_text_t = int (*) (int, int, uint32_t, const char*);
    static const render_boot_text_t render_boot_text = reinterpret_cast<render_boot_text_t>(render_boot_text_p);

    const auto print_boot_info_p = memory::find(bm2dx.region(), "7C A8 68 ? ? ? ?");

    static auto print_boot_hook = safetyhook::create_mid(print_boot_info_p,
        [](safetyhook::Context& ctx) {
            int y = 72 + 24 * 7;
            render_boot_text(32, y, 0xFFFFFFFF, "IIDX SUB HOOK      :");
            render_boot_text(240, y, 0xFF00FFFF, "v0.1 <color ffffff30>test</color>");
        }
    );

    //const auto eam3lib = modules::find("eam3lib.dll");

    //HMODULE eam3lib = ::GetModuleHandleA("eam3lib.dll");


    //const auto xprc_module_add_p = ::GetProcAddress(eam3lib, "xrpc_module_add");
    ////memory::find(eam3lib.region(), "53 55 57 8B 7C 24 10");
    //using xprc_module_add_t = int (__cdecl *) (const char*, int, int, int, int);
    //static const xprc_module_add_t xprc_module_add = reinterpret_cast<xprc_module_add_t>(xprc_module_add_p);

    //static auto xprc_module_add_hook = safetyhook::InlineHook{};
    //xprc_module_add_hook = safetyhook::create_inline(xprc_module_add_p,
    //    +[](const char* name, int a2, int a3, int a4, int a5)
    //    {
    //        std::println("xprc_module_add({}, {}, {}, {}, {})", name, a2, a3, a4, a5);
    //        xprc_module_add_hook.ccall<int>(name, a2, a3, a4, a5);
    //    }
    //);
    
    std::println("bm2dx.base {}", (void*)bm2dx.base);
    std::println("render_boot_text_p {}", (void*)render_boot_text_p);
    std::println("print_boot_info_p {}", (void*)print_boot_info_p);
    //std::println("xprc_module_add_p {}", (void*)xprc_module_add_p);

    return 0;
}

BOOL APIENTRY DllMain(
    HINSTANCE hinstDLL,
    DWORD     fdwReason,
    LPVOID    lpReserved
)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        if (::AllocConsole())
        {
            FILE* fDummy;
            ::freopen_s(&fDummy, "CONOUT$", "w", stdout);
            ::freopen_s(&fDummy, "CONOUT$", "w", stderr);
            ::freopen_s(&fDummy, "CONIN$", "r", stdin);
            ::SetConsoleOutputCP(CP_UTF8);
        }
        ::DisableThreadLibraryCalls(::GetModuleHandleA(nullptr));
        ::CreateThread(nullptr, 0, RunningFn, nullptr, 0, nullptr);
    }
    return TRUE;
}

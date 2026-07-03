#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <atomic>
#include <cstdint>

extern "C"
{
    #include "bemanitools/glue.h"
    #include "bemanitools/iidxio.h"
}

namespace
{
constexpr const char* kLogModule = "iidxio-bio2-vefx";
constexpr const char* kChildEnvVar = "IIDX_SUBSCREEN_IIDXIO_CHILD";
constexpr const char* kDefaultChildDll = "iidxio-bio2-child.dll";
constexpr const char* kVefxDll = "vefxio.dll";

enum pad_bit : uint8_t
{
    PAD_P1_1 = 0x08,
    PAD_P1_START = 0x18,
    PAD_P2_START = 0x19,
    PAD_VEFX = 0x1A,
    PAD_EFFECT = 0x1B,
    PAD_TEST = 0x1C,
    PAD_SERVICE = 0x1D,
    PAD_COIN = 0x1E,
};

using iidx_io_set_loggers_t = void (__cdecl*)(
    log_formatter_t,
    log_formatter_t,
    log_formatter_t,
    log_formatter_t);
using iidx_io_init_t = bool (__cdecl*)(
    thread_create_t,
    thread_join_t,
    thread_destroy_t);
using iidx_io_fini_t = void (__cdecl*)(void);
using iidx_io_ep1_set_deck_lights_t = void (__cdecl*)(uint16_t);
using iidx_io_ep1_set_panel_lights_t = void (__cdecl*)(uint8_t);
using iidx_io_ep1_set_top_lamps_t = void (__cdecl*)(uint8_t);
using iidx_io_ep1_set_top_neons_t = void (__cdecl*)(bool);
using iidx_io_ep1_send_t = bool (__cdecl*)(void);
using iidx_io_ep2_recv_t = bool (__cdecl*)(void);
using iidx_io_ep2_get_turntable_t = uint8_t (__cdecl*)(uint8_t);
using iidx_io_ep2_get_slider_t = uint8_t (__cdecl*)(uint8_t);
using iidx_io_ep2_get_sys_t = uint8_t (__cdecl*)(void);
using iidx_io_ep2_get_panel_t = uint8_t (__cdecl*)(void);
using iidx_io_ep2_get_keys_t = uint16_t (__cdecl*)(void);
using iidx_io_ep3_write_16seg_t = bool (__cdecl*)(const char*);

using vefx_io_set_loggers_t = void (__cdecl*)(
    log_formatter_t,
    log_formatter_t,
    log_formatter_t,
    log_formatter_t);
using vefx_io_init_t = bool (__cdecl*)(
    thread_create_t,
    thread_join_t,
    thread_destroy_t);
using vefx_io_fini_t = void (__cdecl*)(void);
using vefx_io_recv_t = bool (__cdecl*)(uint64_t*);
using vefx_io_get_slider_t = uint8_t (__cdecl*)(uint8_t);
using vefx_io_write_16seg_t = bool (__cdecl*)(const char*);

static log_formatter_t g_log_misc = nullptr;
static log_formatter_t g_log_info = nullptr;
static log_formatter_t g_log_warning = nullptr;
static log_formatter_t g_log_fatal = nullptr;
static bool g_have_loggers = false;

static HMODULE g_child_module = nullptr;
static iidx_io_set_loggers_t g_child_set_loggers = nullptr;
static iidx_io_init_t g_child_init = nullptr;
static iidx_io_fini_t g_child_fini = nullptr;
static iidx_io_ep1_set_deck_lights_t g_child_set_deck_lights = nullptr;
static iidx_io_ep1_set_panel_lights_t g_child_set_panel_lights = nullptr;
static iidx_io_ep1_set_top_lamps_t g_child_set_top_lamps = nullptr;
static iidx_io_ep1_set_top_neons_t g_child_set_top_neons = nullptr;
static iidx_io_ep1_send_t g_child_send = nullptr;
static iidx_io_ep2_recv_t g_child_recv = nullptr;
static iidx_io_ep2_get_turntable_t g_child_get_turntable = nullptr;
static iidx_io_ep2_get_slider_t g_child_get_slider = nullptr;
static iidx_io_ep2_get_sys_t g_child_get_sys = nullptr;
static iidx_io_ep2_get_panel_t g_child_get_panel = nullptr;
static iidx_io_ep2_get_keys_t g_child_get_keys = nullptr;
static iidx_io_ep3_write_16seg_t g_child_write_16seg = nullptr;

static HMODULE g_vefx_module = nullptr;
static vefx_io_set_loggers_t g_vefx_set_loggers = nullptr;
static vefx_io_init_t g_vefx_init = nullptr;
static vefx_io_fini_t g_vefx_fini = nullptr;
static vefx_io_recv_t g_vefx_recv = nullptr;
static vefx_io_get_slider_t g_vefx_get_slider = nullptr;
static vefx_io_write_16seg_t g_vefx_write_16seg = nullptr;

static std::atomic<uint64_t> g_pad_state{ 0 };

template <typename T>
bool load_proc(HMODULE module, const char* dll_name, const char* name, T& out)
{
    out = reinterpret_cast<T>(::GetProcAddress(module, name));

    if (!out)
    {
        if (g_log_warning)
            g_log_warning(kLogModule, "Missing %s in %s", name, dll_name);
        return false;
    }

    return true;
}

void clear_child_procs()
{
    g_child_set_loggers = nullptr;
    g_child_init = nullptr;
    g_child_fini = nullptr;
    g_child_set_deck_lights = nullptr;
    g_child_set_panel_lights = nullptr;
    g_child_set_top_lamps = nullptr;
    g_child_set_top_neons = nullptr;
    g_child_send = nullptr;
    g_child_recv = nullptr;
    g_child_get_turntable = nullptr;
    g_child_get_slider = nullptr;
    g_child_get_sys = nullptr;
    g_child_get_panel = nullptr;
    g_child_get_keys = nullptr;
    g_child_write_16seg = nullptr;
}

void clear_vefx_procs()
{
    g_vefx_set_loggers = nullptr;
    g_vefx_init = nullptr;
    g_vefx_fini = nullptr;
    g_vefx_recv = nullptr;
    g_vefx_get_slider = nullptr;
    g_vefx_write_16seg = nullptr;
}

const char* get_child_dll_name()
{
    static char child_dll[MAX_PATH] = {};
    DWORD len = ::GetEnvironmentVariableA(
        kChildEnvVar,
        child_dll,
        static_cast<DWORD>(sizeof(child_dll)));

    if (len == 0 || len >= sizeof(child_dll))
        return kDefaultChildDll;

    return child_dll;
}

bool load_child()
{
    if (g_child_module)
        return true;

    const char* child_dll = get_child_dll_name();
    g_child_module = ::LoadLibraryA(child_dll);

    if (!g_child_module)
    {
        if (g_log_warning)
        {
            g_log_warning(
                kLogModule,
                "Loading BIO2 child %s failed, GetLastError=%lu",
                child_dll,
                static_cast<unsigned long>(::GetLastError()));
        }

        return false;
    }

    bool ok = true;
    ok &= load_proc(g_child_module, child_dll, "iidx_io_set_loggers", g_child_set_loggers);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_init", g_child_init);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_fini", g_child_fini);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep1_set_deck_lights", g_child_set_deck_lights);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep1_set_panel_lights", g_child_set_panel_lights);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep1_set_top_lamps", g_child_set_top_lamps);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep1_set_top_neons", g_child_set_top_neons);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep1_send", g_child_send);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep2_recv", g_child_recv);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep2_get_turntable", g_child_get_turntable);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep2_get_slider", g_child_get_slider);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep2_get_sys", g_child_get_sys);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep2_get_panel", g_child_get_panel);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep2_get_keys", g_child_get_keys);
    ok &= load_proc(g_child_module, child_dll, "iidx_io_ep3_write_16seg", g_child_write_16seg);

    if (!ok)
    {
        clear_child_procs();
        ::FreeLibrary(g_child_module);
        g_child_module = nullptr;
    }

    return ok;
}

bool load_vefx()
{
    if (g_vefx_module)
        return true;

    g_vefx_module = ::LoadLibraryA(kVefxDll);

    if (!g_vefx_module)
    {
        if (g_log_warning)
        {
            g_log_warning(
                kLogModule,
                "Loading %s failed, GetLastError=%lu",
                kVefxDll,
                static_cast<unsigned long>(::GetLastError()));
        }

        return false;
    }

    bool ok = true;
    ok &= load_proc(g_vefx_module, kVefxDll, "vefx_io_set_loggers", g_vefx_set_loggers);
    ok &= load_proc(g_vefx_module, kVefxDll, "vefx_io_init", g_vefx_init);
    ok &= load_proc(g_vefx_module, kVefxDll, "vefx_io_fini", g_vefx_fini);
    ok &= load_proc(g_vefx_module, kVefxDll, "vefx_io_recv", g_vefx_recv);
    ok &= load_proc(g_vefx_module, kVefxDll, "vefx_io_get_slider", g_vefx_get_slider);
    ok &= load_proc(g_vefx_module, kVefxDll, "vefx_io_write_16seg", g_vefx_write_16seg);

    if (!ok)
    {
        clear_vefx_procs();
        ::FreeLibrary(g_vefx_module);
        g_vefx_module = nullptr;
    }

    return ok;
}

void set_loggers_on_loaded_modules()
{
    if (!g_have_loggers)
        return;

    if (g_vefx_set_loggers)
        g_vefx_set_loggers(g_log_misc, g_log_info, g_log_warning, g_log_fatal);

    if (g_child_set_loggers)
        g_child_set_loggers(g_log_misc, g_log_info, g_log_warning, g_log_fatal);
}

uint64_t bit(uint8_t pos)
{
    return uint64_t{ 1 } << pos;
}

uint64_t encode_child_pad()
{
    uint64_t pad = 0;

    const uint8_t sys = g_child_get_sys ? g_child_get_sys() : 0;
    const uint8_t panel = g_child_get_panel ? g_child_get_panel() : 0;
    const uint16_t keys = g_child_get_keys ? g_child_get_keys() : 0;

    if (sys & (1 << IIDX_IO_SYS_TEST))
        pad |= bit(PAD_TEST);
    if (sys & (1 << IIDX_IO_SYS_SERVICE))
        pad |= bit(PAD_SERVICE);
    if (sys & (1 << IIDX_IO_SYS_COIN))
        pad |= bit(PAD_COIN);

    if (panel & (1 << IIDX_IO_PANEL_P1_START))
        pad |= bit(PAD_P1_START);
    if (panel & (1 << IIDX_IO_PANEL_P2_START))
        pad |= bit(PAD_P2_START);
    if (panel & (1 << IIDX_IO_PANEL_VEFX))
        pad |= bit(PAD_VEFX);
    if (panel & (1 << IIDX_IO_PANEL_EFFECT))
        pad |= bit(PAD_EFFECT);

    for (uint8_t i = 0; i <= IIDX_IO_KEY_P2_7; i++)
    {
        if (keys & (1 << i))
            pad |= bit(PAD_P1_1 + i);
    }

    return pad;
}

uint8_t decode_sys(uint64_t pad)
{
    uint8_t sys = 0;

    if (pad & bit(PAD_TEST))
        sys |= (1 << IIDX_IO_SYS_TEST);
    if (pad & bit(PAD_SERVICE))
        sys |= (1 << IIDX_IO_SYS_SERVICE);
    if (pad & bit(PAD_COIN))
        sys |= (1 << IIDX_IO_SYS_COIN);

    return sys;
}

uint8_t decode_panel(uint64_t pad)
{
    return static_cast<uint8_t>((pad >> PAD_P1_START) & 0x0F);
}

uint16_t decode_keys(uint64_t pad)
{
    return static_cast<uint16_t>((pad >> PAD_P1_1) & 0x3FFF);
}
}

extern "C" __declspec(dllexport) void __cdecl iidx_io_set_loggers(
    log_formatter_t misc,
    log_formatter_t info,
    log_formatter_t warning,
    log_formatter_t fatal)
{
    g_log_misc = misc;
    g_log_info = info;
    g_log_warning = warning;
    g_log_fatal = fatal;
    g_have_loggers = true;

    load_vefx();
    load_child();
    set_loggers_on_loaded_modules();
}

extern "C" __declspec(dllexport) bool __cdecl iidx_io_init(
    thread_create_t thread_create,
    thread_join_t thread_join,
    thread_destroy_t thread_destroy)
{
    const bool have_vefx = load_vefx();

    if (!load_child())
        return false;

    set_loggers_on_loaded_modules();

    if (have_vefx && g_vefx_init)
    {
        if (!g_vefx_init(thread_create, thread_join, thread_destroy) && g_log_warning)
            g_log_warning(kLogModule, "vefx_io_init returned false");
    }

    return g_child_init(thread_create, thread_join, thread_destroy);
}

extern "C" __declspec(dllexport) void __cdecl iidx_io_fini(void)
{
    if (g_child_fini)
        g_child_fini();

    if (g_vefx_fini)
        g_vefx_fini();

    g_pad_state.store(0);

    if (g_child_module)
    {
        clear_child_procs();
        ::FreeLibrary(g_child_module);
        g_child_module = nullptr;
    }

    if (g_vefx_module)
    {
        clear_vefx_procs();
        ::FreeLibrary(g_vefx_module);
        g_vefx_module = nullptr;
    }
}

extern "C" __declspec(dllexport) void __cdecl iidx_io_ep1_set_deck_lights(uint16_t deck_lights)
{
    if (g_child_set_deck_lights)
        g_child_set_deck_lights(deck_lights);
}

extern "C" __declspec(dllexport) void __cdecl iidx_io_ep1_set_panel_lights(uint8_t panel_lights)
{
    if (g_child_set_panel_lights)
        g_child_set_panel_lights(panel_lights);
}

extern "C" __declspec(dllexport) void __cdecl iidx_io_ep1_set_top_lamps(uint8_t top_lamps)
{
    if (g_child_set_top_lamps)
        g_child_set_top_lamps(top_lamps);
}

extern "C" __declspec(dllexport) void __cdecl iidx_io_ep1_set_top_neons(bool top_neons)
{
    if (g_child_set_top_neons)
        g_child_set_top_neons(top_neons);
}

extern "C" __declspec(dllexport) bool __cdecl iidx_io_ep1_send(void)
{
    return g_child_send ? g_child_send() : false;
}

extern "C" __declspec(dllexport) bool __cdecl iidx_io_ep2_recv(void)
{
    if (!g_child_recv || !g_child_recv())
        return false;

    // Keep BIO2 hardware inputs live while allowing vefxio to add overlay bits.
    const uint64_t child_pad = encode_child_pad();
    uint64_t vefx_pad = child_pad;

    if (g_vefx_recv && !g_vefx_recv(&vefx_pad))
        return false;

    g_pad_state.store(child_pad | vefx_pad);
    return true;
}

extern "C" __declspec(dllexport) uint8_t __cdecl iidx_io_ep2_get_turntable(uint8_t player_no)
{
    return g_child_get_turntable ? g_child_get_turntable(player_no) : 0;
}

extern "C" __declspec(dllexport) uint8_t __cdecl iidx_io_ep2_get_slider(uint8_t slider_no)
{
    if (g_vefx_get_slider)
        return g_vefx_get_slider(slider_no);

    return g_child_get_slider ? g_child_get_slider(slider_no) : 0;
}

extern "C" __declspec(dllexport) uint8_t __cdecl iidx_io_ep2_get_sys(void)
{
    return decode_sys(g_pad_state.load());
}

extern "C" __declspec(dllexport) uint8_t __cdecl iidx_io_ep2_get_panel(void)
{
    return decode_panel(g_pad_state.load());
}

extern "C" __declspec(dllexport) uint16_t __cdecl iidx_io_ep2_get_keys(void)
{
    return decode_keys(g_pad_state.load());
}

extern "C" __declspec(dllexport) bool __cdecl iidx_io_ep3_write_16seg(const char* text)
{
    if (g_vefx_write_16seg)
        return g_vefx_write_16seg(text);

    if (g_child_write_16seg)
        return g_child_write_16seg(text);

    return true;
}

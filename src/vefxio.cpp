#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <atomic>

extern "C"
{
    #include "bemanitools/glue.h"
    #include "bemanitools/vefxio.h"
}

#define BT_API extern "C" __declspec(dllexport)

log_formatter_t misc_logger;
log_formatter_t info_logger;
log_formatter_t warning_logger;
log_formatter_t fatal_logger;

thread_create_t create_thread;
thread_join_t join_thread;
thread_destroy_t destroy_thread;

typedef void    (*backend_init_t)(void);
typedef bool    (*backend_write_16seg_t)(const char*);
typedef uint8_t (*backend_read_slider_t)(uint8_t);

static std::atomic_bool         g_backend_ready{ false };
static HMODULE                  g_backend{ nullptr };
static backend_init_t           backend_init{ nullptr };
static backend_write_16seg_t    backend_write_16seg{ nullptr };
static backend_read_slider_t    backend_read_slider{ nullptr };

int initialize(void* data)
{
    while (true)
    {
        g_backend = ::GetModuleHandleW(L"backend.dll");
        if (g_backend)
            break;
        info_logger("iidx_submon_vefxio", "Failed to find backend, retrying...");

        Sleep(1000);
    }

    info_logger("iidx_submon_vefxio", "found backend module (%p)", (void*)g_backend);
    
    backend_init = reinterpret_cast<backend_init_t>(::GetProcAddress(g_backend, "backend_vefxio_read_slider"));
    info_logger("iidx_submon_vefxio", "found backend_vefxio_read_slider (%p)", (void*)backend_init);

    backend_write_16seg = reinterpret_cast<backend_write_16seg_t>(::GetProcAddress(g_backend, "backend_vefxio_write_16seg"));
    info_logger("iidx_submon_vefxio", "found backend_vefxio_write_16seg (%p)", (void*)backend_write_16seg);
    
    backend_read_slider = reinterpret_cast<backend_read_slider_t>(::GetProcAddress(g_backend, "backend_vefxio_read_slider"));
    info_logger("iidx_submon_vefxio", "found backend_vefxio_read_slider (%p)", (void*)backend_read_slider);

    if (backend_init &&
        backend_write_16seg &&
        backend_read_slider)
    {
        backend_init();
        g_backend_ready = true;
    }

    return 0;
}


BT_API void __cdecl vefx_io_set_loggers(
    log_formatter_t misc,
    log_formatter_t info,
    log_formatter_t warning,
    log_formatter_t fatal
)
{
    misc_logger = misc;
    info_logger = info;
    warning_logger = warning;
    fatal_logger = fatal;
}

BT_API bool __cdecl vefx_io_init(
    thread_create_t thread_create,
    thread_join_t thread_join,
    thread_destroy_t thread_destroy
)
{
    create_thread = thread_create;
    join_thread = thread_join;
    destroy_thread = thread_destroy;

    create_thread(initialize, NULL, 0x4000, 0);

    return true;
}

BT_API void __cdecl vefx_io_fini(void)
{
    misc_logger("iidx_submon_vefxio", "Shutting down library");
}

BT_API bool __cdecl vefx_io_recv(uint64_t* ppad)
{
    return true;
}

BT_API uint8_t __cdecl vefx_io_get_slider(uint8_t slider_no)
{
    if (!g_backend_ready) return 0;
    return backend_read_slider(slider_no);
}

BT_API bool __cdecl vefx_io_write_16seg(const char* text)
{
    if (!g_backend_ready) return true;
    return backend_write_16seg(text);
}

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <atomic>

extern "C"
{
    #include "bemanitools/glue.h"
    #include "bemanitools/eamio.h"
}

#define BT_API extern "C" __declspec(dllexport)

log_formatter_t misc_logger;
log_formatter_t info_logger;
log_formatter_t warning_logger;
log_formatter_t fatal_logger;

thread_create_t create_thread;
thread_join_t join_thread;
thread_destroy_t destroy_thread;

typedef void        (*backend_init_t)(void);
typedef uint16_t    (*backend_get_keypad_state_t)(uint8_t);
typedef uint8_t     (*backend_get_sensor_state_t)(uint8_t);
typedef uint8_t     (*backend_read_card_t)(uint8_t, uint8_t*, uint8_t);
typedef uint8_t     (*backend_card_slot_cmd_t)(uint8_t, uint8_t);
typedef uint8_t     (*backend_poll_t)(uint8_t);

static std::atomic_bool             g_backend_ready{ false };
static HMODULE                      g_backend{ nullptr };
static backend_init_t               backend_init{ nullptr };
static backend_get_keypad_state_t   backend_get_keypad_state{ nullptr };
static backend_get_sensor_state_t   backend_get_sensor_state{ nullptr };
static backend_read_card_t          backend_read_card{ nullptr };
static backend_card_slot_cmd_t      backend_card_slot_cmd{ nullptr };
static backend_poll_t               backend_poll{ nullptr };

int initialize(void* data)
{
    while (true)
    {
        g_backend = ::GetModuleHandleW(L"backend.dll");
        if (g_backend)
            break;
        info_logger("iidx_submon_eamio", "Failed to find backend, retrying...");

        Sleep(1000);
    }

    info_logger("iidx_submon_eamio", "found backend module (%p)", (void*)g_backend);

    backend_init = reinterpret_cast<backend_init_t>(::GetProcAddress(g_backend, "backend_eamio_init"));
    info_logger("iidx_submon_eamio", "found backend_eamio_init (%p)", (void*)backend_init);

    backend_get_keypad_state = reinterpret_cast<backend_get_keypad_state_t>(::GetProcAddress(g_backend, "backend_eamio_get_keypad_state"));
    info_logger("iidx_submon_eamio", "found backend_eamio_get_keypad_state (%p)", (void*)backend_get_keypad_state);

    backend_get_sensor_state = reinterpret_cast<backend_get_sensor_state_t>(::GetProcAddress(g_backend, "backend_eamio_get_sensor_state"));
    info_logger("iidx_submon_eamio", "found backend_eamio_get_sensor_state (%p)", (void*)backend_get_sensor_state);

    backend_read_card = reinterpret_cast<backend_read_card_t>(::GetProcAddress(g_backend, "backend_eamio_read_card"));
    info_logger("iidx_submon_eamio", "found backend_eamio_read_card (%p)", (void*)backend_read_card);

    backend_card_slot_cmd = reinterpret_cast<backend_card_slot_cmd_t>(::GetProcAddress(g_backend, "backend_eamio_card_slot_cmd"));
    info_logger("iidx_submon_eamio", "found backend_eamio_card_slot_cmd (%p)", (void*)backend_card_slot_cmd);

    backend_poll = reinterpret_cast<backend_poll_t>(::GetProcAddress(g_backend, "backend_eamio_poll"));
    info_logger("iidx_submon_eamio", "found backend_eamio_poll (%p)", (void*)backend_poll);

    if (backend_init &&
        backend_get_keypad_state &&
        backend_get_sensor_state &&
        backend_read_card &&
        backend_card_slot_cmd &&
        backend_poll)
    {
        backend_init();
        g_backend_ready = true;
    }

    return 0;
}


BT_API void __cdecl eam_io_set_loggers(
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

BT_API bool __cdecl  eam_io_init(
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

BT_API void __cdecl eam_io_fini(void)
{
    misc_logger("iidx_submon_eamio", "Shutting down library");
}

BT_API uint16_t __cdecl eam_io_get_keypad_state(uint8_t unit_no)
{
    if (!g_backend_ready) return 0;
    return backend_get_keypad_state(unit_no);
}

BT_API uint8_t __cdecl eam_io_get_sensor_state(uint8_t unit_no)
{
    if (!g_backend_ready) return 0;
    return backend_get_sensor_state(unit_no);
}

BT_API uint8_t __cdecl eam_io_read_card(uint8_t unit_no, uint8_t* card_id, uint8_t nbytes)
{
    if (!g_backend_ready) return 0;
    return backend_read_card(unit_no, card_id, nbytes);
}

BT_API bool __cdecl eam_io_card_slot_cmd(uint8_t unit_no, uint8_t cmd)
{
    if (!g_backend_ready) return true;
    return backend_card_slot_cmd(unit_no, cmd);
}

BT_API bool __cdecl eam_io_poll(uint8_t unit_no)
{
    if (!g_backend_ready) return true;
    return backend_poll(unit_no);
}

BT_API const struct eam_io_config_api* __cdecl eam_io_get_config_api(void)
{
    return nullptr;
}
#ifndef BEMANITOOLS_IIDXIO_H
#define BEMANITOOLS_IIDXIO_H

/* IO emulation provider for beatmania IIDX. */

#include <stdbool.h>
#include <stdint.h>

#include "bemanitools/glue.h"

enum iidx_io_sys_bit {
    IIDX_IO_SYS_TEST = 0x00,
    IIDX_IO_SYS_SERVICE = 0x01,
    IIDX_IO_SYS_COIN = 0x02
};

enum iidx_io_panel_bit {
    IIDX_IO_PANEL_P1_START = 0x00,
    IIDX_IO_PANEL_P2_START = 0x01,
    IIDX_IO_PANEL_VEFX = 0x02,
    IIDX_IO_PANEL_EFFECT = 0x03
};

enum iidx_io_key_bit {
    IIDX_IO_KEY_P1_1 = 0x00,
    IIDX_IO_KEY_P1_2 = 0x01,
    IIDX_IO_KEY_P1_3 = 0x02,
    IIDX_IO_KEY_P1_4 = 0x03,
    IIDX_IO_KEY_P1_5 = 0x04,
    IIDX_IO_KEY_P1_6 = 0x05,
    IIDX_IO_KEY_P1_7 = 0x06,

    IIDX_IO_KEY_P2_1 = 0x07,
    IIDX_IO_KEY_P2_2 = 0x08,
    IIDX_IO_KEY_P2_3 = 0x09,
    IIDX_IO_KEY_P2_4 = 0x0A,
    IIDX_IO_KEY_P2_5 = 0x0B,
    IIDX_IO_KEY_P2_6 = 0x0C,
    IIDX_IO_KEY_P2_7 = 0x0D
};

enum iidx_io_deck_light {
    IIDX_IO_DECK_LIGHT_P1_1 = 0,
    IIDX_IO_DECK_LIGHT_P1_2 = 1,
    IIDX_IO_DECK_LIGHT_P1_3 = 2,
    IIDX_IO_DECK_LIGHT_P1_4 = 3,
    IIDX_IO_DECK_LIGHT_P1_5 = 4,
    IIDX_IO_DECK_LIGHT_P1_6 = 5,
    IIDX_IO_DECK_LIGHT_P1_7 = 6,

    IIDX_IO_DECK_LIGHT_P2_1 = 8,
    IIDX_IO_DECK_LIGHT_P2_2 = 9,
    IIDX_IO_DECK_LIGHT_P2_3 = 10,
    IIDX_IO_DECK_LIGHT_P2_4 = 11,
    IIDX_IO_DECK_LIGHT_P2_5 = 12,
    IIDX_IO_DECK_LIGHT_P2_6 = 13,
    IIDX_IO_DECK_LIGHT_P2_7 = 14,
};

enum iidx_io_panel_light {
    IIDX_IO_PANEL_LIGHT_P1_START = 0,
    IIDX_IO_PANEL_LIGHT_P2_START = 1,
    IIDX_IO_PANEL_LIGHT_VEFX = 2,
    IIDX_IO_PANEL_LIGHT_EFFECT = 3,
};

enum iidx_io_top_lamp {
    IIDX_IO_TOP_LAMP_LEFT_BLUE = 0,
    IIDX_IO_TOP_LAMP_LEFT_GREEN = 1,
    IIDX_IO_TOP_LAMP_LEFT_YELLOW = 2,
    IIDX_IO_TOP_LAMP_LEFT_RED = 3,
    IIDX_IO_TOP_LAMP_RIGHT_BLUE = 4,
    IIDX_IO_TOP_LAMP_RIGHT_GREEN = 5,
    IIDX_IO_TOP_LAMP_RIGHT_YELLOW = 6,
    IIDX_IO_TOP_LAMP_RIGHT_RED = 7,
};

__declspec(dllexport) void iidx_io_set_loggers(
    log_formatter_t misc,
    log_formatter_t info,
    log_formatter_t warning,
    log_formatter_t fatal);

__declspec(dllexport) bool iidx_io_init(
    thread_create_t thread_create,
    thread_join_t thread_join,
    thread_destroy_t thread_destroy);

__declspec(dllexport) void iidx_io_fini(void);

__declspec(dllexport) void iidx_io_ep1_set_deck_lights(uint16_t deck_lights);
__declspec(dllexport) void iidx_io_ep1_set_panel_lights(uint8_t panel_lights);
__declspec(dllexport) void iidx_io_ep1_set_top_lamps(uint8_t top_lamps);
__declspec(dllexport) void iidx_io_ep1_set_top_neons(bool top_neons);
__declspec(dllexport) bool iidx_io_ep1_send(void);

__declspec(dllexport) bool iidx_io_ep2_recv(void);
__declspec(dllexport) uint8_t iidx_io_ep2_get_turntable(uint8_t player_no);
__declspec(dllexport) uint8_t iidx_io_ep2_get_slider(uint8_t slider_no);
__declspec(dllexport) uint8_t iidx_io_ep2_get_sys(void);
__declspec(dllexport) uint8_t iidx_io_ep2_get_panel(void);
__declspec(dllexport) uint16_t iidx_io_ep2_get_keys(void);

__declspec(dllexport) bool iidx_io_ep3_write_16seg(const char *text);

#endif

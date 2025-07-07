#pragma once

extern "C"
{
#include "bemanitools/glue.h"
#include "bemanitools/vefxio.h"
#include "bemanitools/eamio.h"


__declspec(dllexport) void		backend_vefxio_init();
__declspec(dllexport) bool		backend_vefxio_write_16seg(const char*);
__declspec(dllexport) uint8_t	backend_vefxio_read_slider(uint8_t);

__declspec(dllexport) void		backend_eamio_init();
__declspec(dllexport) uint16_t	backend_eamio_get_keypad_state(uint8_t);
__declspec(dllexport) uint8_t	backend_eamio_get_sensor_state(uint8_t);
__declspec(dllexport) uint8_t	backend_eamio_read_card(uint8_t, uint8_t*, uint8_t);
__declspec(dllexport) uint8_t	backend_eamio_card_slot_cmd(uint8_t, uint8_t);
__declspec(dllexport) uint8_t	backend_eamio_poll(uint8_t);

}

#pragma once

extern "C"
{
	#include "bemanitools/glue.h"
	#include "bemanitools/vefxio.h"
	#include "bemanitools/eamio.h"
}


enum class BtoolsEventTag
{
	VEFXIO_INIT,
	VEFXIO_WRITE_16SEG,
	VEFXIO_READ_SLIDER,

	EAMIO_INIT,
	EAMIO_GET_KEYPAD_STATE,
	EAMIO_GET_SENSOR_STATE,
	EAMIO_READ_CARD,
	EAMIO_CARD_SLOT_CMD,
	EAMIO_POLL
};

struct BtoolsEvent
{
	BtoolsEventTag tag;
	union
	{
		char text[32];
		uint8_t slider_no;
		uint8_t unit_no;
	};
	union
	{
		uint8_t* card_id;
		uint8_t cmd;
	};
	union
	{
		uint8_t nbytes;
	};
};


#define BT_API extern "C" __declspec(dllexport)

BT_API void		backend_vefxio_init();
BT_API bool		backend_vefxio_write_16seg(const char*);
BT_API uint8_t	backend_vefxio_read_slider(uint8_t);

BT_API void		backend_eamio_init();
BT_API uint16_t	backend_eamio_get_keypad_state(uint8_t);
BT_API uint8_t	backend_eamio_get_sensor_state(uint8_t);
BT_API uint8_t	backend_eamio_read_card(uint8_t, uint8_t*, uint8_t);
BT_API uint8_t	backend_eamio_card_slot_cmd(uint8_t, uint8_t);
BT_API uint8_t	backend_eamio_poll(uint8_t);


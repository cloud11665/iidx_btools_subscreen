#include <algorithm>
#include <mutex>
#include <cstring>

#include "api.hpp"
#include "globals.hpp"


static void add_event(BtoolsEvent e)
{
    std::lock_guard<std::mutex> lock(g_btools_equeue_mutex);
    g_btools_equeue.push_front(e);
    if (g_btools_equeue.size() > g_btools_equeue_maxsz)
        g_btools_equeue.pop_back();
}

///////////////////////////////////////////////////////////////////////////////
//                                vefxio
///////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) void backend_vefxio_init()
{
    add_event({
        .tag = BtoolsEventTag::VEFXIO_INIT
    });
    g_vefxio_enabled = true;
}

extern "C" __declspec(dllexport) bool backend_vefxio_write_16seg(
    const char* text
)
{
    BtoolsEvent e = {.tag = BtoolsEventTag::VEFXIO_WRITE_16SEG};
    strcpy_s(e.text, text);
    add_event(e);
    std::lock_guard<std::mutex> lock(g_vefxio_ticker_mutex);
    strcpy_s(g_vefxio_ticker_text, text);
    return true;
}

extern "C" __declspec(dllexport) uint8_t backend_vefxio_read_slider(
    uint8_t slider_no
)
{
    //add_event({
    //    .tag = BtoolsEventTag::VEFXIO_READ_SLIDER,
    //    .slider_no = slider_no
    //});
    std::lock_guard<std::mutex> lock(g_vefxio_effector_mutex);
    uint8_t data = g_vefxio_effector_state[slider_no];
    if (data >= 14)
        data = 15;
    return data;
}

///////////////////////////////////////////////////////////////////////////////
//                                 eamio
///////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) void backend_eamio_init()
{
    add_event({
        .tag = BtoolsEventTag::EAMIO_INIT,
    });
    g_eamio_enabled = true;
}

extern "C" __declspec(dllexport) uint16_t backend_eamio_get_keypad_state(
    uint8_t unit_no
)
{
    add_event({
        .tag = BtoolsEventTag::EAMIO_GET_KEYPAD_STATE,
        .unit_no = unit_no
    });
    std::lock_guard<std::mutex> lock(g_eamio_keypad_mutex);
    if (unit_no == 0)
        return g_eamio_keypad_p1;
    if (unit_no == 1)
        return g_eamio_keypad_p2;
    return 0;
}

extern "C" __declspec(dllexport) uint8_t backend_eamio_get_sensor_state(
    uint8_t unit_no
)
{
    add_event({
        .tag = BtoolsEventTag::EAMIO_GET_SENSOR_STATE,
        .unit_no = unit_no
    });
    std::lock_guard<std::mutex> lock(g_eamio_card_mutex);

    if (unit_no == 0)
        return g_eamio_card_p1.has_value();

    if (unit_no == 1)
        return g_eamio_card_p2.has_value();

    return 0;
}

extern "C" __declspec(dllexport) uint8_t backend_eamio_read_card(
    uint8_t unit_no,
    uint8_t* card_id,
    uint8_t nbytes
)
{
    add_event({
        .tag = BtoolsEventTag::EAMIO_READ_CARD,
        .unit_no = unit_no,
        .card_id = card_id,
        .nbytes = nbytes
    });
    std::lock_guard<std::mutex> lock(g_eamio_card_mutex);

    card_data_t card_data;

    if (unit_no == 0)
    {
        if (!g_eamio_card_p1)
            return EAM_IO_CARD_NONE;
        std::copy(g_eamio_card_p1->begin(),
                  g_eamio_card_p1->end(),
                  card_data.begin());
    }
    else if (unit_no == 1)
    {
        if (!g_eamio_card_p2)
            return EAM_IO_CARD_NONE;
        std::copy(g_eamio_card_p2->begin(),
                  g_eamio_card_p2->end(),
                  card_data.begin());
    }
    else
        return EAM_IO_CARD_NONE;

    std::copy(card_data.begin(), card_data.end(), card_id);

    if (card_data[0] == 0xe0 && card_data[1] == 0x04) {
        return EAM_IO_CARD_ISO15696;
    }
    else {
        return EAM_IO_CARD_FELICA;
    }
}

extern "C" __declspec(dllexport) uint8_t backend_eamio_card_slot_cmd(
    uint8_t unit_no,
    uint8_t cmd
)
{
    add_event({
        .tag = BtoolsEventTag::EAMIO_CARD_SLOT_CMD,
        .unit_no = unit_no,
        .cmd = cmd
    });
    return 0;
}

extern "C" __declspec(dllexport) uint8_t backend_eamio_poll(
    uint8_t unit_no
)
{
    add_event({
        .tag = BtoolsEventTag::EAMIO_POLL,
        .unit_no = unit_no
    });
    return 0;
}

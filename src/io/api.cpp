﻿#include <algorithm>
#include <mutex>
#include <cstring>
#include <thread>
#include <chrono>

#include "io/api.hpp"
#include "globals.hpp"


static void add_event(BtoolsEvent e)
{
    std::lock_guard<std::mutex> lock(g_btools_equeue_mutex);
    g_btools_equeue.push_front(e);
    if (g_btools_equeue.size() > g_btools_equeue_maxsz)
        g_btools_equeue.pop_back();
}

static void resert_card(uint8_t unit_no)
{
    using namespace std::chrono_literals;

    auto t = std::thread([unit_no] {
        std::this_thread::sleep_for(5s);
        std::lock_guard<std::mutex> lock(g_eamio_card_mutex);
        if (unit_no == 0)
            g_eamio_card_p1.reset();
        else
            g_eamio_card_p2.reset();
    });

    t.detach();
}

///////////////////////////////////////////////////////////////////////////////
//                                vefxio
///////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) void backend_vefxio_init()
{
    g_vefxio_enabled = true;
}

extern "C" __declspec(dllexport) bool backend_vefxio_write_16seg(
    const char* text
)
{
    std::lock_guard<std::mutex> lock(g_vefxio_ticker_mutex);
    strcpy_s(g_vefxio_ticker_text, text);
    return true;
}

extern "C" __declspec(dllexport) uint8_t backend_vefxio_read_slider(
    uint8_t slider_no
)
{

    std::lock_guard<std::mutex> lock(g_vefxio_effector_mutex);
    uint8_t data = g_vefxio_effector_state[slider_no];
    if (data >= 14)
        data = 15;
    return data;
}

extern "C" __declspec(dllexport) bool backend_vefxio_io_recv(
    uint64_t* ppad
)
{
    const uint64_t kflag = 1ull << 0x1D;
    uint64_t pending = g_vefxio_ppad.load();
    *ppad = (*ppad & ~kflag) | pending;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//                                 eamio
///////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) void backend_eamio_init()
{
    g_eamio_enabled = true;
}

extern "C" __declspec(dllexport) uint16_t backend_eamio_get_keypad_state(
    uint8_t unit_no
)
{
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
    std::lock_guard<std::mutex> lock(g_eamio_card_mutex);

    return unit_no == 0 ? g_eamio_sensor_p1 : g_eamio_sensor_p2;
}

extern "C" __declspec(dllexport) uint8_t backend_eamio_read_card(
    uint8_t unit_no,
    uint8_t* card_id,
    uint8_t nbytes
)
{
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
    resert_card(unit_no);

    if (card_data[0] == 0xe0 && card_data[1] == 0x04)
        return EAM_IO_CARD_ISO15696;
    else
        return EAM_IO_CARD_FELICA;
}

extern "C" __declspec(dllexport) uint8_t backend_eamio_card_slot_cmd(
    uint8_t unit_no,
    uint8_t cmd
)
{
    std::lock_guard<std::mutex> lock(g_eamio_card_mutex);

    uint8_t* state = unit_no == 0 ? &g_eamio_sensor_p1 : &g_eamio_sensor_p2;
    bool inserted = unit_no == 0 ? g_eamio_card_p1.has_value() : g_eamio_card_p2.has_value();

    switch (cmd) {
    case EAM_IO_CARD_SLOT_CMD_CLOSE:
        *state = 0x00;
        break;
    case EAM_IO_CARD_SLOT_CMD_OPEN:
        *state = inserted ? 0x03 : 0x00;
        break;
    case EAM_IO_CARD_SLOT_CMD_EJECT:
        *state = 0x00;
        break;
    case EAM_IO_CARD_SLOT_CMD_READ:
        *state = inserted ? 0x03 : 0x00;
        break;
    default:
        break;
    }

    return 1;
}

extern "C" __declspec(dllexport) uint8_t backend_eamio_poll(
    uint8_t unit_no
)
{
    std::lock_guard<std::mutex> lock(g_eamio_card_mutex);

    if (unit_no == 0)
        g_eamio_sensor_p1 = g_eamio_card_p1.has_value() ? 0x03 : 0x00;
    if (unit_no == 1)
        g_eamio_sensor_p2 = g_eamio_card_p2.has_value() ? 0x03 : 0x00;

    return 1;
}

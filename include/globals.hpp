#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winuser.h>
#include <Shlwapi.h>
#include <shellscalingapi.h>
#include <d3d11.h>
#include <tchar.h>

#include <cstdint>
#include <atomic>
#include <optional>
#include <vector>
#include <array>
#include <deque>
#include <mutex>
#include <algorithm>
#include <utility>

using card_data_t = std::array<uint8_t, 8>;
using touchpoint_t = std::pair<DWORD, POINTER_INFO>;

// ------------------------------ globals -------------------------------------
inline std::atomic<bool>		g_running{ false };
inline HANDLE					g_backend_h{ nullptr };
inline HWND						g_hwnd{ nullptr };
inline RECT						g_rect{};
inline HMONITOR					g_hmon{ nullptr };

inline HWND						g_iidx_hwnd{ nullptr };
inline HHOOK					g_iidx_msgHook{ nullptr };
inline RECT						g_iidx_rect{};
inline CHAR						g_iidx_name[256] = { 0 };

// ------------------------------- input --------------------------------------
inline int						g_touch_event_count{ 0 };
inline std::mutex				g_touchpoints_mutex;
inline std::array<touchpoint_t, 4> g_touchpoints;
inline int						g_updates_captured{ 0 };
inline std::optional<float>		g_himetric_scale_x{ std::nullopt };
inline std::optional<float>		g_himetric_scale_y{ std::nullopt };


// ---------------------------- DX11 helpers ----------------------------------
inline ID3D11Device*			g_dx11_dev{ nullptr };
inline ID3D11DeviceContext*		g_dx11_ctx{ nullptr };
inline IDXGISwapChain*			g_dx11_swap{ nullptr };
inline ID3D11RenderTargetView*	g_dx11_RTV{ nullptr };



///////////////////////////////////////////////////////////////////////////////
// btools api state
///////////////////////////////////////////////////////////////////////////////
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

inline std::atomic_bool				g_vefxio_enabled{ false };
inline std::mutex					g_vefxio_ticker_mutex;
inline char							g_vefxio_ticker_text[16] = { 0 };
inline std::mutex					g_vefxio_effector_mutex;
inline int							g_vefxio_effector_state[5] = { 7, 7, 7, 0, 0 };

inline std::atomic_bool             g_eamio_enabled{ false };
inline std::mutex                   g_eamio_card_mutex;
inline std::optional<card_data_t>   g_eamio_card_p1{ std::nullopt };
inline std::optional<card_data_t>   g_eamio_card_p2{ std::nullopt };
inline std::mutex                   g_eamio_keypad_mutex;
inline uint16_t                     g_eamio_keypad_p1{ 0 };
inline uint16_t                     g_eamio_keypad_p2{ 0 };

inline std::mutex                   g_btools_equeue_mutex;
inline std::deque<BtoolsEvent>      g_btools_equeue;
inline const int                    g_btools_equeue_maxsz{128};

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

#include "resource_def.h"
#include "window.hpp"
#include "io/api.hpp"


using card_data_t = std::array<uint8_t, 8>;
using touchpoint_t = std::pair<DWORD, POINTER_INFO>;

// --------------------------------- globals ----------------------------------
inline std::atomic<bool>		g_running{ false };
inline HINSTANCE                g_hInstance{ nullptr };
inline HANDLE					g_backend_h{ nullptr };
inline HWND						g_hwnd{ nullptr };
inline WNDCLASSEXW              g_wndclass{};
inline RECT						g_rect{};
inline HMONITOR					g_hmon{ nullptr };
inline bool                     g_debug{ true };
inline float                    g_dpi_scale{ 96.f };
inline HWND						g_iidx_hwnd{ nullptr };
inline HHOOK					g_iidx_msgHook{ nullptr };
inline RECT						g_iidx_rect{};
inline CHAR						g_iidx_name[256] = { 0 };
inline std::optional<int>       g_iidx_version{ std::nullopt };

// ---------------------------------- input -----------------------------------
inline std::atomic_int32_t      g_touch_event_count{ 0 };
inline std::mutex				g_touchpoints_mutex;
inline std::array<touchpoint_t, 4> g_touchpoints;
inline std::atomic_int32_t      g_updates_captured{ 0 };
inline std::optional<float>		g_himetric_scale_x{ std::nullopt };
inline std::optional<float>		g_himetric_scale_y{ std::nullopt };

// ------------------------------- DX11 helpers -------------------------------
inline ID3D11Device*			g_dx11_dev{ nullptr };
inline ID3D11DeviceContext*		g_dx11_ctx{ nullptr };
inline IDXGISwapChain*			g_dx11_swap{ nullptr };
inline ID3D11RenderTargetView*	g_dx11_RTV{ nullptr };

// ----------------------------- btools api state -----------------------------
inline std::atomic_bool				g_vefxio_enabled{ false };
inline std::mutex					g_vefxio_ticker_mutex;
inline char							g_vefxio_ticker_text[16] = { 0 };
inline std::mutex					g_vefxio_effector_mutex;
inline int							g_vefxio_effector_state[5] = { 7, 7, 7, 0, 0 };

inline std::atomic_bool             g_eamio_enabled{ false };
inline std::mutex                   g_eamio_card_mutex;
inline uint8_t                      g_eamio_sensor_p1{ 0x03 };
inline uint8_t                      g_eamio_sensor_p2{ 0x03 };
inline std::optional<card_data_t>   g_eamio_card_p1{ std::nullopt };
inline std::optional<card_data_t>   g_eamio_card_p2{ std::nullopt };
inline std::mutex                   g_eamio_keypad_mutex;
inline uint16_t                     g_eamio_keypad_p1{ 0 };
inline uint16_t                     g_eamio_keypad_p2{ 0 };

inline std::mutex                   g_btools_equeue_mutex;
inline std::deque<BtoolsEvent>      g_btools_equeue;
inline const int                    g_btools_equeue_maxsz{128};

inline std::mutex                   g_aic_mutex;
inline bool                         g_aic_flip_readers{ false };
inline std::vector<std::string>     g_aic_cardio_paths;
inline std::vector<std::thread>     g_aic_threads;

// -------------------------------- gui state ---------------------------------
inline bool                         g_gui_keypad1_visible{ false };
inline bool                         g_gui_keypad2_visible{ false };

// -------------------------------- resources ---------------------------------
inline window::Resource	res_iidx_font		{ IDR_IIDX_FONT };
inline window::Resource	res_icon_home		{ IDR_icon_home };
inline window::Resource	res_icon_keypad		{ IDR_icon_keypad };
inline window::Resource	res_keypad			{ IDR_keypad };
inline window::Resource	res_keypad_close	{ IDR_keypad_close };

inline std::optional<window::Texture> tex_icon_home		{ std::nullopt };
inline std::optional<window::Texture> tex_icon_keypad	{ std::nullopt };
inline std::optional<window::Texture> tex_keypad		{ std::nullopt };
inline std::optional<window::Texture> tex_keypad_close	{ std::nullopt };

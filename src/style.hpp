#pragma once

#include <cstdint>
#include "imgui.h"


struct StyleConfig {
	float    effector_width = 180.f;
	float    effector_height = 500.f;

	uint32_t effector_background_color = IM_COL32(68, 68, 68, 170);

	uint32_t effector_background_frame_color = IM_COL32(170, 170, 170, 255);
	float    effector_background_frame_thickness = 1.f;

	uint32_t effector_fader_color = IM_COL32(200, 200, 200, 255);
	float    effector_fader_width = 140.f;
	float    effector_fader_height = 40.f;

	
	uint32_t effector_track_color = IM_COL32(0, 0, 0, 255);
	float    effector_track_width = 40.f;

	uint32_t effector_tick_color = IM_COL32(255, 255, 255, 200);
	float    effector_tick_small_width = 30.f;
	float    effector_tick_small_thickness = 3.f;
	float    effector_tick_big_width = 60.f;
	float    effector_tick_big_thickness = 7.f;

	float    keypad_unit_width = 90.f;
	float    keypad_padding = 20.f;
	float    keypad_font_size = 60.f;
	float    keypad_font_size_tiny = 22.f;

	ImFont* font_seg16 = nullptr;

	ImColor  keypad_highlight_color = ImColor(256, 256, 256, 100);
};

inline StyleConfig s;

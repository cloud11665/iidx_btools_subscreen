#pragma once

#include <cstdint>
#include "imgui.h"

//void EffectorFader(const char* text_top, const char* text_bottom, int* value, int main_tick);
//
//void Effector(int effector_vals[5]);
//
//int32_t Keypad(int side);
//
//void Ticker16seg(const char* text);
//
//void DrawAll();


namespace ticker
{
	inline ImColor color_seg_on = ImColor(0.15f, 0.15f, 0.15f, 0.8f);
	inline ImColor color_seg_off = ImColor(0.8f, 0.0f, 0.0f, 1.f);

	auto build_segment_font() -> void;
	auto draw_ticker_window() -> void;
}


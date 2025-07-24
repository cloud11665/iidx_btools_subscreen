#pragma once

#include <cstdint>
#include "imgui.h"


namespace ticker
{
	inline ImColor color_seg_on = ImColor(0.15f, 0.15f, 0.15f, 0.8f);
	inline ImColor color_seg_off = ImColor(0.8f, 0.0f, 0.0f, 1.f);

	auto build_segment_font() -> void;
	auto draw() -> void;
}

namespace navbar
{
	auto draw() -> void;
}

namespace keypads
{
	auto draw() -> void;
}

namespace debug_view
{
	auto draw() -> void;
}

namespace card_view
{
	auto draw() -> void;
}
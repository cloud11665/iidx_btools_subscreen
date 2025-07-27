#pragma once

#include "imgui.h"

namespace touch
{
    struct TouchAnimation {
        ImVec2 pos;
        int start_frame;
    };

    auto init_touch() -> void;
    auto render_touch_animations() -> void;
    auto process_touch() -> void;
}
#include "touch_pointer.h"
#include <cmath>

namespace sacredstones {
namespace {
bool inside(float x, float y) {
    return std::isfinite(x) && std::isfinite(y) && x >= 0 && x < 240 && y >= 0 && y < 160;
}
}
PointerInput TouchPointer::translate(const gbarecomp::TouchFrameInfo& frame) {
    using namespace gbarecomp;
    PointerInput input;
    input.frame = frame.frame_count;
    input.physical_keys = frame.host_keyinput;
    input.pointer_down = frame.point_count != 0;
    if (frame.point_count > 1 || frame.host_keyinput != 0x3ff) {
        input.reset = true; edge_hold_ = false; return input;
    }
    for (std::size_t i = 0; i < frame.gesture_count; ++i) {
        const auto& g = frame.gestures[i];
        if (g.kind == GestureKind::Cancel || g.kind == GestureKind::ThreeFingerTap || g.kind == GestureKind::Back) {
            input.reset = true; edge_hold_ = false; return input;
        }
    }
    for (std::size_t i = 0; i < frame.gesture_count; ++i) {
        const auto& g = frame.gestures[i];
        if (!g.start_in_view || !inside(g.view_x, g.view_y)) continue;
        if ((g.kind == GestureKind::Tap && !edge_hold_) || g.kind == GestureKind::TwoFingerTap) {
            input.x = static_cast<int>(g.view_x); input.y = static_cast<int>(g.view_y);
            input.primary = g.kind == GestureKind::Tap;
            input.secondary = !input.primary;
            break; // Never combine separate taps into one guest press.
        }
        if (g.kind == GestureKind::Swipe && (g.swipe == SwipeDirection::Up || g.swipe == SwipeDirection::Down)) {
            input.wheel = g.swipe == SwipeDirection::Up ? -3 : 3;
            input.wheel_x = static_cast<int>(g.view_x); input.wheel_y = static_cast<int>(g.view_y);
            break;
        }
    }
    if (frame.point_count == 1) {
        const auto& p = frame.points[0];
        if (!p.moved && inside(p.view_x, p.view_y) &&
            static_cast<std::uint32_t>(frame.host_ms - p.down_ms) >= 120 &&
            (p.view_x < 16 || p.view_x >= 224 || p.view_y < 16 || p.view_y >= 144)) {
            input.hover_x = static_cast<int>(p.view_x); input.hover_y = static_cast<int>(p.view_y);
            input.pointer_down = false;
            edge_hold_ = true;
        }
    } else edge_hold_ = false;
    return input;
}
}

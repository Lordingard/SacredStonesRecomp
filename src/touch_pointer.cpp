#include "touch_pointer.h"
#include <cmath>
#include <algorithm>

namespace sacredstones {
namespace {
bool inside(float x, float y) {
    return std::isfinite(x) && std::isfinite(y) && x >= 0 && x < 240 && y >= 0 && y < 160;
}
}
void TouchPointer::reset() {
    drag_active_ = false; drag_pointer_ = -1;
    wait_release_ = false;
    last_x_ = last_y_ = remainder_x_ = remainder_y_ = 0;
}
PointerInput TouchPointer::translate(const gbarecomp::TouchFrameInfo& frame) {
    using namespace gbarecomp;
    PointerInput input;
    input.frame = frame.frame_count;
    input.physical_keys = frame.host_keyinput;
    input.pointer_down = frame.point_count != 0;
    if (wait_release_) {
        if (frame.point_count) { input.reset = true; return input; }
        wait_release_ = false;
    }
    if (frame.point_count > 1 || frame.host_keyinput != 0x3ff) {
        input.reset = true; reset(); wait_release_ = frame.point_count != 0; return input;
    }
    for (std::size_t i = 0; i < frame.gesture_count; ++i) {
        const auto& g = frame.gestures[i];
        if (g.kind == GestureKind::Cancel || g.kind == GestureKind::ThreeFingerTap || g.kind == GestureKind::Back) {
            input.reset = true; reset(); wait_release_ = frame.point_count != 0; return input;
        }
    }
    auto move = [&](std::int32_t pointer, float sx, float sy, float x, float y, bool begin) {
        if (!inside(sx, sy) || !std::isfinite(x) || !std::isfinite(y)) {
            reset(); input.reset = true; return;
        }
        if (begin) {
            reset(); drag_active_ = true; drag_pointer_ = pointer;
            last_x_ = sx; last_y_ = sy; input.drag_start = true;
        }
        if (!drag_active_ || pointer != drag_pointer_) return;
        input.dragging = true;
        // Move the content with the finger. Keep fractional logical pixels
        // between frames so small physical movements are not discarded.
        remainder_x_ += std::clamp(last_x_ - x, -480.0f, 480.0f);
        remainder_y_ += std::clamp(last_y_ - y, -320.0f, 320.0f);
        const int dx = static_cast<int>(remainder_x_), dy = static_cast<int>(remainder_y_);
        input.drag_dx += dx; input.drag_dy += dy;
        remainder_x_ -= dx; remainder_y_ -= dy; last_x_ = x; last_y_ = y;
    };
    bool drag_event = false;
    for (std::size_t i = 0; i < frame.gesture_count; ++i) {
        const auto& g = frame.gestures[i];
        if (g.kind == GestureKind::DragBegin || g.kind == GestureKind::DragMove || g.kind == GestureKind::DragEnd) {
            drag_event = true;
            if (g.start_in_view) move(g.pointer, g.start_view_x, g.start_view_y, g.view_x, g.view_y,
                                     g.kind == GestureKind::DragBegin);
            if (g.kind == GestureKind::DragEnd) drag_active_ = false;
            continue;
        }
        if (!g.start_in_view || !inside(g.view_x, g.view_y)) continue;
        if ((g.kind == GestureKind::Tap && !input.dragging && !drag_active_) || g.kind == GestureKind::TwoFingerTap) {
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
    if (!drag_event && frame.point_count == 1) {
        const auto& p = frame.points[0];
        if (p.moved) move(p.pointer, p.start_view_x, p.start_view_y, p.view_x, p.view_y, !drag_active_);
    }
    if (frame.point_count == 0) reset();
    if (input.dragging) input.primary = false;
    return input;
}
}

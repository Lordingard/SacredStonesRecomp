#pragma once
#include "pointer_input.h"
#include "touch_input.h"

namespace sacredstones {
class TouchPointer {
public:
    PointerInput translate(const gbarecomp::TouchFrameInfo& frame);
    void reset();
private:
    bool drag_active_ = false;
    bool wait_release_ = false;
    std::int32_t drag_pointer_ = -1;
    float last_x_ = 0, last_y_ = 0, remainder_x_ = 0, remainder_y_ = 0;
};
}

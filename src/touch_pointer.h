#pragma once
#include "pointer_input.h"
#include "touch_input.h"

namespace sacredstones {
class TouchPointer {
public:
    PointerInput translate(const gbarecomp::TouchFrameInfo& frame);
    void reset() { edge_hold_ = false; }
private:
    bool edge_hold_ = false;
};
}

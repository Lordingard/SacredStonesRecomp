#pragma once
#include <cstddef>
#include <cstdint>

namespace sacredstones {
// Game-owned read-only input view shared by desktop mouse and mobile gestures.
struct PointerInput {
    std::uint64_t frame = 0;
    const std::uint8_t* ewram = nullptr;
    std::size_t ewram_size = 0;
    const std::uint8_t* iwram = nullptr;
    std::size_t iwram_size = 0;
    int wheel = 0, wheel_x = -1, wheel_y = -1;
    std::uint16_t physical_keys = 0x03ff;
    int x = -1, y = -1, hover_x = -1, hover_y = -1;
    bool pointer_down = false, primary = false, secondary = false, reset = false;
    PointerInput() = default;
    template<class T> PointerInput(const T& in)
        : frame(in.frame), ewram(in.ewram), ewram_size(in.ewram_size),
          iwram(in.iwram), iwram_size(in.iwram_size), wheel(in.wheel),
          wheel_x(in.wheel_x), wheel_y(in.wheel_y), physical_keys(in.physical_keys),
          x(in.x), y(in.y), hover_x(in.hover_x), hover_y(in.hover_y),
          pointer_down(in.pointer_down), primary(in.primary), secondary(in.secondary), reset(in.reset) {}
};
}

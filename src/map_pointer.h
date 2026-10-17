#pragma once

#include "pointer_input.h"
#if !defined(SACREDSTONES_ANDROID_TOUCH)
#include "runtime.h"
#endif

namespace sacredstones {
class MapPointer {
public:
    bool enabled = false;
    std::uint16_t update(const PointerInput& input);
    void cancel();
    bool camera_position(int& x, int& y) const;
    bool world_camera_position(int& x, int& y, std::uint32_t& proc) const;
    bool cursor_jump(int& x, int& y, std::uint32_t& proc, std::uint32_t& mode) const;
    void cursor_jump_completed();
private:
    std::uint64_t last_frame_ = ~std::uint64_t{0};
    std::uint64_t started_ = 0;
    std::uint64_t screen_ready_ = 0;
    std::uint32_t proc_ = 0;
    std::uint32_t mode_ = 0;
    std::uint32_t identity_ = 0;
    std::uint32_t target_token_ = 0;
    bool confirm_target_ = false;
    std::uint32_t preview_token_ = 0;
    bool detached_camera_ = false;
    bool jump_pending_ = false;
    int camera_x_ = 0;
    int camera_y_ = 0;
    int target_x_ = 0;
    int target_y_ = 0;
    bool pending_ = false;
    bool cancel_pending_ = false;
    bool release_ = false;
    std::uint16_t output_ = 0;
};
extern MapPointer map_pointer;
bool initialize_mouse_camera();
std::uint16_t map_pointer_input(const PointerInput& input);
#if !defined(SACREDSTONES_ANDROID_TOUCH)
inline std::uint16_t desktop_pointer_input(const gbarecomp::GamePointerInput& input) {
    return map_pointer_input(PointerInput(input));
}
#endif
int map_pointer_get(const char* key, int* value);
int map_pointer_set(const char* key, int value);
}

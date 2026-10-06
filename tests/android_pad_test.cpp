#include "presentation_layout.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

using gbarecomp::PresentationLayout;
struct Backend {
    int last_drawable_w = 0, last_drawable_h = 0;
    float px_per_mm = 1;
    struct { int left = 0, top = 0, right = 0, bottom = 0; } safe_insets;
    PresentationLayout last_layout;
    bool toggle_pos_saved = false;
    float toggle_fx = 0, toggle_fy = 0;
};
#include "pad_geometry.inc"

int main() {
    const float screens[][3] = {{2142, 960, 16}, {2560, 1600, 11},
                                {1920, 1080, 12}, {1280, 720, 10}};
    for (const auto& screen : screens) {
        Backend b;
        b.last_drawable_w = static_cast<int>(screen[0]);
        b.last_drawable_h = static_cast<int>(screen[1]);
        b.px_per_mm = screen[2];
        b.last_layout = gbarecomp::compute_presentation_layout(
            b.last_drawable_w, b.last_drawable_h, 240, 160);
        const auto p = compute_pad_layout(&b);
        const auto check = [&](const PadRect& r, uint16_t expected) {
            const float inset = 0.1f;
            for (float x : {r.x + inset, r.x + r.w / 2, r.x + r.w - inset})
                for (float y : {r.y + inset, r.y + r.h / 2, r.y + r.h - inset})
                    if (pad_buttons_at(p, x, y, b.px_per_mm) != expected)
                        return false;
            return r.x >= 0 && r.y >= 0 && r.x + r.w <= screen[0] &&
                   r.y + r.h <= screen[1];
        };
        const float select_distance = p.select.x - p.dpad.x;
        const float start_distance = p.b.x - p.start.x - p.start.w;
        if (!check(p.select, 1u << 2) || !check(p.start, 1u << 3) ||
            std::abs(p.select.y + p.select.h - (screen[1] - 4 * b.px_per_mm)) > 0.1f ||
            std::abs(p.start.y - p.select.y) > 0.1f ||
            select_distance > 20 * b.px_per_mm ||
            start_distance > 12 * b.px_per_mm ||
            p.select.x + p.select.w + 4 * b.px_per_mm >= p.start.x) {
            std::fprintf(stderr, "FAIL pad geometry: %.0fx%.0f\n", screen[0], screen[1]);
            return 1;
        }
    }
    std::puts("Android thumb reach and exclusive Start/Select hit regions: PASS");
}

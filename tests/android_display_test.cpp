#include "presentation_layout.h"

#include <cstdio>
#include <cstdlib>

int main() {
    // Common landscape surfaces, a rotated surface, and a small window.
    const int surfaces[][2] = {{2142, 960}, {2560, 1600}, {1920, 1080},
                              {960, 2142}, {239, 159}};
    for (const auto& surface : surfaces) {
        const auto layout = gbarecomp::compute_presentation_layout(
            surface[0], surface[1], 240, 160);
        const bool fits = layout.x >= 0 && layout.y >= 0 &&
            layout.x + layout.width <= surface[0] &&
            layout.y + layout.height <= surface[1];
        const bool aspect = layout.width * 2 == layout.height * 3;
        const bool fills = surface[0] - layout.width < 3 ||
                           surface[1] - layout.height < 2;
        float x = 0, y = 0;
        const bool maps = gbarecomp::presentation_point_to_logical(
            layout, 240, 160, layout.x + layout.width / 2.0f,
            layout.y + layout.height / 2.0f, &x, &y);
        if (!fits || !aspect || !fills || !maps || x != 120 || y != 80) {
            std::fprintf(stderr, "FAIL: display %dx%d\n", surface[0], surface[1]);
            return EXIT_FAILURE;
        }
    }
    const auto invalid = gbarecomp::compute_presentation_layout(0, 960, 240, 160);
    if (invalid.width || invalid.height) return EXIT_FAILURE;
    std::puts("Android display fit and touch mapping: PASS");
    return EXIT_SUCCESS;
}

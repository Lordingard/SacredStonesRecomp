#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "presentation_layout.h"
#include <cstdio>
#include <cstdlib>
#include <utility>

static int drawable_width, drawable_height;
extern "C" int SDL_GetRendererOutputSize(SDL_Renderer*, int* w, int* h) {
    *w = drawable_width; *h = drawable_height;
    return 0;
}
extern "C" void SDL_GetWindowSize(SDL_Window*, int* w, int* h) {
    *w = drawable_width; *h = drawable_height;
}

using namespace gbarecomp;
struct Insets { int left = 0, top = 0, right = 0, bottom = 0; };
struct Backend {
    SDL_Renderer* renderer = nullptr;
    SDL_Window* window = nullptr;
    bool resize_driven_view = false, expanded_view = false, touch_policy = false;
    bool anchor_top = false;
    int base_w = 240, base_h = 160;
    Insets safe_insets, display_insets;
};
#define __ANDROID__ 1
#include "game_fit.inc"
#undef __ANDROID__

static void check(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

int main() {
    for (const auto& screen : {std::pair{2142, 960}, std::pair{2560, 1600},
                              std::pair{1920, 1080}, std::pair{960, 2142}}) {
        drawable_width = screen.first; drawable_height = screen.second;
        Backend b;
        b.safe_insets = {25, 48, 25, 80};
        int w = 0, h = 0;
        auto layout = current_presentation_layout(&b, &w, &h);
        const auto expected = compute_presentation_layout(w, h, 240, 160);
        check(layout.x == expected.x && layout.y == expected.y &&
              layout.width == expected.width && layout.height == expected.height,
              "Gesture margins do not shrink the game image");
        check(layout.width * 2 == layout.height * 3, "GBA aspect preserved");
        check(w - layout.width < 3 || h - layout.height < 2, "Largest contained image");
        check(b.safe_insets.top == 48 && b.safe_insets.bottom == 80,
              "Control safe area retained");
        float x = 0, y = 0;
        check(presentation_point_to_logical(layout, 240, 160,
            layout.x + layout.width / 2.0f, layout.y + layout.height / 2.0f, &x, &y)
            && x == 120 && y == 80, "Touch coordinates follow enlarged image");
        b.display_insets = {60, 20, 0, 10};
        layout = current_presentation_layout(&b, &w, &h);
        check(layout.x >= 60 && layout.y >= 20 &&
              layout.x + layout.width <= w && layout.y + layout.height <= h - 10,
              "Display cutout remains protected");
    }
    std::puts("PASS: Android full-height fit, gesture margins, cutout safety and touch mapping");
    return 0;
}

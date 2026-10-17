#include "host_window.h"
#include <SDL.h>
#include <cstdio>
#include <cstdlib>

// The isolated host test has no guest compiler or overlay loader.
namespace gbarecomp { std::uint64_t overlay_game_thread_compile_ns() { return 0; } }

namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s (%s)\n", message, SDL_GetError()); std::exit(1); }
}
}
int main() {
    gbarecomp::HostWindow host;
    require(host.open(3, 240, 160, "Mouse input validation", nullptr, false, false, false, true),
            "open SDL host");
    SDL_Window* window = nullptr;
    for (Uint32 id = 1; id <= 32 && !window; ++id) window = SDL_GetWindowFromID(id);
    require(window != nullptr, "find validation window");
    const auto id = SDL_GetWindowID(window);
    host.pump();
    auto click = [&](int x, int y, int button, Uint32 device = 0) {
        SDL_Event event{};
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.windowID = id;
        event.button.which = device;
        event.button.button = static_cast<Uint8>(button);
        event.button.x = x; event.button.y = y;
        require(SDL_PushEvent(&event) == 1, "queue pointer event");
        return host.pump();
    };
    auto ev = click(360, 240, SDL_BUTTON_LEFT);
    require(ev.pointer_primary == !ev.pointer_blocked && ev.pointer_x == 120 && ev.pointer_y == 80,
            "SDL transforms window coordinates to GBA pixels");
    require(!host.pump().pointer_primary, "held mouse does not repeat click");
    ev = click(360, 240, SDL_BUTTON_RIGHT);
    require(ev.pointer_secondary == !ev.pointer_blocked && !ev.pointer_primary, "right-click edge/focus gate");
    ev = click(360, 240, SDL_BUTTON_LEFT, SDL_TOUCH_MOUSEID);
    require(!ev.pointer_primary, "ignore synthetic touchscreen mouse");
    SDL_SetWindowSize(window, 1000, 480);
    SDL_Delay(50); host.pump();
    ev = click(500, 240, SDL_BUTTON_LEFT);
    require(ev.pointer_primary == !ev.pointer_blocked && ev.pointer_x == 120 && ev.pointer_y == 80,
            "resized window center maps to same GBA point");
    ev = click(20, 240, SDL_BUTTON_LEFT);
    require(!ev.pointer_primary, "pillarbox click ignored");
#if SDL_VERSION_ATLEAST(2, 26, 0)
    auto wheel = [&](int x, int y, int delta, Uint32 direction = SDL_MOUSEWHEEL_NORMAL, Uint32 device = 0) {
        SDL_Event event{}; event.type = SDL_MOUSEWHEEL;
        event.wheel.windowID = id; event.wheel.which = device;
        event.wheel.y = delta; event.wheel.direction = direction;
        event.wheel.mouseX = x; event.wheel.mouseY = y;
        require(SDL_PushEvent(&event) == 1, "queue wheel event");
        return host.pump();
    };
    ev = wheel(500, 240, 2);
    require(ev.wheel == (ev.pointer_blocked ? 0 : 2), "wheel honors focus gate");
    if (!ev.pointer_blocked)
        require(ev.wheel_x == 120 && ev.wheel_y == 80, "wheel maps resized coordinates to game pixels");
    require(host.pump().wheel == 0, "wheel delta never repeats next pump");
    ev = wheel(500, 240, 1, SDL_MOUSEWHEEL_FLIPPED);
    require(ev.wheel == (ev.pointer_blocked ? 0 : -1), "natural wheel direction is normalized");
    ev = wheel(20, 240, 2); require(ev.wheel == 0, "wheel over black bars is ignored");
    ev = wheel(500, 240, 2, SDL_MOUSEWHEEL_NORMAL, SDL_TOUCH_MOUSEID);
    require(ev.wheel == 0, "synthetic touch wheel is ignored");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 18)
    float x = 0, y = 0;
    SDL_RenderWindowToLogical(SDL_GetRenderer(window), 500, 240, &x, &y);
    require(x == 120 && y == 80, "hover conversion matches click center after resize");
    SDL_RenderWindowToLogical(SDL_GetRenderer(window), 20, 240, &x, &y);
    require(x < 0, "hover conversion leaves pillarboxes outside image");
    ev = host.pump();
    if (ev.pointer_blocked)
        require(ev.hover_x == -1 && ev.hover_y == -1, "unfocused window publishes no hover");
#endif
    host.close();
    std::puts("PASS: SDL mouse coordinates, resize, black bars, edge input and touch exclusion");
    return 0;
}

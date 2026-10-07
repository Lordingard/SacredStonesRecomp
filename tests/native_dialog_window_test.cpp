#define SDL_MAIN_HANDLED
#include "native_dialog_window.h"
#include <stdexcept>
#include <iostream>

static void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

int main() {
    SDL_SetMainReady();
    SDL_Window* window = nullptr;
    try {
        require(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL video init failed");
        window = SDL_CreateWindow("Native dialog transition test", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, 720, 480, 0);
        require(window != nullptr, "window creation failed");
        SDL_DisplayMode desktop{};
        require(SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(window), &desktop) == 0,
                "desktop mode unavailable");
        require(SDL_SetWindowDisplayMode(window, &desktop) == 0, "desktop mode selection failed");
        for (Uint32 mode : {Uint32{0}, Uint32{SDL_WINDOW_FULLSCREEN_DESKTOP}, Uint32{SDL_WINDOW_FULLSCREEN}}) {
            require(SDL_SetWindowFullscreen(window, mode) == 0, "test fullscreen switch failed");
            for (bool cancel : {false, true}) {
                try {
                    NativeDialogWindow guard(window);
                    require(guard.ready(), "dialog transition failed");
                    const auto current = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
                    require(current == (mode == SDL_WINDOW_FULLSCREEN ? 0 : mode),
                            "wrong fullscreen mode during dialog");
                    require((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) == 0,
                            "dialog owner remains minimized");
                    if (cancel) throw 42;
                } catch (int) {}
                require((SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) == mode,
                        "fullscreen not restored after completion/cancellation");
                SDL_DisplayMode selected{};
                require(SDL_GetWindowDisplayMode(window, &selected) == 0 &&
                        selected.w == desktop.w && selected.h == desktop.h &&
                        selected.refresh_rate == desktop.refresh_rate, "display mode changed");
            }
        }
        require(!NativeDialogWindow(nullptr).ready(), "missing game window accepted");
        SDL_SetWindowFullscreen(window, 0);
        SDL_DestroyWindow(window);
        SDL_Quit();
        std::cout << "PASS: native dialog transition, cancellation and fullscreen restoration\n";
        return 0;
    } catch (const std::exception& error) {
        if (window) { SDL_SetWindowFullscreen(window, 0); SDL_DestroyWindow(window); }
        SDL_Quit();
        std::cerr << error.what() << '\n';
        return 1;
    }
}

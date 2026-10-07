#pragma once
#include <SDL.h>
#include <cstdio>

// A native modal dialog must not share an exclusive/minimized owner. Keep
// this temporary transition separate from the player's fullscreen preference.
class NativeDialogWindow {
public:
    explicit NativeDialogWindow(SDL_Window* window) : window_(window) {
        if (!window_) return;
        exclusive_ = (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN_DESKTOP)
            == SDL_WINDOW_FULLSCREEN;
        if (exclusive_ && SDL_SetWindowFullscreen(window_, 0) != 0) {
            std::fprintf(stderr, "save_export: cannot leave exclusive fullscreen: %s\n", SDL_GetError());
            return;
        }
        ready_ = true;
        if (exclusive_) {
            SDL_RestoreWindow(window_);
            SDL_RaiseWindow(window_);
            SDL_PumpEvents();
        }
    }
    ~NativeDialogWindow() {
        if (exclusive_ && ready_) {
            if (SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN) != 0)
                std::fprintf(stderr, "save_export: cannot restore exclusive fullscreen: %s\n", SDL_GetError());
            SDL_RaiseWindow(window_);
        }
    }
    NativeDialogWindow(const NativeDialogWindow&) = delete;
    NativeDialogWindow& operator=(const NativeDialogWindow&) = delete;
    bool ready() const { return ready_; }
private:
    SDL_Window* window_ = nullptr;
    bool exclusive_ = false;
    bool ready_ = false;
};

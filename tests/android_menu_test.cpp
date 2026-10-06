#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "recomp_runtime_ui.h"
#include <cstdio>
#include <cstdlib>

struct RecompRuntimeUi {
    bool open = false;
    bool text = false;
    int presses = 0;
};

extern "C" int recomp_runtime_ui_is_open(const RecompRuntimeUi* ui) { return ui->open; }
extern "C" int recomp_runtime_ui_wants_text_input(const RecompRuntimeUi* ui) { return ui->text; }
extern "C" void recomp_runtime_ui_open(RecompRuntimeUi* ui) { ui->open = true; }
extern "C" void recomp_runtime_ui_close(RecompRuntimeUi* ui) { ui->open = false; }
extern "C" int recomp_runtime_ui_handle_input(RecompRuntimeUi* ui,
    RecompRuntimeUiInput input, int pressed, int) {
    if (!pressed) return 0;
    ++ui->presses;
    if (input == RECOMP_RUNTIME_UI_INPUT_BACK) ui->open = false;
    return 1;
}

// Only the extracted mapper sees the Android branch, not the host SDL headers.
#define __ANDROID__ 1
#include "menu_event.inc"
#undef __ANDROID__

static void check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main() {
    RecompRuntimeUi ui;
    SDL_Event event{};
    event.type = SDL_CONTROLLERBUTTONDOWN;
    event.cbutton.button = SDL_CONTROLLER_BUTTON_BACK;
    check(!runtime_ui_event(&ui, event) && !ui.open, "Select stays a game input");
    event.cbutton.button = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    check(runtime_ui_event(&ui, event) && ui.open, "R3 opens the menu");
    event.type = SDL_CONTROLLERBUTTONUP;
    check(runtime_ui_event(&ui, event) && ui.open, "R3 release does not toggle");
    event.type = SDL_CONTROLLERBUTTONDOWN;
    check(runtime_ui_event(&ui, event) && !ui.open, "R3 closes the menu");
    event.type = SDL_KEYDOWN;
    event.key.keysym.scancode = SDL_SCANCODE_AC_BACK;
    check(!runtime_ui_event(&ui, event), "Closed-menu Back reaches the touch router");
    ui.open = true;
    check(runtime_ui_event(&ui, event) && !ui.open && ui.presses == 1,
          "Open-menu Back is handled once");
    event.type = SDL_KEYUP;
    check(!runtime_ui_event(&ui, event) && ui.presses == 1,
          "Back release does not reopen the menu");
    event.type = SDL_CONTROLLERBUTTONDOWN;
    event.cbutton.button = SDL_CONTROLLER_BUTTON_GUIDE;
    check(runtime_ui_event(&ui, event) && ui.open, "Guide remains supported");
    std::puts("PASS: Android menu input, Back routing, R3 and preserved Select");
    return 0;
}

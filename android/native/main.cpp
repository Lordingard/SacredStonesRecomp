#include "mobile_platform.h"
#include "runtime.h"
#include "launcher_seam.h"
#include "host_window.h"
#include "recomp_runtime_ui.h"
#include "map_pointer.h"
#include "touch_pointer.h"
#include "runtime_bus_bridge.h"
#include "gba_bus.h"
#include <fstream>
#include <SDL.h>
#include <jni.h>
#include <cstring>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace {
sacredstones::TouchPointer touch_pointer;
void reset_touch_input() {
    gbarecomp::TouchHub::instance().cancel_all(gbarecomp::touch_clock_ms());
    touch_pointer.reset();
    sacredstones::PointerInput input;
    input.reset = true;
    sacredstones::map_pointer_input(input);
}
int mobile_menu_get(const char* key, int* value) {
    if (std::strcmp(key, "sacredstones.touch_map")) return 0;
    *value = sacredstones::map_pointer.enabled ? 1 : 0;
    return 1;
}
int mobile_menu_set(const char* key, int value) {
    if (std::strcmp(key, "sacredstones.touch_map")) return 0;
    std::ofstream file("touch-controls.cfg.tmp", std::ios::binary | std::ios::trunc);
    file.put(value ? '1' : '0'); file.close();
    if (!file || std::rename("touch-controls.cfg.tmp", "touch-controls.cfg")) {
        std::fprintf(stderr, "touch_preferences_save_failed\n"); return 0;
    }
    reset_touch_input();
    sacredstones::map_pointer.enabled = value != 0;
    gbarecomp::TouchHub::instance().set_claims(value ?
        gbarecomp::kTouchClaimTap | gbarecomp::kTouchClaimLongPress |
        gbarecomp::kTouchClaimTwoFingerTap | gbarecomp::kTouchClaimSwipe | gbarecomp::kTouchClaimDrag : 0);
    return 1;
}
std::uint16_t mobile_touch_input(const gbarecomp::TouchFrameInfo* frame) {
    auto* bus = gbarecomp::active_bus();
    if (!frame || !bus) { reset_touch_input(); return 0x3ff; }
    auto input = touch_pointer.translate(*frame);
    input.ewram = bus->ewram_ptr(); input.ewram_size = 256 * 1024;
    input.iwram = bus->iwram_ptr(); input.iwram_size = 32 * 1024;
    return static_cast<std::uint16_t>(0x3ff & ~sacredstones::map_pointer_input(input));
}
int mobile_menu_action(const char* key) {
    if (std::strcmp(key, "sacredstones.export_saves") != 0) return 0;
    auto* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!env || !activity) return 0;
    jclass type = env->GetObjectClass(activity);
    jmethodID method = env->GetMethodID(type, "exportSavedGames", "()V");
    if (method) env->CallVoidMethod(activity, method);
    const bool failed = env->ExceptionCheck();
    if (failed) { env->ExceptionDescribe(); env->ExceptionClear(); }
    env->DeleteLocalRef(type);
    env->DeleteLocalRef(activity);
    return method && !failed ? 1 : 0;
}

const RecompRuntimeUiItem mobile_items[] = {
    {"sacredstones.export_saves", "Assist Tools", "Export saves", "",
     RECOMP_RUNTIME_UI_ACTION, 0, 0, 0, nullptr, 0, nullptr},
    {"sacredstones.touch_map", "Input", "Direct touch controls (experimental)", "",
     RECOMP_RUNTIME_UI_BOOL, 0, 1, 1, nullptr, 0, nullptr}
};

int run_sacred_stones(int argc, char** argv) {
    std::vector<std::string> args(argv, argv + argc);
    gbarecomp::MobileProcessOptions mobile;
    mobile.game_config = "variants/sacred_stones/game.toml";
    mobile.program_name = "./SacredStonesRecomp";
    if (!gbarecomp::mobile_prepare_process(args, mobile)) return 1;
    if (!sacredstones::initialize_mouse_camera()) return 1;
    std::ifstream preference("touch-controls.cfg", std::ios::binary);
    sacredstones::map_pointer.enabled = !preference.is_open() || preference.get() == '1';

    // Keep FE8's reviewed SRAM policy independent of setup/config propagation.
    setenv("GBARECOMP_SAVE_TYPE", "sram", 1);
    args.emplace_back("--save-path");
    args.emplace_back("saves/SacredStonesRecomp.sav");
    std::error_code error;
    std::filesystem::create_directories("saves", error);
    if (error) {
        std::fprintf(stderr, "save_directory_failed: %s\n", error.message().c_str());
        return 1;
    }

    gbarecomp::RunOptions opts;
    opts.builtin_game_name = "Fire Emblem: The Sacred Stones";
    opts.builtin_rom_sha1 = "c25b145e37456171ada4b0d440bf88a19f4d509f";
    opts.launcher_region = "USA";
    opts.launcher_game_config = mobile.game_config;
    opts.launcher_save_path = "saves/SacredStonesRecomp.sav";
    opts.ui_touch_friendly = true;
    opts.expose_assist_tools = true;
    opts.save_state_slot_count = 9;
    opts.rewind_history_seconds = 30;
    opts.resume_suspend_state_on_launch = true;
    opts.ui_extra_items = mobile_items;
    opts.ui_extra_item_count = 2;
    opts.ui_action = mobile_menu_action;
    opts.ui_get = mobile_menu_get;
    opts.ui_set = mobile_menu_set;
    opts.input_frame = mobile_touch_input;
    opts.input_reset = reset_touch_input;
    opts.touch_gesture_claims = sacredstones::map_pointer.enabled ?
        gbarecomp::kTouchClaimTap | gbarecomp::kTouchClaimLongPress |
        gbarecomp::kTouchClaimTwoFingerTap | gbarecomp::kTouchClaimSwipe | gbarecomp::kTouchClaimDrag : 0;

    // The shared mobile bootstrap adds --no-launcher. The launcher seam owns
    // that flag and must consume it before the runtime's strict CLI parser.
    if (gbarecomp_launcher_preboot(args, opts)) return 0;
    std::fprintf(stderr, "[sacredstones:android] launcher handoff complete\n");

    std::vector<char*> av;
    av.reserve(args.size());
    for (auto& arg : args) av.push_back(arg.data());
    const int result = gbarecomp::run_game(static_cast<int>(av.size()), av.data(), opts);
    std::fprintf(stderr, "[sacredstones:android] game exited code=%d\n", result);
    return result;
}
}

extern "C" JNIEXPORT void JNICALL
Java_org_gbarecomp_GameControls_requestBack(JNIEnv*, jclass) {
    SDL_Event event{};
    event.type = SDL_KEYDOWN;
    event.key.state = SDL_PRESSED;
    event.key.keysym.sym = SDLK_AC_BACK;
    event.key.keysym.scancode = SDL_SCANCODE_AC_BACK;
    SDL_PushEvent(&event);
    event.type = SDL_KEYUP;
    event.key.state = SDL_RELEASED;
    SDL_PushEvent(&event);
}

extern "C" int SDL_main(int argc, char** argv) {
    return gbarecomp::mobile_run_with_stack(run_sacred_stones, argc, argv);
}

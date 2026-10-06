#include "mobile_platform.h"
#include "runtime.h"
#include "launcher_seam.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace {
int run_sacred_stones(int argc, char** argv) {
    std::vector<std::string> args(argv, argv + argc);
    gbarecomp::MobileProcessOptions mobile;
    mobile.game_config = "variants/sacred_stones/game.toml";
    mobile.program_name = "./SacredStonesRecomp";
    if (!gbarecomp::mobile_prepare_process(args, mobile)) return 1;

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

extern "C" int SDL_main(int argc, char** argv) {
    return gbarecomp::mobile_run_with_stack(run_sacred_stones, argc, argv);
}

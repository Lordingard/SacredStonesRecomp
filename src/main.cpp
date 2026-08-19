#include "runtime.h"

#include <filesystem>
#include <string>
#include <vector>

#if defined(RECOMP_LAUNCHER)
#include "launcher_seam.h"
#endif

namespace {
bool has_arg_with_value(const std::vector<std::string>& args, const char* name) {
    for (const std::string& arg : args) {
        if (arg == name) return true;
    }
    return false;
}

void force_exe_local_config(std::vector<std::string>& args) {
    if (has_arg_with_value(args, "--config")) return;

    std::error_code ec;
    std::filesystem::path exe = std::filesystem::absolute(args.empty() ? "" : args[0], ec);
    std::filesystem::path config = exe.parent_path() / "game.toml";
    if (std::filesystem::exists(config, ec)) {
        args.push_back("--config");
        args.push_back(config.string());
    }
}
void force_bios_hle_default(std::vector<std::string>& args) {
    for (const std::string& arg : args) {
        if (arg == "--no-bios-hle" || arg == "--bios-hle" ||
            arg == "--bios-hle-keep-intro") {
            return;
        }
    }
    args.push_back("--bios-hle");
}
}

int main(int argc, char** argv) {
    gbarecomp::RunOptions opts{};
    opts.builtin_game_name = "Fire Emblem: The Sacred Stones";
    opts.builtin_rom_sha1 = "c25b145e37456171ada4b0d440bf88a19f4d509f";
    opts.mod_game_id = "sacred-stones-us";
    opts.freely_resizable_window = true;
    opts.show_fps_by_default = true;
    opts.expose_assist_tools = true;
    opts.assist_tools_enabled_by_default = true;
    opts.save_state_slot_count = 9;
    opts.rewind_history_seconds = 30;
    opts.launcher_region = "USA";
    opts.launcher_config_filename = "sacredstonesrecomp.ini";
    opts.launcher_rom_cache_filename = "sacredstonesrecomp-rom.cfg";
    opts.launcher_bios_cache_filename = "sacredstonesrecomp-bios.cfg";

    std::vector<std::string> args(argv, argv + argc);

#if defined(RECOMP_LAUNCHER)
    if (gbarecomp_launcher_preboot(args, opts)) return 0;
#endif

    force_exe_local_config(args);
    force_bios_hle_default(args);

    std::vector<char*> av;
    av.reserve(args.size());
    for (auto& arg : args) av.push_back(arg.data());
    return gbarecomp::run_game(static_cast<int>(av.size()), av.data(), opts);
}
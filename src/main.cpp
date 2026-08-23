#include "runtime.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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

std::filesystem::path executable_path(const std::vector<std::string>& args) {
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        DWORD written = GetModuleFileNameW(nullptr, buffer.data(),
                                           static_cast<DWORD>(buffer.size()));
        if (written == 0) break;
        if (written < buffer.size() - 1) {
            buffer.resize(written);
            return std::filesystem::path(buffer);
        }
        buffer.resize(buffer.size() * 2);
    }
#endif

    std::error_code ec;
    return std::filesystem::absolute(args.empty() ? "" : args[0], ec);
}

std::filesystem::path exe_local_save_path(const std::vector<std::string>& args) {
    return executable_path(args).parent_path() / "saves" / "SacredStonesRecomp.sav";
}

void force_fe8_save_type() {
#if defined(_WIN32)
    _putenv_s("GBARECOMP_SAVE_TYPE", "sram");
#else
    setenv("GBARECOMP_SAVE_TYPE", "sram", 1);
#endif
}
void force_save_path(std::vector<std::string>& args, const std::string& save_path) {
    if (has_arg_with_value(args, "--save") || has_arg_with_value(args, "--save-path")) return;
    args.push_back("--save-path");
    args.push_back(save_path);
}

std::string arg_value(const std::vector<std::string>& args, const char* name) {
    for (std::size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == name) return args[i + 1];
    }
    return {};
}

bool save_file_is_blank(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return true;

    bool saw_byte = false;
    bool all_zero = true;
    bool all_ff = true;
    char byte = 0;
    while (file.get(byte)) {
        saw_byte = true;
        const auto value = static_cast<unsigned char>(byte);
        all_zero = all_zero && value == 0x00;
        all_ff = all_ff && value == 0xff;
        if (!all_zero && !all_ff) return false;
    }
    return !saw_byte || all_zero || all_ff;
}

bool save_file_is_useful(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) return false;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size == 0) return false;
    return !save_file_is_blank(path);
}

std::filesystem::path rom_named_save_path(const std::filesystem::path& dir,
                                          const std::filesystem::path& rom) {
    std::filesystem::path name = rom.filename();
    name.replace_extension(".sav");
    return dir / name;
}

void ensure_save_directory(const std::filesystem::path& target) {
    std::error_code ec;
    std::filesystem::create_directories(target.parent_path(), ec);
}
void migrate_legacy_save_if_needed(const std::vector<std::string>& args,
                                   const std::filesystem::path& target) {
    if (save_file_is_useful(target)) return;

    const std::string rom_arg = arg_value(args, "--rom");
    if (rom_arg.empty()) return;

    const std::filesystem::path exe_dir = executable_path(args).parent_path();
    const std::filesystem::path rom_path(rom_arg);
    std::filesystem::path rom_side_save = rom_path;
    rom_side_save.replace_extension(".sav");

    const std::filesystem::path candidates[] = {
        rom_named_save_path(exe_dir / "saves", rom_path),
        rom_side_save,
        rom_named_save_path(exe_dir, rom_path),
        exe_dir / "save" / "SacredStonesRecomp.sav",
        rom_named_save_path(exe_dir / "save", rom_path),
    };

    for (const auto& candidate : candidates) {
        if (!save_file_is_useful(candidate)) continue;

        std::error_code ec;
        std::filesystem::create_directories(target.parent_path(), ec);
        ec.clear();
        std::filesystem::copy_file(candidate, target,
                                   std::filesystem::copy_options::overwrite_existing,
                                   ec);
        if (!ec) {
            std::printf("save_migrated from=\"%s\" to=\"%s\"\n",
                        candidate.string().c_str(), target.string().c_str());
        }
        return;
    }
}
void force_exe_local_config(std::vector<std::string>& args) {
    if (has_arg_with_value(args, "--config")) return;

    std::error_code ec;
    std::filesystem::path exe = executable_path(args);
    std::filesystem::path config = exe.parent_path() / "game.toml";
    if (std::filesystem::exists(config, ec)) {
        args.push_back("--config");
        args.push_back(config.string());
    }
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
    opts.launcher_game_config = "game.toml";
    opts.launcher_config_filename = "sacredstonesrecomp.ini";
    opts.launcher_rom_cache_filename = "sacredstonesrecomp-rom.cfg";
    opts.launcher_bios_cache_filename = "sacredstonesrecomp-bios.cfg";

    std::vector<std::string> args(argv, argv + argc);
    if (!args.empty()) args[0] = executable_path(args).string();
    force_fe8_save_type();

    std::string save_path = exe_local_save_path(args).string();
    opts.launcher_save_path = save_path.c_str();

    force_exe_local_config(args);

#if defined(RECOMP_LAUNCHER)
    if (gbarecomp_launcher_preboot(args, opts)) return 0;
#endif

    force_exe_local_config(args);
    force_save_path(args, save_path);
    ensure_save_directory(save_path);
    migrate_legacy_save_if_needed(args, save_path);


    std::vector<char*> av;
    av.reserve(args.size());
    for (auto& arg : args) av.push_back(arg.data());
    return gbarecomp::run_game(static_cast<int>(av.size()), av.data(), opts);
}

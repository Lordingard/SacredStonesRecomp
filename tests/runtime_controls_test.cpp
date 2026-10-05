#include "runtime_control_preferences.h"
#include <iostream>
#include <iterator>
#include <stdexcept>

static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

static std::string read(const std::filesystem::path& path) {
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), {});
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "isolated test directory required");
        std::filesystem::create_directories(argv[1]);
        auto path = std::filesystem::path(argv[1]) / "runtime-controls.toml";
        std::string error;
        gbarecomp::RuntimeControlPreferences selected;
        selected.fast_forward_multiplier = 7;
        selected.rewind_enabled = false;
        selected.state_slot = 8;
        std::ofstream(path) << "unrelated = 'preserve me'\n";
        require(gbarecomp::save_runtime_controls(path, selected, error), "preferences write failed");
        gbarecomp::RuntimeControlPreferences restored;
        require(gbarecomp::load_runtime_controls(path, restored, 9, error), "preferences reload failed");
        require(restored.fast_forward_multiplier == 7 && !restored.rewind_enabled &&
                restored.state_slot == 8 && restored.assist_tools_enabled,
                "preferences did not round-trip");
        require(read(path).find("preserve me") != std::string::npos, "unknown preference lost");
        restored = {};
        require(gbarecomp::load_runtime_controls(path, restored, 3, error) && restored.state_slot == 1,
                "unsupported state slot accepted");
        const auto unicode_path = path.parent_path() / std::filesystem::path(u8"settings-\u00e9.toml");
        require(gbarecomp::save_runtime_controls(unicode_path, selected, error), "Unicode path write failed");
        restored = {};
        require(gbarecomp::load_runtime_controls(unicode_path, restored, 9, error) &&
                restored.fast_forward_multiplier == 7, "Unicode path reload failed");
        restored = {};
        require(!gbarecomp::load_runtime_controls(path / "absent.toml", restored, 9, error) &&
                restored.fast_forward_multiplier == 4 && restored.rewind_enabled,
                "missing preferences changed defaults");
        std::ofstream(path) << "fast_forward_multiplier = 999\nstate_slot = -1\nrewind_enabled = 'wrong type'\n";
        restored = {};
        require(gbarecomp::load_runtime_controls(path, restored, 9, error), "valid TOML rejected");
        require(restored.fast_forward_multiplier == 4 && restored.state_slot == 1 &&
                restored.rewind_enabled, "invalid values changed defaults");
        std::ofstream(path) << "broken = [";
        const auto malformed = read(path);
        require(!gbarecomp::load_runtime_controls(path, restored, 9, error), "malformed TOML accepted");
        require(!gbarecomp::save_runtime_controls(path, selected, error), "malformed file overwritten");
        require(read(path) == malformed, "malformed original lost");
        std::ofstream(path) << "state_slot = 2\n";
#if defined(_WIN32)
        HANDLE lock = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(lock != INVALID_HANDLE_VALUE, "cannot lock preferences");
        const bool replaced = gbarecomp::save_runtime_controls(path, selected, error);
        CloseHandle(lock);
        require(!replaced && read(path) == "state_slot = 2\n", "locked original lost");
#endif
        require(!gbarecomp::save_runtime_controls(path / "missing-parent" / "controls.toml", selected, error),
                "unwritable destination accepted");
        std::cout << "PASS: runtime controls round-trip, defaults, malformed and locked-file safety\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

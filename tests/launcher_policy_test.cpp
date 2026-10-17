#include "launcher_seam.h"
#include "runtime_menu_input.h"
#include "../src/mouse_preferences.h"
extern "C" {
#include "launcher_model.h"
#include "launcher_window_size.h"
}
#include <iostream>
#include <stdexcept>

// NES-only binding integration is outside this policy test.
extern "C" void launcher_binds_set_zapper(int, int) {
    throw std::runtime_error("unexpected NES binding call");
}

static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

static int mouse_value = 0, mouse_sets = 0;
static int get_mouse(const char* key, int* value) {
    if (std::strcmp(key, "test.mouse")) return 0;
    *value = mouse_value; return 1;
}
static int set_mouse(const char* key, int value) {
    if (std::strcmp(key, "test.mouse")) return 0;
    mouse_value = value; ++mouse_sets; return 1;
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "test directory argument required");
        SDL_Event event{};
        event.type = SDL_KEYDOWN;
        event.key.keysym.scancode = SDL_SCANCODE_UP;
        require(!runtime_menu_owns_event(false, event), "closed menu swallowed game keyboard input");
        require(runtime_menu_owns_event(true, event), "open menu did not own navigation");
        event.key.keysym.scancode = SDL_SCANCODE_ESCAPE;
        require(runtime_menu_owns_event(false, event), "Escape did not open menu");
        event.type = SDL_CONTROLLERBUTTONDOWN;
        event.cbutton.button = SDL_CONTROLLER_BUTTON_A;
        require(!runtime_menu_owns_event(false, event), "closed menu swallowed controller A");
        event.cbutton.button = SDL_CONTROLLER_BUTTON_GUIDE;
        require(runtime_menu_owns_event(false, event), "controller Guide did not open menu");
        std::filesystem::create_directories(argv[1]);
        const auto path = std::filesystem::path(argv[1]) / "settings.ini";
        const auto mouse_path = std::filesystem::absolute(std::filesystem::path(argv[1]) / "mouse.ini");
        std::ofstream(mouse_path) << "[Other]\nkeep=yes\n";
        require(!sacredstones::load_mouse_preference(mouse_path), "mouse must default off");
        require(sacredstones::save_mouse_preference(mouse_path, true) &&
                sacredstones::load_mouse_preference(mouse_path), "enabled mouse preference did not round-trip");
        gbarecomp_seam::SeamConfig mouse_cfg;
        gbarecomp_seam::seam_config_save(mouse_path.string(), mouse_cfg);
        require(sacredstones::load_mouse_preference(mouse_path), "launcher save erased mouse preference");
        require(sacredstones::save_mouse_preference(mouse_path, false) &&
                !sacredstones::load_mouse_preference(mouse_path), "mouse preference cannot be disabled");
        std::ifstream mouse_file(mouse_path);
        const std::string mouse_contents((std::istreambuf_iterator<char>(mouse_file)), {});
        require(mouse_contents.find("keep=yes") != std::string::npos, "mouse save erased unrelated settings");
        std::ofstream(mouse_path) << "[Mouse]\nenabled=99\n";
        require(!sacredstones::load_mouse_preference(mouse_path), "invalid mouse preference must remain disabled");
        const RecompRuntimeUiItem extras[] = {
            {"test.mouse", "Input", "Mouse controls (experimental)", "", RECOMP_RUNTIME_UI_BOOL, 0, 1, 1, nullptr, 0, nullptr},
            {"test.action", "Input", "Action", "", RECOMP_RUNTIME_UI_ACTION, 0, 0, 0, nullptr, 0, nullptr},
            {"test.other", "Other", "Other", "", RECOMP_RUNTIME_UI_BOOL, 0, 1, 1, nullptr, 0, nullptr}
        };
        gbarecomp::RunOptions mouse_opts;
        mouse_opts.ui_extra_items = extras; mouse_opts.ui_extra_item_count = 3;
        mouse_opts.ui_get = get_mouse; mouse_opts.ui_set = set_mouse;
        auto toggles = gbarecomp_seam::seam_input_toggles(mouse_opts);
        require(toggles.size() == 1 && toggles[0].value == 0, "launcher must expose only supported input bools");
        gbarecomp_seam::seam_commit_input_toggles(mouse_opts, toggles);
        require(mouse_sets == 0, "unchanged launcher toggle wrote settings");
        toggles[0].value = 1;
        gbarecomp_seam::seam_commit_input_toggles(mouse_opts, toggles);
        require(mouse_value == 1 && mouse_sets == 1, "launcher toggle did not activate game callback");
        RecompLauncherCGameInfo mouse_info{};
        mouse_info.input_toggles = toggles.data(); mouse_info.input_toggle_count = 99;
        LauncherModel mouse_model{};
        launcher_model_init(&mouse_model, nullptr, &mouse_info, nullptr);
        require(mouse_model.input_toggles == toggles.data() && mouse_model.input_toggle_count == 16,
                "model did not preserve host toggle ownership or bound count");
        mouse_opts.ui_get = nullptr;
        require(gbarecomp_seam::seam_input_toggles(mouse_opts).empty(), "missing getter exposed unsupported input settings");
        char sidecar[1024]{};
        require(launcher_window_size_path(path.string().c_str(), sidecar, sizeof(sidecar)),
                "launcher geometry path missing");
        require(std::filesystem::path(sidecar).parent_path() == path.parent_path(),
                "launcher geometry not beside host config");
        require(!launcher_window_size_path(nullptr, sidecar, sizeof(sidecar)),
                "missing host config accepted");
        require(!launcher_window_size_path(path.string().c_str(), sidecar, 2),
                "undersized path buffer accepted");
        launcher_window_size_save(sidecar, 1280, 960);
        int width = LAUNCHER_DEFAULT_WIDTH, height = LAUNCHER_DEFAULT_HEIGHT;
        launcher_window_size_load(sidecar, &width, &height);
        require(width == 1280 && height == 960, "launcher geometry did not round-trip");
        std::ofstream(sidecar) << "logical_width=99999999999999999999\nlogical_height=-1\n";
        width = LAUNCHER_DEFAULT_WIDTH; height = LAUNCHER_DEFAULT_HEIGHT;
        launcher_window_size_load(sidecar, &width, &height);
        require(width == 940 && height == 799, "invalid geometry changed defaults");
        launcher_window_size_load("nonexistent-window-config.ini", &width, &height);
        require(width == 940 && height == 799, "missing geometry changed defaults");
        require(launcher_window_dimension("800junk") == 0 &&
                launcher_window_dimension("800 \r\n") == 800,
                "geometry parser accepted trailing junk or rejected whitespace");
        std::ofstream(path) << "[Other]\nkeep = yes\n";
        gbarecomp_seam::SeamConfig selected;
        selected.input_source = 2;
        selected.gamepad_guid = "030000007e0500000920000000000000";
        gbarecomp_seam::seam_config_save(path.string(), selected);
        gbarecomp_seam::SeamConfig restored;
        gbarecomp_seam::seam_config_load(path.string(), &restored);
        require(restored.input_source == 2 && restored.gamepad_guid == selected.gamepad_guid,
                "controller selection did not round-trip");
        std::ifstream input(path);
        std::string contents((std::istreambuf_iterator<char>(input)), {});
        require(contents.find("keep = yes") != std::string::npos, "unrelated settings lost");
        std::ofstream(path) << "[Launcher]\ninput_source = 99\ngamepad_guid = invalid\n";
        gbarecomp_seam::seam_config_load(path.string(), &restored);
        require(restored.input_source == 1 && restored.gamepad_guid.empty(),
                "invalid controller selection was not sanitized");
        LauncherModel model{};
        model.has_bios = true;
        model.rom_present = true;
        std::strcpy(model.rom_size, "16 MB");
        require(launcher_model_bios_blocks_play(&model), "required missing BIOS did not enable prompt");
        model.setup_bios_ok = true;
        require(!launcher_model_bios_blocks_play(&model), "valid BIOS or accepted fallback blocked");
        model.setup_bios_ok = false;
        std::strcpy(model.s.bios_path, "missing.bin");
        require(launcher_model_bios_blocks_play(&model), "invalid BIOS did not enable prompt");
        model.rom_present = false;
        require(!launcher_model_bios_blocks_play(&model), "missing ROM incorrectly enabled Play");
        std::cout << "PASS: launcher BIOS policy, mouse activation/persistence, controller persistence and window dimensions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

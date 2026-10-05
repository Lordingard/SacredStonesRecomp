#include "launcher_seam.h"
#include "runtime_menu_input.h"
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
        std::cout << "PASS: launcher BIOS policy, controller persistence and window dimensions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "launcher_seam.h"
extern "C" {
#include "launcher_model.h"
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
        std::filesystem::create_directories(argv[1]);
        const auto path = std::filesystem::path(argv[1]) / "settings.ini";
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
        std::cout << "PASS: launcher BIOS policy and controller persistence\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

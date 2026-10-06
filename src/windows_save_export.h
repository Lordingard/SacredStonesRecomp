#pragma once
#include <filesystem>

void configure_save_export(const std::filesystem::path& executable,
                           const std::filesystem::path& save,
                           const std::filesystem::path& rom);
int windows_menu_action(const char* key);

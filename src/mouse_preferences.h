#pragma once

#include <filesystem>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace sacredstones {
inline bool load_mouse_preference(const std::filesystem::path& config) {
    wchar_t value[8]{};
    GetPrivateProfileStringW(L"Mouse", L"enabled", L"0", value, 8, config.c_str());
    return std::wstring(value) == L"1";
}
inline bool save_mouse_preference(const std::filesystem::path& config, bool enabled) {
    return WritePrivateProfileStringW(L"Mouse", L"enabled", enabled ? L"1" : L"0", config.c_str()) != 0;
}
}
#endif

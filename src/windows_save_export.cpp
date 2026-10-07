#include "windows_save_export.h"
#include <cstring>
#include <string>
#include <windows.h>
#include <commdlg.h>
#include "native_dialog_window.h"
#include <SDL_syswm.h>

namespace {
std::filesystem::path script_path, save_path, rom_path;

std::wstring quoted(const std::filesystem::path& path) {
    return L"\"" + path.wstring() + L"\"";
}
}

void configure_save_export(const std::filesystem::path& executable,
                           const std::filesystem::path& save,
                           const std::filesystem::path& rom) {
    script_path = executable.parent_path() / "tools" / "export-windows-saves.ps1";
    save_path = std::filesystem::absolute(save);
    rom_path = std::filesystem::absolute(rom);
}

int windows_menu_action(const char* key) {
    if (std::strcmp(key, "sacredstones.export_saves") != 0) return 0;
    SDL_Window* window = SDL_GetKeyboardFocus();
    if (!window) window = SDL_GetMouseFocus();
    NativeDialogWindow dialog_window(window);
    if (!dialog_window.ready()) return 1;
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(window, &info) || info.subsystem != SDL_SYSWM_WINDOWS) return 1;
    HWND owner = info.info.win.window;
    wchar_t destination[32768] = L"SacredStonesRecomp-saves.zip";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = L"ZIP archive\0*.zip\0\0";
    dialog.lpstrFile = destination;
    dialog.nMaxFile = 32768;
    dialog.lpstrDefExt = L"zip";
    dialog.lpstrTitle = L"Export saves";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&dialog)) {
        if (CommDlgExtendedError())
            MessageBoxW(owner, L"The export destination could not be selected.", L"Export failed", MB_OK | MB_ICONERROR);
        return 1;
    }
    wchar_t system[32768];
    const UINT size = GetSystemDirectoryW(system, 32768);
    if (!size || size >= 32768) return 0;
    const auto powershell = std::filesystem::path(system) / "WindowsPowerShell/v1.0/powershell.exe";
    std::wstring command = quoted(powershell) + L" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File " +
        quoted(script_path) + L" -SavePath " + quoted(save_path) + L" -RomPath " + quoted(rom_path) +
        L" -Destination " + quoted(std::filesystem::path(destination));
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    DWORD exit_code = 1;
    if (CreateProcessW(powershell.c_str(), command.data(), nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) {
        WaitForSingleObject(process.hProcess, INFINITE);
        GetExitCodeProcess(process.hProcess, &exit_code);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
    MessageBoxW(owner, exit_code == 0 ? L"Saved games exported successfully." :
        L"Export failed. Check that saved games exist and the destination is writable. Original saves were not modified.",
        exit_code == 0 ? L"Saves exported" : L"Export failed",
        MB_OK | (exit_code == 0 ? MB_ICONINFORMATION : MB_ICONERROR));
    return 1;
}

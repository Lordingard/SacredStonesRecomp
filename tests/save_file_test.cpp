#include "save_file.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

static std::string read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), {});
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "test directory argument required");
        const std::filesystem::path root(argv[1]);
        std::filesystem::create_directories(root);
        const auto destination = root / "test.sav";
        const auto temporary = root / "test.sav.tmp";
        std::ofstream(destination) << "previous valid save";
        std::ofstream(temporary) << "new valid save";
        std::error_code error;
#if defined(_WIN32)
        HANDLE lock = CreateFileW(destination.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(lock != INVALID_HANDLE_VALUE, "cannot lock save");
        const bool replaced = gbarecomp::replace_save_file(temporary, destination, error);
        CloseHandle(lock);
        require(!replaced && bool(error), "locked replacement must fail");
        require(read(destination) == "previous valid save", "old save lost on failure");
        require(read(temporary) == "new valid save", "recovery file lost on failure");
#endif
        require(gbarecomp::replace_save_file(temporary, destination, error), "replacement failed");
        require(read(destination) == "new valid save", "replacement content differs");
        require(!std::filesystem::exists(temporary), "temporary file remained after success");
        require(!gbarecomp::replace_save_file(root / "missing.tmp", destination, error),
                "missing source must fail");
        require(read(destination) == "new valid save", "missing source damaged save");
        std::cout << "PASS: save replacement and recovery\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

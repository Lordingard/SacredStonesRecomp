cmake_minimum_required(VERSION 3.20)
project(SacredStonesRecomp C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GBARECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/extern/gbarecomp" CACHE PATH "Path to the GBARecomp framework checkout")
set(SACREDSTONES_GENERATED_PROJECT "${CMAKE_CURRENT_SOURCE_DIR}/.generated/gbarecomp" CACHE PATH "Path to generated GBARecomp output")

if(NOT EXISTS "${GBARECOMP_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "GBARecomp source checkout not found at ${GBARECOMP_ROOT}. Run git submodule update --init --recursive first.")
endif()


if(MINGW AND CMAKE_CXX_COMPILER AND NOT DEFINED GBARECOMP_MINGW_RUNTIME_BIN)
    get_filename_component(_sacredstones_mingw_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(GBARECOMP_MINGW_RUNTIME_BIN "${_sacredstones_mingw_bin}" CACHE PATH "Directory containing MinGW runtime DLLs for development builds")
endif()

set(RECOMP_UI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/extern/recomp-ui" CACHE PATH "Path to recomp-ui launcher checkout")
set(GBARECOMP_RUNTIME_UI_ROOT "${RECOMP_UI_ROOT}")
add_subdirectory("${GBARECOMP_ROOT}" "${CMAKE_BINARY_DIR}/gbarecomp-core" EXCLUDE_FROM_ALL)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    foreach(_gbarecomp_noisy_target IN ITEMS
            gbarecomp_armv4t
            gbarecomp_gba
            gbarecomp_runtime
            gbarecomp_debug)
        if(TARGET ${_gbarecomp_noisy_target})
            target_compile_options(${_gbarecomp_noisy_target} PRIVATE -w)
        endif()
    endforeach()
endif()

set(RECOMP_UI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/extern/recomp-ui" CACHE PATH "Path to recomp-ui launcher checkout")
if(NOT EXISTS "${RECOMP_UI_ROOT}/recomp_ui.cmake")
    message(FATAL_ERROR "recomp-ui source checkout not found at ${RECOMP_UI_ROOT}. Run git submodule update --init --recursive first.")
endif()
include("${RECOMP_UI_ROOT}/recomp_ui.cmake")

set(_candidate_game_libs
    "${SACREDSTONES_GENERATED_PROJECT}/build-mingw/libgbarecomp_game.a"
    "${SACREDSTONES_GENERATED_PROJECT}/build/Release/gbarecomp_game.lib"
    "${SACREDSTONES_GENERATED_PROJECT}/build/gbarecomp_game.lib"
    "${SACREDSTONES_GENERATED_PROJECT}/build/libgbarecomp_game.a"
)
set(SACREDSTONES_GAME_LIB "" CACHE FILEPATH "Path to generated gbarecomp_game library")
if(NOT SACREDSTONES_GAME_LIB)
    foreach(_candidate IN LISTS _candidate_game_libs)
        if(EXISTS "${_candidate}")
            set(SACREDSTONES_GAME_LIB "${_candidate}" CACHE FILEPATH "Path to generated gbarecomp_game library" FORCE)
            break()
        endif()
    endforeach()
endif()
if(NOT EXISTS "${SACREDSTONES_GAME_LIB}")
    message(FATAL_ERROR "Generated gbarecomp_game library not found. Run scripts/build.ps1 -Clean, build the generated project with MinGW, or set SACREDSTONES_GAME_LIB.")
endif()

add_library(sacredstones_generated STATIC IMPORTED GLOBAL)
set_target_properties(sacredstones_generated PROPERTIES
    IMPORTED_LOCATION "${SACREDSTONES_GAME_LIB}"
)

add_executable(SacredStonesRecomp
    src/main.cpp
    src/windows_save_export.cpp
)
target_link_libraries(SacredStonesRecomp PRIVATE comdlg32)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_sources(SacredStonesRecomp PRIVATE src/mingw_msvc_chkstk.S)
    endif()
    target_link_libraries(SacredStonesRecomp PRIVATE
        "$<LINK_GROUP:RESCAN,gbarecomp_runtime,gbarecomp_debug,sacredstones_generated,gbarecomp_recompile_core,gbarecomp_heal_gate,gbarecomp_gba,gbarecomp_armv4t>"
    )
else()
    target_link_libraries(SacredStonesRecomp PRIVATE
        gbarecomp_runtime
        gbarecomp_debug
        sacredstones_generated
    )
endif()

target_compile_definitions(SacredStonesRecomp PRIVATE
    GBARECOMP_WINDOW_TITLE="SacredStonesRecomp"
    GBARECOMP_DEFAULT_GAME_CONFIG="game.toml"
    GBARECOMP_DEFAULT_DEBUG_PORT=19842
)

recomp_target_launcher_ui(SacredStonesRecomp
    CONSOLE gba
    BOXART "${CMAKE_CURRENT_SOURCE_DIR}/assets/boxart.tga"
)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(SacredStonesRecomp PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-Wno-stringop-overflow>)
endif()

add_custom_command(TARGET SacredStonesRecomp POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:SacredStonesRecomp>/tools"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/export-windows-saves.ps1"
        "$<TARGET_FILE_DIR:SacredStonesRecomp>/tools/export-windows-saves.ps1"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/game.toml"
        "$<TARGET_FILE_DIR:SacredStonesRecomp>/game.toml"
    VERBATIM
)

if(COMMAND gbarecomp_stage_mingw_runtime)
    gbarecomp_stage_mingw_runtime(SacredStonesRecomp)
endif()

add_executable(sacredstones_save_file_tests EXCLUDE_FROM_ALL tests/save_file_test.cpp)
add_executable(sacredstones_runtime_controls_tests EXCLUDE_FROM_ALL tests/runtime_controls_test.cpp)
target_include_directories(sacredstones_runtime_controls_tests PRIVATE "${GBARECOMP_ROOT}/src/runtime")
target_link_libraries(sacredstones_runtime_controls_tests PRIVATE gbarecomp_runtime_preferences)
target_include_directories(sacredstones_save_file_tests PRIVATE "${GBARECOMP_ROOT}/src/runtime")

if(MINGW)
    find_package(SDL2 REQUIRED)
    add_executable(sacredstones_launcher_policy_tests EXCLUDE_FROM_ALL
        tests/launcher_policy_test.cpp
        "${RECOMP_UI_ROOT}/src/common/launcher_model.c"
        "${RECOMP_UI_ROOT}/src/common/crc32.c"
        "${RECOMP_UI_ROOT}/src/common/sha256.c"
        "${RECOMP_UI_ROOT}/src/common/sha1.c"
        "${RECOMP_UI_ROOT}/src/common/ips_patch.c"
        "${RECOMP_UI_ROOT}/src/consoles/psx/memcard_format.c")
    target_compile_definitions(sacredstones_launcher_policy_tests PRIVATE RECOMP_LAUNCHER SDL_MAIN_HANDLED)
    target_link_libraries(sacredstones_launcher_policy_tests PRIVATE SDL2::SDL2)
    target_include_directories(sacredstones_launcher_policy_tests PRIVATE
        "${GBARECOMP_ROOT}/src/runtime" "${GBARECOMP_ROOT}/src/armv4t"
        "${RECOMP_UI_ROOT}/src" "${RECOMP_UI_ROOT}/src/common")
    target_compile_options(sacredstones_launcher_policy_tests PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(sacredstones_launcher_policy_tests PRIVATE -Wl,--gc-sections)
endif()

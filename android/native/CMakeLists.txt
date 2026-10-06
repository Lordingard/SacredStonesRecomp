cmake_minimum_required(VERSION 3.22)
project(SacredStonesAndroid C CXX)
if(NOT ANDROID)
    message(FATAL_ERROR "Use scripts/build-android.ps1; this target requires the Android NDK.")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT EXISTS "${SACREDSTONES_GENERATED_PROJECT}/generated/dispatch_table.cpp"
   OR NOT EXISTS "${GBARECOMP_GENERATED_BIOS_DIR}/bios_dispatch_table.cpp")
    message(FATAL_ERROR "Regenerate Android game and BIOS sources with scripts/build-android.ps1.")
endif()
add_subdirectory("${GBARECOMP_ROOT}" gbarecomp_build EXCLUDE_FROM_ALL)
file(GLOB GAME_SHARDS CONFIGURE_DEPENDS
    "${SACREDSTONES_GENERATED_PROJECT}/generated/recompiled_*.cpp")
if(NOT GAME_SHARDS)
    message(FATAL_ERROR "No generated game shards found.")
endif()
add_library(main SHARED main.cpp ${GAME_SHARDS}
    "${SACREDSTONES_GENERATED_PROJECT}/generated/dispatch_table.cpp"
    "${SACREDSTONES_GENERATED_PROJECT}/generated/symbol_map.cpp")
target_include_directories(main PRIVATE
    "${SACREDSTONES_GENERATED_PROJECT}/framework/include"
    "${SACREDSTONES_GENERATED_PROJECT}/generated"
    "${GBARECOMP_ROOT}/src/runtime")
target_link_libraries(main PRIVATE "-Wl,--start-group"
    gbarecomp_runtime gbarecomp_debug gbarecomp_gba gbarecomp_armv4t
    "-Wl,--end-group")
include("${RECOMP_UI_ROOT}/recomp_ui.cmake")
recomp_target_launcher_ui(main CONSOLE gba)
target_link_libraries(main PRIVATE SDL2::SDL2)

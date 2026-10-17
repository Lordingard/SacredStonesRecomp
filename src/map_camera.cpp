#include "map_pointer.h"
#include "mod_function_hooks.h"
#include "runtime_arm.h"
#include <initializer_list>
#include <algorithm>
#include <array>

namespace sacredstones {
namespace {
constexpr char kCameraHook[] = "sacredstones.mouse_camera";
constexpr char kCursorHook[] = "sacredstones.mouse_cursor";
constexpr char kWorldCameraHook[] = "sacredstones.world_camera";
constexpr std::uint32_t kBm = 0x0202bcb0;
constexpr std::uint32_t kWorldData = 0x03005280;
bool camera_active = false;
bool camera_handover = false;

bool world_motion() {
    std::array<std::uint32_t, 128> stack{};
    std::array<std::uint32_t, 64> seen{};
    unsigned count = 0, visited = 0;
    for (unsigned i = 0; i < 8; ++i) stack[count++] = bus_read_u32(0x02026a70 + 4 * i);
    while (count && visited < seen.size()) {
        const auto p = stack[--count];
        if (p < 0x02000000 || p > 0x0203ffd4 || (p & 3)) continue;
        bool duplicate = false;
        for (unsigned i = 0; i < visited; ++i) duplicate |= seen[i] == p;
        if (duplicate) continue;
        seen[visited++] = p;
        const auto script = bus_read_u32(p);
        if (script == 0x08a3e4f4 || script == 0x08a3e52c) return true;
        if (count + 2 > stack.size()) return true;
        stack[count++] = bus_read_u32(p + 0x20);
        stack[count++] = bus_read_u32(p + 0x18);
    }
    return count != 0;
}

bool world_idle(std::uint32_t proc, const ArmCpuState* cpu) {
    return cpu && (cpu->R[14] & 1) && cpu->R[0] == proc &&
        proc >= 0x02000000 && proc <= 0x0203ffa8 && !(proc & 3) &&
        bus_read_u32(proc) == 0x08a3d748 && (bus_read_u32(proc + 0xc) & ~1u) == 0x080b9804 &&
        !bus_read_u8(proc + 0x28) && !bus_read_u16(proc + 0x24) &&
        // +0x30 is not initialized or consulted by the native idle loop.
        // It can contain recycled process data; live motion processes own input.
        !(bus_read_u8(proc + 0x27) & 9) && !world_motion() &&
        bus_read_u8(kWorldData + 1) == 1;
}

int world_cursor_entry(std::uint32_t address, int thumb, ArmCpuState* cpu) {
    int x = 0, y = 0;
    std::uint32_t proc = 0, mode = 0;
    if (address != 0x080b96f8 || !thumb || !map_pointer.cursor_jump(x, y, proc, mode) ||
        mode != 0x080b9804 || !world_idle(proc, cpu) || x < 24 || x > 456 || y < 24 || y > 296) return 0;
    const auto ignored = bus_read_u16(0x03000010);
    const int cx = int(bus_read_u32(kWorldData + 8) >> 8), cy = int(bus_read_u32(kWorldData + 12) >> 8);
    if ((ignored & 1) || (x > cx && (ignored & 0x10)) || (x < cx && (ignored & 0x20)) ||
        (y < cy && (ignored & 0x40)) || (y > cy && (ignored & 0x80))) return 0;
    bus_write_u32(kWorldData + 8, std::uint32_t(x) << 8);
    bus_write_u32(kWorldData + 12, std::uint32_t(y) << 8);
    map_pointer.cursor_jump_completed();
    // Continue the original handler: node hit-testing and route decisions
    // remain native. This is an entry observer, not a replacement routine.
    return 0;
}

int world_camera_entry(std::uint32_t address, int thumb, ArmCpuState* cpu) {
    int x = 0, y = 0; std::uint32_t proc = 0;
    if (address != 0x080b9218 || !thumb || !map_pointer.world_camera_position(x, y, proc) ||
        !world_idle(proc, cpu) || x < 0 || x > 240 || y < 0 || y > 160) return 0;
    bus_write_u16(kWorldData + 2, static_cast<std::uint16_t>(x));
    bus_write_u16(kWorldData + 4, static_cast<std::uint16_t>(y));
    cpu->R[15] = cpu->R[14] & ~1u;
    return 1;
}

int cursor_entry(std::uint32_t address, int thumb, ArmCpuState* cpu) {
    int x = 0, y = 0;
    std::uint32_t proc = 0, mode = 0;
    if (address != 0x08015838 || !thumb || !(cpu->R[14] & 1) ||
        !map_pointer.cursor_jump(x, y, proc, mode) ||
        proc < 0x02000000 || proc > 0x0203ffd4 || (proc & 3) ||
        bus_read_u32(proc) != 0x0859aad8 || (bus_read_u32(proc + 0xc) & ~1u) != mode ||
        bus_read_u8(proc + 0x28) || (bus_read_u8(proc + 0x27) & 9) ||
        bus_read_u8(kBm + 1) || bus_read_u8(kBm + 2) || (bus_read_u8(kBm + 4) & 0x50) ||
        x < 0 || y < 0 || x >= bus_read_u16(0x0202e4d4) || y >= bus_read_u16(0x0202e4d6)) return 0;
    if (mode == 0x0801d818) {
        const auto unit = bus_read_u32(0x03004e50);
        if (unit < 0x02000000 || unit > 0x0203ffb8 || (unit & 3) ||
            bus_read_u8(unit + 0x10) != x || bus_read_u8(unit + 0x11) != y) return 0;
        const auto rows = bus_read_u32(0x0202e4d8);
        const int cx = bus_read_u16(kBm + 0x14), cy = bus_read_u16(kBm + 0x16);
        if (cx >= bus_read_u16(0x0202e4d4) || cy >= bus_read_u16(0x0202e4d6) ||
            rows < 0x02000000 || rows > 0x0203ff00 || (rows & 3)) return 0;
        const auto row = bus_read_u32(rows + cy * 4);
        if (row < 0x02000000 || row > 0x0203ffc0 || !bus_read_u8(row + cx) ||
            bus_read_u8(row + cx) == bus_read_u8(unit + 0xb)) return 0;
    } else {
        const auto ignored = bus_read_u16(0x03000010);
        const int cx = bus_read_u16(kBm + 0x14), cy = bus_read_u16(kBm + 0x16);
        if ((ignored & 1) || (x > cx && (ignored & 0x10)) || (x < cx && (ignored & 0x20)) ||
            (y < cy && (ignored & 0x40)) || (y > cy && (ignored & 0x80))) return 0;
    }
    // Match SetCursorMapPosition. Leave units, movement maps and path data
    // alone: the rest of PlayerPhase_RangeDisplayIdle rebuilds the path.
    for (const unsigned offset : {0x14u, 0x1cu, 0x20u}) {
        const int scale = offset == 0x14 ? 1 : 16;
        bus_write_u16(kBm + offset, static_cast<std::uint16_t>(x * scale));
        bus_write_u16(kBm + offset + 2, static_cast<std::uint16_t>(y * scale));
    }
    map_pointer.cursor_jump_completed();
    cpu->R[15] = cpu->R[14] & ~1u;
    return 1;
}

int camera_entry(std::uint32_t address, int thumb, ArmCpuState* cpu) {
    int x = 0, y = 0;
    if (!thumb || !(cpu->R[14] & 1) ||
        bus_read_u8(kBm + 1) || bus_read_u8(kBm + 2) ||
        (bus_read_u8(kBm + 4) & 0x50)) return 0;
    const bool detached = map_pointer.camera_position(x, y);
    if (!detached && address == 0x0801588c && camera_handover) {
        // Native follow advances by 4/8 pixels and assumes a tile-aligned
        // origin. A free camera can stop at any pixel: reset that invariant
        // and its residual motion before letting the original routine run.
        const int max_x = std::min(std::max(0, int(bus_read_u16(0x0202e4d4)) * 16 - 240), int(bus_read_u16(kBm + 0x28)));
        const int max_y = std::min(std::max(0, int(bus_read_u16(0x0202e4d6)) * 16 - 160), int(bus_read_u16(kBm + 0x2a)));
        bus_write_u16(kBm + 0xc, std::clamp(int(static_cast<std::int16_t>(bus_read_u16(kBm + 0xc))), 0, max_x) & ~15);
        bus_write_u16(kBm + 0xe, std::clamp(int(static_cast<std::int16_t>(bus_read_u16(kBm + 0xe))), 0, max_y) & ~15);
        bus_write_u16(kBm + 0x32, 0);
        bus_write_u16(kBm + 0x34, 0);
        bus_write_u8(kBm + 0x36, 0);
        bus_write_u8(kBm + 0x37, 0);
        camera_handover = false;
        return 0;
    }
    if (!detached ||
        x < 0 || y < 0 || x > bus_read_u16(kBm + 0x28) || y > bus_read_u16(kBm + 0x2a)) return 0;
    if (address == 0x0801588c) {
        // Replace only FE8's cursor-follow routine; scripted camera moves
        // remain native. No unit, cursor, selection or save data is written.
        bus_write_u16(kBm + 0xc, static_cast<std::uint16_t>(x));
        bus_write_u16(kBm + 0xe, static_cast<std::uint16_t>(y));
        bus_write_u16(kBm + 0x32, 0);
        bus_write_u16(kBm + 0x34, 0);
        bus_write_u8(kBm + 0x36, 0);
        bus_write_u8(kBm + 0x37, 0);
    } else if (address == 0x08015a98) {
        const auto sx = static_cast<std::int64_t>(static_cast<std::int32_t>(cpu->R[0])) - x;
        const auto sy = static_cast<std::int64_t>(static_cast<std::int32_t>(cpu->R[1])) - y;
        if (sx >= -16 && sx < 240 && sy >= -16 && sy < 160) return 0;
        // Off-screen OAM coordinates otherwise wrap around on large maps.
    } else return 0;
    cpu->R[15] = cpu->R[14] & ~1u;
    return 1;
}
}

bool initialize_mouse_camera() {
    return gba_mod_register_function_entry_plugin(kCameraHook, 0x0801588c, 1, camera_entry) &&
        gba_mod_register_function_entry_plugin(kCameraHook, 0x08015a98, 1, camera_entry) &&
        gba_mod_register_function_entry_plugin(kCursorHook, 0x08015838, 1, cursor_entry) &&
        gba_mod_register_function_entry_plugin(kCursorHook, 0x080b96f8, 1, world_cursor_entry) &&
        gba_mod_register_function_entry_plugin(kWorldCameraHook, 0x080b9218, 1, world_camera_entry);
}

std::uint16_t map_pointer_input(const PointerInput& input) {
    const auto keys = map_pointer.update(input);
    int x = 0, y = 0;
    const bool detached = map_pointer.camera_position(x, y);
    if (camera_active && !detached) camera_handover = true;
    if (detached) camera_handover = false;
    camera_active = detached;
    gba_mod_set_function_hook_enabled(kCameraHook, detached || camera_handover);
    std::uint32_t proc = 0, mode = 0;
    gba_mod_set_function_hook_enabled(kWorldCameraHook, map_pointer.world_camera_position(x, y, proc));
    gba_mod_set_function_hook_enabled(kCursorHook, map_pointer.cursor_jump(x, y, proc, mode));
    return keys;
}
}

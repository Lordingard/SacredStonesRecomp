#include "map_pointer.h"
#include "mod_function_hooks.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
std::array<std::uint8_t, 256 * 1024> ram{};
std::array<std::uint8_t, 32 * 1024> iwram{};
constexpr unsigned base = 0x02000000, bm = 0x0202bcb0, proc = 0x02001000;
void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
void put(unsigned addr, unsigned value, unsigned size) {
    for (unsigned i = 0; i < size; ++i) {
        if (addr >= 0x03000000) iwram.at(addr - 0x03000000 + i) = value >> (8 * i);
        else ram.at(addr - base + i) = value >> (8 * i);
    }
}
}
extern "C" std::uint8_t bus_read_u8(std::uint32_t a) { return a >= 0x03000000 ? iwram.at(a - 0x03000000) : ram.at(a - base); }
extern "C" std::uint16_t bus_read_u16(std::uint32_t a) { return bus_read_u8(a) | (bus_read_u8(a + 1) << 8); }
extern "C" std::uint32_t bus_read_u32(std::uint32_t a) { return bus_read_u16(a) | (std::uint32_t(bus_read_u16(a + 2)) << 16); }
extern "C" void bus_write_u8(std::uint32_t a, std::uint8_t v) { put(a, v, 1); }
extern "C" void bus_write_u16(std::uint32_t a, std::uint16_t v) { put(a, v, 2); }
extern "C" void bus_write_u32(std::uint32_t a, std::uint32_t v) { put(a, v, 4); }

int main() {
    require(sacredstones::initialize_mouse_camera(), "register trusted camera hooks");
    require(sacredstones::initialize_mouse_camera(), "registration is idempotent");
    put(0x02026a70, proc, 4); put(proc, 0x0859aad8, 4); put(proc + 0xc, 0x0801c941, 4);
    put(0x0202e4d4, 32, 2); put(0x0202e4d6, 32, 2);
    put(bm + 0x28, 272, 2); put(bm + 0x2a, 352, 2);
    gbarecomp::GamePointerInput input;
    input.ewram = ram.data(); input.ewram_size = ram.size();
    input.iwram = iwram.data(); input.iwram_size = iwram.size();
    input.frame = 10; input.hover_x = 239; input.hover_y = 159;
    sacredstones::map_pointer.enabled = true;
    require(sacredstones::map_pointer_input(input) == 0, "pan does not emit guest keys");
    auto before = ram;
    ArmCpuState cpu{}; cpu.R[14] = 0x0801c8df; cpu.R[15] = 0x0801588c; cpu.cpsr = 0x6000003f;
    auto original = cpu;
    require(gba_mod_function_entry(0x0801588c, 1, &cpu) == 1, "camera routine replaced");
    require(cpu.R[15] == (original.R[14] & ~1u), "camera hook returns to native caller");
    original.R[15] = cpu.R[15];
    require(std::memcmp(&cpu, &original, sizeof(cpu)) == 0, "camera preserves registers and flags");
    require(ram[bm - base + 0xc] == 4 && ram[bm - base + 0xe] == 4, "camera moves immediately");
    for (unsigned i = 0; i < ram.size(); ++i) {
        const auto a = base + i;
        if ((a >= bm + 0xc && a < bm + 0x10) || (a >= bm + 0x32 && a < bm + 0x38)) continue;
        require(ram[i] == before[i], "no cursor, unit, process or unrelated RAM writes");
    }
    cpu.R[0] = 800; cpu.R[1] = 800;
    require(gba_mod_function_entry(0x08015a98, 1, &cpu) == 1, "offscreen cursor clipped to avoid OAM wrap");
    cpu.R[0] = 48; cpu.R[1] = 64;
    require(gba_mod_function_entry(0x08015a98, 1, &cpu) == 0, "visible cursor remains native");
    put(bm + 1, 1, 1);
    require(gba_mod_function_entry(0x0801588c, 1, &cpu) == 0, "locked scene keeps native camera");
    put(bm + 1, 0, 1);
    put(bm + 0x28, 0, 2);
    require(gba_mod_function_entry(0x0801588c, 1, &cpu) == 0, "restored smaller map rejects stale camera coordinates");
    put(bm + 0x28, 272, 2);
    put(bm + 0xc, 271, 2); put(bm + 0xe, 351, 2);
    put(bm + 0x32, 15, 2); put(bm + 0x34, 15, 2);
    put(bm + 0x36, 4, 1); put(bm + 0x37, 4, 1);
    input.reset = true; ++input.frame; sacredstones::map_pointer_input(input);
    require(gba_mod_function_entry(0x0801588c, 1, &cpu) == 0, "state/focus reset disables replacement");
    require(bus_read_u16(bm + 0xc) == 256 && bus_read_u16(bm + 0xe) == 336 &&
            !bus_read_u16(bm + 0x32) && !bus_read_u16(bm + 0x34) &&
            !bus_read_u8(bm + 0x36) && !bus_read_u8(bm + 0x37),
            "off-grid camera handover clears residual motion and restores native alignment");
    put(bm + 0xc, 4, 2); put(bm + 0xe, 4, 2);
    input.reset = false; input.primary = true; input.hover_x = -1;
    input.x = 100; input.y = 80; ++input.frame;
    require(sacredstones::map_pointer_input(input) == 0, "cursor jump withholds A");
    require(gba_mod_function_entry(0x0801588c, 1, &cpu) == 1 &&
            bus_read_u16(bm + 0xc) == 4 && bus_read_u16(bm + 0xe) == 4,
            "click holds camera instead of native chasing the teleported cursor");
    before = ram; original = cpu;
    require(gba_mod_function_entry(0x08015838, 1, &cpu) == 1, "instant cursor animation replacement");
    require(bus_read_u16(bm + 0x14) == 6 && bus_read_u16(bm + 0x16) == 5 &&
            bus_read_u16(bm + 0x1c) == 96 && bus_read_u16(bm + 0x20) == 96 &&
            bus_read_u16(bm + 0x1e) == 80 && bus_read_u16(bm + 0x22) == 80,
            "logical, target and display cursor coordinates agree instantly");
    original.R[15] = original.R[14] & ~1u;
    require(std::memcmp(&cpu, &original, sizeof(cpu)) == 0, "cursor hook return ABI and flags preserved");
    for (unsigned i = 0; i < ram.size(); ++i) {
        const auto a = base + i;
        if ((a >= bm + 0x14 && a < bm + 0x18) || (a >= bm + 0x1c && a < bm + 0x24)) continue;
        require(ram[i] == before[i], "cursor hook cannot alter units, path data or movement range");
    }
    require(gba_mod_function_entry(0x08015838, 1, &cpu) == 0, "jump applies only once");
    input.primary = false; ++input.frame;
    require(sacredstones::map_pointer_input(input) == 1, "confirmation only after observed native jump");
    input.reset = true; ++input.frame; sacredstones::map_pointer_input(input);
    input.reset = false; input.primary = true; input.x = 120; ++input.frame;
    sacredstones::map_pointer_input(input);
    put(0x03000010, 0x10, 2); before = ram;
    require(gba_mod_function_entry(0x08015838, 1, &cpu) == 0 && ram == before,
            "cursor hook rechecks newly ignored direction before any write");
    input.reset = true; ++input.frame; sacredstones::map_pointer_input(input);
    put(proc + 0xc, 0x0801d819, 4); put(0x03004e50, 0x02003000, 4);
    put(0x02003010, 5, 1); put(0x02003011, 5, 1);
    put(0x0200300b, 1, 1); put(0x0202e4d8, 0x02004000, 4);
    put(0x02004014, 0x02004100, 4); put(0x02004106, 0x81, 1);
    input.reset = false; input.primary = true; input.x = 84; input.y = 84; ++input.frame;
    sacredstones::map_pointer_input(input);
    put(0x02003010, 4, 1); before = ram;
    require(gba_mod_function_entry(0x08015838, 1, &cpu) == 0 && ram == before,
            "forced-A hook rejects active unit changed after click");
    put(0x02003010, 5, 1); before = ram;
    require(gba_mod_function_entry(0x08015838, 1, &cpu) == 1 && bus_read_u16(bm + 0x14) == 5,
            "forced-A hook restores cursor to active unit anchor");
    for (unsigned i = 0; i < ram.size(); ++i) {
        const auto a = base + i;
        if ((a >= bm + 0x14 && a < bm + 0x18) || (a >= bm + 0x1c && a < bm + 0x24)) continue;
        require(ram[i] == before[i], "forced recovery preserves unit and native path data");
    }
    input.reset = true; ++input.frame; sacredstones::map_pointer_input(input);
    put(0x03000010, 0, 2); put(proc, 0x08a3d748, 4); put(proc + 0xc, 0x080b9805, 4);
    put(0x03005281, 1, 1); put(0x03005288, 120 << 8, 4); put(0x0300528c, 80 << 8, 4);
    put(proc + 0x30, 0x40000000, 4);
    input.reset = false; input.primary = true; input.x = 180; input.y = 90; ++input.frame;
    require(sacredstones::map_pointer_input(input) == 0, "world cursor waits for native input boundary");
    cpu.R[0] = proc; cpu.R[14] = 0x080b980b; cpu.R[15] = 0x080b96f8;
    auto internal_before = iwram; before = ram; original = cpu;
    cpu.R[0] = proc + 0x80;
    require(gba_mod_function_entry(0x080b96f8, 1, &cpu) == 0 && iwram == internal_before,
            "world cursor refuses a different native caller");
    cpu.R[0] = proc; put(0x03000010, 0x10, 2); auto restricted = iwram;
    require(gba_mod_function_entry(0x080b96f8, 1, &cpu) == 0 && iwram == restricted,
            "world cursor rechecks restrictions immediately before writes");
    put(0x03000010, 0, 2);
    const auto old_motion_script = bus_read_u32(0x02001800);
    put(proc + 0x18, 0x02001800, 4); put(0x02001800, 0x08a3e52c, 4);
    require(gba_mod_function_entry(0x080b96f8, 1, &cpu) == 0 && iwram == internal_before,
            "world hook rejects live scripted camera movement started after click");
    put(proc + 0x18, 0, 4); put(0x02001800, old_motion_script, 4);
    require(gba_mod_function_entry(0x080b96f8, 1, &cpu) == 0, "world entry hook continues original node handler");
    require(bus_read_u32(0x03005288) == (180 << 8) && bus_read_u32(0x0300528c) == (90 << 8),
            "world cursor places exact fixed-point pixel position");
    require(ram == before && std::memcmp(&cpu, &original, sizeof(cpu)) == 0,
            "world cursor preserves all EWRAM and CPU registers");
    for (unsigned i = 0; i < iwram.size(); ++i)
        if (i < 0x5288 || i >= 0x5290) require(iwram[i] == internal_before[i], "world cursor writes only coordinate pair");
    input.primary = false; ++input.frame;
    require(sacredstones::map_pointer_input(input) == 1, "world native confirmation follows completed hook");
    require(gba_mod_function_entry(0x080b9218, 1, &cpu) == 1, "world cursor follow replaced only while detached");
    input.reset = true; ++input.frame; sacredstones::map_pointer_input(input);
    require(gba_mod_function_entry(0x080b9218, 1, &cpu) == 0, "reset restores native world camera");
    std::puts("PASS: trusted battle/world camera ABI, write bounds, cursor placement and reset");
}

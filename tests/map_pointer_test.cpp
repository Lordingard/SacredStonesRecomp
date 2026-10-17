#include "map_pointer.h"

#include <array>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <vector>
#include "snapshot.h"

namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
struct Fixture {
    std::array<std::uint8_t, 256 * 1024> ram{};
    std::array<std::uint8_t, 32 * 1024> iwram{};
    sacredstones::MapPointer pointer;
    sacredstones::PointerInput input;
    static constexpr unsigned bm = 0x0202bcb0, proc = 0x02001000;
    Fixture() {
        pointer.enabled = true;
        input.ewram = ram.data(); input.ewram_size = ram.size(); input.frame = 10;
        input.iwram = iwram.data(); input.iwram_size = iwram.size();
        put(0x02026a70, proc, 4);
        put(proc, 0x0859aad8, 4);
        put(proc + 0x0c, 0x0801c941, 4);
        put(0x0202e4d4, 32, 2); put(0x0202e4d6, 32, 2);
        put(bm + 0x28, 272, 2); put(bm + 0x2a, 352, 2);
        cursor(3, 4);
    }
    void put(unsigned address, unsigned value, unsigned count) {
        for (unsigned i = 0; i < count; ++i) {
            if (address >= 0x03000000) iwram.at(address - 0x03000000 + i) = value >> (8 * i);
            else ram.at(address - 0x02000000 + i) = value >> (8 * i);
        }
    }
    void cursor(int x, int y) {
        put(bm + 0x14, x, 2); put(bm + 0x16, y, 2);
        put(bm + 0x20, x * 16, 2); put(bm + 0x22, y * 16, 2);
    }
    unsigned click(int x, int y, bool right = false) {
        input.x = x; input.y = y; input.primary = !right; input.secondary = right;
        const auto result = pointer.update(input);
        input.primary = input.secondary = false;
        return result;
    }
    unsigned tick() { ++input.frame; return pointer.update(input); }
    unsigned next_pulse() {
        for (unsigned i = 0; i < 8; ++i) { const auto keys = tick(); if (keys) return keys; }
        return 0;
    }
    void menu() {
        put(bm + 1, 1, 1);
        put(proc, 0x085b6000, 4); put(proc + 0xc, 0x0804f165, 4);
        put(proc + 0x30, 0x08590000, 4);
        put(proc + 0x2c, 1, 1); put(proc + 0x2d, 2, 1);
        put(proc + 0x2e, 10, 1); put(proc + 0x2f, 8, 1);
        put(proc + 0x60, 3, 1); put(proc + 0x61, 0, 1);
        for (unsigned i = 0; i < 3; ++i) {
            const auto item = 0x02002000 + i * 0x80;
            put(proc + 0x34 + i * 4, item, 4);
            put(item + 0x2a, 2, 2); put(item + 0x2c, 3 + 2 * i, 2);
            put(item + 0x3d, 1, 1);
        }
    }
    void world() {
        put(proc, 0x08a3d748, 4); put(proc + 0xc, 0x080b9805, 4);
        put(0x03005281, 1, 1);
        put(0x03005288, 120 << 8, 4); put(0x0300528c, 80 << 8, 4);
        put(bm + 2, 1, 1); // Battle-map graphics state is unrelated here.
    }
    void targets() {
        put(bm + 1, 1, 1);
        put(proc, 0x085b655c, 4); put(proc + 0xc, 0x0804f96d, 4);
        put(proc + 0x2c, 0x0859d3f8, 4); put(proc + 0x30, 0x02003000, 4);
        for (unsigned i = 0; i < 2; ++i) {
            const auto node = 0x02003000 + i * 12, other = 0x02003000 + (1 - i) * 12;
            put(node, 3 + i * 2, 1); put(node + 1, 4, 1);
            put(node + 4, other, 4); put(node + 8, other, 4);
        }
    }
    void trade(bool destination = false) {
        put(bm + 1, 1, 1);
        put(proc, 0x0859bb1c, 4);
        put(proc + 0xc, destination ? 0x0802d981 : 0x0802d835, 4);
        for (unsigned col = 0; col < 2; ++col)
            for (unsigned row = 0; row < 3; ++row) put(proc + 0x34 + col * 6 + row, 1, 1);
    }
    void config() {
        put(proc, 0x08a2ece0, 4); put(proc + 0xc, 0x080b220d, 4);
        put(bm + 1, 1, 1); put(bm + 2, 1, 1);
        put(0x020201bc, 13, 2);
    }
    void guide(unsigned state = 0) {
        put(proc, 0x08b12c64, 4); put(proc + 0xc, 0x080cefd5, 4);
        put(bm + 1, 1, 1); put(bm + 2, 1, 1);
        put(0x02020188 + 0x2f, state, 1);
        put(0x02020188 + 0x3c, 9, 1); put(0x02020188 + 0x3d, 8, 1);
        put(0x02020188 + 0x3e, 10, 1); put(0x02020188 + 0x3f, 12, 1);
    }
    void unit_list() {
        put(proc, 0x08a177a0, 4); put(proc + 0xc, 0x08091aed, 4);
        put(bm + 1, 1, 1); put(bm + 2, 1, 1);
        put(proc + 0x2f, 1, 1); put(proc + 0x2e, 6, 1);
        put(0x0200f158, 10, 1);
        for (unsigned i = 0; i < 10; ++i) {
            put(0x0200d6e0 + i * 4, 0x02002000 + i * 4, 4);
            put(0x02002000 + i * 4, 0x02003000 + i * 0x48, 4);
            put(0x02003000 + i * 0x48, 0x08012340, 4);
        }
    }
};
}

int main(int argc, char** argv) {
    for (unsigned slot = 0; slot < 3; ++slot) for (unsigned filled : {0u, 1u}) {
      Fixture f; f.put(Fixture::proc, 0x08a203a8, 4); f.put(Fixture::proc + 0xc, 0x080a9495, 4);
      f.put(Fixture::proc + 0x2f, 220, 1); f.put(Fixture::proc + 0x42, 0x40, 2);
      f.put(Fixture::proc + 0x2c, slot, 1); f.put(Fixture::proc + 0x37 + slot, filled ? 3 : 0xff, 1);
      const auto before = f.ram;
      require(f.click(80, 36 + slot * 32) == 1 && f.ram == before,
              "post-chapter accepts every empty or occupied slot through native A only");
      f.put(Fixture::proc + 0x36, 1, 1);
      require(f.next_pulse() == 0, "slot click never confirms post-chapter Yes automatically");
      ++f.input.frame; require(f.click(60, 144) == 1, "post-chapter Yes needs separate explicit click");
    }
    { Fixture f; f.put(Fixture::proc, 0x08a203a8, 4); f.put(Fixture::proc + 0xc, 0x080a9495, 4);
      f.put(Fixture::proc + 0x42, 0x40, 2); f.put(Fixture::proc + 0x36, 1, 1);
      require(f.click(110, 144) == 0x10, "post-chapter No navigates right without saving");
      f.put(Fixture::proc + 0x36, 2, 1);
      require(f.next_pulse() == 1, "post-chapter No confirms after native cursor moves"); }
    { Fixture f; f.put(Fixture::proc, 0x08a203a8, 4); f.put(Fixture::proc + 0xc, 0x080a9495, 4);
      f.put(Fixture::proc + 0x42, 0x40, 2); f.put(Fixture::proc + 0x36, 1, 1);
      require(f.click(80, 80, true) == 2, "right click dismisses post-chapter confirmation");
      f.put(Fixture::proc + 0xc, 0x080a96ed, 4);
      ++f.input.frame; require(f.click(80, 80) == 0, "saving/scrolling callback refuses new clicks"); }
    { Fixture f; f.put(Fixture::proc, 0x08a203a8, 4); f.put(Fixture::proc + 0xc, 0x080a9495, 4);
      f.put(Fixture::proc + 0x2f, 220, 1); f.put(Fixture::proc + 0x42, 0x40, 2);
      f.put(Fixture::proc + 0x40, 3, 1);
      require(f.click(80, 36) == 0 && f.tick() == 0, "save-slot click waits for native help animation");
      f.put(Fixture::proc + 0x40, 0, 1); require(f.tick() == 1, "save-slot click resumes after help animation"); }
    { Fixture f; f.world(); const auto before = f.iwram;
      require(f.click(120, 80) == 1 && f.tick() == 0, "world-map current point confirms once despite stale battle graphics lock");
      require(f.iwram == before, "world-map adapter never writes cursor, units or routes"); }
    { Fixture f; f.world(); f.put(Fixture::proc + 0x30, 0x40000000, 4);
      require(f.click(140, 80) == 0, "world click with recycled process data waits for placement");
      int x, y; std::uint32_t proc, mode;
      require(f.pointer.cursor_jump(x, y, proc, mode) && x == 140 && y == 80,
              "unused world process field cannot block destination selection"); }
    { Fixture f; f.world(); f.put(0x03005282, 100, 2); f.put(0x03005284, 40, 2);
      require(f.click(80, 60) == 0, "world-map remote click waits for native placement");
      int x, y; std::uint32_t proc, mode;
      require(f.pointer.cursor_jump(x, y, proc, mode) && x == 180 && y == 100 && mode == 0x080b9804,
              "world-map click uses its own pixel camera, not battle-map tile coordinates");
      f.put(0x03005288, 180 << 8, 4); f.put(0x0300528c, 100 << 8, 4); f.pointer.cursor_jump_completed();
      require(f.tick() == 1 && f.tick() == 0 && f.tick() == 0, "world-map native A occurs once after observed placement"); }
    { Fixture f; f.world(); f.input.hover_x = 239; f.input.hover_y = 159;
      require(f.pointer.update(f.input) == 0, "world edge hover never confirms destination");
      int x, y; std::uint32_t proc;
      require(f.pointer.world_camera_position(x, y, proc) && x == 4 && y == 4,
              "world edge pan is immediate and independent of cursor");
      for (unsigned i = 0; i < 100; ++i) f.tick();
      require(f.pointer.world_camera_position(x, y, proc) && x == 240 && y == 160,
              "world camera cannot leave 480x320 map bounds"); }
    { Fixture f; f.world(); f.put(Fixture::proc + 0x18, 0x02001800, 4); f.put(0x02001800, 0x08a3e4f4, 4);
      require(f.click(120, 80) == 0, "native world cursor animation owns input");
      f.put(0x02001800, 0x08a3e52c, 4);
      require(f.click(120, 80) == 0, "scripted world camera owns input"); }
    { Fixture f; f.world(); f.click(140, 80); f.put(Fixture::proc + 0xc, 0, 4);
      require(f.tick() == 0, "world travel transition cancels queued destination"); }
    { Fixture f; f.world(); f.put(0x03000010, 0x10, 2);
      require(f.click(140, 80) == 0, "world pointer respects ignored directions");
      int x, y; std::uint32_t proc, mode;
      require(!f.pointer.cursor_jump(x, y, proc, mode), "restricted world click never queues a jump"); }
    { Fixture f; f.world();
      require(f.click(0, 0) == 0, "world outer-frame click cannot be clamped onto a nearby city");
      int x, y; std::uint32_t proc, mode;
      require(!f.pointer.cursor_jump(x, y, proc, mode), "outside native world bounds queues no placement"); }
    { Fixture f; f.menu(); f.put(Fixture::bm + 2, 1, 1);
      f.put(0x02026a74, 0x02001800, 4); f.put(0x02001800, 0x08a3d748, 4); f.put(0x02001828, 1, 1);
      require(f.click(20, 28) == 1, "world modal menu ignores stale battlefield graphics lock"); }
    { Fixture f; f.put(0x03000010, 0xf0, 2);
      require(f.click(88, 72) == 0, "tutorial direction mask rejects instant movement");
      int x, y; std::uint32_t p, mode;
      require(!f.pointer.cursor_jump(x, y, p, mode), "blocked direction cannot queue native jump");
      ++f.input.frame; require(f.click(56, 72) == 1, "tutorial can still select current tile"); }
    { Fixture f; f.click(88, 72); f.put(0x03000010, 1, 2);
      require(f.tick() == 0, "new tutorial A mask cancels queued confirmation");
      int x, y; std::uint32_t p, mode;
      require(!f.pointer.cursor_jump(x, y, p, mode), "mask change cancels queued jump"); }
    { Fixture f; f.put(Fixture::proc + 0xc, 0x0801d819, 4);
      f.put(0x03004e50, 0x02003000, 4); f.put(0x02003010, 4, 1); f.put(0x02003011, 4, 1);
      f.put(0x0200300b, 1, 1); f.put(0x0202e4d8, 0x02004000, 4);
      f.put(0x02004010, 0x02004100, 4); f.put(0x02004103, 0x81, 1);
      f.put(0x03000010, 0x3fe, 2);
      const auto before = f.ram;
      require(f.click(56, 72) == 0, "forced-A recovery never synthesizes another A");
      int x, y; std::uint32_t p, mode;
      require(f.pointer.cursor_jump(x, y, p, mode) && x == 4 && y == 4,
              "forced-A recovery anchors cursor to active unit only");
      require(f.ram == before, "forced-A policy does not move unit or rewrite script");
      f.cursor(4, 4); f.pointer.cursor_jump_completed();
      require(f.tick() == 0 && f.tick() == 0, "native forced A remains sole confirmation");
      ++f.input.frame; f.click(104, 72);
      require(!f.pointer.cursor_jump(x, y, p, mode), "forced-A rejects arbitrary destination");
      ++f.input.frame; require(f.click(56, 72, true) == 0, "forced-A cannot cancel tutorial"); }
    { Fixture f; f.put(Fixture::proc + 0xc, 0x0801d819, 4);
      f.put(0x03004e50, 0x02003000, 4); f.put(0x02003010, 4, 1); f.put(0x02003011, 4, 1);
      f.put(0x0202e4d8, 0x02004000, 4); f.put(0x02004010, 0x02004100, 4);
      require(f.click(56, 72) == 0, "valid guided movement remains native");
      int x, y; std::uint32_t p, mode;
      require(!f.pointer.cursor_jump(x, y, p, mode), "forced-A never resets an unoccupied scripted destination"); }
    { Fixture f; f.trade(); f.put(Fixture::proc + 0x48, 2, 1);
      require(f.click(144, 76) == 0x10, "guided trade first asks for native Right");
      f.put(Fixture::proc + 0x41, 1, 1); f.put(Fixture::proc + 0x48, 3, 1);
      require(f.next_pulse() == 0, "guided trade instruction change cancels old click");
      f.put(Fixture::proc + 0x30, 0x02003000, 4); f.put(0x0200301e, 0x016c, 2);
      ++f.input.frame; require(f.click(144, 92) == 0, "guided trade rejects non-Vulnerary item");
      ++f.input.frame; require(f.click(144, 76) == 1, "guided trade accepts Vulnerary explicitly"); }
    { Fixture f; f.trade(true); f.put(Fixture::proc + 0x48, 5, 1);
      require(f.click(144, 76) == 0, "guided trade rejects another destination");
      ++f.input.frame; require(f.click(32, 76) == 1, "guided trade confirms native destination");
      f.put(Fixture::proc + 0x48, 8, 1); require(f.next_pulse() == 0, "trade completion cannot inherit A");
      ++f.input.frame; require(f.click(32, 76) == 0, "trade completion requires cancel not another item");
      ++f.input.frame; require(f.click(32, 76, true) == 2, "right click finishes guided trade"); }
    for (unsigned state : {1u, 4u}) {
      Fixture f; f.trade(); f.put(Fixture::proc + 0x48, state, 1);
      require(f.click(144, 76) == 0 && f.click(32, 76, true) == 0,
              "automatic trade tutorial phases cannot receive pointer input");
    }
    { Fixture f; f.targets();
      const auto child = 0x02001800u;
      f.put(Fixture::proc + 0x18, child, 4); f.put(child, 0x085913f0, 4); f.put(child + 0xc, 0x08007c4d, 4);
      require(f.click(100, 100) == 1, "tutorial text wait owns input above live combat target");
      f.put(child, 0, 4); f.put(child + 0xc, 0, 4);
      require(f.next_pulse() == 0, "tutorial advance cannot confirm underlying attack"); }
    { Fixture f; f.menu(); f.put(Fixture::proc + 0x63, 0x40, 1);
      const auto event = 0x02001800u;
      f.put(Fixture::proc + 0x18, event, 4); f.put(event, 0x08591ac0, 4); f.put(event + 0xc, 0x0800ce4d, 4);
      f.put(event + 0x3c, 2, 2); f.put(event + 0x41, 1, 1);
      require(f.click(20, 28) == 0, "frozen menu cannot fall through to cinematic skip"); }
    { Fixture f; f.put(Fixture::proc, 0x08a01760, 4); f.put(Fixture::proc + 0xc, 0x0808b179, 4);
      require(f.click(100, 100) == 1, "tutorial box dialogue can advance");
      f.put(0x0203e82a, 0x8a, 2); ++f.input.frame;
      require(f.click(100, 100) == 0, "box no-fast flag prevents acceleration");
      ++f.input.frame; require(f.click(100, 100, true) == 0, "box no-skip flag respected");
      const auto child = 0x02001800u;
      f.put(Fixture::proc + 0x18, child, 4); f.put(child, 0x085913f0, 4); f.put(child + 0xc, 0x08007c4d, 4);
      ++f.input.frame; require(f.click(100, 100) == 1, "explicit box wait can advance despite no-fast");
      f.menu(); require(f.next_pulse() == 0, "box advance never confirms arriving menu"); }
    for (unsigned flags : {2u, 0x12u, 6u}) {
      Fixture f; f.world();
      const unsigned event = 0x02001800, talk = 0x02001900;
      f.put(Fixture::proc + 0x18, event, 4); f.put(event, 0x08591ac0, 4);
      f.put(event + 0xc, 0x0800ce4d, 4); f.put(event + 0x3c, flags, 2);
      f.put(event + 0x18, talk, 4); f.put(talk, 0x08591340, 4); f.put(talk + 0xc, 0x08006b4d, 4);
      f.put(0x030000c8, 4, 2);
      const auto before = f.ram; const auto internal = f.iwram;
      require(f.click(100, 100, true) == (flags == 2 ? 8u : 0u),
              "world dialogue right click obeys event skip gate rather than text NOSKIP");
      require(f.ram == before && f.iwram == internal, "world skip never modifies event or talk flags");
      require(f.next_pulse() == 0, "world skip pulse does not repeat"); }
    { Fixture f; f.world();
      const unsigned talk = 0x02001800;
      f.put(Fixture::proc + 0x18, talk, 4); f.put(talk, 0x08591340, 4); f.put(talk + 0xc, 0x08006b4d, 4);
      require(f.click(100, 100, true) == 0, "world talk without a skippable event cannot supply Start"); }
    { Fixture f; f.world();
      const unsigned event = 0x02001800, talk = 0x02001900, choice = 0x02001a00;
      f.put(Fixture::proc + 0x18, event, 4); f.put(event, 0x08591ac0, 4);
      f.put(event + 0xc, 0x0800ce4d, 4); f.put(event + 0x3c, 2, 2);
      f.put(event + 0x18, talk, 4); f.put(talk, 0x08591340, 4); f.put(talk + 0xc, 0x08006b4d, 4);
      f.put(talk + 0x18, choice, 4); f.put(choice, 0x085914b0, 4); f.put(choice + 0xc, 0x08007e9d, 4);
      f.put(choice + 0x2a, 1, 2);
      require(f.click(100, 100, true) == 2, "world dialogue choices keep native Back rather than skipping scene"); }
    { Fixture f;
      const auto before = f.ram;
      require(f.click(3 * 16 + 8, 4 * 16 + 8) == 1, "click current tile confirms");
      require(f.pointer.update(f.input) == 1, "same-frame pump keeps pulse");
      require(f.tick() == 0 && f.tick() == 0, "one click never repeats confirm");
      require(f.ram == before, "input adapter never writes guest RAM"); }
    { Fixture f;
      require(f.click(5 * 16 + 8, 5 * 16 + 8) == 0, "instant cursor request never emits D-pad or A");
      int x = 0, y = 0; std::uint32_t proc = 0, mode = 0;
      require(f.pointer.cursor_jump(x, y, proc, mode) && x == 5 && y == 5 && proc == Fixture::proc,
              "request directly targets clicked tile");
      require(f.tick() == 0, "no confirmation before native cursor hook executes");
      f.cursor(5, 5); f.pointer.cursor_jump_completed();
      require(f.tick() == 1 && f.tick() == 0, "confirm once after native path update"); }
    { Fixture f; f.put(Fixture::bm + 0x0c, 32, 2); f.put(Fixture::bm + 0x0e, 16, 2);
      require(f.click(16 + 8, 48 + 8) == 1, "map camera offset included"); }
    { Fixture f; f.put(Fixture::proc + 0x0c, 0x0801cd1d, 4);
      require(f.click(48 + 8, 64 + 8) == 1, "movement selection accepts click"); }
    { Fixture f; require(f.click(0, 0, true) == 2 && f.tick() == 0, "right click is one B pulse"); }
    { Fixture f; f.put(Fixture::bm + 0x20, 49, 2);
      require(f.click(0, 0, true) == 0, "cancel waits for cursor animation");
      f.cursor(3, 4); require(f.tick() == 2 && f.tick() == 0,
                              "animated cancel delivered once when game accepts it"); }
    { Fixture f; f.pointer.enabled = false; require(f.click(56, 72) == 0, "disabled by default policy"); }
    { Fixture f; require(f.click(-1, 72) == 0 && f.click(240, 72) == 0 && f.click(56, 160) == 0,
                         "outside game image ignored"); }
    { Fixture f; f.put(0x0202e4d4, 4, 2); require(f.click(100, 72) == 0, "outside map ignored"); }
    { Fixture f; f.put(Fixture::bm + 0x0c, 0xffff, 2); require(f.click(56, 72) == 0, "invalid camera ignored"); }
    { Fixture f; f.put(Fixture::proc + 0x28, 1, 1); require(f.click(56, 72) == 0, "blocking child/menu ignored"); }
    { Fixture f; f.put(Fixture::proc + 0x27, 8, 1); require(f.click(56, 72) == 0, "starting proc ignored"); }
    { Fixture f; f.put(Fixture::proc + 0x0c, 0x08012345, 4); require(f.click(56, 72) == 0, "other states ignored"); }
    { Fixture f; f.put(0x02026a70, 0, 4); require(f.click(56, 72) == 0, "stale orphan proc ignored"); }
    { Fixture f; f.put(Fixture::proc, 0, 4); f.put(Fixture::proc + 0x20, Fixture::proc, 4);
      require(f.click(56, 72) == 0, "cycle in process graph terminates safely"); }
    { Fixture f; f.input.ewram_size = 2; require(f.click(56, 72) == 0, "truncated RAM ignored"); }
    { Fixture f; f.input.ewram = nullptr; require(f.click(56, 72) == 0, "missing RAM ignored"); }
    { Fixture f; require(f.click(100, 72) == 0, "start pending instant click");
      f.input.physical_keys = 0x3fe; require(f.tick() == 0, "physical input wins");
      f.input.physical_keys = 0x3ff; require(f.tick() == 0, "pending click cancelled by physical input"); }
    { Fixture f; f.click(100, 72); f.input.reset = true;
      require(f.tick() == 0, "pause/menu/focus/load reset cancels");
      f.input.reset = false; require(f.tick() == 0, "reset never resumes old target"); }
    { Fixture f; f.click(100, 72); f.input.frame = 1;
      require(f.pointer.update(f.input) == 0, "backward frame counter cancels"); }
    { Fixture f; f.click(100, 72); f.put(Fixture::proc + 0x0c, 0x0801cd1d, 4);
      require(f.tick() == 0, "phase change cancels target"); }
    { Fixture f; f.click(100, 72); f.put(Fixture::bm + 0x20, 49, 2);
      require(f.tick() == 0 && f.tick() == 0, "wait for cursor interpolation"); }
    { Fixture f; f.click(100, 72); f.input.frame += 601;
      require(f.pointer.update(f.input) == 0, "stuck target times out"); }
    { Fixture f; f.menu(); const auto before = f.ram;
      require(f.click(24, 32) == 1, "click selected action or weapon menu row");
      require(f.ram == before, "menu input is read-only"); }
    { Fixture f; f.menu();
      require(f.click(24, 64) == 0x80, "navigate towards clicked action row");
      f.put(Fixture::proc + 0x61, 1, 1); require(f.tick() == 0 && f.tick() == 0x80, "second menu down pulse");
      f.put(Fixture::proc + 0x61, 2, 1); require(f.tick() == 0 && f.tick() == 1 && f.tick() == 0,
                                                        "menu confirms clicked row only once"); }
    { Fixture f; f.menu(); f.put(0x02002000 + 0x80 + 0x3d, 2, 1);
      require(f.click(24, 48) == 0, "disabled action cannot be clicked"); }
    { Fixture f; f.menu(); f.put(Fixture::proc + 0x30, 0x0859d184, 4);
      f.put(0x02002000 + 0x3d, 2, 1); const auto before = f.ram;
      require(f.click(24, 32) == 1, "grey Item inventory entry opens native action submenu");
      require(f.ram == before, "grey inventory selection never changes items or availability");
      f.put(Fixture::proc + 0x30, 0x0859d118, 4);
      require(f.next_pulse() == 0, "inventory click cannot carry into action submenu");
      ++f.input.frame; require(f.click(24, 32) == 0, "grey Use/Equip command remains disabled"); }
    { Fixture f; f.menu(); f.put(Fixture::proc + 0x30, 0x0859d184, 4);
      f.put(0x02002000 + 0x3d, 3, 1);
      require(f.click(24, 32) == 0, "hidden inventory entry is never selectable"); }
    { Fixture f; const auto before = f.ram; const auto internal = f.iwram;
      f.input.dragging = f.input.drag_start = f.input.pointer_down = true;
      f.input.drag_dx = 20; f.input.drag_dy = 10;
      require(f.pointer.update(f.input) == 0, "map drag moves camera without buttons");
      int x, y; require(f.pointer.camera_position(x, y) && x == 20 && y == 10, "map drag starts anywhere in image");
      require(f.ram == before && f.iwram == internal, "map drag policy never writes cursor or units");
      require(f.pointer.update(f.input) == 0 && f.pointer.camera_position(x, y) && x == 20,
              "same-frame drag is not accumulated twice");
      f.input.drag_start = false; f.input.drag_dx = 3; f.input.drag_dy = -4;
      require(f.tick() == 0 && f.pointer.camera_position(x, y) && x == 23 && y == 6, "map drag accumulates before native hook catches up");
      f.input.dragging = f.input.pointer_down = false;
      require(f.tick() == 0 && f.pointer.camera_position(x, y) && x == 23 && y == 6, "drag release never selects or recenters"); }
    { Fixture f; f.click(100, 72); f.input.dragging = f.input.drag_start = f.input.pointer_down = true;
      f.input.drag_dx = 25;
      require(f.tick() == 0, "new drag cancels queued tile selection");
      int x, y; std::uint32_t p, mode;
      require(!f.pointer.cursor_jump(x, y, p, mode), "drag cannot retain a pending cursor jump"); }
    { Fixture f; f.input.dragging = f.input.pointer_down = true; f.input.drag_dx = 20;
      int x, y; require(f.pointer.update(f.input) == 0 && !f.pointer.camera_position(x, y), "orphan drag cannot acquire map camera"); }
    { Fixture f; f.input.dragging = f.input.drag_start = f.input.pointer_down = true; f.input.drag_dx = 20;
      f.pointer.update(f.input); f.input.drag_start = false; f.input.reset = true; f.tick();
      f.input.reset = false; int x, y;
      require(f.tick() == 0 && !f.pointer.camera_position(x, y), "state/menu reset cancels ownership until a fresh drag"); }
    for (unsigned mode : {0x0801c941u, 0x0801d819u}) {
      Fixture f; f.put(Fixture::proc + 0xc, mode, 4);
      if (mode == 0x0801c941) f.put(0x03000010, 0xf0, 2);
      f.input.dragging = f.input.drag_start = f.input.pointer_down = true; f.input.drag_dx = 20;
      int x, y; require(f.pointer.update(f.input) == 0 && !f.pointer.camera_position(x, y), "drag preserves scripted tutorial restrictions"); }
    { Fixture f; f.world(); f.input.dragging = f.input.drag_start = f.input.pointer_down = true;
      f.input.drag_dx = 400; f.input.drag_dy = 300; int x, y; std::uint32_t p;
      require(f.pointer.update(f.input) == 0 && f.pointer.world_camera_position(x, y, p) && x == 240 && y == 160,
              "world-map drag uses independent bounded pixel camera"); }
    { Fixture f; f.menu(); f.click(24, 64);
      f.input.dragging = f.input.drag_start = f.input.pointer_down = true;
      require(f.next_pulse() == 0, "dragging a menu cannot finish an older click plan"); }
    { Fixture f; f.menu(); require(f.click(100, 48) == 0 && f.click(24, 0) == 0,
                                  "outside menu rows never leaks into map"); }
    { Fixture f; f.menu(); f.put(Fixture::proc + 0x63, 0x40, 1);
      require(f.click(24, 32) == 0, "frozen menu ignored"); }
    { Fixture f; f.menu(); f.put(Fixture::proc + 0x60, 10, 1);
      require(f.click(24, 32) == 0, "scrolling menus not guessed"); }
    { Fixture f; f.menu(); f.click(24, 64); f.put(Fixture::proc + 0x30, 0x08590100, 4);
      require(f.tick() == 0, "reused process with different menu invalidates click"); }
    { Fixture f; f.menu(); f.click(24, 64); f.put(0x02002100 + 0x3d, 2, 1);
      require(f.tick() == 0 && f.tick() == 0, "action disabled while navigating cancels click"); }
    { Fixture f; f.menu(); require(f.click(0, 0, true) == 2, "right click cancels action menu"); }
    { Fixture f; f.targets();
      require(f.click(56, 72) == 0, "presel target needs explicit preview click");
      require(f.tick() == 0, "preview never auto-confirms");
      ++f.input.frame; require(f.click(56, 72) == 1, "second click confirms selected forecast target"); }
    { Fixture f; f.menu(); require(f.click(24, 32) == 1, "weapon menu A pulse");
      f.targets();
      require(f.pointer.update(f.input) == 0, "menu A cannot leak into target selection in same frame");
      require(f.tick() == 0 && f.tick() == 0, "target stage never auto-confirms after weapon menu");
      ++f.input.frame; require(f.click(56, 72) == 0, "first target click after weapon is preview only");
      ++f.input.frame; require(f.click(56, 72) == 1, "fresh target confirmation after weapon"); }
    { Fixture f; f.menu(); require(f.click(24, 32) == 1, "first menu A pulse");
      f.put(Fixture::proc + 0x30, 0x08590100, 4);
      require(f.pointer.update(f.input) == 0, "A cannot leak to another menu in reused process"); }
    { Fixture f; f.targets(); f.click(56, 72);
      f.input.reset = true; require(f.tick() == 0, "focus/menu/state reset disarms preview");
      f.input.reset = false; ++f.input.frame;
      require(f.click(56, 72) == 0, "new preview required after reset"); }
    { Fixture f; f.targets();
      require(f.click(88, 72) == 0x40, "click another valid target navigates target ring");
      f.put(Fixture::proc + 0x30, 0x0200300c, 4);
      require(f.tick() == 0 && f.tick() == 0 && f.tick() == 0,
              "new target preview never automatically starts action");
      ++f.input.frame; require(f.click(88, 72) == 1, "second click confirms forecast target"); }
    { Fixture f; f.targets(); require(f.click(120, 72) == 0, "non-target tile ignored"); }
    { Fixture f; f.targets(); f.put(0x02003000 + 4, 0x03000000, 4);
      require(f.click(88, 72) == 0, "invalid target list ignored"); }
    { Fixture f; f.targets(); f.put(Fixture::proc + 0x34, 0x40, 1);
      require(f.click(56, 72) == 0, "frozen target selection ignored"); }
    { Fixture f; f.targets(); f.click(88, 72);
      f.put(Fixture::proc + 0x28, 1, 1); f.put(Fixture::proc + 0x18, 0x02004000, 4);
      f.put(0x02004000, 0x0859a548, 4);
      require(f.tick() == 0, "target camera animation releases buttons");
      f.put(Fixture::proc + 0x28, 0, 1); f.put(Fixture::proc + 0x18, 0, 4);
      require(f.tick() == 0 && f.tick() == 0x40, "target navigation survives known camera wait"); }
    { Fixture f; f.targets(); f.click(88, 72); f.put(Fixture::proc + 0x2c, 0x0859d478, 4);
      require(f.tick() == 0 && f.tick() == 0, "changed target action cancels pending click"); }
    { Fixture f; f.targets(); require(f.click(0, 0, true) == 2, "right click cancels targeting"); }
    { Fixture f; f.guide(); const auto before = f.ram;
      require(f.click(30, 76) == 0x80, "Guide category click uses native Down despite suspended map");
      require(f.ram == before, "Guide input never writes guide memory");
      f.put(0x020201b1, 2, 1);
      require(f.next_pulse() == 0, "Guide category selection does not automatically enter article list"); }
    { Fixture f; f.guide();
      require(f.click(100, 60) == 1, "Guide right-pane click first enters native entry list");
      f.put(0x020201b7, 1, 1);
      require(f.next_pulse() == 0x80, "Guide then moves to clicked article");
      f.put(0x020201b3, 1, 1);
      require(f.next_pulse() == 1, "Guide explicitly clicked article opens with native A");
      f.put(0x020201b7, 2, 1);
      require(f.next_pulse() == 0, "Guide opening cannot inherit a text-page advance"); }
    { Fixture f; f.guide(2);
      require(f.click(30, 76) == 2, "Guide category click leaves detail view natively");
      f.put(0x020201b7, 1, 1); require(f.next_pulse() == 2, "Guide category click leaves entry focus");
      f.put(0x020201b7, 0, 1); require(f.next_pulse() == 0x80, "Guide then selects requested category"); }
    { Fixture f; f.guide(); f.input.wheel = -3; f.input.wheel_x = 40; f.input.wheel_y = 80;
      require(f.pointer.update(f.input) == 0x80, "Guide wheel scrolls categories");
      f.input.wheel = 0; f.put(0x020201b1, 3, 1);
      require(f.next_pulse() == 0, "Guide wheel category navigation never confirms"); }
    { Fixture f; f.guide(1); f.put(0x020201b4, 2, 1); f.put(0x020201b3, 3, 1);
      require(f.click(100, 44) == 0x40, "Guide entry click uses visible scroll offset"); }
    { Fixture f; f.guide(1); f.input.wheel = -3; f.input.wheel_x = 235; f.input.wheel_y = 80;
      require(f.pointer.update(f.input) == 0x80, "Guide scrollbar wheel moves entries");
      f.input.wheel = 0; f.put(0x020201b3, 3, 1);
      require(f.next_pulse() == 0, "Guide wheel never opens article"); }
    { Fixture f; f.guide(2); f.input.wheel = -3; f.input.wheel_x = 160; f.input.wheel_y = 80;
      require(f.pointer.update(f.input) == 0x80, "Guide text wheel scrolls individual lines");
      f.input.wheel = 0; f.put(0x020201b6, 3, 1);
      require(f.next_pulse() == 0, "Guide text wheel stops at requested line"); }
    { Fixture f; f.guide(2); f.put(0x020201b6, 8, 1);
      f.input.wheel = -8; f.input.wheel_x = 160; f.input.wheel_y = 80;
      require(f.pointer.update(f.input) == 0, "Guide text wheel clamps at final line"); }
    { Fixture f; f.guide(2);
      require(f.click(160, 80) == 1, "Guide text click advances a native page"); }
    { Fixture f; f.guide(2);
      require(f.click(144, 60) == 0x40, "Guide upper text arrow scrolls up"); }
    { Fixture f; f.guide(); require(f.click(190, 8) == 4, "Guide Sort badge supplies Select"); }
    { Fixture f; f.guide(1); require(f.click(190, 8) == 0, "Guide hidden Sort badge does nothing"); }
    { Fixture f; f.guide(); require(f.click(190, 20) == 2, "Guide Back badge supplies B"); }
    { Fixture f; f.guide(2); require(f.click(100, 80, true) == 2, "Guide right click returns natively"); }
    { Fixture f; f.guide(); require(f.click(78, 60) == 0, "Guide pane divider is not clickable"); }
    { Fixture f; f.guide(1); f.put(0x020201c6, 1, 1);
      require(f.click(100, 76) == 0, "Guide refuses nonexistent visible article"); }
    { Fixture f; f.guide(); f.put(0x020201c5, 21, 1);
      require(f.click(30, 76) == 0, "Guide rejects corrupt category count"); }
    { Fixture f; f.guide(); f.put(Fixture::proc, 0x08b12d1c, 4);
      require(f.click(30, 76) == 0x80, "Guide also recognizes alternate world-map entry script"); }
    { Fixture f; f.guide(1); require(f.click(100, 76) == 0x80, "Guide queues article navigation");
      const unsigned child = 0x02001800;
      f.put(Fixture::proc + 0x18, child, 4); f.put(Fixture::proc + 0x28, 1, 1);
      f.put(child, 0x08b12b9c, 4);
      require(f.next_pulse() == 0, "Guide waits through native redraw lock");
      f.put(Fixture::proc + 0x18, 0, 4); f.put(Fixture::proc + 0x28, 0, 1);
      f.put(0x020201b3, 2, 1);
      require(f.next_pulse() == 1, "Guide queued article resumes after redraw"); }
    { Fixture f; f.guide(1); require(f.click(100, 76) == 0x80, "Guide starts another bounded plan");
      f.put(0x020201b8, 1, 1);
      require(f.next_pulse() == 0, "Guide sort changes cancel stale plans"); }
    { Fixture f; f.guide(1); require(f.click(100, 76) == 0x80, "Guide begins entry plan");
      f.input.physical_keys = 0x3fe;
      require(f.next_pulse() == 0, "Guide gives physical controls priority"); }
    { Fixture f; f.trade(); const auto before = f.ram;
      require(f.click(32, 80) == 1, "select source item in Trade");
      require(f.ram == before, "trade input never writes inventory"); }
    { Fixture f; f.trade(true);
      require(f.click(160, 112) == 0x10, "Trade chooses right inventory column");
      f.put(Fixture::proc + 0x41, 1, 1); require(f.tick() == 0 && f.tick() == 0x80, "Trade down pulse");
      f.put(Fixture::proc + 0x42, 1, 1); require(f.tick() == 0 && f.tick() == 0x80, "Trade second down pulse");
      f.put(Fixture::proc + 0x42, 2, 1); require(f.tick() == 0 && f.tick() == 1 && f.tick() == 0,
                                                      "Trade confirms destination once"); }
    { Fixture f; f.trade(true); f.put(Fixture::proc + 0x34 + 6 + 3, 1, 1);
      require(f.click(160, 128) == 0x10, "Trade permits game-enabled empty destination slot"); }
    { Fixture f; f.trade(); require(f.click(120, 80) == 0 && f.click(32, 144) == 0,
                                   "Trade gap and empty unselectable source ignored"); }
    { Fixture f; f.trade(); f.put(Fixture::proc + 0x48, 2, 1);
      require(f.click(32, 80) == 0, "scripted Trade tutorial keeps original controls"); }
    { Fixture f; f.trade(); require(f.click(0, 0, true) == 2, "right click cancels Trade stage"); }
    { Fixture f; f.input.hover_x = 239; f.input.hover_y = 80;
      require(f.pointer.update(f.input) == 0, "edge hover never emits direction or A");
      int x = 0, y = 0;
      require(f.pointer.camera_position(x, y) && x == 4 && y == 0, "camera pans immediately without moving selection");
      require(f.pointer.update(f.input) == 0 && f.pointer.camera_position(x, y) && x == 4,
              "same-frame camera pump does not double pan");
      f.input.hover_x = 120;
      require(f.pointer.update(f.input) == 0 && f.tick() == 0, "leaving edge releases immediately");
      require(f.pointer.camera_position(x, y) && x == 4, "camera stays detached without snapping to cursor"); }
    { Fixture f; f.input.hover_x = 0; f.input.hover_y = 0;
      f.pointer.update(f.input); f.input.frame += 12;
      int x = 0, y = 0;
      require(f.pointer.update(f.input) == 0 && f.pointer.camera_position(x, y) && x == 0 && y == 0,
              "top-left camera clamps to map limits"); }
    { Fixture f; f.input.hover_x = 239; f.input.hover_y = 159;
      f.pointer.update(f.input); f.input.frame += 12;
      int x = 0, y = 0;
      require(f.pointer.update(f.input) == 0 && f.pointer.camera_position(x, y) && x == 4 && y == 4,
              "bottom-right corner pans camera diagonally without keys"); }
    { Fixture f; f.cursor(0, 0); f.input.hover_x = 0; f.input.hover_y = 0;
      f.pointer.update(f.input); f.input.frame += 100;
      require(f.pointer.update(f.input) == 0, "edge respects map boundaries"); }
    { Fixture f; f.input.hover_x = 240; f.input.hover_y = 80;
      f.pointer.update(f.input); f.input.frame += 100;
      require(f.pointer.update(f.input) == 0, "black bars/outside image cannot scroll"); }
    { Fixture f; f.input.hover_x = 239; f.input.hover_y = 80; f.input.pointer_down = true;
      f.pointer.update(f.input); f.input.frame += 100;
      require(f.pointer.update(f.input) == 0, "held mouse suppresses edge pan"); }
    { Fixture f; f.menu(); f.input.hover_x = 239; f.input.hover_y = 80;
      f.pointer.update(f.input); f.input.frame += 100;
      require(f.pointer.update(f.input) == 0, "menus cannot edge-scroll map underneath"); }
    { Fixture f; f.targets(); f.input.hover_x = 239; f.input.hover_y = 80;
      f.pointer.update(f.input); f.input.frame += 100;
      require(f.pointer.update(f.input) == 0, "target preview cannot edge-scroll"); }
    { Fixture f; f.input.hover_x = 239; f.input.hover_y = 80;
      require(f.click(56, 72) == 1, "explicit click wins over edge hover"); }
    { Fixture f; f.input.hover_x = 239; f.input.hover_y = 80;
      f.pointer.update(f.input); f.input.frame += 12;
      int x = 0, y = 0;
      require(f.pointer.update(f.input) == 0 && f.pointer.camera_position(x, y), "start independent camera pan");
      f.input.physical_keys = 0x3ef; require(f.tick() == 0, "manual controls interrupt edge pan");
      require(!f.pointer.camera_position(x, y), "manual input restores native camera");
      f.input.physical_keys = 0x3ff; require(f.tick() == 0, "hover pan never synthesizes GBA keys"); }
    { Fixture f; f.input.hover_x = 224; f.input.hover_y = 80;
      int x = 0, y = 0; const auto before = f.ram;
      f.pointer.update(f.input);
      require(f.pointer.camera_position(x, y) && x == 1 && f.ram == before,
              "progressive camera speed preserves read-only input adapter"); }
    { Fixture f; f.put(Fixture::proc + 0xc, 0x080c6355, 4);
      require(f.click(100, 100) == 8 && f.tick() == 0 && f.tick() == 0, "title click is one Start pulse");
      f.menu(); require(f.tick() == 0, "intro click never confirms subsequent menu"); }
    { Fixture f; f.put(Fixture::proc + 0xc, 0x080c6705, 4);
      require(f.click(100, 100) == 8, "opening animation click skips through original handler"); }
    { Fixture f; f.put(Fixture::proc, 0x08a200b8, 4); f.put(Fixture::proc + 0xc, 0x080a9025, 4);
      f.put(Fixture::proc + 0x30, 0x31, 1); f.put(Fixture::proc + 0x31, 3, 1);
      require(f.click(80, 85) == 0x80, "main menu hit testing uses native dynamic row count");
      f.put(Fixture::proc + 0x2b, 1, 1); require(f.tick() == 0 && f.tick() == 0x80, "navigate main menu");
      f.put(Fixture::proc + 0x2b, 2, 1); require(f.tick() == 0 && f.tick() == 1, "confirm main menu row"); }
    { Fixture f; f.put(Fixture::proc + 0xc, 0x080ac289, 4);
      require(f.click(50, 120) == 0x80, "difficulty menu hit testing");
      f.put(Fixture::proc + 0x30, 2, 1); require(f.tick() == 0 && f.tick() == 1, "confirm difficulty row"); }
    { Fixture f; f.put(Fixture::proc, 0x08a200b8, 4); f.put(Fixture::proc + 0xc, 0x080a9495, 4);
      f.put(Fixture::proc + 0x2f, 220, 1); f.put(Fixture::proc + 0x42, 2, 2);
      require(f.click(80, 110) == 0x80, "save slot click navigates to slot");
      f.put(Fixture::proc + 0x2c, 2, 1); require(f.tick() == 0 && f.tick() == 1, "confirm native save slot");
      f.put(Fixture::proc + 0x36, 2, 1); require(f.tick() == 0, "slot click cannot confirm overwrite dialog");
      ++f.input.frame; require(f.click(60, 144) == 0x20, "explicit confirmation dialog chooses left action");
      f.put(Fixture::proc + 0x36, 1, 1); require(f.tick() == 0 && f.tick() == 1, "confirm explicit dialog action"); }
    { Fixture f; f.menu(); f.put(0x02024cc4, 1, 2);
      require(f.click(20, 28) == 0, "menu confirmation waits for actual native A release");
      f.put(0x02024cc4, 0, 2); require(f.next_pulse() == 1, "pending click confirms after native key-up"); }
    for (int row = 0; row < 13; ++row) {
      Fixture f; f.config(); const int head = std::min(row, 7);
      const int x[] = {168,146,208,138,185,185,138,138,138,138,138,138,160};
      const unsigned shift[] = {17,7,5,1,2,19,16,15,4,14,8,9,10};
      const unsigned last[] = {2,1,3,1,2,2,1,1,1,1,1,1,3};
      f.put(0x020201b2, row, 2); f.put(0x020201b4, head, 2);
      const auto before = f.ram;
      require(f.click(x[row], 44 + (row - head) * 16) == 0x10 && f.ram == before,
              "every Options row selects its final visible value through read-only native input");
      f.put(0x0202bd30, last[row] << shift[row], 4);
      require(f.next_pulse() == 0, "every Options row stops exactly at the chosen value");
    }
    { Fixture f; f.config(); const auto before = f.ram;
      require(f.click(146, 45) == 0x10, "Options clicks choose exact animation value through native input");
      f.put(0x0202bd32, 6, 1); require(f.next_pulse() == 0x10, "animation bit order is remapped");
      f.put(0x0202bd32, 2, 1); require(f.tick() == 0 && f.tick() == 0, "stop at chosen option without A");
      f.ram = before; ++f.input.frame;
      require(f.click(145, 61) == 0x80, "Options row is selected before changing its value");
      f.put(0x020201b2, 1, 2); require(f.next_pulse() == 0x10, "change game speed only after native row navigation");
      f.put(0x0202bd30, 0x80, 1); require(f.tick() == 0 && f.tick() == 0, "requested value never repeats or auto-confirms"); }
    { Fixture f; f.config(); f.put(0x020201b2, 7, 2); f.put(0x020201b4, 6, 2);
      require(f.click(138, 109) == 0x80, "scrolled Options row uses live head index");
      f.put(Fixture::proc + 0x30, 4, 2); require(f.tick() == 0 && f.tick() == 0, "wait for native option scrolling");
      f.put(Fixture::proc + 0x30, 0, 2); f.put(0x020201b2, 10, 2);
      require(f.next_pulse() == 0x10, "scrolled Music selector waits for settling then emits native change event"); }
    { Fixture f; f.config(); f.put(0x0202bd32, 4, 1);
      require(f.click(198, 45) == 1, "explicit Solo prompt opens native per-unit animation menu");
      f.unit_list(); require(f.pointer.update(f.input) == 0, "Solo click cannot deploy or toggle an arriving unit list"); }
    { Fixture f; f.config(); const auto before = f.ram;
      require(f.click(30, 45) == 0 && f.ram == before, "Options label only selects and never writes RAM");
      ++f.input.frame; require(f.click(234, 120) == 0x80, "Options scrollbar lower half steps down");
      f.input.physical_keys = 0x3df; require(f.tick() == 0, "manual input cancels pending option changes"); }
    { Fixture f; f.config(); require(f.click(138, 45) == 0, "gap between option selectors never changes value");
      ++f.input.frame; require(f.click(20, 145) == 0, "help text is not a settings row");
      ++f.input.frame; require(f.click(-1, 45, true) == 0, "outside-image right click is ignored");
      ++f.input.frame; require(f.click(20, 20, true) == 2 && f.tick() == 0, "right click exits Options once"); }
    { Fixture f; f.config(); f.put(0x020201bc, 0xffff, 2);
      require(f.click(145, 45) == 0, "malformed option count blocked"); }
    { Fixture f; f.unit_list(); require(f.click(150, 90) == 0x80, "Unit data-row click navigates natively");
      f.put(Fixture::proc + 0x30, 2, 1); f.put(Fixture::proc + 0x2c, 2, 1);
      require(f.next_pulse() == 0x100 && f.tick() == 0, "Unit data-row opens statistics once");
      f.put(Fixture::proc, 0x08a009d8, 4); f.put(Fixture::proc + 0xc, 0x0808873d, 4);
      f.put(0x02003bfd, 3, 1); require(f.pointer.update(f.input) == 0, "R does not leak into arriving stats help");
      ++f.input.frame; require(f.click(219, 10) == 0x10, "statistics right page arrow");
      ++f.input.frame; require(f.click(30, 20, true) == 0 && f.next_pulse() == 2, "statistics right-click waits for prior page command"); }
    { Fixture f; f.unit_list(); require(f.click(35, 60) == 1, "Unit name focuses the native map unit");
      require(f.tick() == 0 && f.tick() == 0, "unit focus never repeats"); }
    { Fixture f; f.unit_list(); f.put(Fixture::proc + 0x3e, 32, 2); f.put(Fixture::proc + 0x30, 2, 1);
      require(f.click(145, 90) == 0x80, "Unit row target includes scroll offset");
      f.put(0x02002010, 0x02003f00, 4);
      require(f.tick() == 0 && f.tick() == 0, "reordered/stale unit target is discarded"); }
    { Fixture f; f.unit_list(); require(f.click(155, 45) == 0x40, "column click enters native sort header");
      f.put(Fixture::proc + 0x29, 3, 1); require(f.next_pulse() == 0x10, "navigate sort columns");
      f.put(Fixture::proc + 0x2d, 3, 1); require(f.next_pulse() == 1, "sort only selected header once");
      require(f.tick() == 0 && f.tick() == 0, "sort direction not toggled repeatedly"); }
    { Fixture f; f.unit_list(); require(f.click(232, 45) == 0x10, "Unit right page arrow");
      f.put(Fixture::proc + 0x2f, 2, 1); require(f.pointer.update(f.input) == 0, "page change discards old pulse in same frame"); }
    { Fixture f; f.unit_list(); f.put(Fixture::proc + 0x29, 1, 1);
      require(f.click(145, 90) == 0, "ignore row clicks during unit scrolling");
      ++f.input.frame; require(f.click(40, 30, true) == 0, "defer list cancel until scrolling completes");
      f.put(Fixture::proc + 0x29, 0, 1); require(f.tick() == 0 && f.tick() == 2, "deferred cancel waits for settling and remains one B"); }
    { Fixture f; f.unit_list(); f.put(Fixture::proc + 0x2b, 1, 1);
      require(f.click(145, 90) == 0, "help overlay blocks list selection");
      ++f.input.frame; require(f.click(20, 20, true) == 2, "right click dismisses unit-list help"); }
    { Fixture f; f.put(Fixture::proc, 0x08a00ad0, 4); f.put(Fixture::proc + 0xc, 0x08089089, 4);
      require(f.click(100, 100) == 0, "help boxes never forward primary clicks");
      ++f.input.frame; require(f.click(100, 100, true) == 2, "right click dismisses native help"); }
    { Fixture f; f.put(Fixture::proc, 0x08a01b54, 4); f.put(Fixture::proc + 0xc, 0x0808e609, 4);
      require(f.click(80, 45) == 0x10, "Status enemy faction selector");
      f.put(Fixture::proc + 0x2e, 1, 1); require(f.tick() == 0 && f.tick() == 0, "faction selection never focuses or exits automatically");
      ++f.input.frame; require(f.click(160, 80) == 1, "Status leader focus requires separate explicit click"); }
    { Fixture f; f.config(); f.input.wheel = -3; f.input.wheel_x = 230; f.input.wheel_y = 70;
      require(f.pointer.update(f.input) == 0x80, "wheel over Options scrollbar navigates rows");
      f.input.wheel = 0; f.put(0x020201b2, 3, 2);
      require(f.next_pulse() == 0, "wheel never changes option values or confirms"); }
    { Fixture f; f.config(); f.input.wheel = -2; f.input.wheel_x = 30; f.input.wheel_y = 70;
      require(f.pointer.update(f.input) == 0x80, "wheel works anywhere over the Options list");
      f.pointer.update(f.input); f.input.wheel = 0; f.put(0x020201b2, 4, 2);
      require(f.next_pulse() == 0, "multiple wheel events accumulate within the same frame"); }
    { Fixture f; f.input.wheel = -8; f.input.wheel_x = 230; f.input.wheel_y = 70;
      require(f.pointer.update(f.input) == 0, "map ignores wheel rather than selecting/moving units");
      f.config(); ++f.input.frame; f.input.wheel_x = -1;
      require(f.pointer.update(f.input) == 0, "out-of-image wheel ignored"); }
    { Fixture f; f.unit_list(); f.input.wheel = -2; f.input.wheel_x = 230; f.input.wheel_y = 100;
      require(f.pointer.update(f.input) == 0x80, "Unit list wheel selects rows without activation");
      f.input.wheel = 0; f.put(Fixture::proc + 0x30, 2, 1);
      require(f.next_pulse() == 0, "Unit wheel never opens a stat screen or focuses map"); }
    { Fixture f; f.put(Fixture::proc, 0x08a200b8, 4); f.put(Fixture::proc + 0xc, 0x080a9b91, 4);
      f.put(Fixture::proc + 0x32, 7, 1); f.put(Fixture::proc + 0x33, 3, 1); f.put(Fixture::proc + 0x46, 220, 2);
      require(f.click(80, 90) == 0x80, "Extras list uses its own layout and dynamic count");
      f.put(Fixture::proc + 0x34, 2, 1); require(f.next_pulse() == 1, "Extras selection confirms native entry once");
      f.put(Fixture::proc + 0xc, 0x080a9025, 4); require(f.pointer.update(f.input) == 0, "Extras cannot click through its transition"); }
    { Fixture f; f.put(Fixture::proc, 0x08a21338, 4); f.put(Fixture::proc + 0xc, 0x080af8a1, 4);
      f.put(Fixture::proc + 0x36, 40, 1);
      require(f.click(165, 85) == 0x80, "Sound Room grid navigates rows natively");
      f.put(Fixture::proc + 0x35, 6, 1); require(f.next_pulse() == 1, "Sound Room click plays selected song via native checks");
      ++f.input.frame; f.input.wheel = -1; f.input.wheel_x = 230; f.input.wheel_y = 90;
      f.pointer.update(f.input); f.input.wheel = 0;
      require(f.next_pulse() == 0x80, "Sound Room wheel steps a grid row without playing"); }
    { Fixture f; f.put(Fixture::proc, 0x08591340, 4); f.put(Fixture::proc + 0xc, 0x08006b4d, 4);
      require(f.click(100, 100) == 1 && f.next_pulse() == 0, "dialogue click speeds up printing once");
      ++f.input.frame; require(f.click(100, 100, true) == 8, "dialogue right click uses native Start skip"); }
    { Fixture f; f.put(Fixture::proc, 0x08591340, 4); f.put(Fixture::proc + 0xc, 0x08006b4d, 4);
      f.put(0x030000c8, 12, 2);
      require(f.click(100, 100) == 0, "no-fast dialogue is not accelerated");
      ++f.input.frame; require(f.click(100, 100, true) == 0, "no-skip dialogue cannot be skipped"); }
    { Fixture f; f.put(Fixture::proc, 0x08591340, 4); f.put(Fixture::proc + 0xc, 0x08006b4d, 4);
      const auto child = 0x02001800u;
      f.put(Fixture::proc + 0x18, child, 4); f.put(child, 0x085913f0, 4); f.put(child + 0xc, 0x08007c4d, 4);
      require(f.click(100, 100) == 1, "dialogue wait overrides background skip listener");
      f.put(child, 0x085914b0, 4); f.put(child + 0xc, 0x08007e9d, 4);
      f.put(child + 0x2a, 1, 2); f.put(child + 0x2c, 80, 2); f.put(child + 0x2e, 120, 2);
      require(f.pointer.update(f.input) == 0 && f.next_pulse() == 0, "advance click never confirms arriving Yes/No choice");
      ++f.input.frame; require(f.click(125, 125) == 0x10, "dialogue answer hit-test selects No");
      f.put(child + 0x2a, 2, 2); require(f.next_pulse() == 1, "explicit answer click confirms chosen answer only"); }
    { Fixture f; f.put(Fixture::proc, 0x08591ac0, 4); f.put(Fixture::proc + 0xc, 0x0800ce4d, 4);
      f.put(Fixture::proc + 0x3c, 2, 2); f.put(Fixture::proc + 0x41, 1, 1);
      require(f.click(100, 100) == 8, "chapter cinematic click uses native Start");
      f.put(Fixture::proc + 0x3c, 6, 2); require(f.next_pulse() == 0, "skip cannot repeat while scene is skipping"); }
    { Fixture f; f.put(Fixture::proc, 0x08591ac0, 4); f.put(Fixture::proc + 0xc, 0x0800ce4d, 4);
      f.put(Fixture::proc + 0x3c, 18, 2); f.put(Fixture::proc + 0x41, 1, 1);
      require(f.click(100, 100) == 0, "unskippable cinematic is respected");
      f.put(Fixture::proc + 0x3c, 2, 2); f.put(Fixture::proc + 0x41, 2, 1);
      ++f.input.frame; require(f.click(100, 100) == 0, "gameplay event cannot be mistaken for opening cinematic"); }
    { Fixture f; f.put(Fixture::proc, 0x08591ac0, 4); f.put(Fixture::proc + 0xc, 0x0800ce4d, 4);
      f.put(Fixture::proc + 0x3c, 2, 2); f.put(Fixture::proc + 0x41, 1, 1);
      f.put(Fixture::proc + 0x18, 0x02001800, 4); f.put(0x02001800, 0x08591f18, 4);
      require(f.click(100, 100) == 0, "scripted combat blocks cinematic skip input"); }
    { Fixture f; f.put(Fixture::proc, 0x08a01e64, 4); f.put(Fixture::proc + 0xc, 0x0808f8b5, 4);
      require(f.click(100, 100) == 1, "CG narration click advances text rather than skips chapter");
      f.put(0x0203e874, 64u << 10, 4); ++f.input.frame;
      require(f.click(100, 100, true) == 0, "CG narration no-skip flag is respected"); }
    { Fixture f; f.put(Fixture::proc, 0x08aa21bc, 4); f.put(Fixture::proc + 0xc, 0, 4);
      require(f.click(100, 80) == 8, "new-game subtitle script gets native Start between animation callbacks");
      f.put(Fixture::proc, 0x0859aad8, 4); f.put(Fixture::proc + 0xc, 0x0801c941, 4);
      require(f.pointer.update(f.input) == 0, "subtitle skip never carries into gameplay"); }
    int value = -1;
    require(sacredstones::map_pointer_get("sacredstones.mouse_map", &value) && value == 0,
            "experimental menu starts disabled");
    require(sacredstones::map_pointer_set("sacredstones.mouse_map", 1) &&
            sacredstones::map_pointer_get("sacredstones.mouse_map", &value) && value == 1,
            "menu toggles feature");
    require(!sacredstones::map_pointer_get("other", &value), "unrelated menu key ignored");
    if (argc == 2) {
        std::ifstream file(argv[1], std::ios::binary);
        require(bool(file), "open external savestate fixture");
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), {});
        require(bytes.size() >= 52 && bytes.size() <= 4 * 1024 * 1024,
                "bounded snapshot container");
        gbarecomp::debug::SnapshotReader reader(bytes.data(), bytes.size());
        char magic[4], sha1[41]{};
        reader.bytes(magic, 4);
        require(std::memcmp(magic, "GBAS", 4) == 0, "snapshot magic");
        const auto version = reader.u32();
        require(version == 1 || version == 2, "snapshot version");
        reader.bytes(sha1, 40);
        require(std::strcmp(sha1, "c25b145e37456171ada4b0d440bf88a19f4d509f") == 0,
                "FE8 USA snapshot identity");
        const auto sections = reader.u32();
        require(sections <= 32, "bounded snapshot sections");
        Fixture f;
        bool found = false;
        for (unsigned i = 0; i < sections; ++i) {
            char tag[4]; reader.bytes(tag, 4);
            const auto size = reader.u32();
            require(reader.ok() && size <= reader.remaining(), "snapshot section bounds");
            if (std::memcmp(tag, "BUS0", 4) == 0) {
                require(size >= f.ram.size(), "snapshot RAM size");
                reader.bytes(f.ram.data(), f.ram.size());
                reader.skip(size - f.ram.size()); found = true;
            } else reader.skip(size);
        }
        require(reader.ok() && found, "external RAM fixture loaded");
        auto s16 = [&](unsigned offset) { return int(f.ram[offset] | (f.ram[offset + 1] << 8)); };
        const int x = s16(0x2bcb0 + 0x14) * 16 - s16(0x2bcb0 + 0x0c) + 8;
        const int y = s16(0x2bcb0 + 0x16) * 16 - s16(0x2bcb0 + 0x0e) + 8;
        const auto before = f.ram;
        require(f.click(x, y) == 1, "recognize live selectable cursor in external snapshot");
        require(f.ram == before, "external snapshot RAM unchanged");
        std::puts("PASS: live FE8 player-process/camera/cursor from external savestate");
    }
    std::puts("PASS: map pointer policy, movement, cancellation and safety tests");
}

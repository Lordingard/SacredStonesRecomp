#include "runtime.h"
#include "map_pointer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "runtime_bus_bridge.h"
#include "gba_bus.h"

namespace {
unsigned samples = 0;
int initial_cursor_x = 0, initial_cursor_y = 0, initial_camera_x = 0, held_camera_x = 0;
bool failed = false, panned = false, held = false, restored = false;
bool vertical = false, forward = true;
int required_pan = 0;
bool cursor_mode = false, instant_cursor = false;
int click_x = 0, click_y = 0;
int cursor_camera_x = 0, cursor_camera_y = 0, settled_x = 0, settled_y = 0;
bool camera_stable = true;
std::uint64_t cursor_frame = 0;
const char* menu_mode = nullptr;
int menu_stage = 0, menu_before = 0;
unsigned intro_advances = 0, intro_skips = 0;
unsigned trade_states_seen = 0;
std::uint32_t world_event = 0;
bool world_start_sent = false;
unsigned guide_entry_target = 0;
std::uint64_t intro_clicked = 0;
std::uint64_t menu_frame = ~std::uint64_t{0};
std::uint64_t save_before = 0;
std::uint64_t save_checksum() {
    const auto* bus = gbarecomp::active_bus();
    if (!bus) return 0;
    std::uint64_t value = 14695981039346656037ull;
    for (std::size_t i = 0; i < bus->save().sram_size(); ++i)
        value = (value ^ bus->save().sram_read(static_cast<std::uint32_t>(i))) * 1099511628211ull;
    return value;
}
std::uint16_t menu_probe(const gbarecomp::GamePointerInput& source) {
    if (!source.ewram || source.ewram_size < 256 * 1024) return 0;
    if (source.reset) return sacredstones::map_pointer_input(source);
    auto read = [&](std::uint32_t a, unsigned size = 1) {
        std::uint32_t v = 0;
        if (a < 0x02000000 || a - 0x02000000 + size > source.ewram_size) return v;
        for (unsigned i = 0; i < size; ++i) v |= std::uint32_t(source.ewram[a - 0x02000000 + i]) << (8 * i);
        return v;
    };
    auto find = [&](std::uint32_t callback, std::uint32_t script_match = 0) {
        std::uint32_t stack[128]{}, seen[64]{}; unsigned count = 0, visited = 0;
        for (unsigned i = 0; i < 8; ++i) stack[count++] = read(0x02026a70 + i * 4, 4);
        while (count && visited < 64) {
            const auto p = stack[--count];
            if (p < 0x02000000 || p > 0x0203ffd4 || (p & 3)) continue;
            bool duplicate = false;
            for (unsigned i = 0; i < visited; ++i) duplicate |= seen[i] == p;
            if (duplicate) continue;
            seen[visited++] = p;
            if (((callback && (read(p + 0xc, 4) & ~1u) == callback) ||
                 (script_match && read(p, 4) == script_match)) && !read(p + 0x28) &&
                !read(p + 0x24, 2) && !(read(p + 0x27) & 9)) return p;
            if (count + 2 <= 128) { stack[count++] = read(p + 0x20, 4); stack[count++] = read(p + 0x18, 4); }
        }
        return std::uint32_t{0};
    };
    auto input = source;
    input.reset = false; input.primary = input.secondary = false;
    input.physical_keys = 0x3ff; input.hover_x = input.hover_y = -1;
    const bool fresh = menu_frame != source.frame;
    menu_frame = source.frame;
    auto click = [&](int x, int y, bool right = false) { input.x = x; input.y = y; input.primary = !right; input.secondary = right; };
    const bool wheel_mode = std::strcmp(menu_mode, "wheel") == 0;
    const bool intro_mode = std::strcmp(menu_mode, "intro") == 0;
    const bool tutorial = std::strcmp(menu_mode, "tutorial") == 0;
    const bool trade_tutorial = std::strncmp(menu_mode, "trade-tutorial", 14) == 0;
    const bool trade_text = std::strcmp(menu_mode, "trade-tutorial-text") == 0;
    const bool world_mode = std::strncmp(menu_mode, "world", 5) == 0;
    const bool world_skip = std::strcmp(menu_mode, "world-skip") == 0;
    const bool guide_mode = std::strncmp(menu_mode, "guide", 5) == 0;
    const unsigned guide_category = std::strcmp(menu_mode, "guide-list") == 0 ? 0u : 1u;
    const bool chapter_save = std::strncmp(menu_mode, "chapter-save", 12) == 0;
    const bool save_no = std::strcmp(menu_mode, "chapter-save-no") == 0;
    const bool save_back = std::strcmp(menu_mode, "chapter-save-back") == 0;
    const unsigned save_slot = std::strcmp(menu_mode, "chapter-save-overwrite") == 0 ? 0 : 2;
    const bool options = std::strcmp(menu_mode, "options") == 0 || wheel_mode;
    const bool status = std::strcmp(menu_mode, "status") == 0;
    const auto list = find(0x08091aec), config = find(0x080b220c), stats = find(0x0808873c), chapter = find(0x0808e608);
    const int old_stage = menu_stage;
    if (fresh) {
        ++samples;
        if (chapter_save) {
            const auto p = find(0x080a9494, 0x08a203a8);
            if (!menu_stage && p) {
                save_before = save_checksum(); menu_before = static_cast<int>(p);
                click(80, 40 + 32 * save_slot); menu_stage = 1;
            } else if (menu_stage == 1 && p && read(p + 0x36) != 0 && read(p + 0x2c) == save_slot) {
                intro_clicked = source.frame; menu_stage = 2;
            } else if (menu_stage == 2 && source.frame >= intro_clicked + 12) {
                if (!p || read(p + 0x36) == 0) { failed = true; menu_stage = -1; }
                else { click(save_no ? 110 : 60, 144, save_back); menu_stage = 3; intro_clicked = source.frame; }
            } else if (menu_stage == 3) {
                const auto saved = static_cast<std::uint32_t>(menu_before);
                if (!save_no && !save_back && (read(saved + 0xc, 4) & ~1u) == 0x080a96ec &&
                    read(saved + 0x29) >= 9 && read(saved + 0x37 + save_slot) != 0xff) {
                    failed = save_before == save_checksum(); menu_stage = failed ? -1 : 99;
                    std::printf("chapter_save native_slot=%u chapter=%u sram_changed=%d\n", save_slot, read(saved + 0x37 + save_slot), !failed);
                } else if (save_back && p && read(p + 0x36) == 0) {
                    failed = save_before != save_checksum(); menu_stage = failed ? -1 : 99;
                } else if (save_no && !p && source.frame >= intro_clicked + 16) {
                    failed = save_before != save_checksum(); menu_stage = failed ? -1 : 99;
                }
            }
        } else if (guide_mode) {
            const auto guide = find(0x080cefd4);
            const auto state = read(0x020201b7), category = read(0x020201b1), entry = read(0x020201b3);
            const auto head = read(0x020201b4), line = read(0x020201b6), lines = read(0x020201c7);
            const auto sort = read(0x020201b8), count = read(0x020201c6);
            if (samples % 120 == 0) std::printf("guide_progress stage=%d proc=%08x state=%u category=%u entry=%u count=%u line=%u/%u sort=%u\n",
                menu_stage, guide, state, category, entry, count, line, lines, sort);
            if (menu_stage == 10 && !guide && !find(0, 0x08b12c64) && !find(0, 0x08b12d1c)) menu_stage = 99;
            if (guide && samples >= 30 && source.frame >= intro_clicked + 24) {
                auto wheel = [&](int x, int delta) { input.wheel = delta; input.wheel_x = x; input.wheel_y = 80; };
                if (!menu_stage) { wheel(40, 8); menu_stage = 1; }
                else if (menu_stage == 1 && state == 0 && category == 0) { click(30, 44 + 16 * guide_category); menu_stage = 2; }
                else if (menu_stage == 2 && state == 0 && category == guide_category && count) {
                    guide_entry_target = std::min(1u, count - 1); wheel(180, -1); menu_stage = 3;
                }
                else if (menu_stage == 3 && state == 1 && entry == guide_entry_target) { click(120, 44 + 16 * (entry - head)); menu_stage = 4; }
                else if (menu_stage == 4 && state == 2) {
                    if (lines > 4) { menu_before = std::min(3u, lines - 4); wheel(160, -3); menu_stage = 5; }
                    else if (entry + 1 < count) { click(100, 100, true); menu_stage = 11; }
                    else { failed = true; menu_stage = -1; }
                } else if (menu_stage == 11 && state == 1) { click(120, 44 + 16 * (entry + 1 - head)); menu_stage = 4; }
                else if (menu_stage == 5 && state == 2 && int(line) == menu_before) { wheel(160, 8); menu_stage = 6; }
                else if (menu_stage == 6 && state == 2 && line == 0) { click(160, 80); menu_stage = 7; }
                else if (menu_stage == 7 && state == 2 && line > 0) {
                    std::printf("guide_native detail_lines=%u scrolled=%u\n", lines, line);
                    click(30, 44); menu_stage = 8;
                } else if (menu_stage == 8 && state == 0 && category == 0) { click(190, 8); menu_stage = 9; }
                else if (menu_stage == 9 && state == 0 && sort == 1) { click(190, 20); menu_stage = 10; }
                intro_clicked = source.frame;
            }
        } else if (world_mode && source.iwram && source.iwram_size >= 0x5350) {
            const auto* world = source.iwram + 0x5280;
            const auto word = [&](unsigned off) { return unsigned(world[off]) | (unsigned(world[off + 1]) << 8); };
            const auto p = find(0x080b9804);
            if (samples < 12 || samples % 120 == 0) std::printf("world_frame=%llu cursor=%u,%u held=%u new=%u loc=%u valid2=%u cb=%08x\n",
                static_cast<unsigned long long>(source.frame), word(9), word(13), read(0x02024cc4, 2), read(0x02024cc8, 2), world[0x11], world[0x38], p);
            if (!menu_stage && p && samples >= 30) {
                std::printf("world_initial proc=%08x recycled30=%08x location=%u cursor=%u,%u\n", p,
                    read(p + 0x30, 4), world[0x11], word(9), word(13));
                menu_before = int(p);
                click(120 - int(word(2)), 120 - int(word(4))); menu_stage = 1;
            } else if (menu_stage == 1 && word(9) == 120 && word(13) == 120) { menu_stage = 2; intro_clicked = source.frame; }
            else if (menu_stage == 2 && (world[0x11] == 2 ||
                     (!p && read(static_cast<unsigned>(menu_before) + 0x3e) == 2 &&
                      (find(0x08007c4c) || find(0x08006b4c) || find(0x0808f8b4) || find(0x0808b178))))) {
                menu_stage = world_skip ? 3 : 99; intro_clicked = source.frame;
                world_event = find(0x0800ce4c);
                std::printf("world_native selected_node=%u location=%u intro=%d event=%08x flags=%u type=%u\n",
                    read(static_cast<unsigned>(menu_before) + 0x3e), world[0x11], !p, world_event,
                    read(world_event + 0x3c, 2), read(world_event + 0x41));
            }
            if (menu_stage == 2 && source.frame == intro_clicked + 24) click(120 - int(word(2)), 120 - int(word(4)));
            if (menu_stage == 3 && source.frame == intro_clicked + 24) click(100, 100, true);
            if (menu_stage == 3 && world_event && (read(world_event + 0x3c, 2) & 4)) {
                menu_stage = 4; std::printf("world_native skipping=1 start_sent=%d\n", world_start_sent);
            }
            if (menu_stage == 4 && find(0x0800ce4c) != world_event &&
                !find(0x08006b4c) && !find(0x08007c4c)) {
                failed |= !world_start_sent; menu_stage = failed ? -1 : 99;
            }
        } else if (trade_tutorial) {
            const auto trade = find(0, 0x0859bb1c), target = find(0x0804f96c);
            const auto talk = find(0x08007c4c) || find(0x0808b178) || find(0x08006b4c);
            if (samples % 60 == 1) std::printf("trade_progress trade=%08x state=%u target=%08x talk=%d gfx=%u flags=%u\n",
                trade, trade ? read(trade + 0x48) : 0, target, bool(talk), read(0x0202bcb2), read(0x0202bcb4));
            if (trade) trade_states_seen |= 1u << std::min(read(trade + 0x48), 31u);
            if (menu_stage == 2 && !trade && find(0x0804f164)) {
                constexpr unsigned required = (1u << 2) | (1u << 3) | (1u << 5) | (1u << 8);
                failed |= (trade_states_seen & required) != required;
                menu_stage = failed ? -1 : 99;
                std::printf("trade_states_seen=%08x required=%08x\n", trade_states_seen, required);
            }
            if (menu_stage != 99 && source.frame >= intro_clicked + 24) {
                if (talk) click(100, 100);
                else if (trade) {
                    const auto state = read(trade + 0x48);
                    if (state == 2 || state == 3) {
                        const auto unit = read(trade + 0x30, 4);
                        for (unsigned row = 0; row < 5; ++row) if (read(unit + 0x1e + row * 2) == 0x6c) {
                            click(trade_text ? 184 : 144, (trade_text ? 80 : 76) + row * 16); break;
                        }
                        menu_stage = 1;
                    } else if (state == 5) {
                        click(32 + read(trade + 0x41) * 112, 76 + read(trade + 0x42) * 16);
                    } else if (state == 8) {
                        failed |= !read(trade + 0x40);
                        const auto unit = read(trade + 0x2c, 4);
                        bool received = false;
                        for (unsigned row = 0; row < 5; ++row) received |= read(unit + 0x1e + row * 2) == 0x6c;
                        failed |= !received;
                        click(32, 76, true); menu_stage = 2;
                        std::printf("trade_native swapped=%u received=%d\n", read(trade + 0x40), received);
                    }
                } else if (target && !menu_stage) {
                    const auto node = read(target + 0x30, 4);
                    click(int(read(node)) * 16 - int(read(0x0202bcbc, 2)) + 8,
                          int(read(node + 1)) * 16 - int(read(0x0202bcbe, 2)) + 8);
                }
                intro_clicked = source.frame;
            }
        } else if (tutorial) {
            const auto menu = find(0x0804f164), target = find(0x0804f96c);
            const auto talk = find(0x08007c4c) || find(0x0808b178) || find(0x08006b4c);
            if (menu_stage >= 3 && (find(0, 0x085b9358) || find(0, 0x085b9378) || find(0, 0x089a3508))) menu_stage = 99;
            else if (menu_stage != 99 && source.frame >= intro_clicked + 24) {
                if (talk) click(100, 100);
                else if (!menu_stage) {
                    click(int(read(0x0202bcc4, 2)) * 16 - int(read(0x0202bcbc, 2)) + 8,
                          int(read(0x0202bcc6, 2)) * 16 - int(read(0x0202bcbe, 2)) + 8);
                    menu_stage = 1;
                } else if (menu && !(read(menu + 0x63) & 0xec) &&
                           (menu_stage == 1 || (menu_stage == 2 && int(read(menu + 0x30, 4)) != menu_before))) {
                    if (menu_stage == 1 && source.iwram && source.iwram_size >= 0x4e54) {
                        const auto* u = source.iwram + 0x4e50;
                        const auto unit = std::uint32_t(u[0]) | (std::uint32_t(u[1]) << 8) |
                            (std::uint32_t(u[2]) << 16) | (std::uint32_t(u[3]) << 24);
                        if (read(unit + 0x10) != 4 || read(unit + 0x11) != 5) { failed = true; menu_stage = -1; }
                    }
                    if (menu_stage > 0) for (unsigned i = 0; i < read(menu + 0x60); ++i) {
                        const auto item = read(menu + 0x34 + i * 4, 4);
                        if (read(item + 0x3d) != 1) continue;
                        click(int(read(item + 0x2a, 2)) * 8 + 4, int(read(item + 0x2c, 2)) * 8 + 4);
                        menu_before = int(read(menu + 0x30, 4)); ++menu_stage; break;
                    }
                } else if (target && menu_stage >= 3) {
                    const auto node = read(target + 0x30, 4);
                    click(int(read(node)) * 16 - int(read(0x0202bcbc, 2)) + 8,
                          int(read(node + 1)) * 16 - int(read(0x0202bcbe, 2)) + 8);
                }
                intro_clicked = source.frame;
            }
        } else if (intro_mode) {
            const auto talk = find(0x08007c4c) || find(0x08006b4c) || find(0x0808f8b4);
            if (intro_skips && !find(0, 0x08aa21bc)) menu_stage = 99;
            else if (menu_stage != 99 && source.frame >= intro_clicked + 24) {
                if (talk) {
                    click(100, 100, intro_advances > 0); intro_clicked = source.frame;
                } else {
                    const auto main = find(0x080a9024), slots = find(0x080a9494);
                    if (main) {
                        const unsigned mask = read(main + 0x30), count = read(main + 0x31);
                        int row = 0; for (unsigned bit = 0; bit < 4; ++bit) row += (mask >> bit) & 1;
                        const int offset = int(read(main + 0x2f) + read(main + 0x46, 2));
                        if (mask & 16) click(80 - offset, std::max(2, 68 - int(count) * 25 / 2) + row * 25 + 5);
                    } else if (slots) {
                        for (int i = 0; i < 3; ++i) if (read(slots + 0x37 + i) == 0xff) {
                            click(268 - int(read(slots + 0x2f)), 36 + i * 32); break;
                        }
                    } else if (find(0x080ac288)) click(50, 55);
                    else click(100, 80);
                    intro_clicked = source.frame;
                }
            }
        } else if (!menu_stage) {
            click(232, 152); menu_stage = 1;
        } else if (menu_stage == 1) {
            const auto menu = find(0x0804f164);
            if (menu) for (unsigned i = 0; i < read(menu + 0x60); ++i) {
                const auto item = read(menu + 0x34 + i * 4, 4);
                const auto def = 0x0859ce6c + (options ? 4 : status ? 1 : 0) * 36;
                if (read(item + 0x30, 4) == def) {
                    click(int(read(item + 0x2a, 2)) * 8 + 4, int(read(item + 0x2c, 2)) * 8 + 4);
                    menu_stage = 2; break;
                }
            }
        } else if (options) {
            if (menu_stage == 2 && config && !read(config + 0x30, 2)) {
                menu_before = (read(0x0202bd30) >> 7) & 1;
                click(menu_before ? 115 : 146, 60); menu_stage = 3;
            } else if (menu_stage == 3 && config && read(0x020201b2, 2) == 1 &&
                       int((read(0x0202bd30) >> 7) & 1) != menu_before) {
                if (wheel_mode) { input.wheel = -6; input.wheel_x = 230; input.wheel_y = 80; }
                else click(40, 128);
                menu_stage = 4;
            } else if (menu_stage == 4 && config && read(0x020201b4, 2) > 0 &&
                       read(0x020201b2, 2) == (wheel_mode ? 7u : 5u) && !read(config + 0x30, 2)) {
                click(20, 20, true); menu_stage = 5;
            } else if (menu_stage == 5 && find(0x0801c940) && !read(0x0202bcb1)) menu_stage = 99;
        } else if (status) {
            if (menu_stage == 2 && chapter) { click(80, 45); menu_stage = 3; }
            else if (menu_stage == 3 && chapter && read(chapter + 0x2e) == 1) { click(20, 20, true); menu_stage = 5; }
            else if (menu_stage == 5 && find(0x0801c940) && !read(0x0202bcb1)) menu_stage = 99;
        } else {
            if (menu_stage == 2 && list && !read(list + 0x29)) { click(150, 64); menu_stage = 3; }
            else if (menu_stage == 3 && stats && !read(0x02003c04)) {
                menu_before = read(0x02003bfc); click(220, 10); menu_stage = 4;
            } else if (menu_stage == 4 && stats && !read(0x02003c04) && int(read(0x02003bfc)) != menu_before) {
                click(20, 20, true); menu_stage = 5;
            } else if (menu_stage == 5 && list && !read(list + 0x29)) {
                menu_before = read(list + 0x33); click(24, 44); menu_stage = 6;
            } else if (menu_stage == 6 && list && read(list + 0x29) == 3 && int(read(list + 0x33)) != menu_before) {
                click(20, 20, true); menu_stage = 7;
            } else if (menu_stage == 7 && find(0x0801c940) && !read(0x0202bcb1)) menu_stage = 99;
        }
    }
    if (menu_stage != old_stage) std::printf("menu_probe=%s stage=%d frame=%llu\n", menu_mode, menu_stage, static_cast<unsigned long long>(source.frame));
    if (tutorial && fresh && samples % 120 == 0) {
        const auto m = find(0x0804f164);
        std::printf("tutorial_progress stage=%d menu=%08x flags=%u talk=%u cursor=%u,%u\n", menu_stage, m,
            m ? read(m + 0x63) : 0, bool(find(0x08007c4c) || find(0x0808b178)),
            read(0x0202bcc4, 2), read(0x0202bcc6, 2));
        for (const auto cb : {0x08007c4cu, 0x0808b178u, 0x0808a87cu, 0x0804f96cu}) {
            const auto p = find(cb);
            if (p) std::printf("tutorial_proc callback=%08x script=%08x flags=%u locks=%u\n", cb, read(p, 4),
                read(p + 0x34), read(p + 0x28));
        }
    }
    const auto keys = sacredstones::map_pointer_input(input);
    if (world_skip && fresh && keys == 8) world_start_sent = true;
    if (intro_mode && fresh) {
        if (keys == 1 && (find(0x08007c4c) || find(0x08006b4c) || find(0x0808f8b4))) ++intro_advances;
        if (keys == 8 && (intro_advances || find(0, 0x08aa21bc))) ++intro_skips;
    }
    if (keys && fresh && (samples < 250 || intro_mode)) std::printf("menu_keys=%u stage=%d frame=%llu moving=%u held=%u reset=%d\n", keys, menu_stage,
        static_cast<unsigned long long>(source.frame), config ? read(config + 0x30, 2) : 0, read(0x02024cc4, 2), source.reset);
    return keys;
}
std::uint16_t cursor_probe(const gbarecomp::GamePointerInput& source) {
    if (!source.ewram || source.ewram_size < 0x2e4d8) return 0;
    auto read = [&](unsigned offset) { return int(source.ewram[offset] | (source.ewram[offset + 1] << 8)); };
    const int camera_x = read(0x2bcbc), camera_y = read(0x2bcbe);
    const int cx = read(0x2bcc4), cy = read(0x2bcc6);
    if (!samples) {
        cursor_camera_x = camera_x; cursor_camera_y = camera_y;
        click_x = std::min(read(0x2e4d4) - 1, camera_x / 16 + 14);
        click_y = std::min(read(0x2e4d6) - 1, camera_y / 16 + 9);
        if (cx == click_x && cy == click_y) click_x = camera_x / 16 + 2;
    }
    const bool fresh = !samples || source.frame != cursor_frame;
    cursor_frame = source.frame;
    if (fresh) ++samples;
    if (samples == 2 && fresh) instant_cursor = cx == click_x && cy == click_y &&
        read(0x2bcd0) == click_x * 16 && read(0x2bcd2) == click_y * 16;
    if (camera_x > read(0x2bcd8) || camera_y > read(0x2bcda)) camera_stable = false;
    if (samples > 1 && samples < 20 && (camera_x != cursor_camera_x || camera_y != cursor_camera_y)) camera_stable = false;
    if (samples == 80) { settled_x = camera_x; settled_y = camera_y; }
    if (samples > 80 && (camera_x != settled_x || camera_y != settled_y)) camera_stable = false;
    auto input = source;
    input.physical_keys = 0x3ff; input.primary = samples == 1 && fresh; input.secondary = false;
    input.reset = samples == 20;
    input.x = click_x * 16 - camera_x + 8; input.y = click_y * 16 - camera_y + 8;
    input.hover_x = input.hover_y = -1;
    const auto keys = sacredstones::map_pointer_input(input);
    if (samples <= 5 && fresh) {
        int x = 0, y = 0; std::uint32_t proc = 0, mode = 0;
        const bool pending = sacredstones::map_pointer.cursor_jump(x, y, proc, mode);
        std::printf("cursor_sample=%u frame=%llu cursor=%d,%d display=%d,%d click=%d,%d pending=%d proc=%08x mode=%08x keys=%u reset=%d\n",
            samples, static_cast<unsigned long long>(source.frame), cx, cy, read(0x2bcd0), read(0x2bcd2), click_x, click_y, pending, proc, mode, keys, source.reset);
    }
    // Keep the map idle: test camera ownership/handback without opening a menu.
    return keys == 1 ? 0 : keys;
}
std::uint16_t input_probe(const gbarecomp::GamePointerInput& source) {
    if (menu_mode) return menu_probe(source);
    if (cursor_mode) return cursor_probe(source);
    if (!source.ewram || source.ewram_size < 0x2e4d8) return 0;
    auto read = [&](unsigned offset) { return int(source.ewram[offset] | (source.ewram[offset + 1] << 8)); };
    if (!samples) vertical = read(0x2bcda) >= read(0x2bcd8);
    const int camera = read(vertical ? 0x2bcbe : 0x2bcbc), cx = read(0x2bcc4), cy = read(0x2bcc6);
    if (read(0x2bcbc) > read(0x2bcd8) || read(0x2bcbe) > read(0x2bcda)) camera_stable = false;
    if (!samples) {
        initial_cursor_x = cx; initial_cursor_y = cy; initial_camera_x = camera;
        const int maximum = read(vertical ? 0x2bcda : 0x2bcd8);
        forward = camera <= maximum / 2;
        required_pan = maximum / 2 < 40 ? maximum / 2 : 40;
    }
    if (cx != initial_cursor_x || cy != initial_cursor_y) failed = true;
    ++samples;
    if (samples == 25) { panned = required_pan > 0 && std::abs(camera - initial_camera_x) >= required_pan; held_camera_x = camera; }
    if (samples == 40) held = camera == held_camera_x;
    if (samples >= 100) restored = camera != held_camera_x;
    if (samples == 80) settled_x = camera;
    if (samples > 80 && camera != settled_x) camera_stable = false;
    auto input = source;
    // This executable tests the game/input seam, not desktop focus routing.
    input.physical_keys = 0x3ff; input.primary = input.secondary = false;
    input.reset = samples == 41;
    input.hover_x = samples < 25 ? (vertical ? 120 : forward ? 235 : 4) : -1;
    input.hover_y = vertical ? (forward ? 155 : 4) : 80;
    return sacredstones::map_pointer_input(input);
}
// Headless snapshot tests supply host keys at native frame boundaries.
// The production policy stays read-only; SDL focus is tested separately.
void chapter_frame_probe(const gbarecomp::ExtendedViewFrameInfo* frame) {
    auto* bus = gbarecomp::active_bus();
    if (!frame || !bus) return;
    gbarecomp::GamePointerInput input;
    input.frame = frame->frame_count;
    input.ewram = bus->ewram_ptr(); input.ewram_size = 256 * 1024;
    input.iwram = bus->iwram_ptr(); input.iwram_size = 32 * 1024;
    const auto buttons = input_probe(input);
    bus->io().set_keyinput(static_cast<std::uint16_t>(0x3ff & ~buttons));
}
}
int main(int argc, char** argv) {
    menu_mode = std::getenv("SACREDSTONES_TEST_MENU");
    cursor_mode = std::getenv("SACREDSTONES_TEST_CURSOR") != nullptr;
    if (!sacredstones::initialize_mouse_camera()) return 1;
    sacredstones::map_pointer.enabled = true;
    _putenv_s("GBARECOMP_SAVE_TYPE", "sram");
    gbarecomp::RunOptions options;
    options.builtin_game_name = "Sacred Stones mouse camera validation";
    options.builtin_rom_sha1 = "c25b145e37456171ada4b0d440bf88a19f4d509f";
    options.freely_resizable_window = true;
    options.pointer_input = input_probe;
    if (menu_mode && (std::strncmp(menu_mode, "chapter-save", 12) == 0 || std::strncmp(menu_mode, "trade-tutorial", 14) == 0 || std::strncmp(menu_mode, "world", 5) == 0 || std::strncmp(menu_mode, "guide", 5) == 0)) {
        options.pointer_input = nullptr;
        options.extended_view_frame = chapter_frame_probe;
    }
    const int result = gbarecomp::run_game(argc, argv, options);
    if (menu_mode) {
        std::printf("menu_game_probe mode=%s stage=%d complete=%d\n", menu_mode, menu_stage, menu_stage == 99);
        if (std::strcmp(menu_mode, "intro") == 0) std::printf("intro_advances=%u intro_skips=%u\n", intro_advances, intro_skips);
        return result || failed || menu_stage != 99 ? 1 : 0;
    }
    if (cursor_mode) {
        std::printf("mouse_cursor_probe samples=%u instant_cursor=%d camera_stable=%d\n", samples, instant_cursor, camera_stable);
        return result || samples < 100 || !instant_cursor || !camera_stable ? 1 : 0;
    }
    std::printf("mouse_game_probe samples=%u panned=%d held=%d restored=%d cursor_unchanged=%d\n",
                samples, panned, held, restored, !failed);
    std::printf("camera_stable=%d\n", camera_stable);
    return result || samples < 100 || failed || !panned || !held || !restored || !camera_stable ? 1 : 0;
}

#include "map_pointer.h"

#include <array>
#include <algorithm>
#include <cstring>

namespace sacredstones {
namespace {
// FE8 USA: fireemblem8u/include/{types,proc}.h and fireemblem8.map.
constexpr std::uint32_t kRam = 0x02000000;
constexpr std::uint32_t kBm = 0x0202bcb0;
constexpr std::uint32_t kMapSize = 0x0202e4d4;
constexpr std::uint32_t kRoots = 0x02026a70;
constexpr std::uint32_t kPlayerScript = 0x0859aad8;
constexpr std::uint32_t kSelect = 0x0801c940;
constexpr std::uint32_t kMove = 0x0801cd1c;
constexpr std::uint32_t kForcedMove = 0x0801d818;
constexpr std::uint32_t kMenu = 0x0804f164;
constexpr std::uint32_t kTarget = 0x0804f96c;
constexpr std::uint32_t kTargetScript = 0x085b655c;
constexpr std::uint32_t kTradeScript = 0x0859bb1c;
constexpr std::uint32_t kTradeSource = 0x0802d834;
constexpr std::uint32_t kTradeDestination = 0x0802d980;
constexpr std::uint32_t kCameraScript = 0x0859a548;
constexpr std::uint32_t kWorldScript = 0x08a3d748;
constexpr std::uint32_t kWorldLoop = 0x080b9804;
constexpr std::uint32_t kWorldData = 0x03005280;
constexpr std::uint32_t kGuideData = 0x02020188;
bool guide_redraw(std::uint32_t script) {
    return script == 0x08b12b3c || script == 0x08b12b9c || script == 0x08b12bec ||
        script == 0x08b12c14 || script == 0x08b12c3c;
}
constexpr char kSetting[] = "sacredstones.mouse_map";

struct Ram {
    const PointerInput& in;
    bool contains(std::uint32_t address, std::size_t count) const {
        const bool internal = address >= 0x03000000;
        const auto base = internal ? 0x03000000u : kRam;
        const auto size = internal ? in.iwram_size : in.ewram_size;
        return (internal ? in.iwram : in.ewram) && address >= base && address - base <= size &&
            count <= size - (address - base);
    }
    std::uint32_t read(std::uint32_t address, unsigned count) const {
        if (!contains(address, count)) return 0;
        std::uint32_t value = 0;
        const bool internal = address >= 0x03000000;
        const auto* data = internal ? in.iwram : in.ewram;
        const auto base = internal ? 0x03000000u : kRam;
        for (unsigned i = 0; i < count; ++i)
            value |= std::uint32_t(data[address - base + i]) << (8 * i);
        return value;
    }
    int s16(std::uint32_t address) const {
        const auto value = read(address, 2);
        return value < 0x8000 ? int(value) : int(value) - 0x10000;
    }
    int s8(std::uint32_t address) const {
        const auto value = read(address, 1);
        return value < 0x80 ? int(value) : int(value) - 0x100;
    }
};

enum class Kind { None, Map, Menu, Target, Trade, Continue, MainList, Slots, Difficulty,
                  UnitList, Config, Stats, Help, Chapter, Extras, Talk, Scene, Choice, Music, World, Guide };
struct Context {
    Kind kind = Kind::None;
    std::uint32_t proc = 0, mode = 0, identity = 0;
    bool camera_wait = false;
    bool skip = false;
    bool world = false;
};

// Walk live process trees, not stale entries in the allocation pool.
Context active_context(const Ram& ram) {
    std::array<std::uint32_t, 128> stack{};
    std::array<std::uint32_t, 64> seen{};
    std::size_t count = 0, visited = 0;
    Context map, modal, talk, scene;
    int talk_priority = -1;
    bool battle = false;
    bool interactive_blocked = false;
    bool world_present = false, world_busy = false;
    bool guide_busy = false;
    unsigned modal_count = 0;
    for (unsigned i = 0; i < 8; ++i) stack[count++] = ram.read(kRoots + i * 4, 4);
    while (count && visited < seen.size()) {
        const auto p = stack[--count];
        if (!p || (p & 3) || !ram.contains(p, 0x2c)) continue;
        bool duplicate = false;
        for (std::size_t i = 0; i < visited; ++i) duplicate |= seen[i] == p;
        if (duplicate) continue;
        seen[visited++] = p;
        const auto script = ram.read(p, 4);
        guide_busy |= guide_redraw(script);
        world_present |= script == kWorldScript;
        world_busy |= script == 0x08a3e4f4 || script == 0x08a3e52c;
        const auto callback = ram.read(p + 0x0c, 4) & ~1u;
        interactive_blocked |= script == kWorldScript || callback == kMenu || (script == kTargetScript && callback == kTarget) ||
            (script == kTradeScript && (callback == kTradeSource || callback == kTradeDestination));
        battle |= script == 0x08591f18 || script == 0x089a2c80;
        if (!ram.read(p + 0x24, 2) && !(ram.read(p + 0x27, 1) & 9)) {
            const auto mode = ram.read(p + 0x0c, 4) & ~1u;
            const auto locks = ram.read(p + 0x28, 1);
            const auto child = ram.read(p + 0x18, 4);
            const bool camera_wait = locks == 1 && ram.contains(child, 0x2c) &&
                ram.read(child, 4) == kCameraScript;
            Context candidate;
            candidate.proc = p;
            candidate.mode = mode;
            candidate.camera_wait = camera_wait;
            if (script == kPlayerScript && !locks && (mode == kSelect || mode == kMove || mode == kForcedMove)) {
                candidate.kind = Kind::Map; map = candidate;
            } else if (script == kWorldScript && mode == kWorldLoop && !locks &&
                       ram.contains(p, 0x58) &&
                       ram.contains(kWorldData, 0xd0) && ram.read(kWorldData + 1, 1) == 1) {
                candidate.kind = Kind::World; candidate.world = true; map = candidate;
            } else if (mode == kMenu && !locks && ram.contains(p, 0x6a) &&
                       !(ram.read(p + 0x63, 1) & 0xec)) {
                candidate.kind = Kind::Menu;
                candidate.identity = ram.read(p + 0x30, 4);
            } else if (script == kTargetScript && mode == kTarget &&
                       (!locks || camera_wait) && ram.contains(p, 0x3c) &&
                       !(ram.read(p + 0x34, 1) & 0x40)) {
                candidate.kind = Kind::Target;
                candidate.identity = ram.read(p + 0x2c, 4);
            } else if (script == kTradeScript && !locks && ram.contains(p, 0x50) &&
                       (mode == kTradeSource || mode == kTradeDestination) &&
                       (ram.read(p + 0x48, 1) == 0 || ram.read(p + 0x48, 1) == 2 ||
                        ram.read(p + 0x48, 1) == 3 || ram.read(p + 0x48, 1) == 5 || ram.read(p + 0x48, 1) == 8)) {
                candidate.kind = Kind::Trade;
                candidate.identity = ram.read(p + 0x48, 1);
            } else if (!locks && (mode == 0x080c6354 || mode == 0x080c5580 ||
                       mode == 0x080c6704 || mode == 0x080cc32c || mode == 0x0801fdbc)) {
                candidate.kind = Kind::Continue;
            } else if (!locks && ram.contains(p, 0x64) &&
                       (script == 0x08a200b8 || (script == 0x08a203a8 && mode == 0x080a9494))) {
                if (mode == 0x080a9024) {
                    candidate.kind = Kind::MainList;
                    candidate.identity = ram.read(p + 0x30, 1);
                } else if (mode == 0x080a9494 || mode == 0x080a9e1c) {
                    candidate.kind = Kind::Slots;
                    candidate.identity = ram.read(p + 0x42, 2) |
                        (ram.read(p + 0x36, 1) ? 0x10000 : 0) |
                        (mode == 0x080a9e1c ? ram.read(p + 0x35, 1) << 20 : 0);
                } else if (mode == 0x080a9b90) {
                    candidate.kind = Kind::Extras;
                    candidate.identity = ram.read(p + 0x32, 1);
                }
            } else if ((script == 0x08b12c64 || script == 0x08b12d1c) && mode == 0x080cefd4 &&
                       (!locks || (locks == 1 && guide_redraw(ram.read(child, 4)))) && ram.contains(kGuideData, 0x7c)) {
                candidate.kind = Kind::Guide;
                candidate.identity = ram.read(kGuideData + 0x30, 1);
                candidate.camera_wait = locks != 0;
            } else if (!locks && mode == 0x080ac288 && ram.contains(p, 0x34)) {
                candidate.kind = Kind::Difficulty;
            } else if (!locks && mode == 0x08091aec && ram.contains(p, 0x48) &&
                       (script == 0x08a177a0 || script == 0x08a178c0 ||
                        script == 0x08a17990 || script == 0x08a17a30)) {
                candidate.kind = Kind::UnitList;
                candidate.identity = ram.read(p + 0x2f, 1) | (ram.read(p + 0x39, 1) << 8) |
                    (ram.read(p + 0x2b, 1) << 16);
            } else if (!locks && mode == 0x080b220c && ram.contains(p, 0x37) &&
                       (script == 0x08a2ece0 || script == 0x08a2ed88 || script == 0x08a2ee00)) {
                candidate.kind = Kind::Config;
                candidate.identity = ram.read(0x020201bf, 1);
            } else if (!locks && mode == 0x0808873c && script == 0x08a009d8) {
                candidate.kind = Kind::Stats;
                candidate.identity = ram.read(0x02003c08, 4);
            } else if (!locks && ((mode == 0x08089088 && script == 0x08a00ad0) ||
                                  (mode == 0x08089430 && script == 0x08a00b00) ||
                                  (mode == 0x080a9dbc && script == 0x08a20068) ||
                                  ((mode == 0x080afdf4 || mode == 0x080afc98) && script == 0x08a21338) ||
                                  (mode == 0x080a15b0 && script == 0x08a1975c) ||
                                  (mode == 0x080a2448 && script == 0x08a19880))) {
                candidate.kind = Kind::Help;
            } else if (!locks && mode == 0x0808e608 && ram.contains(p, 0x40) &&
                       (script == 0x08a01b54 || script == 0x08a01c04)) {
                candidate.kind = Kind::Chapter;
            } else if (!locks && script == 0x08a21338 && mode == 0x080af8a0 && ram.contains(p, 0x50)) {
                candidate.kind = Kind::Music;
                candidate.identity = ram.read(p + 0x36, 1);
            } else if (!locks && ((script == 0x085914b0 && mode == 0x08007e9c) ||
                                  (script == 0x08a01ec4 && mode == 0x08090014)) && ram.contains(p, 0x30)) {
                candidate.kind = Kind::Choice;
            } else if (!locks && ((script == 0x08591340 && mode == 0x08006b4c) ||
                                  (script == 0x085913f0 && mode == 0x08007c4c) ||
                                  (script == 0x08a01dbc && mode == 0x0808f04c) ||
                                  (script == 0x08a01e64 && mode == 0x0808f8b4) ||
                                  (script == 0x08a016e0 && mode == 0x0808a87c) ||
                                  (script == 0x08a01760 && mode == 0x0808b178))) {
                candidate.kind = Kind::Talk;
                candidate.skip = mode == 0x08006b4c || mode == 0x08007c4c ?
                    ram.contains(0x030000c8, 2) && !(ram.read(0x030000c8, 2) & 4) :
                    mode == 0x0808a87c || mode == 0x0808b178 ? !(ram.read(0x0203e82a, 2) & 0x82) :
                    !((ram.read(0x0203e874, 4) >> 10) & 64);
                const int priority = mode == 0x08007c4c ? 2 : (mode == 0x0808f8b4 || mode == 0x0808b178) ? 1 : 0;
                if (priority > talk_priority) { talk = candidate; talk_priority = priority; }
                candidate.kind = Kind::None;
            } else if (!locks && script == 0x08aa21bc &&
                       (mode == 0 || mode == 0x080c4c3c || mode == 0x080c4da0 ||
                        mode == 0x080c4e18 || mode == 0x080c4f60 || mode == 0x080c501c ||
                        mode == 0x080c5104 || mode == 0x080c538c)) {
                // The new-game subtitle sequence uses script commands between
                // its animation callbacks. Send Start; its own handler decides
                // whether this phase can skip. Never alter its timers/fades.
                scene = candidate; scene.kind = Kind::Scene; scene.skip = true;
            } else if (script == 0x08591ac0 && mode == 0x0800ce4c && ram.contains(p, 0x44)) {
                candidate.kind = Kind::Scene;
                const auto flags = ram.read(p + 0x3c, 2);
                candidate.skip = !locks && (flags & 2) && !(flags & 0x15) && ram.read(p + 0x41, 1) <= 1;
                scene = candidate;
                // Event engines are background skip listeners; real controls
                // (including dialogue choices) take precedence.
                candidate.kind = Kind::None;
            }
            if (candidate.kind != Kind::None && candidate.kind != Kind::Map && candidate.kind != Kind::World) {
                // Intro skip listeners can coexist; a real interactive menu
                // takes precedence over decorative title/intro listeners.
                if (candidate.kind != Kind::Continue || !modal_count) {
                    if (modal.kind == Kind::Continue) modal_count = 0;
                    modal = candidate; ++modal_count;
                }
            }
        }
        // RunProcessRecursive visits previous siblings, then children.
        if (count + 2 <= stack.size()) {
            stack[count++] = ram.read(p + 0x20, 4);
            stack[count++] = ram.read(p + 0x18, 4);
        }
    }
    if (modal_count > 1) return {};
    // World-map sprite text disables text-local skipping, but Start belongs
    // to the world event engine. Use its native skip gate, never clear flags.
    if (talk_priority >= 0 && world_present) {
        talk.skip = scene.proc && ram.read(scene.proc + 0x41, 1) == 0 && scene.skip;
    }
    // Tutorial explanation boxes can overlay an otherwise live target/menu.
    // Actual text/waits own input; a background skip listener alone does not.
    if (talk_priority >= 1 && modal.kind != Kind::Choice) {
        if (battle) talk.skip = false;
        return talk;
    }
    if (modal_count == 1) {
        modal.world = world_present;
        if (modal.kind == Kind::Guide) modal.camera_wait |= guide_busy;
        return modal;
    }
    if (talk_priority >= 0) { if (battle) talk.skip = false; return talk; }
    if (map.kind == Kind::World && !world_busy) return map;
    if (interactive_blocked) return {};
    if (scene.proc) { if (battle) scene.skip = false; return scene; }
    return map;
}

bool valid_menu(const Ram& ram, std::uint32_t p) {
    const auto count = ram.read(p + 0x60, 1);
    return count >= 1 && count <= 9 && ram.read(p + 0x61, 1) < count &&
        ram.s8(p + 0x2e) >= 3 && ram.s8(p + 0x2e) <= 30;
}

std::uint32_t menu_item(const Ram& ram, std::uint32_t p, int index) {
    if (!valid_menu(ram, p) || index < 0 || unsigned(index) >= ram.read(p + 0x60, 1)) return 0;
    const auto item = ram.read(p + 0x34 + index * 4, 4);
    return ram.contains(item, 0x3e) ? item : 0;
}

// Grey inventory entries can still open native item actions. Other
// disabled commands retain their rejection policy.
bool menu_selectable(const Ram& ram, std::uint32_t menu, std::uint32_t item) {
    const auto availability = ram.read(item + 0x3d, 1);
    return availability == 1 || (availability == 2 && ram.read(menu + 0x30, 4) == 0x0859d184);
}

int menu_hit(const Ram& ram, std::uint32_t p, int x, int y) {
    if (!valid_menu(ram, p)) return -1;
    const int right = (ram.s8(p + 0x2c) + ram.s8(p + 0x2e) - 1) * 8;
    for (unsigned i = 0; i < ram.read(p + 0x60, 1); ++i) {
        const auto item = menu_item(ram, p, i);
        if (!item || !menu_selectable(ram, p, item)) continue;
        const int left = ram.s16(item + 0x2a) * 8;
        const int top = ram.s16(item + 0x2c) * 8;
        if (x >= left && x < right && y >= top && y < top + 16) return int(i);
    }
    return -1;
}

// FE8 targets form a bounded ring. Follow that ring instead of synthesizing
// cursor motion over invalid units, and reject malformed/incomplete lists.
struct TargetRing {
    std::array<std::uint32_t, 64> nodes{};
    unsigned count = 0;
    bool valid = false;
    TargetRing(const Ram& ram, std::uint32_t current) {
        auto node = current;
        while (count < nodes.size() && ram.contains(node, 12)) {
            for (unsigned i = 0; i < count; ++i) if (nodes[i] == node) return;
            nodes[count++] = node;
            const auto next = ram.read(node + 4, 4);
            if (!ram.contains(next, 12) || ram.read(next + 8, 4) != node) return;
            if (next == current) { valid = true; return; }
            node = next;
        }
    }
};

bool trade_cell(const Ram& ram, std::uint32_t p, int column, int row) {
    return column >= 0 && column < 2 && row >= 0 && row < 5 &&
        ram.read(p + 0x34 + column * 6 + row, 1) != 0;
}

// FE8 USA uiconfig.c: visible order, selectors and PlaySt::config bit layout.
struct Option { unsigned shift, mask; std::array<int, 4> x; std::array<int, 4> width; };
constexpr std::array<Option, 13> kOptions{{
    {17, 3, {112,127,142,165}, {8,8,16,16}},
    {7, 1, {112,143,0,0}, {24,24,0,0}},
    {5, 3, {112,143,174,205}, {24,24,24,16}},
    {1, 1, {112,135,0,0}, {16,16,0,0}},
    {2, 3, {112,143,182,0}, {24,32,16,0}},
    {19,3, {112,143,182,0}, {24,24,16,0}},
    {16,1, {112,135,0,0}, {16,16,0,0}},
    {15,1, {112,135,0,0}, {16,16,0,0}},
    {4, 1, {112,135,0,0}, {16,16,0,0}},
    {14,1, {112,135,0,0}, {16,16,0,0}},
    {8, 1, {112,135,0,0}, {16,16,0,0}},
    {9, 1, {112,135,0,0}, {16,16,0,0}},
    {10,3, {112,127,142,157}, {8,8,8,8}},
}};
int option_value(const Ram& ram, int row) {
    if (row < 0 || row >= int(kOptions.size())) return -1;
    const auto& o = kOptions[row];
    int value = (ram.read(0x0202bd30, 4) >> o.shift) & o.mask;
    if (!row) { constexpr int anim[] = {0,2,3,1}; value = anim[value]; }
    return o.width[value] ? value : -1;
}
constexpr int kColumns[7][9] = {
    {20,64,136,192,0,0,0,0,0}, {20,68,129,152,176,196,0,0,0},
    {20,64,91,113,136,161,184,203,0}, {20,80,135,166,200,0,0,0,0},
    {20,68,96,123,140,188,0,0,0}, {20,76,92,108,124,140,156,172,188},
    {20,74,0,0,0,0,0,0,0},
};
std::uint32_t listed_unit(const Ram& ram, int row) {
    if (row < 0 || row >= int(ram.read(0x0200f158, 1)) || row >= 64) return 0;
    const auto entry = ram.read(0x0200d6e0 + row * 4, 4);
    if (!ram.contains(entry, 4)) return 0;
    const auto unit = ram.read(entry, 4);
    return ram.contains(unit, 0x48) && ram.read(unit, 4) ? unit : 0;
}
}

MapPointer map_pointer;

void MapPointer::cancel() {
    pending_ = false;
    cancel_pending_ = false;
    release_ = false;
    output_ = 0;
    preview_token_ = 0;
    detached_camera_ = false;
    jump_pending_ = false;
    drag_owned_ = false;
    screen_ready_ = 0;
}

bool MapPointer::cursor_jump(int& x, int& y, std::uint32_t& proc, std::uint32_t& mode) const {
    x = target_x_; y = target_y_; proc = proc_; mode = mode_;
    return enabled && pending_ && jump_pending_ && (mode == kSelect || mode == kMove || mode == kForcedMove || mode == kWorldLoop);
}

void MapPointer::cursor_jump_completed() { jump_pending_ = false; }

bool MapPointer::camera_position(int& x, int& y) const {
    x = camera_x_; y = camera_y_;
    return enabled && detached_camera_ && mode_ != kWorldLoop;
}

bool MapPointer::world_camera_position(int& x, int& y, std::uint32_t& proc) const {
    x = camera_x_; y = camera_y_; proc = proc_;
    return enabled && detached_camera_ && mode_ == kWorldLoop;
}

std::uint16_t MapPointer::update(const PointerInput& input) {
    const Ram ram{input};
    if (!enabled || input.reset || input.physical_keys != 0x03ff ||
        ((input.primary || input.secondary) &&
         (input.x < 0 || input.x >= 240 || input.y < 0 || input.y >= 160)) ||
        !ram.contains(kMapSize, 4)) {
        cancel();
        last_frame_ = input.frame;
        return 0;
    }
    const auto context = active_context(ram);
    const auto proc = context.proc, mode = context.mode;
    const bool front = context.kind == Kind::Continue || context.kind == Kind::MainList ||
        context.kind == Kind::Slots || context.kind == Kind::Difficulty || context.kind == Kind::Extras;
    const bool narrative = context.kind == Kind::Talk || context.kind == Kind::Scene || context.kind == Kind::Choice;
    const bool screen = context.kind == Kind::UnitList || context.kind == Kind::Config ||
        context.kind == Kind::Stats || context.kind == Kind::Help || context.kind == Kind::Chapter || context.kind == Kind::Music || context.kind == Kind::Guide;
    if (!proc || (!context.world && !front && !screen && !narrative && (ram.read(kBm + 2, 1) || (ram.read(kBm + 4, 1) & 0x50))) ||
        (context.kind == Kind::Map && ram.read(kBm + 1, 1))) {
        cancel(); last_frame_ = input.frame; return 0;
    }
    const bool new_frame = input.frame != last_frame_;
    // An A pulse belongs to exactly one interface. Never carry it from a
    // weapon menu into targeting, including multiple pumps in the same frame.
    if (input.frame < last_frame_ ||
        proc != proc_ || mode != mode_ || identity_ != context.identity) {
        const bool held = output_ != 0;
        cancel();
        release_ = held;
    }
    proc_ = proc; mode_ = mode; identity_ = context.identity;
    last_frame_ = input.frame;

    if (!input.pointer_down && !input.dragging) drag_owned_ = false;
    if (input.dragging && context.kind != Kind::Map && context.kind != Kind::World) {
        const bool held = output_ != 0;
        cancel(); release_ = held;
    }

    if (context.kind == Kind::World) {
        const int camera_x = ram.s16(kWorldData + 2), camera_y = ram.s16(kWorldData + 4);
        const auto cx = ram.read(kWorldData + 8, 4), cy = ram.read(kWorldData + 12, 4);
        const auto ignored = ram.read(0x03000010, 2);
        if (camera_x < 0 || camera_x > 240 || camera_y < 0 || camera_y > 160 ||
            cx > (479u << 8) || cy > (319u << 8)) { cancel(); return 0; }
        if (input.dragging) {
            if (!new_frame) return 0;
            const bool owned = drag_owned_ || input.drag_start;
            const int origin_x = detached_camera_ ? camera_x_ : camera_x;
            const int origin_y = detached_camera_ ? camera_y_ : camera_y;
            cancel();
            if (!owned || (ignored & 0xf0)) return 0;
            drag_owned_ = true;
            camera_x_ = std::clamp(origin_x + std::clamp(input.drag_dx, -480, 480), 0, 240);
            camera_y_ = std::clamp(origin_y + std::clamp(input.drag_dy, -320, 320), 0, 160);
            detached_camera_ = true;
            return 0;
        }
        if (input.secondary) {
            cancel(); if (ignored & 2) return 0;
            cancel_pending_ = true; started_ = input.frame;
        } else if (input.primary) {
            const int x = input.x + camera_x, y = input.y + camera_y;
            // Never clamp an outer-frame click onto a different destination.
            if (x < 24 || x > 456 || y < 24 || y > 296) { cancel(); return 0; }
            if ((ignored & 1) || (x > int(cx >> 8) && (ignored & 0x10)) ||
                (x < int(cx >> 8) && (ignored & 0x20)) || (y < int(cy >> 8) && (ignored & 0x40)) ||
                (y > int(cy >> 8) && (ignored & 0x80))) { cancel(); return 0; }
            target_x_ = x; target_y_ = y;
            pending_ = true; cancel_pending_ = false; jump_pending_ = false; started_ = input.frame;
            camera_x_ = camera_x; camera_y_ = camera_y; detached_camera_ = true;
        }
        if (!new_frame) return output_;
        output_ = 0;
        if (release_) { release_ = false; return 0; }
        if ((pending_ || cancel_pending_) && input.frame - started_ > 120) { cancel(); return 0; }
        if (cancel_pending_) {
            cancel_pending_ = false; if (!(ignored & 2)) output_ = 2;
        } else if (pending_) {
            if ((ignored & 1) || (target_x_ > int(cx >> 8) && (ignored & 0x10)) ||
                (target_x_ < int(cx >> 8) && (ignored & 0x20)) || (target_y_ < int(cy >> 8) && (ignored & 0x40)) ||
                (target_y_ > int(cy >> 8) && (ignored & 0x80))) { cancel(); return 0; }
            if (cx != (std::uint32_t(target_x_) << 8) || cy != (std::uint32_t(target_y_) << 8)) {
                jump_pending_ = true; return 0;
            }
            jump_pending_ = false;
            if (!(ram.read(0x02024cc4, 2) & 1)) { output_ = 1; pending_ = false; }
        } else if (!input.primary && !input.secondary && !input.pointer_down && !(ignored & 0xf0) &&
                   input.hover_x >= 0 && input.hover_x < 240 && input.hover_y >= 0 && input.hover_y < 160) {
            auto speed = [](int p, int extent) { return p < 16 ? -(1 + (15 - p) / 4) :
                p >= extent - 16 ? 1 + (p - extent + 16) / 4 : 0; };
            if (!detached_camera_) { camera_x_ = camera_x; camera_y_ = camera_y; }
            const int dx = speed(input.hover_x, 240), dy = speed(input.hover_y, 160);
            if (dx || dy) {
                camera_x_ = std::clamp(camera_x_ + dx, 0, 240);
                camera_y_ = std::clamp(camera_y_ + dy, 0, 160); detached_camera_ = true;
            }
        }
        if (output_) release_ = true;
        return output_;
    }

    if (narrative) {
        if (input.primary || input.secondary) {
            const bool released = release_ || output_;
            cancel(); release_ = released;
            target_x_ = 0;
            if (context.kind == Kind::Choice) {
                const int x = ram.s16(proc + 0x2c), y = ram.s16(proc + 0x2e);
                if (input.secondary) target_x_ = -1;
                else if (input.x >= x && input.x < x + 80 && input.y >= y && input.y < y + 16)
                    target_x_ = 1 + (input.x - x) / 40;
            } else if (input.secondary) target_x_ = context.skip ? 8 : 0;
            else if (context.kind == Kind::Scene) target_x_ = context.skip ? 8 : 0;
            else if (mode == 0x08006b4c) {
                if (ram.contains(0x030000c8, 2) && !(ram.read(0x030000c8, 2) & 8)) target_x_ = 1;
            } else if (mode != 0x0808b178 || !(ram.read(0x0203e82a, 2) & 8)) target_x_ = 1;
            pending_ = target_x_ != 0; started_ = input.frame;
        }
        if (!new_frame) return output_;
        output_ = 0;
        if (release_) { release_ = false; return 0; }
        if (!pending_ || input.frame - started_ > 60) { pending_ = false; return 0; }
        if (context.kind == Kind::Choice) {
            const int current = ram.s16(proc + 0x2a);
            if (current < 1 || current > 2) { cancel(); return 0; }
            output_ = target_x_ < 0 ? 2 : current < target_x_ ? 0x10 : current > target_x_ ? 0x20 : 1;
        } else {
            if (target_x_ == 8 && !context.skip) { cancel(); return 0; }
            output_ = target_x_;
        }
        if (ram.read(0x02024cc4, 2) & output_) { output_ = 0; return 0; }
        if (output_ & 0xb) pending_ = false;
        release_ = true;
        return output_;
    }


    if (context.kind == Kind::Guide) {
        const int state = ram.read(kGuideData + 0x2f, 1), sort = ram.read(kGuideData + 0x30, 1);
        const int category = ram.s8(kGuideData + 0x29), cat_head = ram.s8(kGuideData + 0x2a);
        const int entry = ram.s8(kGuideData + 0x2b), head = ram.s8(kGuideData + 0x2c);
        const int categories = ram.read(kGuideData + (sort ? 0x3c : 0x3d), 1);
        const int entries = ram.read(kGuideData + 0x3e, 1);
        const int line = ram.s8(kGuideData + 0x2e), lines = ram.read(kGuideData + 0x3f, 1);
        if (state > 2 || sort > 1 || categories < 1 || categories > 20 || entries > 20 ||
            category < 0 || category >= categories || cat_head < 0 || cat_head > category || category - cat_head > 5 ||
            entry < 0 || (entries && entry >= entries) || head < 0 || head > entry || entry - head > 5 ||
            line < 0 || lines > 127 || (state == 2 && (!entries || line > std::max(0, lines - 4)))) {
            cancel(); return 0;
        }
        if (context.camera_wait) { output_ = 0; return 0; }
        // Plans may cross native list states only when the click explicitly
        // requested another pane. Opening an article consumes its A pulse.
        if (input.primary || input.secondary || (input.wheel && !input.pointer_down)) {
            const bool held = release_ || output_ != 0;
            const auto ready = screen_ready_;
            cancel(); release_ = held; screen_ready_ = ready;
            target_x_ = -1; target_y_ = 5; target_token_ = category;
            if (input.secondary) target_x_ = 2;
            else if (input.primary) {
                const int x = input.x, y = input.y;
                if (x >= 176 && y >= 15 && y < 29) target_x_ = 2;
                else if (!state && x >= 176 && y >= 3 && y < 15) target_x_ = 4;
                else if (x >= 16 && x < 72 && y >= 40 && y < 136) {
                    target_x_ = cat_head + (y - 40) / 16; target_y_ = 0;
                    if (target_x_ >= categories) target_x_ = -1;
                } else if (state < 2 && x >= 88 && x < 232 && y >= 40 && y < 136) {
                    target_x_ = head + (y - 40) / 16; target_y_ = 1;
                    if (target_x_ >= entries) target_x_ = -1;
                } else if (state == 2 && x >= 88 && x < 232 && y >= 56 && y < 136) {
                    target_x_ = y < 64 ? 0x40 : y >= 128 ? 0x80 : 1;
                } else if (!state && x >= 24 && x < 48 && ((y >= 32 && y < 40) || (y >= 136 && y < 144))) {
                    target_x_ = std::clamp(category + (y < 40 ? -1 : 1), 0, categories - 1); target_y_ = 2;
                } else if (state == 1 && entries && x >= 232 && y >= 40 && y < 136) {
                    target_x_ = std::clamp(entry + (y < 88 ? -1 : 1), 0, entries - 1); target_y_ = 3;
                }
            } else {
                const int x = input.wheel_x, y = input.wheel_y;
                const int delta = -std::clamp(input.wheel, -8, 8);
                if (x >= 16 && x < 72 && y >= 32 && y < 144) {
                    target_x_ = std::clamp(category + delta, 0, categories - 1); target_y_ = 2;
                } else if (x >= 80 && x < 240 && y >= 40 && y < 136) {
                    target_y_ = state == 2 ? 4 : 3;
                    target_x_ = state == 2 ? std::clamp(line + delta, 0, std::max(0, lines - 4)) :
                        entries ? std::clamp(entry + delta, 0, entries - 1) : -1;
                }
            }
            if (target_x_ >= 0) { pending_ = true; started_ = input.frame; }
        }
        if (!new_frame) return output_;
        output_ = 0;
        if (release_) { release_ = false; return 0; }
        if (!pending_) return 0;
        if (input.frame - started_ > 600) { cancel(); return 0; }
        if (input.frame < screen_ready_) return 0;
        if (target_y_ == 0 || target_y_ == 2) {
            if (target_x_ >= categories) { cancel(); return 0; }
            if (state) output_ = 2;
            else if (category != target_x_) output_ = category < target_x_ ? 0x80 : 0x40;
            else pending_ = false;
        } else if (target_y_ == 1 || target_y_ == 3) {
            if (category != int(target_token_) || target_x_ >= entries || state == 2) { cancel(); return 0; }
            if (!state) output_ = 1;
            else if (entry != target_x_) output_ = entry < target_x_ ? 0x80 : 0x40;
            else { output_ = target_y_ == 1 ? 1 : 0; pending_ = false; }
        } else if (target_y_ == 4) {
            if (state != 2 || category != int(target_token_)) { cancel(); return 0; }
            if (line != target_x_) output_ = line < target_x_ ? 0x80 : 0x40;
            else pending_ = false;
        } else { output_ = target_x_; pending_ = false; }
        if (output_ && (ram.read(0x02024cc4, 2) & output_)) { output_ = 0; pending_ = true; return 0; }
        if (output_) { release_ = true; screen_ready_ = input.frame + 3; }
        return output_;
    }

    if (screen) {
        const bool list = context.kind == Kind::UnitList, config = context.kind == Kind::Config;
        const bool music = context.kind == Kind::Music;
        const int state = list ? int(ram.read(proc + 0x29, 1)) : 0;
        const int page = list ? int(ram.read(proc + 0x2f, 1)) : 0;
        const int count = list ? int(ram.read(0x0200f158, 1)) : music ? int(ram.read(proc + 0x36, 1)) : 0;
        const int head = config ? ram.s16(0x020201b4) : 0;
        const int current = list ? int(ram.read(proc + 0x30, 1)) : config ? ram.s16(0x020201b2) : music ? int(ram.read(proc + 0x35, 1)) : 0;
        if ((list && (page > 6 || state > 3 || count < 1 || count > 64 || current >= count ||
                      ram.read(proc + 0x2c, 1) > 5 || ram.read(proc + 0x39, 1) > 5 ||
                      ram.read(proc + 0x2e, 1) > 6 || ram.read(proc + 0x2d, 1) > 8)) ||
            (config && (ram.s16(0x020201bc) != 13 || current < 0 || current >= 13 || head < 0 || head > 7)) ||
            (music && (count < 1 || count > 128 || current >= count))) {
            cancel(); return 0;
        }
        const bool moving = (list && (state == 1 || state == 2 || ram.read(proc + 0x3e, 2) % 16)) ||
            (config && ram.read(proc + 0x30, 2)) ||
            (music && ram.read(proc + 0x37, 1)) ||
            (context.kind == Kind::Stats && ram.read(0x02003c04, 1));
        if (moving) screen_ready_ = input.frame + 2;
        if (input.secondary) {
            const auto ready = screen_ready_;
            cancel(); cancel_pending_ = true; started_ = input.frame;
            screen_ready_ = ready;
        } else if (input.primary) {
            const bool held = release_ || output_ != 0;
            const auto ready = screen_ready_;
            cancel();
            release_ = held;
            screen_ready_ = ready;
            if (moving || input.x < 0 || input.x >= 240 || input.y < 0 || input.y >= 160) return 0;
            target_x_ = -1; target_y_ = 0; target_token_ = 0;
            const int x = input.x, y = input.y;
            if (config) {
                if (x >= 16 && x < 224 && y >= 40 && y < 136) {
                    target_x_ = head + (y - 40) / 16; target_y_ = -1;
                    if (target_x_ < 13) {
                        const auto& o = kOptions[target_x_];
                        for (int i = 0; i < 4; ++i)
                            if (o.width[i] && x >= o.x[i] && x < o.x[i] + o.width[i]) target_y_ = i;
                        if (!target_x_ && !current && x >= 192 && x < 208 && option_value(ram, 0) == 3) target_y_ = 4;
                    }
                } else if (x >= 224 && y >= 40 && y < 136) {
                    target_x_ = std::clamp(current + (y < 88 ? -1 : 1), 0, 12); target_y_ = -1;
                }
            } else if (list && !ram.read(proc + 0x2b, 1)) {
                if (x >= 16 && x < 224 && y >= 58 && y < 152) {
                    target_x_ = int(ram.read(proc + 0x3e, 2)) / 16 + (y - 56) / 16;
                    target_y_ = x < 64 ? 0 : 1;
                    target_token_ = listed_unit(ram, target_x_);
                    if (!target_token_) target_x_ = -1;
                } else if (y >= 40 && y < 56) {
                    if (x < 16 && page > 1) { target_x_ = 0x20; target_y_ = 3; }
                    else if (x >= 224 && page < int(ram.read(proc + 0x2e, 1)) && page) { target_x_ = 0x10; target_y_ = 3; }
                    else for (int i = 0; i < 9 && kColumns[page][i]; ++i) {
                        const int end = i == 8 || !kColumns[page][i + 1] ? 224 : kColumns[page][i + 1];
                        if (x >= kColumns[page][i] && x < end) { target_x_ = i; target_y_ = 2; }
                    }
                } else if (x >= 224 && y >= 58 && y < 152) {
                    target_x_ = std::clamp(current + (y < 105 ? -1 : 1), 0, count - 1);
                    target_y_ = 4;
                }
            } else if (music && x >= 96 && x < 224 && y >= 64 && y < 144) {
                target_x_ = (int(ram.read(proc + 0x2a, 2)) / 16 + (y - 64) / 16) * 4 + (x - 96) / 32;
                if (target_x_ >= count) target_x_ = -1;
            } else if (context.kind == Kind::Stats && ram.read(0x02003bfd, 1) > 0 && ram.read(0x02003bfd, 1) <= 3) {
                if (y < 20 && x >= 96 && x < 120) target_x_ = 0x20;
                else if (y < 20 && x >= 208 && x < 232) target_x_ = 0x10;
            } else if (context.kind == Kind::Chapter) {
                if (y >= 36 && y < 60 && x < 120) { target_x_ = x < 56 ? 0 : 1; }
                else if (x >= 128 && x < 224 && y >= 60 && y < 108) {
                    target_x_ = ram.read(proc + 0x2e, 1); target_y_ = 1;
                }
            }
            if (target_x_ < 0 || (config && target_x_ >= 13)) return 0;
            pending_ = true; started_ = input.frame;
        } else if (input.wheel && !input.pointer_down &&
                   ((config && input.wheel_x >= 16 && input.wheel_x < 240 && input.wheel_y >= 40 && input.wheel_y < 136) ||
                    (list && !ram.read(proc + 0x2b, 1) && input.wheel_x >= 16 && input.wheel_x < 240 && input.wheel_y >= 56 && input.wheel_y < 152) ||
                    (music && input.wheel_x >= 96 && input.wheel_x < 240 && input.wheel_y >= 64 && input.wheel_y < 144))) {
            const int delta = -std::clamp(input.wheel, -8, 8);
            const int origin = pending_ && target_y_ == (config ? -1 : 4) ? target_x_ : current;
            target_x_ = std::clamp(origin + delta * (music ? 4 : 1), 0, config ? 12 : count - 1);
            target_y_ = config ? -1 : 4;
            pending_ = true; cancel_pending_ = false; started_ = input.frame;
        }
        if (!new_frame) return output_;
        output_ = 0;
        if (release_) { release_ = false; return 0; }
        if (input.frame - started_ > 300) { cancel(); return 0; }
        if (moving) { screen_ready_ = input.frame + 2; return 0; }
        if (input.frame < screen_ready_) return 0;
        if (cancel_pending_) { output_ = 2; cancel_pending_ = false; }
        else if (pending_) {
            if (config) {
                if (current != target_x_) output_ = current < target_x_ ? 0x80 : 0x40;
                else if (target_y_ < 0) pending_ = false;
                else {
                    const int value = option_value(ram, current);
                    if (value < 0) { cancel(); return 0; }
                    if (target_y_ == 4) { output_ = value == 3 ? 1 : 0; pending_ = false; }
                    else if (value == target_y_) pending_ = false;
                    else output_ = value < target_y_ ? 0x10 : 0x20;
                }
            } else if (list) {
                if (ram.read(proc + 0x2b, 1)) { cancel(); return 0; }
                if (target_y_ == 2) {
                    if (state != 3) output_ = 0x40;
                    else {
                        const int col = ram.read(proc + 0x2d, 1);
                        output_ = col < target_x_ ? 0x10 : col > target_x_ ? 0x20 : 1;
                        if (output_ == 1) pending_ = false;
                    }
                } else if (target_y_ == 3) {
                    if (state == 3) output_ = 0x80;
                    else { output_ = target_x_; pending_ = false; }
                } else {
                    if (target_y_ != 4 && listed_unit(ram, target_x_) != target_token_) { cancel(); return 0; }
                    if (state == 3) output_ = 0x80;
                    else if (current != target_x_) output_ = current < target_x_ ? 0x80 : 0x40;
                    else { output_ = target_y_ == 4 ? 0 : target_y_ == 1 ? 0x100 : 1; pending_ = false; }
                }
            } else if (music) {
                const bool partial_last_row = target_x_ / 4 == (count - 1) / 4 && (current & 3) > ((count - 1) & 3);
                if (current / 4 != target_x_ / 4 && !partial_last_row) output_ = current < target_x_ ? 0x80 : 0x40;
                else if ((current & 3) != (target_x_ & 3)) output_ = (current & 3) < (target_x_ & 3) ? 0x10 : 0x20;
                else { output_ = target_y_ == 4 ? 0 : 1; pending_ = false; }
            } else if (context.kind == Kind::Stats) { output_ = target_x_; pending_ = false; }
            else if (context.kind == Kind::Chapter) {
                const int faction = ram.read(proc + 0x2e, 1);
                if (faction > 1) { cancel(); return 0; }
                if (faction != target_x_) output_ = faction < target_x_ ? 0x10 : 0x20;
                else { output_ = target_y_ ? 1 : 0; pending_ = false; }
            }
        }
        if (output_) {
            release_ = true;
            screen_ready_ = input.frame + 3;
        }
        return output_;
    }

    if (front) {
        const bool confirm_box = context.kind == Kind::Slots && ram.read(proc + 0x36, 1) != 0;
        int current = 0, count = 1, hit = -1;
        if (context.kind == Kind::Extras) {
            count = ram.read(proc + 0x33, 1); current = ram.read(proc + 0x34, 1);
            const int step = count == 7 ? 21 : 25;
            const int top = count == 7 ? 2 : std::max(2, 68 - count * 12);
            const int left = 268 - int(ram.read(proc + 0x46, 2));
            if (input.x >= left && input.x < std::min(240, left + 160) && input.y >= top)
                hit = (input.y - top) / step;
        } else if (context.kind == Kind::MainList) {
            count = int(ram.read(proc + 0x31, 1)); current = int(ram.read(proc + 0x2b, 1));
            const int offset = int(ram.read(proc + 0x2f, 1) + ram.read(proc + 0x46, 2));
            const int top = std::max(2, 68 - count * 25 / 2);
            if (input.x >= 48 - offset && input.x < 208 - offset && input.y >= top)
                hit = (input.y - top) / 25;
        } else if (context.kind == Kind::Difficulty) {
            count = 3; current = int(ram.read(proc + 0x30, 1));
            if (input.x >= 8 && input.x < 136 && input.y >= 48 && input.y < 144)
                hit = (input.y - 48) / 32;
        } else if (context.kind == Kind::Slots) {
            if (confirm_box) {
                count = 2; current = int(ram.read(proc + 0x36, 1)) - 1;
                if (input.y >= 136 && input.y < 152 && input.x >= 56 && input.x < 144)
                    hit = (input.x - 56) / 44;
            } else {
                count = 3; current = int(ram.read(proc + 0x2c, 1));
                const int left = 248 - int(ram.read(proc + 0x2f, 1));
                if (input.x >= left && input.x < left + 176 && input.y >= 32 && input.y < 128)
                    hit = (input.y - 32) / 32;
                // Never navigate towards a slot the native menu skips.
                if (hit >= 0 && hit < 3) {
                    const auto action = ram.read(proc + 0x42, 2);
                    const bool filled = ram.read(proc + 0x37 + hit, 1) != 0xff;
                    // The native between-chapter action accepts empty slots too.
                    if (action != 0x40 && ((action == 0x10 && filled) || (action != 0x10 && !filled))) hit = -1;
                }
            }
        } else if (input.x >= 0 && input.x < 240 && input.y >= 0 && input.y < 160) hit = 0;
        if (count < 1 || count > 8 || current < 0 || current >= count) { cancel(); return 0; }
        if (input.secondary) { cancel(); cancel_pending_ = true; }
        else if (input.primary) {
            if (hit < 0 || hit >= count) { cancel(); return 0; }
            target_x_ = hit; pending_ = true; cancel_pending_ = false; started_ = input.frame;
        }
        if (!new_frame) return output_;
        output_ = 0;
        if (release_) { release_ = false; return 0; }
        if (context.kind == Kind::Slots && ram.read(proc + 0x40, 1) > 0 &&
            ram.read(proc + 0x40, 1) < 8) return 0;
        if (cancel_pending_) { cancel_pending_ = false; output_ = 2; }
        else if (pending_) {
            if (input.frame - started_ > 120) { cancel(); return 0; }
            output_ = current < target_x_ ? (confirm_box ? 0x10 : 0x80) :
                current > target_x_ ? (confirm_box ? 0x20 : 0x40) :
                context.kind == Kind::Continue ? 8 : 1;
            if (output_ == 1 && (ram.read(0x02024cc4, 2) & 1)) output_ = 0;
            if (output_ == 1 || output_ == 8) pending_ = false;
            if (output_ == 1 && context.kind == Kind::Difficulty) {
                const auto sprites = ram.read(proc + 0x34, 4);
                if (ram.contains(sprites, 0x2a) && (ram.read(sprites + 0x29, 1) & 2)) {
                    output_ = 0; pending_ = true;
                }
            }
        }
        if (output_) release_ = true;
        return output_;
    }

    if (context.kind != Kind::Map) {
        if (context.camera_wait) { output_ = 0; return 0; }
        if (input.secondary) {
            if (context.kind == Kind::Trade && context.identity != 0 && context.identity != 8) { cancel(); return 0; }
            cancel(); cancel_pending_ = true;
            proc_ = proc; mode_ = mode; identity_ = context.identity;
            started_ = input.frame;
        } else if (input.primary && input.x >= 0 && input.x < 240 && input.y >= 0 && input.y < 160) {
            int x = -1, y = 0;
            std::uint32_t token = 0;
            bool confirm = true;
            if (context.kind == Kind::Menu) {
                x = menu_hit(ram, proc, input.x, input.y);
                if (x >= 0) token = menu_item(ram, proc, x);
            } else if (context.kind == Kind::Target) {
                const int camera_x = ram.s16(kBm + 0xc), camera_y = ram.s16(kBm + 0xe);
                if (camera_x < 0 || camera_y < 0) { cancel(); return 0; }
                const int tx = (input.x + camera_x) / 16, ty = (input.y + camera_y) / 16;
                const auto current = ram.read(proc + 0x30, 4);
                const TargetRing ring(ram, current);
                if (ring.valid) for (unsigned i = 0; i < ring.count; ++i) {
                    const auto node = ring.nodes[i];
                    if (ram.s8(node) == tx && ram.s8(node + 1) == ty) {
                        x = tx; y = ty; token = node;
                        confirm = node == current && preview_token_ == node;
                        if (!confirm) preview_token_ = 0;
                        break;
                    }
                }
            } else if (context.kind == Kind::Trade) {
                x = input.x >= 16 && input.x < 112 ? 0 :
                    input.x >= 128 && input.x < 224 ? 1 : -1;
                y = input.y >= 72 && input.y < 152 ? (input.y - 72) / 16 : -1;
                if (!trade_cell(ram, proc, x, y)) x = -1;
                const int tc = ram.read(proc + 0x41, 1), tr = ram.read(proc + 0x42, 1);
                if (context.identity == 2 && x != 1) x = -1;
                if (context.identity == 3) {
                    const auto unit = ram.read(proc + 0x2c + 4, 4);
                    if (x != 1 || y < 0 || !ram.contains(unit, 0x28) ||
                        ram.read(unit + 0x1e + y * 2, 1) != 0x6c) x = -1;
                }
                if (context.identity == 5 && (x != tc || y != tr)) x = -1;
                if (context.identity == 8) x = -1;
            }
            if (x < 0) { cancel(); return 0; }
            target_x_ = x; target_y_ = y; target_token_ = token;
            confirm_target_ = confirm;
            proc_ = proc; mode_ = mode; identity_ = context.identity;
            pending_ = true; cancel_pending_ = false; started_ = input.frame;
        }
        if (!new_frame) return output_;
        output_ = 0;
        if (release_) { release_ = false; return 0; }
        if (!pending_ && !cancel_pending_) return 0;
        if (input.frame - started_ > 600) { cancel(); return 0; }
        if (cancel_pending_) { output_ = 2; cancel_pending_ = false; }
        else if (context.kind == Kind::Menu) {
            const auto item = menu_item(ram, proc, target_x_);
            if (!item || item != target_token_ || !menu_selectable(ram, proc, item)) { cancel(); return 0; }
            const int current = int(ram.read(proc + 0x61, 1));
            output_ = current < target_x_ ? 0x80 : current > target_x_ ? 0x40 : 1;
            // Opening a menu can clear newKeys before heldKeys observes the
            // release. Wait for native key-up rather than merging two A taps.
            if (output_ == 1 && (ram.read(0x02024cc4, 2) & 1)) output_ = 0;
        } else if (context.kind == Kind::Target) {
            const auto current = ram.read(proc + 0x30, 4);
            const TargetRing ring(ram, current);
            unsigned index = 0;
            while (index < ring.count && ring.nodes[index] != target_token_) ++index;
            if (!ring.valid || index == ring.count || ram.s8(target_token_) != target_x_ ||
                ram.s8(target_token_ + 1) != target_y_) { cancel(); return 0; }
            if (index == 0) {
                pending_ = false;
                output_ = confirm_target_ ? 1 : 0;
                preview_token_ = confirm_target_ ? 0 : target_token_;
            } else output_ = index <= ring.count / 2 ? 0x40 : 0x80;
        } else {
            const int column = int(ram.read(proc + 0x41, 1)), row = int(ram.read(proc + 0x42, 1));
            if (column > 1 || row > 4 || !trade_cell(ram, proc, target_x_, target_y_)) { cancel(); return 0; }
            if (column != target_x_) output_ = target_x_ == 0 ? 0x20 : 0x10;
            else output_ = row < target_y_ ? 0x80 : row > target_y_ ? 0x40 : 1;
        }
        if (output_ == 1) pending_ = false;
        if (output_) release_ = true;
        return output_;
    }

    const int width = ram.s16(kMapSize), height = ram.s16(kMapSize + 2);
    const int cx = ram.s16(kBm + 0x14), cy = ram.s16(kBm + 0x16);
    const int camera_x = ram.s16(kBm + 0x0c), camera_y = ram.s16(kBm + 0x0e);
    const auto ignored = ram.read(0x03000010, 2);
    if (width <= 0 || height <= 0 || width > 64 || height > 64 ||
        cx < 0 || cy < 0 || cx >= width || cy >= height || camera_x < 0 || camera_y < 0) {
        cancel(); return 0;
    }
    if (input.dragging) {
        if (!new_frame) return 0;
        const bool owned = drag_owned_ || input.drag_start;
        const int origin_x = detached_camera_ ? camera_x_ : camera_x;
        const int origin_y = detached_camera_ ? camera_y_ : camera_y;
        cancel();
        if (!owned || mode == kForcedMove || (ignored & 0xf0)) return 0;
        drag_owned_ = true;
        const int max_x = std::min(std::max(0, width * 16 - 240), std::max(0, ram.s16(kBm + 0x28)));
        const int max_y = std::min(std::max(0, height * 16 - 160), std::max(0, ram.s16(kBm + 0x2a)));
        camera_x_ = std::clamp(origin_x + std::clamp(input.drag_dx, -480, 480), 0, max_x);
        camera_y_ = std::clamp(origin_y + std::clamp(input.drag_dy, -320, 320), 0, max_y);
        detached_camera_ = true;
        return 0;
    }
    if (input.secondary) {
        if (mode == kForcedMove || (ignored & 2)) { cancel(); return 0; }
        cancel();
        cancel_pending_ = true;
        proc_ = proc;
        mode_ = mode;
        identity_ = context.identity;
        started_ = input.frame;
    } else if (input.primary && input.x >= 0 && input.x < 240 && input.y >= 0 && input.y < 160) {
        int x = (input.x + camera_x) / 16;
        int y = (input.y + camera_y) / 16;
        if (x >= width || y >= height) { cancel(); return 0; }
        if (mode == kForcedMove) {
            const auto unit = ram.read(0x03004e50, 4);
            if (!ram.contains(unit, 0x14)) { cancel(); return 0; }
            const int ux = ram.s8(unit + 0x10), uy = ram.s8(unit + 0x11);
            if (ux < 0 || uy < 0 || ux >= width || uy >= height ||
                !((x == cx && y == cy) || (x == ux && y == uy))) { cancel(); return 0; }
            // Only recover a rejected occupied destination. Other guided
            // moves use the same callback and must retain their chosen tile.
            const auto rows = ram.read(0x0202e4d8, 4);
            if (!ram.contains(rows, height * 4)) { cancel(); return 0; }
            const auto row = ram.read(rows + cy * 4, 4);
            if (!ram.contains(row, width) || !ram.read(row + cx, 1) ||
                ram.read(row + cx, 1) == ram.read(unit + 0xb, 1)) { cancel(); return 0; }
            x = ux; y = uy;
        } else if ((ignored & 1) || (x > cx && (ignored & 0x10)) ||
                   (x < cx && (ignored & 0x20)) || (y < cy && (ignored & 0x40)) ||
                   (y > cy && (ignored & 0x80))) { cancel(); return 0; }
        target_x_ = x;
        target_y_ = y;
        proc_ = proc;
        mode_ = mode;
        identity_ = context.identity;
        started_ = input.frame;
        pending_ = true;
        cancel_pending_ = false;
        // A visible tile click must not make native cursor-follow chase the
        // newly teleported cursor. Edge hover owns mouse camera movement.
        camera_x_ = std::clamp(camera_x, 0, std::min(std::max(0, width * 16 - 240), std::max(0, ram.s16(kBm + 0x28))));
        camera_y_ = std::clamp(camera_y, 0, std::min(std::max(0, height * 16 - 160), std::max(0, ram.s16(kBm + 0x2a))));
        detached_camera_ = true;
    }
    int dx = 0, dy = 0;
    if (!pending_ && !cancel_pending_ && !input.primary && !input.secondary &&
        mode != kForcedMove && !(ignored & 0xf0) &&
        !input.pointer_down && input.hover_x >= 0 && input.hover_x < 240 &&
        input.hover_y >= 0 && input.hover_y < 160) {
        auto speed = [](int value, int extent) {
            if (value < 16) return -(1 + (15 - value) / 4);
            if (value >= extent - 16) return 1 + (value - (extent - 16)) / 4;
            return 0;
        };
        dx = speed(input.hover_x, 240);
        dy = speed(input.hover_y, 160);
    }
    if (!new_frame) return output_;
    output_ = 0;
    if (release_) { release_ = false; return 0; }
    if (!pending_ && !cancel_pending_) {
        if (dx || dy) {
            const int max_x = std::min(std::max(0, width * 16 - 240), std::max(0, ram.s16(kBm + 0x28)));
            const int max_y = std::min(std::max(0, height * 16 - 160), std::max(0, ram.s16(kBm + 0x2a)));
            camera_x_ = std::clamp(camera_x + dx, 0, max_x);
            camera_y_ = std::clamp(camera_y + dy, 0, max_y);
            detached_camera_ = true;
        }
        return output_;
    }
    if (input.frame - started_ > 600) { cancel(); return 0; }
    if (pending_ && mode != kForcedMove && ((ignored & 1) ||
        (target_x_ > cx && (ignored & 0x10)) || (target_x_ < cx && (ignored & 0x20)) ||
        (target_y_ < cy && (ignored & 0x40)) || (target_y_ > cy && (ignored & 0x80)))) {
        cancel(); return 0;
    }

    if (cancel_pending_) {
        if (ram.s16(kBm + 0x20) != cx * 16 || ram.s16(kBm + 0x22) != cy * 16) return 0;
        output_ = 2; cancel_pending_ = false;
    } else {
        if (cx != target_x_ || cy != target_y_ ||
            ram.s16(kBm + 0x20) != target_x_ * 16 || ram.s16(kBm + 0x22) != target_y_ * 16) {
            // The hook places the cursor during the native map update. A is
            // withheld until a later frame, after FE8 rebuilds its path arrow.
            jump_pending_ = true;
            return 0;
        }
        jump_pending_ = false;
        if (mode == kForcedMove) { pending_ = false; return 0; }
        output_ = 1; pending_ = false;
    }
    release_ = true;
    return output_;
}

int map_pointer_get(const char* key, int* value) {
    if (std::strcmp(key, kSetting)) return 0;
    *value = map_pointer.enabled ? 1 : 0;
    return 1;
}
int map_pointer_set(const char* key, int value) {
    if (std::strcmp(key, kSetting)) return 0;
    map_pointer.enabled = value != 0;
    map_pointer.cancel();
    return 1;
}
}

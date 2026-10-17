#include "touch_pointer.h"
#include "map_pointer.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>

using namespace gbarecomp;
static void check(bool ok, const char* text) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", text); std::exit(1); }
}
int main() {
    sacredstones::TouchPointer touch;
    TouchFrameInfo frame;
    frame.frame_count = 10;
    Gesture g;
    g.kind = GestureKind::Tap; g.start_in_view = true; g.view_x = 56; g.view_y = 72;
    frame.gestures = &g; frame.gesture_count = 1;
    auto input = touch.translate(frame);
    check(input.primary && !input.secondary && input.x == 56 && input.y == 72, "tap becomes single primary click");
    std::array<std::uint8_t, 256 * 1024> ram{};
    std::array<std::uint8_t, 32 * 1024> iwram{};
    auto put = [&](unsigned a, unsigned v, unsigned n) {
        for (unsigned i = 0; i < n; ++i) {
            if (a >= 0x03000000) iwram.at(a - 0x03000000 + i) = v >> (i * 8);
            else ram.at(a - 0x02000000 + i) = v >> (i * 8);
        }
    };
    put(0x02026a70, 0x02001000, 4); put(0x02001000, 0x0859aad8, 4);
    put(0x0200100c, 0x0801c941, 4); put(0x0202e4d4, 32, 2); put(0x0202e4d6, 32, 2);
    put(0x0202bcc4, 3, 2); put(0x0202bcc6, 4, 2);
    put(0x0202bcd0, 48, 2); put(0x0202bcd2, 64, 2);
    input.ewram = ram.data(); input.ewram_size = ram.size();
    input.iwram = iwram.data(); input.iwram_size = iwram.size();
    sacredstones::MapPointer pointer; pointer.enabled = true;
    const auto before = ram;
    check(pointer.update(input) == 1 && ram == before, "touch uses read-only production map selection");
    input.primary = false; ++input.frame;
    check(pointer.update(input) == 0, "held gesture never repeats A");
    pointer.cancel(); put(0x02001000, 0x08a3d748, 4); put(0x0200100c, 0x080b9805, 4);
    put(0x03005281, 1, 1); put(0x03005288, 56 << 8, 4); put(0x0300528c, 72 << 8, 4);
    input.primary = true; ++input.frame;
    check(pointer.update(input) == 1, "same touch policy selects a native world-map location");
    pointer.cancel(); put(0x02001000, 0x08a203a8, 4); put(0x0200100c, 0x080a9495, 4);
    put(0x0200102c, 0, 1); put(0x0200102f, 220, 1); put(0x02001037, 0xff, 1); put(0x02001042, 0x40, 2);
    g.view_x = 80; g.view_y = 36; ++frame.frame_count;
    auto chapter_input = touch.translate(frame);
    chapter_input.ewram = ram.data(); chapter_input.ewram_size = ram.size();
    chapter_input.iwram = iwram.data(); chapter_input.iwram_size = iwram.size();
    check(pointer.update(chapter_input) == 1, "touch selects an empty between-chapter save slot through common policy");
    pointer.cancel(); put(0x02001000, 0x08b12c64, 4); put(0x0200100c, 0x080cefd5, 4);
    put(0x020201b7, 0, 1); put(0x020201b8, 0, 1);
    put(0x020201c5, 8, 1); put(0x020201c6, 10, 1);
    g.view_x = 30; g.view_y = 76; ++frame.frame_count;
    auto guide_input = touch.translate(frame);
    guide_input.ewram = ram.data(); guide_input.ewram_size = ram.size();
    guide_input.iwram = iwram.data(); guide_input.iwram_size = iwram.size();
    const auto guide_before = ram;
    check(pointer.update(guide_input) == 0x80 && ram == guide_before, "touch selects Guide categories through read-only native policy");
    pointer.cancel(); put(0x020201b7, 1, 1);
    g.kind = GestureKind::Swipe; g.swipe = SwipeDirection::Up; g.view_x = 160; g.view_y = 80; ++frame.frame_count;
    guide_input = touch.translate(frame);
    guide_input.ewram = ram.data(); guide_input.ewram_size = ram.size();
    guide_input.iwram = iwram.data(); guide_input.iwram_size = iwram.size();
    check(pointer.update(guide_input) == 0x80, "touch swipe scrolls Guide articles without opening one");
    frame.gesture_count = 0;
    check(!touch.translate(frame).primary, "pad-owned fingers produce no map click");
    g.kind = GestureKind::TwoFingerTap; frame.gesture_count = 1;
    input = touch.translate(frame);
    check(input.secondary && !input.primary, "two-finger tap is B not A");
    g.start_in_view = false;
    check(!touch.translate(frame).secondary, "outside-image start cannot cancel or select");
    g.start_in_view = true; g.view_x = std::numeric_limits<float>::quiet_NaN();
    check(!touch.translate(frame).secondary, "nonfinite gesture ignored");
    g.view_x = 56; g.kind = GestureKind::Swipe; g.swipe = SwipeDirection::Up;
    input = touch.translate(frame);
    check(input.wheel == -3 && !input.primary, "vertical swipe scrolls without selecting");
    g.kind = GestureKind::Cancel;
    check(touch.translate(frame).reset, "cancel resets pending native hooks");
    g.kind = GestureKind::ThreeFingerTap;
    check(touch.translate(frame).reset, "three-finger menu gesture never confirms game");
    frame.gesture_count = 0; frame.host_keyinput = 0x3fe;
    check(touch.translate(frame).reset, "physical and virtual GBA keys take priority");
    frame.host_keyinput = 0x3ff; frame.point_count = 2;
    check(touch.translate(frame).reset, "multitouch cancels pending single-finger input");
    TouchPointSnapshot point;
    point.view_x = 235; point.view_y = 80; point.down_ms = 100;
    frame.points = &point; frame.point_count = 1; frame.host_ms = 219;
    check(touch.translate(frame).hover_x == -1, "short edge tap does not pan");
    frame.host_ms = 220; input = touch.translate(frame);
    check(input.hover_x == 235 && !input.pointer_down && !input.primary, "edge hold pans without selecting");
    frame.point_count = 0; g.kind = GestureKind::Tap; g.view_x = 235; frame.gesture_count = 1;
    check(!touch.translate(frame).primary, "releasing edge pan never selects underlying tile");
    touch.reset();
    check(touch.translate(frame).primary, "fresh edge tap remains selectable after reset");
    // Exercise the pinned engine's recognizer, not only handcrafted gestures.
    GestureRecognizer recognizer;
    std::vector<Gesture> gestures;
    TouchEvent event;
    event.pointer = 1; event.in_view = true;
    event.view_x = 56; event.view_y = 72; event.drawable_x = 224; event.drawable_y = 288;
    event.t_ms = 100; recognizer.on_event(event, gestures);
    event.phase = TouchPhase::Up; event.t_ms = 180; recognizer.on_event(event, gestures);
    frame.points = nullptr; frame.point_count = 0; frame.gestures = gestures.data(); frame.gesture_count = gestures.size();
    check(touch.translate(frame).primary, "engine-recognized tap activates production adapter");
    gestures.clear(); event.phase = TouchPhase::Down; event.t_ms = 200;
    recognizer.on_event(event, gestures); event.pointer = 2; event.t_ms = 230;
    recognizer.on_event(event, gestures); event.phase = TouchPhase::Up; event.t_ms = 280;
    recognizer.on_event(event, gestures); event.pointer = 1; event.t_ms = 290;
    recognizer.on_event(event, gestures);
    frame.gestures = gestures.data(); frame.gesture_count = gestures.size();
    input = touch.translate(frame);
    check(input.secondary && !input.primary, "real two-finger recognizer never emits an attack A");
    gestures.clear(); event.phase = TouchPhase::Down; event.t_ms = 400;
    recognizer.on_event(event, gestures); recognizer.cancel_all(420, gestures);
    gestures.clear(); event.phase = TouchPhase::Up; event.t_ms = 450;
    recognizer.on_event(event, gestures);
    check(gestures.empty(), "release after menu/state reset cannot become a new tap");
    std::puts("PASS: touch translation, native map selection, cancellation, edge hold, swipe and ownership");
}

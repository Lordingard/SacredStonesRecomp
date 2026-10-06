#include "gba_ppu.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>

void store16(unsigned char* destination, unsigned short value) {
    destination[0] = value & 255;
    destination[1] = value >> 8;
}

int main() {
    std::array<unsigned char, 0x400> io{}, oam{}, palette{};
    std::array<unsigned char, 0x18000> vram{};
    std::array<unsigned char, gba::GbaPpu::kFramebufferBytes> rgb{};
    gba::GbaPpu ppu;
    for (int i = 0; i < 128; ++i) store16(oam.data() + i * 8, 0x0200);
    store16(oam.data(), 0);
    store16(palette.data(), 0x7c00); // Blue backdrop, red foreground sprite.
    store16(palette.data() + 0x202, 0x001f);
    std::fill_n(vram.begin() + 0x10000, 32, 0x11);
    store16(io.data() + 0x52, 0x1000);
    store16(io.data() + 0x54, 16);
    const auto check = [&](unsigned short control, unsigned short object,
                           std::array<unsigned char, 3> expected, const char* label) {
        store16(io.data() + 0x50, control);
        store16(oam.data(), object);
        ppu.render(rgb.data(), 0x1000, io.data(), vram.data(), oam.data(), palette.data());
        if (!std::equal(expected.begin(), expected.end(), rgb.begin())) {
            std::fprintf(stderr, "%s: got %u,%u,%u expected %u,%u,%u\n", label,
                rgb[0], rgb[1], rgb[2], expected[0], expected[1], expected[2]);
            std::exit(1);
        }
    };
    check(0x00d0, 0, {0, 0, 0}, "ordinary sprite follows fade-to-black");
    check(0x0090, 0, {255, 255, 255}, "ordinary sprite follows fade-to-white");
    check(0x2050, 0, {0, 0, 255}, "ordinary first-target sprite follows alpha fade");
    check(0x2000, 0x0400, {0, 0, 255}, "semitransparent sprite forces alpha");
    check(0x00c0, 0, {255, 0, 0}, "unselected sprite does not follow brightness");
    std::puts("PASS: FE8 sprite alpha and brightness transition rules");
}

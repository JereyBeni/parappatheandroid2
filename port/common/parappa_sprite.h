#pragma once

#include <cstdint>
#include <vector>

// Tiny procedural Parappa-like sprite (no ROM needed).
// Classic look: cream dog face, big eyes, yellow beanie.

namespace ParappaSprite {

inline void fill_parappa_rgba(std::vector<uint8_t>& out, uint32_t& w, uint32_t& h) {
    // 32x40 logical pixels, scaled later by raster
    w = 32;
    h = 40;
    out.assign(static_cast<size_t>(w) * h * 4, 0);

    auto put = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        if (x < 0 || y < 0 || (uint32_t)x >= w || (uint32_t)y >= h) return;
        size_t i = (static_cast<size_t>(y) * w + static_cast<size_t>(x)) * 4;
        out[i] = r; out[i+1] = g; out[i+2] = b; out[i+3] = a;
    };

    auto fill = [&](int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
                put(x, y, r, g, b, a);
    };

    // transparent background already 0 alpha

    // Beanie (yellow / orange)
    fill(8, 2, 23, 10, 255, 200, 40);
    fill(6, 6, 25, 12, 255, 180, 30);
    // beanie tip
    fill(14, 0, 17, 3, 255, 160, 20);
    // spiral hint (darker yellow)
    for (int i = 0; i < 5; ++i)
        put(12 + i, 5 + (i % 2), 220, 140, 20);

    // Head (cream / white dog)
    fill(7, 11, 24, 26, 255, 245, 220);
    fill(9, 12, 22, 25, 255, 250, 230);

    // Snout
    fill(12, 20, 19, 26, 255, 250, 235);

    // Eyes (big black with white shine)
    fill(10, 15, 13, 19, 20, 20, 30);
    fill(18, 15, 21, 19, 20, 20, 30);
    put(11, 16, 255, 255, 255);
    put(19, 16, 255, 255, 255);

    // Nose
    fill(14, 21, 17, 23, 30, 30, 40);

    // Smile
    put(13, 25, 60, 40, 40);
    put(14, 26, 60, 40, 40);
    put(15, 26, 60, 40, 40);
    put(16, 26, 60, 40, 40);
    put(17, 25, 60, 40, 40);

    // Body (shirt / simple torso — blue-ish like classic)
    fill(10, 27, 21, 36, 80, 140, 220);
    fill(11, 28, 20, 35, 100, 160, 240);

    // Arms
    fill(6, 28, 9, 34, 255, 245, 220);
    fill(22, 28, 25, 34, 255, 245, 220);

    // Legs
    fill(11, 37, 14, 39, 255, 245, 220);
    fill(17, 37, 20, 39, 255, 245, 220);
}

// Stage-ish background strip colors (purple floor like PTR vibe)
inline void fill_stage_tile(std::vector<uint8_t>& out, uint32_t& w, uint32_t& h) {
    w = 16;
    h = 16;
    out.assign(static_cast<size_t>(w) * h * 4, 0);
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            size_t i = (y * w + x) * 4;
            bool stripe = ((x + y) / 4) & 1;
            out[i+0] = stripe ? 160 : 120;
            out[i+1] = stripe ? 80 : 50;
            out[i+2] = stripe ? 200 : 160;
            out[i+3] = 255;
        }
    }
}

} // namespace ParappaSprite

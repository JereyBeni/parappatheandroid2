#pragma once
#include <cstdint>

namespace GS {
enum class Psm : uint8_t {
    PSMCT32 = 0x00, PSMCT24 = 0x01, PSMCT16 = 0x02,
    PSMT8 = 0x13, PSMT4 = 0x14
};
enum class Prim : uint8_t {
    Point = 0, Line = 1, Triangle = 3, Sprite = 6
};
struct FrameBuffer {
    uint32_t fbp = 0, fbw = 0, width = 0, height = 0;
    Psm psm = Psm::PSMCT32;
};
struct TexEnv {
    uint32_t tbp0 = 0, tbw = 0, tw = 0, th = 0;
    Psm psm = Psm::PSMCT32;
    bool enabled = false;
};
struct GsState {
    FrameBuffer frame, zbuf;
    TexEnv tex;
    Prim prim = Prim::Sprite;
};
struct DrawRequest {
    Prim prim = Prim::Sprite;
    uint32_t vertex_count = 0;
    uint32_t tex_id = 0; // 0 = flat color
    struct V {
        float x, y, z, r, g, b, a, u, v;
    };
    static constexpr uint32_t kMaxVerts = 4096;
    V verts[kMaxVerts];
};
} // namespace GS

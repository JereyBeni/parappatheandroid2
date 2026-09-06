#pragma once

#include <cstdint>

// Minimal PS2 GS concepts for the translation layer.
// Not a full GSdx/paraLLEl-GS - just the surface we need to map to Vulkan.

namespace GS {

// Common GS pixel formats (subset)
enum class Psm : uint8_t {
    PSMCT32  = 0x00,
    PSMCT24  = 0x01,
    PSMCT16  = 0x02,
    PSMCT16S = 0x0A,
    PSMT8    = 0x13,
    PSMT4    = 0x14,
    PSMT8H   = 0x1B,
    PSMT4HL  = 0x24,
    PSMT4HH  = 0x2C,
    PSMZ32   = 0x30,
    PSMZ24   = 0x31,
    PSMZ16   = 0x32,
    PSMZ16S  = 0x3A,
};

// PRIM types
enum class Prim : uint8_t {
    Point    = 0,
    Line     = 1,
    LineStrip= 2,
    Triangle = 3,
    TriStrip = 4,
    TriFan   = 5,
    Sprite   = 6,
};

struct FrameBuffer {
    uint32_t fbp = 0;   // frame buffer base pointer (GS words)
    uint32_t fbw = 0;   // width / 64
    Psm psm = Psm::PSMCT32;
    uint32_t width = 0; // pixels (derived)
    uint32_t height = 0;
};

struct TexEnv {
    uint32_t tbp0 = 0;
    uint32_t tbw = 0;
    Psm psm = Psm::PSMCT32;
    uint32_t tw = 0; // log2 width
    uint32_t th = 0; // log2 height
    bool enabled = false;
};

struct GsState {
    FrameBuffer frame;
    FrameBuffer zbuf;
    TexEnv tex;
    Prim prim = Prim::Sprite;
    bool aa1 = false;
    uint8_t alpha = 0x80;
};

// One high-level draw request after translation from GS packets
struct DrawRequest {
    Prim prim = Prim::Sprite;
    uint32_t vertex_count = 0;
    // vertices as XYZ2 + RGBAQ simplified (host side)
    // x,y in GS space; z; r,g,b,a; u,v
    struct V {
        float x, y, z;
        float r, g, b, a;
        float u, v;
    };
    static constexpr uint32_t kMaxVerts = 4096;
    V verts[kMaxVerts];
};

} // namespace GS

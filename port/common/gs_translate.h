#pragma once

#include "gs_types.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// GS packet -> host draw list translator (software front-end).
// Full accuracy is a multi-year project; this is the pipeline skeleton.

namespace GS {

class Translator {
public:
    Translator() = default;

    // Feed raw GIF/GS path data (later: real PATH1/2/3 packets)
    // For now accepts a simplified command stream for testing.
    void reset();
    const GsState& state() const { return m_state; }

    // Test helpers used by the harness
    void set_frame(uint32_t width, uint32_t height, Psm psm);
    void set_tex(uint32_t w, uint32_t h, Psm psm);

    // Emit a simple textured sprite in GS space (for pipeline test)
    bool emit_sprite(float x0, float y0, float x1, float y1,
                     float u0, float v0, float u1, float v1);

    const std::vector<DrawRequest>& draws() const { return m_draws; }
    void clear_draws() { m_draws.clear(); }

    std::string debug_summary() const;

private:
    GsState m_state;
    std::vector<DrawRequest> m_draws;
};

} // namespace GS

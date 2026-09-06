#pragma once
#include "gs_types.h"
#include <string>
#include <vector>

namespace GS {
class Translator {
public:
    void reset();
    const GsState& state() const { return m_state; }
    void set_frame(uint32_t w, uint32_t h, Psm psm);
    void set_tex(uint32_t w, uint32_t h, Psm psm);
    bool emit_sprite(float x0, float y0, float x1, float y1,
                     float u0, float v0, float u1, float v1,
                     uint32_t tex_id = 1);
    bool emit_rect_flat(float x0, float y0, float x1, float y1,
                        float r, float g, float b, float a = 1.f);
    const std::vector<DrawRequest>& draws() const { return m_draws; }
    void clear_draws() { m_draws.clear(); }
    std::string debug_summary() const;
private:
    GsState m_state;
    std::vector<DrawRequest> m_draws;
};
} // namespace GS

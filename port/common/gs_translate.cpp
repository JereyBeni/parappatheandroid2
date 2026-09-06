#include "gs_translate.h"
#include <sstream>

namespace GS {

void Translator::reset() { m_state = {}; m_draws.clear(); }

void Translator::set_frame(uint32_t w, uint32_t h, Psm psm) {
    m_state.frame.width = w;
    m_state.frame.height = h;
    m_state.frame.psm = psm;
    m_state.frame.fbw = (w + 63) / 64;
}

void Translator::set_tex(uint32_t w, uint32_t h, Psm psm) {
    m_state.tex.enabled = true;
    m_state.tex.psm = psm;
    m_state.tex.tw = w;
    m_state.tex.th = h;
}

bool Translator::emit_sprite(float x0, float y0, float x1, float y1,
                             float u0, float v0, float u1, float v1,
                             uint32_t tex_id) {
    DrawRequest d;
    d.prim = Prim::Sprite;
    d.vertex_count = 4;
    d.tex_id = tex_id;
    auto put = [&](uint32_t i, float x, float y, float u, float v) {
        d.verts[i] = {x, y, 0, 1, 1, 1, 1, u, v};
    };
    put(0, x0, y0, u0, v0);
    put(1, x1, y0, u1, v0);
    put(2, x0, y1, u0, v1);
    put(3, x1, y1, u1, v1);
    m_draws.push_back(d);
    return true;
}

bool Translator::emit_rect_flat(float x0, float y0, float x1, float y1,
                                float r, float g, float b, float a) {
    DrawRequest d;
    d.prim = Prim::Sprite;
    d.vertex_count = 4;
    d.tex_id = 0;
    auto put = [&](uint32_t i, float x, float y) {
        d.verts[i] = {x, y, 0, r, g, b, a, 0, 0};
    };
    put(0, x0, y0);
    put(1, x1, y0);
    put(2, x0, y1);
    put(3, x1, y1);
    m_draws.push_back(d);
    return true;
}

std::string Translator::debug_summary() const {
    std::ostringstream o;
    o << "GS Translator frame=" << m_state.frame.width << "x" << m_state.frame.height
      << " draws=" << m_draws.size();
    return o.str();
}

} // namespace GS

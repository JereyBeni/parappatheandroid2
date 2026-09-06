#include "gs_translate.h"
#include <sstream>

namespace GS {

void Translator::reset() {
    m_state = GsState{};
    m_draws.clear();
}

void Translator::set_frame(uint32_t width, uint32_t height, Psm psm) {
    m_state.frame.width = width;
    m_state.frame.height = height;
    m_state.frame.psm = psm;
    m_state.frame.fbw = (width + 63) / 64;
}

void Translator::set_tex(uint32_t w, uint32_t h, Psm psm) {
    m_state.tex.enabled = true;
    m_state.tex.psm = psm;
    // store actual size in tw/th as raw for now (not log2)
    m_state.tex.tw = w;
    m_state.tex.th = h;
}

bool Translator::emit_sprite(float x0, float y0, float x1, float y1,
                            float u0, float v0, float u1, float v1) {
    DrawRequest d;
    d.prim = Prim::Sprite;
    d.vertex_count = 4;
    auto put = [&](uint32_t i, float x, float y, float u, float v) {
        d.verts[i].x = x; d.verts[i].y = y; d.verts[i].z = 0;
        d.verts[i].r = 1; d.verts[i].g = 1; d.verts[i].b = 1; d.verts[i].a = 1;
        d.verts[i].u = u; d.verts[i].v = v;
    };
    put(0, x0, y0, u0, v0);
    put(1, x1, y0, u1, v0);
    put(2, x0, y1, u0, v1);
    put(3, x1, y1, u1, v1);
    m_draws.push_back(d);
    return true;
}

std::string Translator::debug_summary() const {
    std::ostringstream o;
    o << "GS Translator\n";
    o << "  frame: " << m_state.frame.width << "x" << m_state.frame.height << "\n";
    o << "  tex:   " << (m_state.tex.enabled ? "on" : "off");
    if (m_state.tex.enabled)
        o << " " << m_state.tex.tw << "x" << m_state.tex.th;
    o << "\n";
    o << "  draws: " << m_draws.size() << "\n";
    return o.str();
}

} // namespace GS

#include "vk_backend.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace VKBackend {

bool SoftDevice::init(const Config& cfg) {
    m_cfg = cfg;
    if (!m_cfg.width || !m_cfg.height) return false;
    m_fb.assign((size_t)m_cfg.width * m_cfg.height * 4, 0);
    m_tex.clear();
    m_batches = 0;
    m_ok = true;
    clear(20, 20, 40, 255);
    return true;
}

void SoftDevice::shutdown() { m_ok = false; m_fb.clear(); m_tex.clear(); }

void SoftDevice::clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!m_ok) return;
    for (size_t i = 0; i + 3 < m_fb.size(); i += 4) {
        m_fb[i] = r; m_fb[i+1] = g; m_fb[i+2] = b; m_fb[i+3] = a;
    }
}

bool SoftDevice::upload_texture_rgba(uint32_t id, uint32_t w, uint32_t h, const uint8_t* rgba) {
    if (!m_ok || !rgba || !w || !h) return false;
    Tex t; t.w = w; t.h = h;
    t.rgba.assign(rgba, rgba + (size_t)w * h * 4);
    m_tex[id] = std::move(t);
    return true;
}

void SoftDevice::put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || y < 0 || (uint32_t)x >= m_cfg.width || (uint32_t)y >= m_cfg.height) return;
    size_t i = ((size_t)y * m_cfg.width + (size_t)x) * 4;
    if (a >= 250) {
        m_fb[i] = r; m_fb[i+1] = g; m_fb[i+2] = b; m_fb[i+3] = 255;
    } else if (a > 0) {
        float af = a / 255.f;
        m_fb[i] = (uint8_t)(r * af + m_fb[i] * (1 - af));
        m_fb[i+1] = (uint8_t)(g * af + m_fb[i+1] * (1 - af));
        m_fb[i+2] = (uint8_t)(b * af + m_fb[i+2] * (1 - af));
        m_fb[i+3] = 255;
    }
}

void SoftDevice::draw_sprite(const GS::DrawRequest& d) {
    if (d.vertex_count < 4) return;
    const auto& v0 = d.verts[0];
    const auto& v3 = d.verts[3];
    int x0 = (int)std::floor(std::min(v0.x, v3.x));
    int y0 = (int)std::floor(std::min(v0.y, v3.y));
    int x1 = (int)std::ceil(std::max(v0.x, v3.x));
    int y1 = (int)std::ceil(std::max(v0.y, v3.y));
    float u0 = v0.u, v0t = v0.v;
    float u1 = d.verts[1].u, v1t = d.verts[3].v;
    const Tex* tex = nullptr;
    if (d.tex_id) {
        auto it = m_tex.find(d.tex_id);
        if (it != m_tex.end()) tex = &it->second;
    }
    int bw = std::max(1, x1 - x0);
    int bh = std::max(1, y1 - y0);
    for (int y = y0; y < y1; ++y) {
        float fy = (bh <= 1) ? 0.f : (float)(y - y0) / (float)(bh - 1);
        for (int x = x0; x < x1; ++x) {
            float fx = (bw <= 1) ? 0.f : (float)(x - x0) / (float)(bw - 1);
            uint8_t r = (uint8_t)(v0.r * 255), g = (uint8_t)(v0.g * 255);
            uint8_t b = (uint8_t)(v0.b * 255), a = (uint8_t)(v0.a * 255);
            if (tex && tex->w && tex->h) {
                float u = u0 + (u1 - u0) * fx;
                float vv = v0t + (v1t - v0t) * fy;
                int tx = ((int)(u * (tex->w - 1)) % (int)tex->w + (int)tex->w) % (int)tex->w;
                int ty = ((int)(vv * (tex->h - 1)) % (int)tex->h + (int)tex->h) % (int)tex->h;
                size_t ti = ((size_t)ty * tex->w + (size_t)tx) * 4;
                r = tex->rgba[ti]; g = tex->rgba[ti+1]; b = tex->rgba[ti+2]; a = tex->rgba[ti+3];
            }
            put_pixel(x, y, r, g, b, a);
        }
    }
}

bool SoftDevice::submit(const GS::DrawRequest* draws, uint32_t count) {
    if (!m_ok || !draws) return false;
    m_batches += count;
    for (uint32_t i = 0; i < count; ++i)
        if (draws[i].vertex_count >= 4) draw_sprite(draws[i]);
    return true;
}

std::string SoftDevice::info() const {
    std::ostringstream o;
    o << "SoftDevice " << m_cfg.width << "x" << m_cfg.height << " batches=" << m_batches;
    return o.str();
}

Device* create_device() { return new SoftDevice(); }

} // namespace VKBackend

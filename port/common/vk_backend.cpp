#include "vk_backend.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace VKBackend {

bool SoftDevice::init(const Config& cfg) {
    m_cfg = cfg;
    if (m_cfg.width == 0 || m_cfg.height == 0) return false;
    m_fb.assign(static_cast<size_t>(m_cfg.width) * m_cfg.height * 4, 0);
    m_tex.clear();
    m_tex_uploads = m_draw_batches = m_draw_verts = 0;
    m_ok = true;
    clear(20, 20, 40, 255); // dark blue clear like a boot screen
    return true;
}

void SoftDevice::shutdown() {
    m_ok = false;
    m_fb.clear();
    m_tex.clear();
}

void SoftDevice::clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!m_ok) return;
    for (size_t i = 0; i + 3 < m_fb.size(); i += 4) {
        m_fb[i] = r;
        m_fb[i + 1] = g;
        m_fb[i + 2] = b;
        m_fb[i + 3] = a;
    }
}

bool SoftDevice::upload_texture_rgba(uint32_t id, uint32_t w, uint32_t h, const uint8_t* rgba) {
    if (!m_ok || !rgba || w == 0 || h == 0) return false;
    Tex t;
    t.w = w;
    t.h = h;
    t.rgba.assign(rgba, rgba + static_cast<size_t>(w) * h * 4);
    m_tex[id] = std::move(t);
    m_tex_uploads++;
    return true;
}

void SoftDevice::put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || y < 0 || (uint32_t)x >= m_cfg.width || (uint32_t)y >= m_cfg.height) return;
    size_t i = (static_cast<size_t>(y) * m_cfg.width + static_cast<size_t>(x)) * 4;
    // simple alpha over
    if (a >= 250) {
        m_fb[i] = r;
        m_fb[i + 1] = g;
        m_fb[i + 2] = b;
        m_fb[i + 3] = 255;
    } else if (a > 0) {
        float af = a / 255.f;
        m_fb[i] = (uint8_t)(r * af + m_fb[i] * (1 - af));
        m_fb[i + 1] = (uint8_t)(g * af + m_fb[i + 1] * (1 - af));
        m_fb[i + 2] = (uint8_t)(b * af + m_fb[i + 2] * (1 - af));
        m_fb[i + 3] = 255;
    }
}

void SoftDevice::draw_sprite(const GS::DrawRequest& d) {
    if (d.vertex_count < 4) return;
    // verts: 0=TL 1=TR 2=BL 3=BR from emit_sprite
    const auto& v0 = d.verts[0];
    const auto& v3 = d.verts[3];

    int x0 = (int)std::floor(std::min(v0.x, v3.x));
    int y0 = (int)std::floor(std::min(v0.y, v3.y));
    int x1 = (int)std::ceil(std::max(v0.x, v3.x));
    int y1 = (int)std::ceil(std::max(v0.y, v3.y));

    float u0 = v0.u, v0t = v0.v;
    float u1 = d.verts[1].u, v1t = d.verts[3].v;

    const Tex* tex = nullptr;
    auto it = m_tex.find(1);
    if (it != m_tex.end()) tex = &it->second;

    int bw = std::max(1, x1 - x0);
    int bh = std::max(1, y1 - y0);

    for (int y = y0; y < y1; ++y) {
        float fy = (bh <= 1) ? 0.f : (float)(y - y0) / (float)(bh - 1);
        for (int x = x0; x < x1; ++x) {
            float fx = (bw <= 1) ? 0.f : (float)(x - x0) / (float)(bw - 1);
            uint8_t r = (uint8_t)(v0.r * 255), g = (uint8_t)(v0.g * 255), b = (uint8_t)(v0.b * 255), a = (uint8_t)(v0.a * 255);
            if (tex && tex->w && tex->h) {
                float u = u0 + (u1 - u0) * fx;
                float vv = v0t + (v1t - v0t) * fy;
                int tx = (int)(u * (tex->w - 1)) % (int)tex->w;
                int ty = (int)(vv * (tex->h - 1)) % (int)tex->h;
                if (tx < 0) tx += tex->w;
                if (ty < 0) ty += tex->h;
                size_t ti = (static_cast<size_t>(ty) * tex->w + static_cast<size_t>(tx)) * 4;
                r = tex->rgba[ti];
                g = tex->rgba[ti + 1];
                b = tex->rgba[ti + 2];
                a = tex->rgba[ti + 3];
            } else {
                // flat color per-sprite variation from position
                r = (uint8_t)std::min(255, 80 + (x0 * 3) % 120);
                g = (uint8_t)std::min(255, 120 + (y0 * 2) % 100);
                b = (uint8_t)std::min(255, 200 - (x0 % 80));
                a = 255;
            }
            put_pixel(x, y, r, g, b, a);
        }
    }
}

bool SoftDevice::submit(const GS::DrawRequest* draws, uint32_t count) {
    if (!m_ok || !draws) return false;
    m_draw_batches += count;
    for (uint32_t i = 0; i < count; ++i) {
        m_draw_verts += draws[i].vertex_count;
        if (draws[i].prim == GS::Prim::Sprite || draws[i].vertex_count >= 4)
            draw_sprite(draws[i]);
    }
    return true;
}

std::string SoftDevice::info() const {
    std::ostringstream o;
    o << "VKBackend: SoftDevice (CPU raster — visible pixels)\n";
    o << "  target: " << m_cfg.width << "x" << m_cfg.height << "\n";
    o << "  tex_uploads: " << m_tex_uploads << "\n";
    o << "  draw_batches: " << m_draw_batches << "\n";
    o << "  draw_verts: " << m_draw_verts << "\n";
    return o.str();
}

Device* create_device() {
    return new SoftDevice();
}

} // namespace VKBackend

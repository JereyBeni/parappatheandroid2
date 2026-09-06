#include "vk_backend.h"
#include <sstream>

namespace VKBackend {

bool NullDevice::init(const Config& cfg) {
    m_cfg = cfg;
    m_ok = true;
    m_tex_uploads = m_draw_batches = m_draw_verts = 0;
    return true;
}

void NullDevice::shutdown() {
    m_ok = false;
}

bool NullDevice::upload_texture_rgba(uint32_t /*id*/, uint32_t /*w*/, uint32_t /*h*/, const uint8_t* rgba) {
    if (!m_ok || !rgba) return false;
    m_tex_uploads++;
    return true;
}

bool NullDevice::submit(const GS::DrawRequest* draws, uint32_t count) {
    if (!m_ok || !draws) return false;
    m_draw_batches += count;
    for (uint32_t i = 0; i < count; ++i)
        m_draw_verts += draws[i].vertex_count;
    return true;
}

std::string NullDevice::info() const {
    std::ostringstream o;
    o << "VKBackend: NullDevice (translation test only)\n";
    o << "  target: " << m_cfg.width << "x" << m_cfg.height << "\n";
    o << "  tex_uploads: " << m_tex_uploads << "\n";
    o << "  draw_batches: " << m_draw_batches << "\n";
    o << "  draw_verts: " << m_draw_verts << "\n";
    o << "  next: real Vulkan device (Android VK / PC)\n";
    return o.str();
}

Device* create_device() {
    // Future: try VkDevice, fall back to null
    return new NullDevice();
}

} // namespace VKBackend

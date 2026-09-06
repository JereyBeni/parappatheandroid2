#pragma once

#include "gs_types.h"
#include <string>

// Vulkan backend interface for GS translation output.
// On Android we can later use VK_ANDROID_external_memory_android_hardware_buffer etc.
// For now: stub that records what it would submit.

namespace VKBackend {

struct Config {
    uint32_t width = 640;
    uint32_t height = 448; // typical PS2 progressive-ish test size
    bool enable_validation = false;
};

class Device {
public:
    virtual ~Device() = default;

    virtual bool init(const Config& cfg) = 0;
    virtual void shutdown() = 0;

    // Upload a decoded TIM2 RGBA texture (host -> GPU)
    virtual bool upload_texture_rgba(uint32_t id, uint32_t w, uint32_t h,
                                     const uint8_t* rgba) = 0;

    // Submit translated GS draws
    virtual bool submit(const GS::DrawRequest* draws, uint32_t count) = 0;

    virtual std::string info() const = 0;
};

// Software/null backend - always available (no Vulkan required to test translation)
class NullDevice final : public Device {
public:
    bool init(const Config& cfg) override;
    void shutdown() override;
    bool upload_texture_rgba(uint32_t id, uint32_t w, uint32_t h, const uint8_t* rgba) override;
    bool submit(const GS::DrawRequest* draws, uint32_t count) override;
    std::string info() const override;

private:
    Config m_cfg;
    bool m_ok = false;
    uint32_t m_tex_uploads = 0;
    uint32_t m_draw_batches = 0;
    uint32_t m_draw_verts = 0;
};

// Factory: tries real Vulkan later; today returns NullDevice
Device* create_device();

} // namespace VKBackend

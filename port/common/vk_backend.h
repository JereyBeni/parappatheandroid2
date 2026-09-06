#pragma once
#include "gs_types.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace VKBackend {

struct Config {
    uint32_t width = 640;
    uint32_t height = 448;
};

class Device {
public:
    virtual ~Device() = default;
    virtual bool init(const Config& cfg) = 0;
    virtual void shutdown() = 0;
    virtual bool upload_texture_rgba(uint32_t id, uint32_t w, uint32_t h, const uint8_t* rgba) = 0;
    virtual bool submit(const GS::DrawRequest* draws, uint32_t count) = 0;
    virtual void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
    virtual const uint8_t* framebuffer() const = 0;
    virtual uint32_t fb_width() const = 0;
    virtual uint32_t fb_height() const = 0;
    virtual std::string info() const = 0;
};

class SoftDevice final : public Device {
public:
    bool init(const Config& cfg) override;
    void shutdown() override;
    bool upload_texture_rgba(uint32_t id, uint32_t w, uint32_t h, const uint8_t* rgba) override;
    bool submit(const GS::DrawRequest* draws, uint32_t count) override;
    void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
    const uint8_t* framebuffer() const override { return m_fb.empty() ? nullptr : m_fb.data(); }
    uint32_t fb_width() const override { return m_cfg.width; }
    uint32_t fb_height() const override { return m_cfg.height; }
    std::string info() const override;
private:
    void draw_sprite(const GS::DrawRequest& d);
    void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    Config m_cfg;
    bool m_ok = false;
    std::vector<uint8_t> m_fb;
    struct Tex { uint32_t w = 0, h = 0; std::vector<uint8_t> rgba; };
    std::unordered_map<uint32_t, Tex> m_tex;
    uint32_t m_batches = 0;
};

Device* create_device();

} // namespace VKBackend

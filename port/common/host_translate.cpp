#include "host_translate.h"

#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Host {

// ========================= Time =========================

class TimePC final : public Time {
public:
    TimePC() : m_start(std::chrono::steady_clock::now()) {}

    double seconds() const override {
        using namespace std::chrono;
        return duration<double>(steady_clock::now() - m_start).count();
    }

    uint64_t vblank_count() const override { return m_vblank; }

    void wait_vblank() override {
        using namespace std::chrono;
        const double period = 1.0 / kVblankHz;
        double target = (m_vblank + 1) * period;
        double now = seconds();
        if (target > now) {
            auto ms = duration<double>(target - now);
            std::this_thread::sleep_for(duration_cast<milliseconds>(ms));
        }
        m_vblank++;
    }

    void pump() override {
        // Advance vblank to match wall clock (no sleep)
        uint64_t expect = (uint64_t)(seconds() * kVblankHz);
        if (expect > m_vblank) m_vblank = expect;
    }

private:
    std::chrono::steady_clock::time_point m_start;
    uint64_t m_vblank = 0;
};

Time* create_time() { return new TimePC(); }

// ========================= Input =========================

class InputPC final : public Input {
public:
    void poll() override {
        m_pad = PadState{};
        m_pad.connected = true;
#ifdef _WIN32
        // Async key state — works without a focused console message pump
        auto down = [](int vk) -> bool {
            return (GetAsyncKeyState(vk) & 0x8000) != 0;
        };
        if (down('Z') || down('z')) m_pad.buttons |= (uint16_t)Button::Cross;    // confirm
        if (down('X') || down('x')) m_pad.buttons |= (uint16_t)Button::Circle;
        if (down('A') || down('a')) m_pad.buttons |= (uint16_t)Button::Square;
        if (down('S') || down('s')) m_pad.buttons |= (uint16_t)Button::Triangle;
        if (down(VK_RETURN))        m_pad.buttons |= (uint16_t)Button::Start;
        if (down(VK_BACK) || down(VK_RSHIFT)) m_pad.buttons |= (uint16_t)Button::Select;
        if (down(VK_UP)    || down('W')) m_pad.buttons |= (uint16_t)Button::Up;
        if (down(VK_DOWN)  || down('S')) m_pad.buttons |= (uint16_t)Button::Down;
        if (down(VK_LEFT)  || down('A')) m_pad.buttons |= (uint16_t)Button::Left;
        if (down(VK_RIGHT) || down('D')) m_pad.buttons |= (uint16_t)Button::Right;
        if (down('Q')) m_pad.buttons |= (uint16_t)Button::L1;
        if (down('E')) m_pad.buttons |= (uint16_t)Button::R1;

        // Simple digital stick from arrows
        m_pad.lx = 128;
        m_pad.ly = 128;
        if (down(VK_LEFT)  || down('A')) m_pad.lx = 0;
        if (down(VK_RIGHT) || down('D')) m_pad.lx = 255;
        if (down(VK_UP)    || down('W')) m_pad.ly = 0;
        if (down(VK_DOWN)  || down('S')) m_pad.ly = 255;
#else
        // Non-Windows: leave neutral (can wire SDL later)
        (void)m_pad;
#endif
    }

    PadState pad(int port) const override {
        if (port != 0) {
            PadState empty;
            empty.connected = false;
            return empty;
        }
        return m_pad;
    }

    std::string info() const override {
        return "InputPC: keyboard map (Z=Cross X=Circle arrows/WASD, Enter=Start)";
    }

private:
    PadState m_pad;
};

Input* create_input() { return new InputPC(); }

// ========================= Audio =========================

class AudioPC final : public Audio {
public:
    bool open(int sample_rate, int channels) override {
        m_rate = sample_rate;
        m_ch = channels;
        m_open = true;
        m_queued_frames = 0;
        // Stub: no device yet — we accept queues and count them (WASAPI/SDL later)
        return true;
    }

    void close() override {
        m_open = false;
    }

    bool queue(const int16_t* interleaved, int frames) override {
        if (!m_open || !interleaved || frames <= 0) return false;
        m_queued_frames += (uint64_t)frames;
        m_callbacks++;
        return true;
    }

    void set_master_volume(float v01) override {
        if (v01 < 0) v01 = 0;
        if (v01 > 1) v01 = 1;
        m_vol = v01;
    }

    std::string info() const override {
        std::ostringstream o;
        o << "AudioPC: stub PCM sink (WASAPI/SDL next)\n";
        o << "  rate=" << m_rate << " ch=" << m_ch
          << " vol=" << m_vol
          << " queued_frames=" << m_queued_frames
          << " submits=" << m_callbacks;
        return o.str();
    }

private:
    bool m_open = false;
    int m_rate = 48000;
    int m_ch = 2;
    float m_vol = 1.f;
    uint64_t m_queued_frames = 0;
    uint64_t m_callbacks = 0;
};

Audio* create_audio() { return new AudioPC(); }

// ========================= Disc =========================

class DiscPC final : public Disc {
public:
    bool open(const std::string& path) override {
        close();
        m_img = std::make_unique<Iso9660::Image>();
        if (!m_img->open(path)) {
            m_err = m_img->error();
            m_img.reset();
            return false;
        }
        m_path = path;
        return true;
    }

    void close() override {
        if (m_img) m_img->close();
        m_img.reset();
        m_path.clear();
        m_err.clear();
    }

    bool ok() const override { return m_img && m_img->ok(); }
    std::string error() const override { return m_err; }
    std::string volume_id() const override {
        return m_img ? m_img->volume_id() : "";
    }

    std::vector<uint8_t> read_file(const std::string& iso_path) override {
        if (!m_img) return {};
        return m_img->read_file(iso_path);
    }

    std::vector<Iso9660::FileEntry> list() override {
        if (!m_img) return {};
        return m_img->list_all();
    }

private:
    std::unique_ptr<Iso9660::Image> m_img;
    std::string m_path;
    std::string m_err;
};

Disc* create_disc() { return new DiscPC(); }

// ========================= GPU =========================

class GpuPC final : public Gpu {
public:
    bool init(uint32_t w, uint32_t h) override {
        m_dev.reset(VKBackend::create_device());
        VKBackend::Config c;
        c.width = w;
        c.height = h;
        return m_dev && m_dev->init(c);
    }

    void shutdown() override {
        if (m_dev) m_dev->shutdown();
        m_dev.reset();
    }

    bool upload_texture_rgba(uint32_t id, uint32_t w, uint32_t h, const uint8_t* rgba) override {
        return m_dev && m_dev->upload_texture_rgba(id, w, h, rgba);
    }

    void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) override {
        if (m_dev) m_dev->clear(r, g, b, a);
    }

    bool submit(const GS::DrawRequest* draws, uint32_t count) override {
        return m_dev && m_dev->submit(draws, count);
    }

    const uint8_t* framebuffer() const override {
        return m_dev ? m_dev->framebuffer() : nullptr;
    }

    uint32_t width() const override { return m_dev ? m_dev->fb_width() : 0; }
    uint32_t height() const override { return m_dev ? m_dev->fb_height() : 0; }

    std::string info() const override {
        return m_dev ? m_dev->info() : "GpuPC: no device";
    }

private:
    std::unique_ptr<VKBackend::Device> m_dev;
};

Gpu* create_gpu() { return new GpuPC(); }

// ========================= Context =========================

bool Context::init_all() {
    time.reset(create_time());
    input.reset(create_input());
    audio.reset(create_audio());
    disc.reset(create_disc());
    gpu.reset(create_gpu());
    if (!audio->open(48000, 2)) return false;
    if (!gpu->init(640, 448)) return false;
    return true;
}

void Context::shutdown_all() {
    if (gpu) gpu->shutdown();
    if (audio) audio->close();
    if (disc) disc->close();
    gpu.reset();
    audio.reset();
    disc.reset();
    input.reset();
    time.reset();
}

std::string Context::summary() const {
    std::ostringstream o;
    o << translation_map_text() << "\n";
    if (input) o << input->info() << "\n";
    if (audio) o << audio->info() << "\n";
    if (gpu) o << gpu->info() << "\n";
    if (time) o << "TimePC: vblank=" << time->vblank_count()
                << " t=" << time->seconds() << "s\n";
    if (disc) o << "Disc: " << (disc->ok() ? disc->volume_id() : "(closed)") << "\n";
    return o.str();
}

std::string translation_map_text() {
    return
        "=== PS2 -> PC translation map ===\n"
        "  GS      -> Host::Gpu     (SoftDevice raster / Vulkan later)\n"
        "  libpad  -> Host::Input   (keyboard; XInput later)\n"
        "  SPU2    -> Host::Audio   (PCM queue; WASAPI/SDL later)\n"
        "  CDVD    -> Host::Disc    (ISO9660 .bin/.iso)\n"
        "  VBlank  -> Host::Time    (59.94 Hz virtual)\n"
        "================================\n";
}

} // namespace Host

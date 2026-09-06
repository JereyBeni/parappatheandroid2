#pragma once

// =============================================================================
// host_translate — PS2 subsystems -> PC host APIs
//
// This is the real port boundary. Game/decomp code should talk to these
// interfaces, not to sceGs / libpad / SPU2 directly.
//
// Mapping:
//   GS (Graphics Synthesizer)  -> HostGPU   (Vulkan later, SoftDevice now)
//   PAD / libpad               -> HostInput (keyboard + XInput-style)
//   SPU2 / IOP audio           -> HostAudio (stereo PCM callback)
//   DVD / CDVD                 -> HostDisc  (ISO9660 .bin/.iso)
//   VBlank / timers            -> HostTime
// =============================================================================

#include "gs_types.h"
#include "vk_backend.h"
#include "iso9660.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Host {

// ---- Time / VBlank ----------------------------------------------------------

struct Time {
    // PS2-ish: ~59.94 Hz NTSC field
    static constexpr double kVblankHz = 59.94;

    virtual ~Time() = default;
    virtual double seconds() const = 0;       // monotonic
    virtual uint64_t vblank_count() const = 0;
    virtual void wait_vblank() = 0;          // sleep until next "vblank"
    virtual void pump() = 0;                // advance counters
};

Time* create_time();

// ---- Input (libpad replacement) ---------------------------------------------

enum class Button : uint16_t {
    Select   = 1 << 0,
    L3       = 1 << 1,
    R3       = 1 << 2,
    Start    = 1 << 3,
    Up       = 1 << 4,
    Right    = 1 << 5,
    Down     = 1 << 6,
    Left     = 1 << 7,
    L2       = 1 << 8,
    R2       = 1 << 9,
    L1       = 1 << 10,
    R1       = 1 << 11,
    Triangle = 1 << 12,
    Circle   = 1 << 13,
    Cross    = 1 << 14,
    Square   = 1 << 15,
};

struct PadState {
    uint16_t buttons = 0; // bitmask of Button
    uint8_t lx = 128, ly = 128, rx = 128, ry = 128; // 0..255, 128 center
    bool connected = true;
};

struct Input {
    virtual ~Input() = default;
    virtual void poll() = 0;
    virtual PadState pad(int port = 0) const = 0; // port 0..1
    virtual std::string info() const = 0;
};

Input* create_input();

// ---- Audio (SPU2 / wavep2 stand-in) -----------------------------------------

struct Audio {
    virtual ~Audio() = default;
    // Open device: stereo s16, sample_rate (e.g. 48000)
    virtual bool open(int sample_rate = 48000, int channels = 2) = 0;
    virtual void close() = 0;
    // Queue interleaved s16 PCM (frames = samples per channel)
    virtual bool queue(const int16_t* interleaved, int frames) = 0;
    virtual void set_master_volume(float v01) = 0;
    virtual std::string info() const = 0;
};

Audio* create_audio();

// ---- Disc (CDVD / DVD) ------------------------------------------------------

struct Disc {
    virtual ~Disc() = default;
    virtual bool open(const std::string& path) = 0; // .bin / .iso
    virtual void close() = 0;
    virtual bool ok() const = 0;
    virtual std::string error() const = 0;
    virtual std::string volume_id() const = 0;
    virtual std::vector<uint8_t> read_file(const std::string& iso_path) = 0;
    virtual std::vector<Iso9660::FileEntry> list() = 0;
};

Disc* create_disc();

// ---- GPU (GS translation target) --------------------------------------------

struct Gpu {
    virtual ~Gpu() = default;
    virtual bool init(uint32_t w, uint32_t h) = 0;
    virtual void shutdown() = 0;
    virtual bool upload_texture_rgba(uint32_t id, uint32_t w, uint32_t h, const uint8_t* rgba) = 0;
    virtual void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
    virtual bool submit(const GS::DrawRequest* draws, uint32_t count) = 0;
    virtual const uint8_t* framebuffer() const = 0; // RGBA8888 if readable
    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
    virtual std::string info() const = 0;
};

// Wraps VKBackend::Device (SoftDevice today, Vulkan later)
Gpu* create_gpu();

// ---- Full host context ------------------------------------------------------

struct Context {
    std::unique_ptr<Time>  time;
    std::unique_ptr<Input> input;
    std::unique_ptr<Audio> audio;
    std::unique_ptr<Disc>  disc;
    std::unique_ptr<Gpu>   gpu;

    bool init_all();
    void shutdown_all();
    std::string summary() const;
};

// One-shot: describe the translation map (for logs / UI)
std::string translation_map_text();

} // namespace Host

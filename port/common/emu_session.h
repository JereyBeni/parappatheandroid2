#pragma once

#include "gs_translate.h"
#include "vk_backend.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Lightweight "emulation session" for the harness.
// Not a full PS2 CPU core — drives prlib/GS translation pipeline and emits a game log.

namespace Emu {

using LogFn = std::function<void(const std::string&)>;

struct SessionConfig {
    uint32_t width = 640;
    uint32_t height = 448;
    bool use_tim2_placeholder = true;
};

class Session {
public:
    explicit Session(LogFn log = nullptr);

    void set_log(LogFn log) { m_log = std::move(log); }

    bool boot(const SessionConfig& cfg = {});
    bool step_frame(int frame_index);
    std::string run_demo(int frames = 3);

    const GS::Translator& translator() const { return m_tr; }
    VKBackend::Device* device() { return m_dev.get(); }

private:
    void log(const std::string& s);
    LogFn m_log;
    GS::Translator m_tr;
    std::unique_ptr<VKBackend::Device> m_dev;
    SessionConfig m_cfg;
    bool m_booted = false;
    std::vector<std::string> m_lines;
};

} // namespace Emu

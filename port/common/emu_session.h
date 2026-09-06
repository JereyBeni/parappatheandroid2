#pragma once

#include "gs_translate.h"
#include "vk_backend.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Emu {

using LogFn = std::function<void(const std::string&)>;

struct SessionConfig {
    uint32_t width = 640;
    uint32_t height = 448;
    bool use_tim2_placeholder = true;
    /** true when host confirmed July 12 .BIN is present */
    bool rom_present = false;
    std::string rom_name;
};

class Session {
public:
    explicit Session(LogFn log = nullptr);

    void set_log(LogFn log) { m_log = std::move(log); }

    bool boot(const SessionConfig& cfg = {});
    bool step_frame(int frame_index);
    std::string run_demo(int frames = 3);

    VKBackend::Device* device() { return m_dev.get(); }
    const GS::Translator& translator() const { return m_tr; }
    bool rom_present() const { return m_cfg.rom_present; }

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

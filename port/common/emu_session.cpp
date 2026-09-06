#include "emu_session.h"
#include <memory>
#include <sstream>

namespace Emu {

Session::Session(LogFn log) : m_log(std::move(log)) {}

void Session::log(const std::string& s) {
    if (m_log) m_log(s);
    else m_lines.push_back(s);
}

bool Session::boot(const SessionConfig& cfg) {
    m_cfg = cfg;
    m_lines.clear();
    m_booted = false;

    log("[EE]  Reset Emotion Engine");
    log("[EE]  BIOS stub — harness mode (no full CPU)");
    log("[IOP] Load module: WAVE2PS2.IRX (stub)");
    log("[IOP] Load module: TAPCTRL.IRX (stub)");
    log("[DVD] Mount July 12 prototype image (logical)");
    log("[prlib] PrInitializeModule()");
    log("[prlib] PrInitializeScene(640x448)");

    m_tr.reset();
    m_tr.set_frame(cfg.width, cfg.height, GS::Psm::PSMCT32);
    m_tr.set_tex(64, 64, GS::Psm::PSMCT32);
    log("[GS]  FRAME " + std::to_string(cfg.width) + "x" + std::to_string(cfg.height) + " PSMCT32");

    m_dev.reset(VKBackend::create_device());
    VKBackend::Config vc;
    vc.width = cfg.width;
    vc.height = cfg.height;
    if (!m_dev->init(vc)) {
        log("[SOFT] FAIL init raster backend");
        return false;
    }
    log("[SOFT] " + m_dev->info());

    if (cfg.use_tim2_placeholder) {
        const uint32_t tw = 16, th = 16;
        std::vector<uint8_t> rgba(tw * th * 4);
        for (uint32_t y = 0; y < th; ++y) {
            for (uint32_t x = 0; x < tw; ++x) {
                bool on = ((x / 4) ^ (y / 4)) & 1;
                size_t i = (y * tw + x) * 4;
                // Parappa-ish yellow/black checker
                rgba[i+0] = on ? 255 : 30;
                rgba[i+1] = on ? 220 : 30;
                rgba[i+2] = on ? 40 : 40;
                rgba[i+3] = 255;
            }
        }
        if (m_dev->upload_texture_rgba(1, tw, th, rgba.data()))
            log("[TIM2] placeholder tex 16x16 uploaded");
        else
            log("[TIM2] upload FAIL");
    }

    log("[EMU] Boot complete — framebuffer live");
    m_booted = true;
    return true;
}

bool Session::step_frame(int frame_index) {
    if (!m_booted || !m_dev) return false;

    // clear each frame so motion is visible
    m_dev->clear(18, 18, 36, 255);
    m_tr.clear_draws();

    float o = (float)(frame_index % 40) * 4.0f;
    float bob = (float)((frame_index * 3) % 20);

    // "stage" floor strip
    m_tr.emit_sprite(0, 360, 640, 448, 0, 0, 1, 1);
    // moving character-ish quad
    m_tr.emit_sprite(80 + o, 200 - bob, 180 + o, 340 - bob, 0, 0, 1, 1);
    // UI / score box
    m_tr.emit_sprite(480, 20, 620, 80, 0, 0, 1, 1);
    // secondary prop
    m_tr.emit_sprite(300, 120 + bob * 0.5f, 380, 200 + bob * 0.5f, 0, 0, 1, 1);

    auto& draws = m_tr.draws();
    bool ok = m_dev->submit(draws.data(), (uint32_t)draws.size());

    std::ostringstream line;
    line << "[FRAME " << frame_index << "] prims=" << draws.size()
         << " submit=" << (ok ? "OK" : "FAIL");
    log(line.str());
    if (frame_index == 0)
        log("[prlib] PrRender(scene) — first visible frame");
    return ok;
}

std::string Session::run_demo(int frames) {
    m_lines.clear();
    if (!boot()) return "BOOT FAIL\n";
    for (int i = 0; i < frames; ++i)
        step_frame(i);
    log("[EMU] Demo stop");
    if (m_dev) log(m_dev->info());

    std::ostringstream out;
    for (auto& l : m_lines) out << l << "\n";
    return out.str();
}

} // namespace Emu

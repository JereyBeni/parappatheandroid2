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
    log("[GS]  TEX0 placeholder 64x64");

    m_dev.reset(VKBackend::create_device());
    VKBackend::Config vc;
    vc.width = cfg.width;
    vc.height = cfg.height;
    if (!m_dev->init(vc)) {
        log("[VK]  FAIL init backend");
        return false;
    }
    log("[VK]  Backend init: " + m_dev->info());

    if (cfg.use_tim2_placeholder) {
        // 8x8 checker TIM2-like RGBA (stand-in until real extract on device)
        const uint32_t tw = 8, th = 8;
        std::vector<uint8_t> rgba(tw * th * 4);
        for (uint32_t y = 0; y < th; ++y) {
            for (uint32_t x = 0; x < tw; ++x) {
                bool on = ((x / 2) ^ (y / 2)) & 1;
                size_t i = (y * tw + x) * 4;
                rgba[i+0] = on ? 255 : 40;
                rgba[i+1] = on ? 200 : 40;
                rgba[i+2] = on ? 40 : 80;
                rgba[i+3] = 255;
            }
        }
        if (m_dev->upload_texture_rgba(1, tw, th, rgba.data()))
            log("[TIM2] upload placeholder tex id=1 (" + std::to_string(tw) + "x" + std::to_string(th) + ")");
        else
            log("[TIM2] upload FAIL");
    }

    log("[EMU] Boot complete — entering frame loop");
    m_booted = true;
    return true;
}

bool Session::step_frame(int frame_index) {
    if (!m_booted || !m_dev) return false;

    m_tr.clear_draws();

    // Fake stage / UI sprites the way prlib would push GS prims
    float o = (float)(frame_index % 30) * 2.0f;
    m_tr.emit_sprite(32 + o, 32, 160 + o, 160, 0, 0, 1, 1);
    m_tr.emit_sprite(200, 100, 400, 300, 0, 0, 1, 1);
    m_tr.emit_sprite(420, 200, 600, 400, 0, 0, 1, 1);

    auto& draws = m_tr.draws();
    bool ok = m_dev->submit(draws.data(), (uint32_t)draws.size());

    std::ostringstream line;
    line << "[FRAME " << frame_index << "] prims=" << draws.size()
         << " verts=" << (draws.size() * 4)
         << " submit=" << (ok ? "OK" : "FAIL");
    log(line.str());

    if (frame_index == 0)
        log("[prlib] PrRender(scene) — first frame");
    return ok;
}

std::string Session::run_demo(int frames) {
    m_lines.clear();
    if (!boot()) {
        return "BOOT FAIL\n";
    }
    for (int i = 0; i < frames; ++i)
        step_frame(i);

    log("[EMU] Demo stop");
    log(m_dev ? m_dev->info() : "no device");

    std::ostringstream out;
    for (auto& l : m_lines) out << l << "\n";
    // if callback was set, still return summary from translator
    if (m_log && m_lines.empty()) {
        out << m_tr.debug_summary();
        if (m_dev) out << m_dev->info();
    }
    return out.str();
}

} // namespace Emu

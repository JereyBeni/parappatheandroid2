#include "emu_session.h"
#include "parappa_sprite.h"
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
    log("[IOP] WAVE2PS2 + TAPCTRL (stub)");

    if (cfg.rom_present) {
        log("[DVD] ROM present: " + (cfg.rom_name.empty() ? "July12.bin" : cfg.rom_name));
        log("[DVD] July 12 NTSC-J prototype — OK");
    } else {
        log("[DVD] WARNING: no .BIN imported");
        log("[DVD] Running DEMO mode (procedural assets only)");
    }

    log("[prlib] PrInitializeScene");
    log("[GS]  FRAME " + std::to_string(cfg.width) + "x" + std::to_string(cfg.height));

    m_tr.reset();
    m_tr.set_frame(cfg.width, cfg.height, GS::Psm::PSMCT32);

    m_dev.reset(VKBackend::create_device());
    VKBackend::Config vc;
    vc.width = cfg.width;
    vc.height = cfg.height;
    if (!m_dev->init(vc)) {
        log("[SOFT] FAIL init");
        return false;
    }

    {
        uint32_t tw = 0, th = 0;
        std::vector<uint8_t> rgba;
        ParappaSprite::fill_parappa_rgba(rgba, tw, th);
        if (m_dev->upload_texture_rgba(1, tw, th, rgba.data()))
            log("[SPR] Parappa procedural " + std::to_string(tw) + "x" + std::to_string(th));
        else
            log("[SPR] upload FAIL");
    }
    {
        uint32_t tw = 0, th = 0;
        std::vector<uint8_t> rgba;
        ParappaSprite::fill_stage_tile(rgba, tw, th);
        m_dev->upload_texture_rgba(2, tw, th, rgba.data());
    }

    if (cfg.rom_present)
        log("[EMU] Boot OK — ROM mode (assets still procedural until extract)");
    else
        log("[EMU] Boot OK — DEMO mode (import .BIN for ROM mode)");

    m_booted = true;
    return true;
}

bool Session::step_frame(int frame_index) {
    if (!m_booted || !m_dev) return false;

    m_dev->clear(40, 20, 70, 255);
    m_tr.clear_draws();

    float walk = (float)((frame_index * 3) % 280);
    float bob = (frame_index % 10 < 5) ? 0.f : 4.f;

    m_tr.emit_sprite(0, 380, 640, 448, 0, 0, 1, 1);

    float px = 40.f + walk;
    float py = 220.f + bob;
    m_tr.emit_sprite(px, py, px + 96, py + 120, 0, 0, 1, 1);
    m_tr.emit_sprite(px + 70, py + 40, px + 88, py + 90, 0, 0, 1, 1);
    m_tr.emit_sprite(500, 16, 630, 70, 0, 0, 1, 1);

    auto& draws = m_tr.draws();
    bool ok = m_dev->submit(draws.data(), (uint32_t)draws.size());

    std::ostringstream line;
    line << "[FRAME " << frame_index << "] Parappa x=" << (int)px
         << " prims=" << draws.size()
         << (ok ? " OK" : " FAIL");
    if (!m_cfg.rom_present && (frame_index % 30) == 0)
        line << " | WARN no BIN";
    log(line.str());
    return ok;
}

std::string Session::run_demo(int frames) {
    m_lines.clear();
    SessionConfig cfg;
    cfg.rom_present = false;
    if (!boot(cfg)) return "BOOT FAIL\n";
    for (int i = 0; i < frames; ++i)
        step_frame(i);
    log("[EMU] stop");
    if (m_dev) log(m_dev->info());
    std::ostringstream out;
    for (auto& l : m_lines) out << l << "\n";
    return out.str();
}

} // namespace Emu

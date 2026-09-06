// TestForIssues console — PC host translation layer
#include "host_translate.h"
#include "rom_check.h"
#include "gs_translate.h"
#include <cstdio>
#include <string>

int main(int argc, char** argv) {
    std::printf("%s\n", Host::translation_map_text().c_str());

    Host::Context host;
    if (!host.init_all()) {
        std::printf("FAIL host.init_all\n");
        return 1;
    }
    std::printf("%s\n", host.summary().c_str());

    if (argc >= 2) {
        std::string path = argv[1];
        auto fast = RomCheck::verify_iso_fast(path);
        std::printf("[ROM] %s\n", fast.message.c_str());
        if (host.disc->open(path)) {
            std::printf("[DVD] volume=%s\n", host.disc->volume_id().c_str());
            auto list = host.disc->list();
            std::printf("[DVD] entries=%zu\n", list.size());
        } else {
            std::printf("[DVD] %s\n", host.disc->error().c_str());
        }
    } else {
        std::printf("Usage: TestForIssues <path-to-July12.bin>\n");
        std::printf("(no BIN — translation layer still OK)\n");
    }

    // One GS frame through Host::Gpu
    GS::Translator tr;
    tr.set_frame(640, 448, GS::Psm::PSMCT32);
    tr.emit_rect_flat(0, 380, 640, 448, 0.4f, 0.2f, 0.6f);
    tr.emit_sprite(100, 200, 196, 320, 0, 0, 1, 1, 0);
    host.gpu->clear(30, 20, 50, 255);
    host.gpu->submit(tr.draws().data(), (uint32_t)tr.draws().size());
    std::printf("[GS] %s\n", tr.debug_summary().c_str());
    std::printf("[GPU] %s\n", host.gpu->info().c_str());

    host.input->poll();
    auto pad = host.input->pad(0);
    std::printf("[PAD] buttons=0x%04x stick=(%u,%u)\n", pad.buttons, pad.lx, pad.ly);

    host.time->pump();
    std::printf("[VBL] count=%llu t=%.3fs\n",
                (unsigned long long)host.time->vblank_count(), host.time->seconds());

    // Dummy silence audio queue
    std::vector<int16_t> silence(480 * 2, 0);
    host.audio->queue(silence.data(), 480);
    std::printf("[AUD] %s\n", host.audio->info().c_str());

    host.shutdown_all();
    std::printf("OK — PS2->PC translation layer smoke passed\n");
    return 0;
}

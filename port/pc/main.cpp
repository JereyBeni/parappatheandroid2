#include "host_translate.h"
#include "gs_translate.h"
#include "rom_check.h"
#include <cstdio>
#include <string>

#define _CRT_SECURE_NO_WARNINGS

extern "C" {
    void PrLib_Init();
    void PrLib_Update(uint32_t pad_buttons);
    void PrLib_Render();
}

int main(int argc, char** argv) {
    Host::Context host;
    if (!host.init_all()) {
        std::printf("[FATAL] Error al inicializar el host nativo.\n");
        return 1;
    }

    std::string bin_path = (argc >= 2) ? argv[1] : "PS2 - Parappa 7-12-07.bin";

    if (!host.disc->open(bin_path)) {
        std::printf("[prlib ERROR] No se pudo abrir la ISO: %s\n", host.disc->error().c_str());
        host.shutdown_all();
        return 1;
    }

    std::printf("[prlib] ISO vinculada correctamente. Inicializando motor...\n");
    PrLib_Init();

    GS::Translator tr;
    tr.set_frame(640, 448, GS::Psm::PSMCT32);

    bool running = true;
    while (running) {
        host.time->pump();
        host.input->poll();
        auto pad = host.input->pad(0);

        if (pad.buttons & 0x0001) {
            running = false;
        }

        // 1. Logica del juego
        PrLib_Update(pad.buttons);

        // 2. Reiniciar el estado del traductor (remplaza tr.clear())
        tr.reset();
        PrLib_Render();

        // 3. Renderizado a GPU (submit ya ejecuta la presentacion en Host::Gpu)
        host.gpu->clear(0, 0, 0, 255);
        host.gpu->submit(tr.draws().data(), static_cast<uint32_t>(tr.draws().size()));
    }

    host.shutdown_all();
    std::printf("[prlib] Cierre de ejecucion nativa OK.\n");
    return 0;
}

#include "host_translate.h"
#include "gs_translate.h"
#include "rom_check.h"
#include <cstdio>

// Importamos los puntos de entrada reales de prlib decompilado
extern "C" {
    void PrLib_Init();
    void PrLib_Update(uint32_t pad_buttons);
    void PrLib_Render();
}

int main(int argc, char** argv) {
    Host::Context host;
    if (!host.init_all()) return 1;

    std::string bin_path = (argc >= 2) ? argv[1] : "PS2 - Parappa 7-12-07.bin";

    // Mapear el disco en la capa del host
    if (!host.disc->open(bin_path)) {
        std::printf("[prlib ERROR] No se pudo abrir la ISO: %s\n", host.disc->error().c_str());
        return 1;
    }

    std::printf("[prlib] Inicializando subsistemas decompilados de PaRappa 2...\n");
    PrLib_Init(); // Arranca el motor del juego

    GS::Translator tr;
    tr.set_frame(640, 448, GS::Psm::PSMCT32);

    bool running = true;
    while (running) {
        host.time->pump();
        host.input->poll();
        auto pad = host.input->pad(0);

        if (pad.buttons & 0x0001) running = false; // Tecla salir

        // Ejecuta la rutina original de timing/notas/animación de PaRappa
        PrLib_Update(pad.buttons);

        // Limpia el traductor y deja que prlib emita los frames
        tr.clear();
        PrLib_Render();

        // Dibuja en la GPU de PC/Android
        host.gpu->clear(0, 0, 0, 255);
        host.gpu->submit(tr.draws().data(), (uint32_t)tr.draws().size());
        host.gpu->present();
    }

    host.shutdown_all();
    return 0;
}

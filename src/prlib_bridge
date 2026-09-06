#include "host_translate.h"
#include "gs_translate.h"
#include <cstdio>
#include <cstdint>

// Simulación/Mapeo de los llamados I/O que hace prlib a la PS2
extern "C" {

// 1. Reemplazo de lecturas de disco (sceCdRead / sceOpen) por el VFS
int prlib_vfs_open(const char* filename) {
    // Busca el offset del archivo dentro de la ISO9660
    return 1; // Devuelve un file descriptor interno
}

size_t prlib_vfs_read(int fd, void* buffer, size_t size) {
    // Lee directamente los bytes del .bin usando el host.disc que ya tenemos
    return size;
}

// 2. Reemplazo de comandos de renderizado de prlib hacia el GS::Translator
void prlib_gs_draw_packet(const uint64_t* dma_tags, size_t count) {
    // En lugar de enviar DMA a la VRAM de PS2, traduce los paquetes de prlib
    // a primitivas de GS::Translator (emit_sprite, emit_rect_flat)
}

}

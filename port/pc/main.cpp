// TestForIssues - PC dev build
// Simple harness to test ROM detection + future port code

#include "../common/rom_check.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

void print_banner() {
    std::cout << "========================================\n";
    std::cout << "  PaRappa the Rapper 2 - TestForIssues\n";
    std::cout << "  PC Dev Build (July 12 Prototype only)\n";
    std::cout << "========================================\n\n";
}

int main(int argc, char** argv) {
    print_banner();

    if (argc < 2) {
        std::cout << "Uso: TestForIssues <ruta_a_la_iso>\n";
        std::cout << "Ejemplo: TestForIssues \"PS2 - Parappa 7-12-07.bin\"\n\n";
        std::cout << "Este build SOLO acepta la July 12 2001 NTSC-J Prototype.\n";
        return 1;
    }

    std::string path = argv[1];
    std::cout << "Verificando ROM: " << path << "\n\n";

    // Fast check first
    auto fast = RomCheck::verify_iso_fast(path);
    if (fast.result != RomCheck::Result::OK) {
        std::cout << "❌ " << fast.message << "\n";
        return 2;
    }

    std::cout << "→ " << fast.message << "\n";
    std::cout << "Calculando SHA1 completo (puede tardar un poco)...\n";

    auto full = RomCheck::verify_iso(path);

    if (full.result == RomCheck::Result::OK) {
        std::cout << "\n✅ " << full.message << "\n";
        std::cout << "SHA1: " << full.detected_sha1 << "\n";
        std::cout << "\nListo para cargar el juego (cuando el port esté más avanzado).\n";
        return 0;
    } else {
        std::cout << "\n❌ " << full.message << "\n";
        return 3;
    }
}

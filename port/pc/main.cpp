// TestForIssues - PC dev build
// Simple harness to test ROM detection + future port code
// Accepts .bin (official Hidden Palace dump) or .iso if byte-identical

#include "../common/rom_check.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#endif

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool has_extension(const std::string& path, const char* ext) {
    auto lower = to_lower(path);
    auto e = to_lower(ext);
    if (lower.size() < e.size()) return false;
    return lower.compare(lower.size() - e.size(), e.size(), e) == 0;
}

void print_banner() {
    std::cout << "========================================\n";
    std::cout << "  PaRappa the Rapper 2 - TestForIssues\n";
    std::cout << "  PC Dev Build (July 12 Prototype only)\n";
    std::cout << "========================================\n\n";
}

int main(int argc, char** argv) {
    print_banner();

    if (argc < 2) {
        std::cout << "Uso: TestForIssues <ruta_al_archivo.bin>\n\n";
        std::cout << "Formato esperado:\n";
        std::cout << "  - .bin  (dump oficial Hidden Palace)\n";
        std::cout << "  - .iso  (solo si el contenido es identico al .bin)\n\n";
        std::cout << "Ejemplo:\n";
        std::cout << "  TestForIssues \"PS2 - Parappa 7-12-07.bin\"\n\n";
        std::cout << "Archivo oficial:\n";
        std::cout << "  PS2 - Parappa 7-12-07.bin\n";
        std::cout << "  Tamaño: 4159078400 bytes\n";
        std::cout << "  SHA1:   28964c33cee578ec3ce476285067044243363d08\n\n";
        std::cout << "Este build SOLO acepta la July 12 2001 NTSC-J Prototype.\n";
        return 1;
    }

    std::string path = argv[1];
    std::cout << "Verificando: " << path << "\n";

    if (!has_extension(path, ".bin") && !has_extension(path, ".iso")) {
        std::cout << "\n⚠️ Extension no tipica. Se esperaba .bin (o .iso).\n";
        std::cout << "Igual se va a verificar por tamaño + SHA1...\n\n";
    } else if (has_extension(path, ".bin")) {
        std::cout << "Formato: .bin (dump esperado)\n\n";
    } else {
        std::cout << "Formato: .iso (se acepta si el contenido matchea el .bin oficial)\n\n";
    }

    // Fast check first
    auto fast = RomCheck::verify_iso_fast(path);
    if (fast.result != RomCheck::Result::OK) {
        std::cout << "❌ " << fast.message << "\n";
        return 2;
    }

    std::cout << "→ " << fast.message << "\n";
    std::cout << "Calculando SHA1 completo (puede tardar, el archivo pesa ~3.87 GB)...\n";

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

// TestForIssues - PC dev build
// Accepts .bin (official Hidden Palace dump) or .iso if byte-identical

#include "../common/rom_check.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdio>

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

static void setup_console() {
#ifdef _WIN32
    // UTF-8 console + force stdout visible
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::cout.setf(std::ios::unitbuf); // auto-flush
    std::cerr.setf(std::ios::unitbuf);
}

static void pause_if_windows() {
#ifdef _WIN32
    std::cout << "\nPresiona Enter para salir...\n";
    std::cout.flush();
    std::cin.get();
#endif
}

int main(int argc, char** argv) {
    setup_console();

    std::cout << "========================================\n";
    std::cout << "  PaRappa the Rapper 2 - TestForIssues\n";
    std::cout << "  PC Dev Build (July 12 Prototype only)\n";
    std::cout << "========================================\n\n";
    std::cout.flush();

    if (argc < 2) {
        std::cout << "Uso:\n";
        std::cout << "  TestForIssues.exe \"ruta\\al\\PS2 - Parappa 7-12-07.bin\"\n\n";
        std::cout << "Formato esperado: .bin (dump oficial) o .iso identico\n";
        std::cout << "Tamano esperado: 4159078400 bytes\n";
        std::cout << "SHA1 esperado:   28964c33cee578ec3ce476285067044243363d08\n\n";
        pause_if_windows();
        return 1;
    }

    std::string path = argv[1];
    std::cout << "Archivo: " << path << "\n";

    if (has_extension(path, ".bin")) {
        std::cout << "Extension: .bin (ok)\n\n";
    } else if (has_extension(path, ".iso")) {
        std::cout << "Extension: .iso (ok si el contenido matchea)\n\n";
    } else {
        std::cout << "Extension rara, igual verifico por tamano+SHA1...\n\n";
    }
    std::cout.flush();

    std::cout << "[1/2] Check rapido (tamano)...\n";
    std::cout.flush();

    auto fast = RomCheck::verify_iso_fast(path);
    if (fast.result != RomCheck::Result::OK) {
        std::cout << "FAIL: " << fast.message << "\n";
        pause_if_windows();
        return 2;
    }
    std::cout << "OK: " << fast.message << "\n\n";
    std::cout.flush();

    std::cout << "[2/2] Calculando SHA1 completo (~3.87 GB, puede tardar 20-60s)...\n";
    std::cout.flush();

    auto full = RomCheck::verify_iso(path);

    if (full.result == RomCheck::Result::OK) {
        std::cout << "\nSUCCESS: " << full.message << "\n";
        std::cout << "SHA1: " << full.detected_sha1 << "\n";
        std::cout << "\nROM correcta. Listo para el port.\n";
        pause_if_windows();
        return 0;
    }

    std::cout << "\nFAIL: " << full.message << "\n";
    pause_if_windows();
    return 3;
}

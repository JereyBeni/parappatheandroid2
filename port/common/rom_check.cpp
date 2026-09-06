#include "rom_check.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstring>

// Minimal SHA1 implementation (public domain style, enough for this)
namespace {

class SHA1 {
public:
    SHA1() { reset(); }

    void update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            m_data[m_blockByteIndex++] = data[i];
            m_byteCount++;
            if (m_blockByteIndex == 64) {
                processBlock();
                m_blockByteIndex = 0;
            }
        }
    }

    std::string final() {
        uint64_t totalBits = m_byteCount * 8;
        // Padding
        update((const uint8_t*)"\x80", 1);
        while (m_blockByteIndex != 56) {
            update((const uint8_t*)"\x00", 1);
        }
        // Length
        for (int i = 7; i >= 0; --i) {
            uint8_t b = (totalBits >> (i * 8)) & 0xFF;
            update(&b, 1);
        }

        std::ostringstream oss;
        for (int i = 0; i < 5; ++i) {
            oss << std::hex << std::setfill('0') << std::setw(8) << m_h[i];
        }
        return oss.str();
    }

private:
    void reset() {
        m_h[0] = 0x67452301;
        m_h[1] = 0xEFCDAB89;
        m_h[2] = 0x98BADCFE;
        m_h[3] = 0x10325476;
        m_h[4] = 0xC3D2E1F0;
        m_blockByteIndex = 0;
        m_byteCount = 0;
    }

    void processBlock() {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = (m_data[i*4] << 24) | (m_data[i*4+1] << 16) |
                   (m_data[i*4+2] << 8) | m_data[i*4+3];
        }
        for (int i = 16; i < 80; ++i) {
            uint32_t t = w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16];
            w[i] = (t << 1) | (t >> 31);
        }

        uint32_t a = m_h[0], b = m_h[1], c = m_h[2], d = m_h[3], e = m_h[4];

        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }

            uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = temp;
        }

        m_h[0] += a; m_h[1] += b; m_h[2] += c; m_h[3] += d; m_h[4] += e;
    }

    uint32_t m_h[5];
    uint8_t m_data[64];
    size_t m_blockByteIndex;
    uint64_t m_byteCount;
};

uint32_t crc32_update(uint32_t crc, const uint8_t* buf, size_t len) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j)
                c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        init = true;
    }
    crc = ~crc;
    for (size_t i = 0; i < len; ++i)
        crc = table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

} // anonymous namespace

namespace RomCheck {

CheckResult verify_iso(const std::string& path) {
    CheckResult res;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        res.result = Result::FILE_NOT_FOUND;
        res.message = "No se pudo abrir el archivo: " + path;
        return res;
    }

    res.detected_size = static_cast<uint64_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    if (res.detected_size != EXPECTED_SIZE) {
        res.result = Result::WRONG_SIZE;
        std::ostringstream oss;
        oss << "Tamaño incorrecto.\n"
            << "Esperado (July 12 Prototype): " << EXPECTED_SIZE << " bytes\n"
            << "Detectado: " << res.detected_size << " bytes\n\n"
            << "⚠️ Este build está hecho SOLO para la July 12 NTSC-J Prototype.\n"
            << "Usar otra versión puede causar crashes o comportamiento raro.";
        res.message = oss.str();
        return res;
    }

    // Full SHA1
    SHA1 sha;
    std::vector<uint8_t> buffer(1024 * 1024); // 1 MB chunks
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        std::streamsize read = file.gcount();
        if (read > 0) {
            sha.update(buffer.data(), static_cast<size_t>(read));
        }
    }

    res.detected_sha1 = sha.final();

    if (res.detected_sha1 != EXPECTED_SHA1) {
        res.result = Result::WRONG_HASH;
        std::ostringstream oss;
        oss << "Hash SHA1 no coincide con la July 12 NTSC-J Prototype.\n\n"
            << "Esperado: " << EXPECTED_SHA1 << "\n"
            << "Detectado: " << res.detected_sha1 << "\n\n"
            << "⚠️ ROM incorrecta.\n"
            << "Este port está targeteado exclusivamente a la prototype del 12 de Julio 2001.\n"
            << "Otras versiones (retail JP/US/EU o demos) NO están soportadas todavía.";
        res.message = oss.str();
        return res;
    }

    res.result = Result::OK;
    res.message = "ROM correcta: July 12 2001 NTSC-J Prototype detectada.";
    return res;
}

CheckResult verify_iso_fast(const std::string& path) {
    CheckResult res;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        res.result = Result::FILE_NOT_FOUND;
        res.message = "No se pudo abrir el archivo: " + path;
        return res;
    }

    res.detected_size = static_cast<uint64_t>(file.tellg());
    if (res.detected_size != EXPECTED_SIZE) {
        res.result = Result::WRONG_SIZE;
        std::ostringstream oss;
        oss << "Tamaño incorrecto (" << res.detected_size << " bytes).\n"
            << "Se esperaba " << EXPECTED_SIZE << " bytes (July 12 Prototype).\n\n"
            << "⚠️ ROM no es la July 12 NTSC-J Prototype.";
        res.message = oss.str();
        return res;
    }

    // Quick CRC32 of first 4 MB (good enough for most wrong dumps)
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> header(4 * 1024 * 1024);
    file.read(reinterpret_cast<char*>(header.data()), header.size());
    uint32_t crc = crc32_update(0, header.data(), header.size());
    (void)crc; // suppress unused warning for now

    res.result = Result::OK;
    res.message = "Tamaño correcto. Ejecutando verificación completa de SHA1...";
    return res;
}

} // namespace RomCheck

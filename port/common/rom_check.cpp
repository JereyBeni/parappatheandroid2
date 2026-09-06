#include "rom_check.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>

namespace {
struct SHA1 {
    uint32_t h0=0x67452301,h1=0xEFCDAB89,h2=0x98BADCFE,h3=0x10325476,h4=0xC3D2E1F0;
    uint64_t total = 0;
    uint8_t buf[64];
    size_t buf_len = 0;
    static uint32_t rol(uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }
    void block(const uint8_t* p) {
        uint32_t w[80];
        for (int i = 0; i < 16; i++)
            w[i] = ((uint32_t)p[i*4]<<24)|((uint32_t)p[i*4+1]<<16)|((uint32_t)p[i*4+2]<<8)|p[i*4+3];
        for (int i = 16; i < 80; i++) w[i] = rol(w[i-3]^w[i-8]^w[i-14]^w[i-16], 1);
        uint32_t a=h0,b=h1,c=h2,d=h3,e=h4;
        for (int i = 0; i < 80; i++) {
            uint32_t f,k;
            if (i < 20) { f=(b&c)|((~b)&d); k=0x5A827999; }
            else if (i < 40) { f=b^c^d; k=0x6ED9EBA1; }
            else if (i < 60) { f=(b&c)|(b&d)|(c&d); k=0x8F1BBCDC; }
            else { f=b^c^d; k=0xCA62C1D6; }
            uint32_t t = rol(a,5)+f+e+k+w[i]; e=d; d=c; c=rol(b,30); b=a; a=t;
        }
        h0+=a; h1+=b; h2+=c; h3+=d; h4+=e;
    }
    void update(const void* data, size_t len) {
        const uint8_t* p = (const uint8_t*)data;
        total += len;
        while (len) {
            size_t n = std::min(len, 64 - buf_len);
            memcpy(buf + buf_len, p, n);
            buf_len += n; p += n; len -= n;
            if (buf_len == 64) { block(buf); buf_len = 0; }
        }
    }
    std::string final_hex() {
        uint64_t bits = total * 8;
        uint8_t pad = 0x80;
        update(&pad, 1);
        pad = 0;
        while (buf_len != 56) update(&pad, 1);
        uint8_t lenb[8];
        for (int i = 0; i < 8; i++) lenb[7-i] = (uint8_t)(bits >> (i*8));
        update(lenb, 8);
        std::ostringstream o;
        o << std::hex << std::setfill('0');
        for (uint32_t h : {h0,h1,h2,h3,h4}) o << std::setw(8) << h;
        return o.str();
    }
};
}

namespace RomCheck {

Report verify_iso_fast(const std::string& path) {
    Report r;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) { r.message = "cannot open"; return r; }
#ifdef _WIN32
    _fseeki64(f, 0, SEEK_END);
    r.size = (uint64_t)_ftelli64(f);
#else
    fseeko(f, 0, SEEK_END);
    r.size = (uint64_t)ftello(f);
#endif
    fclose(f);
    if (r.size != EXPECTED_SIZE) {
        r.result = Result::FAIL_SIZE;
        r.message = "size " + std::to_string(r.size) + " != " + std::to_string(EXPECTED_SIZE);
        return r;
    }
    r.result = Result::OK;
    r.message = "size OK (July 12 prototype)";
    return r;
}

Report verify_iso(const std::string& path) {
    Report r = verify_iso_fast(path);
    if (r.result != Result::OK) return r;
    std::ifstream in(path, std::ios::binary);
    if (!in) { r.result = Result::FAIL_OPEN; r.message = "cannot read"; return r; }
    SHA1 sha;
    std::vector<char> buf(1024 * 1024);
    while (in) {
        in.read(buf.data(), (std::streamsize)buf.size());
        auto n = in.gcount();
        if (n > 0) sha.update(buf.data(), (size_t)n);
    }
    r.detected_sha1 = sha.final_hex();
    if (r.detected_sha1 != EXPECTED_SHA1) {
        r.result = Result::FAIL_HASH;
        r.message = "SHA1 mismatch: " + r.detected_sha1;
        return r;
    }
    r.result = Result::OK;
    r.message = "July 12 prototype verified";
    return r;
}

} // namespace RomCheck

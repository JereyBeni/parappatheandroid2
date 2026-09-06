#include "iso9660.h"
#include <cstdio>
#include <cstring>
#include <cctype>

#ifdef _WIN32
#define FSEEK64(fp, off, wh) _fseeki64((fp), (off), (wh))
#else
#define FSEEK64(fp, off, wh) fseeko((fp), (off), (wh))
#endif

namespace Iso9660 {

static std::string to_upper(std::string s) {
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}
static bool ieq(const std::string& a, const std::string& b) { return to_upper(a) == to_upper(b); }
static uint32_t ru32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}

bool Image::open(const std::string& path) {
    close();
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) { m_error = "open failed: " + path; return false; }
    m_fp = fp;
    if (!parse_pvd()) { close(); return false; }
    m_ok = true;
    return true;
}

void Image::close() {
    if (m_fp) { fclose((FILE*)m_fp); m_fp = nullptr; }
    m_ok = false; m_error.clear(); m_volume_id.clear();
}

bool Image::read_sectors(uint32_t lba, uint32_t count, std::vector<uint8_t>& out) {
    FILE* fp = (FILE*)m_fp;
    if (!fp) return false;
    long long off = (long long)lba * 2048;
    if (FSEEK64(fp, off, SEEK_SET) != 0) return false;
    out.resize((size_t)count * 2048);
    return fread(out.data(), 2048, count, fp) == count;
}

bool Image::parse_pvd() {
    std::vector<uint8_t> sec;
    if (!read_sectors(16, 1, sec)) { m_error = "PVD read fail"; return false; }
    if (sec[0] != 1 || memcmp(&sec[1], "CD001", 5) != 0) {
        m_error = "not ISO9660"; return false;
    }
    m_volume_id.assign((char*)&sec[40], 32);
    while (!m_volume_id.empty() && (m_volume_id.back() == ' ' || m_volume_id.back() == 0))
        m_volume_id.pop_back();
    m_root_lba = ru32(&sec[156 + 2]);
    m_root_size = ru32(&sec[156 + 10]);
    return m_root_lba && m_root_size;
}

void Image::walk_dir(uint32_t lba, uint32_t size, const std::string& prefix, std::vector<FileEntry>& out) {
    uint32_t sectors = (size + 2047) / 2048;
    std::vector<uint8_t> data;
    if (!read_sectors(lba, sectors, data)) return;
    size_t pos = 0;
    while (pos + 33 < data.size()) {
        uint8_t len = data[pos];
        if (!len) { size_t n = ((pos / 2048) + 1) * 2048; if (n <= pos) break; pos = n; continue; }
        if (pos + len > data.size()) break;
        uint8_t name_len = data[pos + 32];
        uint8_t flags = data[pos + 25];
        uint32_t extent = ru32(&data[pos + 2]);
        uint32_t esize = ru32(&data[pos + 10]);
        if (name_len == 1 && (data[pos + 33] == 0 || data[pos + 33] == 1)) { pos += len; continue; }
        std::string name((char*)&data[pos + 33], name_len);
        auto sc = name.find(';'); if (sc != std::string::npos) name = name.substr(0, sc);
        bool is_dir = (flags & 2) != 0;
        std::string full = prefix.empty() ? name : prefix + "/" + name;
        out.push_back({full, extent, esize, is_dir});
        if (is_dir) walk_dir(extent, esize, full, out);
        pos += len;
    }
}

std::vector<FileEntry> Image::list_all() {
    std::vector<FileEntry> out;
    if (m_ok) walk_dir(m_root_lba, m_root_size, "", out);
    return out;
}

std::vector<uint8_t> Image::read_file(const std::string& path) {
    if (!m_ok) return {};
    for (auto& e : list_all()) {
        if (!e.is_dir && ieq(e.path, path)) {
            uint32_t sec = (e.size + 2047) / 2048;
            std::vector<uint8_t> data;
            if (!read_sectors(e.lba, sec, data)) return {};
            data.resize(e.size);
            return data;
        }
    }
    return {};
}

} // namespace Iso9660

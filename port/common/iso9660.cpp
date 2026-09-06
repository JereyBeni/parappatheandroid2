#include "iso9660.h"

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#define FSEEK64(fp, off, wh) _fseeki64((fp), (off), (wh))
#else
#include <sys/types.h>
#define MKDIR(p) mkdir(p, 0755)
#define FSEEK64(fp, off, wh) fseeko((fp), (off), (wh))
#endif

namespace Iso9660 {

static std::string to_upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

static bool ieq(const std::string& a, const std::string& b) {
    return to_upper(a) == to_upper(b);
}

static void ensure_parent_dirs(const std::string& path) {
    std::string cur;
    for (size_t i = 0; i < path.size(); ++i) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            if (!cur.empty()) MKDIR(cur.c_str());
        }
        cur.push_back(c == '\\' ? '/' : c);
    }
}

bool Image::open(const std::string& path) {
    close();
    m_path = path;
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) {
        m_error = "No se pudo abrir: " + path;
        return false;
    }
    m_fp = fp;
    if (!parse_pvd()) {
        close();
        return false;
    }
    m_ok = true;
    return true;
}

void Image::close() {
    if (m_fp) {
        fclose(static_cast<FILE*>(m_fp));
        m_fp = nullptr;
    }
    m_ok = false;
    m_error.clear();
    m_volume_id.clear();
    m_root_lba = m_root_size = 0;
}

bool Image::read_sectors(uint32_t lba, uint32_t count, std::vector<uint8_t>& out) {
    FILE* fp = static_cast<FILE*>(m_fp);
    if (!fp) return false;
    const uint32_t sector = 2048;
    long long off = static_cast<long long>(lba) * static_cast<long long>(sector);
    if (FSEEK64(fp, off, SEEK_SET) != 0) return false;
    out.resize(static_cast<size_t>(count) * sector);
    size_t got = fread(out.data(), sector, count, fp);
    return got == count;
}

bool Image::parse_pvd() {
    std::vector<uint8_t> sec;
    if (!read_sectors(16, 1, sec)) {
        m_error = "No se pudo leer PVD (sector 16). No parece ISO9660.";
        return false;
    }
    if (sec[0] != 1 || memcmp(&sec[1], "CD001", 5) != 0) {
        m_error = "PVD invalido. El .bin no es ISO9660 estandar.";
        return false;
    }
    m_volume_id.assign(reinterpret_cast<char*>(&sec[40]), 32);
    while (!m_volume_id.empty() && (m_volume_id.back() == ' ' || m_volume_id.back() == '\0'))
        m_volume_id.pop_back();

    auto rd32 = [&](size_t off) -> uint32_t {
        return (uint32_t)sec[off] | ((uint32_t)sec[off+1] << 8) |
               ((uint32_t)sec[off+2] << 16) | ((uint32_t)sec[off+3] << 24);
    };
    m_root_lba = rd32(156 + 2);
    m_root_size = rd32(156 + 10);
    if (m_root_lba == 0 || m_root_size == 0) {
        m_error = "Root directory invalido en PVD.";
        return false;
    }
    return true;
}

void Image::walk_dir(uint32_t lba, uint32_t size, const std::string& prefix,
                     std::vector<FileEntry>& out) {
    uint32_t sectors = (size + 2047) / 2048;
    std::vector<uint8_t> data;
    if (!read_sectors(lba, sectors, data)) return;

    size_t pos = 0;
    while (pos + 33 < data.size()) {
        uint8_t len = data[pos];
        if (len == 0) {
            size_t next = ((pos / 2048) + 1) * 2048;
            if (next <= pos) break;
            pos = next;
            continue;
        }
        if (pos + len > data.size()) break;

        uint8_t name_len = data[pos + 32];
        uint8_t flags = data[pos + 25];
        auto rd32 = [&](size_t off) -> uint32_t {
            return (uint32_t)data[off] | ((uint32_t)data[off+1] << 8) |
                   ((uint32_t)data[off+2] << 16) | ((uint32_t)data[off+3] << 24);
        };
        uint32_t extent = rd32(pos + 2);
        uint32_t esize = rd32(pos + 10);

        if (name_len == 1 && (data[pos + 33] == 0 || data[pos + 33] == 1)) {
            pos += len;
            continue;
        }
        std::string name(reinterpret_cast<char*>(&data[pos + 33]), name_len);
        auto sc = name.find(';');
        if (sc != std::string::npos) name = name.substr(0, sc);

        bool is_dir = (flags & 0x02) != 0;
        std::string full = prefix.empty() ? name : (prefix + "/" + name);

        FileEntry e;
        e.path = full;
        e.lba = extent;
        e.size = esize;
        e.is_dir = is_dir;
        out.push_back(e);

        if (is_dir) walk_dir(extent, esize, full, out);
        pos += len;
    }
}

std::vector<FileEntry> Image::list_all() {
    std::vector<FileEntry> out;
    if (!m_ok) return out;
    walk_dir(m_root_lba, m_root_size, "", out);
    return out;
}

std::vector<uint8_t> Image::read_file(const std::string& path) {
    if (!m_ok) return {};
    auto all = list_all();
    for (auto& e : all) {
        if (e.is_dir) continue;
        if (ieq(e.path, path)) {
            uint32_t sectors = (e.size + 2047) / 2048;
            std::vector<uint8_t> data;
            if (!read_sectors(e.lba, sectors, data)) return {};
            data.resize(e.size);
            return data;
        }
    }
    return {};
}

bool Image::extract_file(const std::string& iso_path, const std::string& out_path) {
    auto data = read_file(iso_path);
    if (data.empty()) return false;
    ensure_parent_dirs(out_path);
    FILE* out = fopen(out_path.c_str(), "wb");
    if (!out) return false;
    size_t w = fwrite(data.data(), 1, data.size(), out);
    fclose(out);
    return w == data.size();
}

Image::ExtractReport Image::extract_decomp_essentials(const std::string& out_dir) {
    ExtractReport rep;
    if (!m_ok) {
        rep.messages.push_back("Image no abierta");
        return rep;
    }

    auto all = list_all();

    auto find_ci = [&](const std::string& want) -> const FileEntry* {
        for (auto& e : all) {
            if (!e.is_dir && ieq(e.path, want)) return &e;
        }
        for (auto& e : all) {
            if (e.is_dir) continue;
            auto slash = e.path.find_last_of("/");
            std::string base = (slash == std::string::npos) ? e.path : e.path.substr(slash + 1);
            if (ieq(base, want)) return &e;
        }
        return nullptr;
    };

    auto pull = [&](const std::string& iso_name, const std::string& rel_out) {
        const FileEntry* e = find_ci(iso_name);
        if (!e) e = find_ci(rel_out);
        if (!e) {
            rep.missing++;
            rep.messages.push_back("MISSING: " + iso_name);
            return;
        }
        std::string outp = out_dir + "/" + rel_out;
        if (extract_file(e->path, outp)) {
            rep.extracted++;
            rep.messages.push_back("OK: " + e->path + " -> " + rel_out +
                                   " (" + std::to_string(e->size) + " bytes)");
        } else {
            rep.missing++;
            rep.messages.push_back("FAIL extract: " + e->path);
        }
    };

    pull("SCPS_150.17", "SCPS_150.17");
    pull("TAPCTRL.IRX", "IRX/TAPCTRL.IRX");
    pull("WAVE2PS2.IRX", "IRX/WAVE2PS2.IRX");

    for (auto& e : all) {
        if (e.is_dir) continue;
        auto up = to_upper(e.path);
        if (up.size() >= 4 && up.compare(up.size() - 4, 4, ".OLM") == 0) {
            std::string rel = e.path;
            if (up.find("MDL/") == std::string::npos) {
                auto slash = e.path.find_last_of("/");
                std::string base = (slash == std::string::npos) ? e.path : e.path.substr(slash + 1);
                rel = std::string("MDL/") + base;
            }
            std::string outp = out_dir + "/" + rel;
            if (extract_file(e.path, outp)) {
                rep.extracted++;
                rep.messages.push_back("OK: " + e.path + " -> " + rel +
                                       " (" + std::to_string(e.size) + " bytes)");
            } else {
                rep.missing++;
                rep.messages.push_back("FAIL: " + e.path);
            }
        }
    }

    return rep;
}

} // namespace Iso9660

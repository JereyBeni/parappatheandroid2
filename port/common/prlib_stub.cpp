#include "prlib_stub.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace PrlibStub {

static uint16_t ru16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t ru32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static std::vector<uint8_t> read_all(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    f.seekg(0, std::ios::end);
    auto n = f.tellg();
    if (n <= 0) return {};
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf((size_t)n);
    f.read(reinterpret_cast<char*>(buf.data()), n);
    return buf;
}

static const char* image_type_name(uint8_t t) {
    switch (t) {
    case TIM2_RGB16: return "RGB16";
    case TIM2_RGB24: return "RGB24";
    case TIM2_RGB32: return "RGB32";
    case TIM2_IDTEX4: return "IDTEX4";
    case TIM2_IDTEX8: return "IDTEX8";
    default: return "UNKNOWN";
    }
}

// Decode a single TIM2 picture to RGBA8888 when possible
static void decode_picture(const uint8_t* base, size_t total_size,
                           size_t pic_off, Tim2PictureInfo& pic) {
    if (pic_off + 0x30 > total_size) return;
    const uint8_t* ph = base + pic_off;

    uint32_t total = ru32(ph + 0x00);
    uint32_t clut_size = ru32(ph + 0x04);
    uint32_t image_size = ru32(ph + 0x08);
    uint16_t header_size = ru16(ph + 0x0c);
    pic.clut_colors = ru16(ph + 0x0e);
    pic.clut_type = ph[0x12];
    pic.image_type = ph[0x13];
    pic.width = ru16(ph + 0x14);
    pic.height = ru16(ph + 0x16);
    pic.image_size = image_size;
    pic.clut_size = clut_size;

    if (header_size < 0x30) return;
    if (pic_off + total > total_size) return;

    const uint8_t* image = ph + header_size;
    const uint8_t* clut = nullptr;
    if (clut_size > 0) {
        clut = ph + header_size + image_size;
    }

    size_t px = (size_t)pic.width * (size_t)pic.height;
    if (px == 0 || px > 16 * 1024 * 1024) return;

    pic.rgba.assign(px * 4, 0);

    auto write_rgba = [&](size_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (i >= px) return;
        pic.rgba[i * 4 + 0] = r;
        pic.rgba[i * 4 + 1] = g;
        pic.rgba[i * 4 + 2] = b;
        pic.rgba[i * 4 + 3] = a;
    };

    // PS2 16-bit color A1B5G5R5 -> RGBA
    auto from_rgb16 = [](uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        r = (uint8_t)((c & 0x1f) << 3);
        g = (uint8_t)(((c >> 5) & 0x1f) << 3);
        b = (uint8_t)(((c >> 10) & 0x1f) << 3);
        a = (c & 0x8000) ? 255 : 0;
    };

    if (pic.image_type == TIM2_RGB32 && image_size >= px * 4) {
        for (size_t i = 0; i < px; ++i) {
            write_rgba(i, image[i * 4 + 0], image[i * 4 + 1], image[i * 4 + 2], image[i * 4 + 3]);
        }
    } else if (pic.image_type == TIM2_RGB24 && image_size >= px * 3) {
        for (size_t i = 0; i < px; ++i) {
            write_rgba(i, image[i * 3 + 0], image[i * 3 + 1], image[i * 3 + 2], 255);
        }
    } else if (pic.image_type == TIM2_RGB16 && image_size >= px * 2) {
        for (size_t i = 0; i < px; ++i) {
            uint16_t c = ru16(image + i * 2);
            uint8_t r, g, b, a;
            from_rgb16(c, r, g, b, a);
            write_rgba(i, r, g, b, a ? 255 : 0);
        }
    } else if (pic.image_type == TIM2_IDTEX8 && clut && image_size >= px) {
        for (size_t i = 0; i < px; ++i) {
            uint8_t idx = image[i];
            if (pic.clut_type == TIM2_RGB32 && (size_t)idx * 4 + 3 < clut_size) {
                write_rgba(i, clut[idx * 4 + 0], clut[idx * 4 + 1], clut[idx * 4 + 2], clut[idx * 4 + 3]);
            } else if (pic.clut_type == TIM2_RGB16 && (size_t)idx * 2 + 1 < clut_size) {
                uint16_t c = ru16(clut + idx * 2);
                uint8_t r, g, b, a;
                from_rgb16(c, r, g, b, a);
                write_rgba(i, r, g, b, a ? 255 : 0);
            }
        }
    } else if (pic.image_type == TIM2_IDTEX4 && clut && image_size >= (px + 1) / 2) {
        for (size_t i = 0; i < px; ++i) {
            uint8_t byte = image[i / 2];
            uint8_t idx = (i & 1) ? (byte >> 4) : (byte & 0x0f);
            if (pic.clut_type == TIM2_RGB32 && (size_t)idx * 4 + 3 < clut_size) {
                write_rgba(i, clut[idx * 4 + 0], clut[idx * 4 + 1], clut[idx * 4 + 2], clut[idx * 4 + 3]);
            } else if (pic.clut_type == TIM2_RGB16 && (size_t)idx * 2 + 1 < clut_size) {
                uint16_t c = ru16(clut + idx * 2);
                uint8_t r, g, b, a;
                from_rgb16(c, r, g, b, a);
                write_rgba(i, r, g, b, a ? 255 : 0);
            }
        }
    } else {
        // leave rgba empty -> unsupported for preview
        pic.rgba.clear();
    }
}

Tim2Info parse_tim2(const uint8_t* data, size_t size) {
    Tim2Info info;
    if (!data || size < 16) {
        info.error = "buffer demasiado chico";
        return info;
    }
    if (memcmp(data, "TIM2", 4) != 0) {
        info.error = "no es TIM2 (FileId)";
        return info;
    }
    info.format_version = data[4];
    info.pictures = ru16(data + 6);

    size_t off = 16; // after TIM2_FILEHEADER
    // FormatId 0 = 16-byte align, 1 = 128-byte align sometimes
    for (uint16_t i = 0; i < info.pictures && off + 0x30 <= size; ++i) {
        Tim2PictureInfo pic;
        decode_picture(data, size, off, pic);
        uint32_t total = ru32(data + off);
        info.pics.push_back(std::move(pic));
        if (total == 0) break;
        off += total;
    }
    info.ok = true;
    return info;
}

Tim2Info parse_tim2_file(const std::string& path) {
    auto buf = read_all(path);
    if (buf.empty()) {
        Tim2Info i;
        i.error = "no se pudo leer " + path;
        return i;
    }
    return parse_tim2(buf.data(), buf.size());
}

SpmInfo parse_spm(const uint8_t* data, size_t size) {
    SpmInfo info;
    info.file_size = size;
    if (!data || size < 0x20) {
        info.error = "buffer demasiado chico para SPM";
        return info;
    }
    info.magic = ru32(data + 0);
    info.version = ru16(data + 4);
    info.flags = ru16(data + 6);
    if (info.magic != SPM_MAGIC) {
        info.error = "magic SPM incorrecto";
        return info;
    }
    // node_num at offset 0x68 per model.h layout (after paddings)
    // From model.h:
    // 0x00 magic, 0x04 version, 0x06 flags, 0x08 pad 0x28 -> 0x30 vectors...
    // m_node_num at 0x68
    if (size >= 0x6C) {
        info.node_num = ru32(data + 0x68);
    }
    info.ok = true;
    return info;
}

SpmInfo parse_spm_file(const std::string& path) {
    auto buf = read_all(path);
    if (buf.empty()) {
        SpmInfo i;
        i.error = "no se pudo leer " + path;
        return i;
    }
    return parse_spm(buf.data(), buf.size());
}

static void scan_file(const std::string& path, ScanResult& res) {
    auto buf = read_all(path);
    if (buf.size() < 4) {
        res.other_count++;
        return;
    }

    // TIM2?
    if (buf.size() >= 4 && memcmp(buf.data(), "TIM2", 4) == 0) {
        auto t = parse_tim2(buf.data(), buf.size());
        res.tim2_count++;
        std::string line = "[TIM2] " + path;
        if (t.ok && !t.pics.empty()) {
            line += " pics=" + std::to_string(t.pictures);
            line += " " + std::to_string(t.pics[0].width) + "x" + std::to_string(t.pics[0].height);
            line += " " + std::string(image_type_name(t.pics[0].image_type));
            if (!t.pics[0].rgba.empty()) line += " decoded_ok";
        } else {
            line += " parse_fail: " + t.error;
        }
        res.lines.push_back(line);
        return;
    }

    // SPM?
    if (buf.size() >= 4 && ru32(buf.data()) == SPM_MAGIC) {
        auto s = parse_spm(buf.data(), buf.size());
        res.spm_count++;
        std::string line = "[SPM]  " + path;
        if (s.ok) {
            line += " ver=" + std::to_string(s.version);
            line += " nodes=" + std::to_string(s.node_num);
            line += " flags=0x" + [&]() {
                char b[16];
                snprintf(b, sizeof(b), "%04x", s.flags);
                return std::string(b);
            }();
            line += " size=" + std::to_string(s.file_size);
        } else {
            line += " " + s.error;
        }
        res.lines.push_back(line);
        return;
    }

    // Maybe OLM container: search for TIM2/SPM magic inside
    bool found = false;
    for (size_t i = 0; i + 4 <= buf.size(); i += 4) {
        if (memcmp(buf.data() + i, "TIM2", 4) == 0) {
            auto t = parse_tim2(buf.data() + i, buf.size() - i);
            res.tim2_count++;
            std::string line = "[TIM2@" + std::to_string(i) + "] " + path;
            if (t.ok && !t.pics.empty()) {
                line += " " + std::to_string(t.pics[0].width) + "x" + std::to_string(t.pics[0].height);
            }
            res.lines.push_back(line);
            found = true;
            break; // one hit enough for scan summary
        }
        if (ru32(buf.data() + i) == SPM_MAGIC) {
            auto s = parse_spm(buf.data() + i, buf.size() - i);
            res.spm_count++;
            std::string line = "[SPM@" + std::to_string(i) + "] " + path;
            if (s.ok) line += " nodes=" + std::to_string(s.node_num);
            res.lines.push_back(line);
            found = true;
            break;
        }
    }
    if (!found) {
        res.other_count++;
        res.lines.push_back("[?]    " + path + " (" + std::to_string(buf.size()) + " bytes)");
    }
}

#ifdef _WIN32
static void walk_dir(const std::string& dir, ScanResult& res) {
    std::string pattern = dir + "\\*";
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
        std::string full = dir + "\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            walk_dir(full, res);
        } else {
            scan_file(full, res);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}
#else
static void walk_dir(const std::string& dir, ScanResult& res) {
    DIR* d = opendir(dir.c_str());
    if (!d) return;
    while (auto* ent = readdir(d)) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        std::string full = dir + "/" + ent->d_name;
        struct stat st;
        if (stat(full.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) walk_dir(full, res);
        else scan_file(full, res);
    }
    closedir(d);
}
#endif

ScanResult scan_extracted_dir(const std::string& dir) {
    ScanResult res;
    walk_dir(dir, res);
    res.lines.insert(res.lines.begin(),
        "=== prlib asset scan: " + dir + " ===");
    res.lines.push_back("TIM2: " + std::to_string(res.tim2_count) +
                        " | SPM: " + std::to_string(res.spm_count) +
                        " | other: " + std::to_string(res.other_count));
    return res;
}

} // namespace PrlibStub

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

// Minimal ISO9660 reader for PS2 DVD .bin dumps (2048-byte sectors).
// Enough to list the FS and extract files the decomp cares about.

namespace Iso9660 {

struct FileEntry {
    std::string path;      // e.g. "IRX/WAVE2PS2.IRX"
    uint32_t lba = 0;      // start sector
    uint32_t size = 0;     // bytes
    bool is_dir = false;
};

class Image {
public:
    bool open(const std::string& path);
    void close();

    bool ok() const { return m_ok; }
    const std::string& error() const { return m_error; }
    const std::string& volume_id() const { return m_volume_id; }

    // Full recursive listing from root
    std::vector<FileEntry> list_all();

    // Extract one file by path (case-insensitive). Returns empty on fail.
    std::vector<uint8_t> read_file(const std::string& path);

    // Extract to disk. Returns false on fail.
    bool extract_file(const std::string& iso_path, const std::string& out_path);

    // Convenience: extract the files the decomp build guide asks for.
    // Writes into out_dir mirroring paths (SCPS_150.17, IRX/..., MDL/...)
    struct ExtractReport {
        int extracted = 0;
        int missing = 0;
        std::vector<std::string> messages;
    };
    ExtractReport extract_decomp_essentials(const std::string& out_dir);

private:
    bool read_sectors(uint32_t lba, uint32_t count, std::vector<uint8_t>& out);
    bool parse_pvd();
    void walk_dir(uint32_t lba, uint32_t size, const std::string& prefix,
                  std::vector<FileEntry>& out);

    std::string m_path;
    std::string m_error;
    std::string m_volume_id;
    bool m_ok = false;
    uint32_t m_root_lba = 0;
    uint32_t m_root_size = 0;
    // FILE* kept as void* to avoid leaking stdio in header
    void* m_fp = nullptr;
};

} // namespace Iso9660

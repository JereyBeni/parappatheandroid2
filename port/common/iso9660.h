#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Iso9660 {
struct FileEntry {
    std::string path;
    uint32_t lba = 0;
    uint32_t size = 0;
    bool is_dir = false;
};
class Image {
public:
    bool open(const std::string& path);
    void close();
    bool ok() const { return m_ok; }
    const std::string& error() const { return m_error; }
    const std::string& volume_id() const { return m_volume_id; }
    std::vector<FileEntry> list_all();
    std::vector<uint8_t> read_file(const std::string& path);
private:
    bool read_sectors(uint32_t lba, uint32_t count, std::vector<uint8_t>& out);
    bool parse_pvd();
    void walk_dir(uint32_t lba, uint32_t size, const std::string& prefix, std::vector<FileEntry>& out);
    std::string m_error, m_volume_id;
    bool m_ok = false;
    uint32_t m_root_lba = 0, m_root_size = 0;
    void* m_fp = nullptr;
};
} // namespace Iso9660

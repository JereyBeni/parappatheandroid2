#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Minimal prlib-facing asset parsers for the PC harness.
// Based on structures from parappadev/parappa2 src/prlib.

namespace PrlibStub {

// ---- TIM2 (Sony texture) ----
// Matches TIM2_FILEHEADER / TIM2_PICTUREHEADER from prlib/tim2.h

enum Tim2ImageType : uint8_t {
    TIM2_NONE   = 0,
    TIM2_RGB16  = 1,
    TIM2_RGB24  = 2,
    TIM2_RGB32  = 3,
    TIM2_IDTEX4 = 4,
    TIM2_IDTEX8 = 5,
};

struct Tim2PictureInfo {
    uint16_t width = 0;
    uint16_t height = 0;
    uint8_t image_type = 0;
    uint8_t clut_type = 0;
    uint16_t clut_colors = 0;
    uint32_t image_size = 0;
    uint32_t clut_size = 0;
    // RGBA8888 decoded preview (may be empty if format unsupported yet)
    std::vector<uint8_t> rgba;
};

struct Tim2Info {
    bool ok = false;
    std::string error;
    uint8_t format_version = 0;
    uint16_t pictures = 0;
    std::vector<Tim2PictureInfo> pics;
};

// Parse a TIM2 blob in memory (FileId == "TIM2")
Tim2Info parse_tim2(const uint8_t* data, size_t size);
Tim2Info parse_tim2_file(const std::string& path);

// ---- SPM (prlib model) ----
// SpmFileHeader: magic 0x18df540a, version 5

constexpr uint32_t SPM_MAGIC = 0x18df540a;
constexpr uint16_t SPM_VERSION = 5;

struct SpmInfo {
    bool ok = false;
    std::string error;
    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t flags = 0;
    uint32_t node_num = 0;
    size_t file_size = 0;
};

// Probe if buffer looks like SPM and read header fields
SpmInfo parse_spm(const uint8_t* data, size_t size);
SpmInfo parse_spm_file(const std::string& path);

// ---- Scan extracted_iso folder ----
struct ScanResult {
    int tim2_count = 0;
    int spm_count = 0;
    int other_count = 0;
    std::vector<std::string> lines;
};

// Recursively scan a directory for TIM2 / SPM signatures
ScanResult scan_extracted_dir(const std::string& dir);

} // namespace PrlibStub

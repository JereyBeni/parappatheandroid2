#pragma once

#include <string>
#include <cstdint>

// July 12 2001 NTSC-J Prototype (Hidden Palace dump)
// File: PS2 - Parappa 7-12-07.bin
namespace RomCheck {

constexpr uint64_t EXPECTED_SIZE = 4159078400ULL;
constexpr const char* EXPECTED_SHA1 = "28964c33cee578ec3ce476285067044243363d08";
constexpr const char* EXPECTED_MD5  = "6ed6bc34ecfbafd2c4d9a12c85edb54f";
constexpr uint32_t EXPECTED_CRC32   = 0xF9C41366;

enum class Result {
    OK,                 // Exact match - July 12 prototype
    WRONG_SIZE,
    WRONG_HASH,
    FILE_NOT_FOUND,
    READ_ERROR
};

struct CheckResult {
    Result result;
    std::string message;
    std::string detected_sha1;
    uint64_t detected_size = 0;
};

// Returns detailed result. Does a full SHA1 of the file (slow on big ISOs).
CheckResult verify_iso(const std::string& path);

// Fast path: only checks size + first/last few MB + CRC32 of header region.
// Good enough for a warning in most cases.
CheckResult verify_iso_fast(const std::string& path);

} // namespace RomCheck

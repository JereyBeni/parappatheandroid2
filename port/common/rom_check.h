#pragma once
#include <cstdint>
#include <string>

namespace RomCheck {
enum class Result { OK, FAIL_SIZE, FAIL_OPEN, FAIL_HASH };
struct Report {
    Result result = Result::FAIL_OPEN;
    std::string message;
    std::string detected_sha1;
    uint64_t size = 0;
};
// July 12 2001 NTSC-J prototype
constexpr uint64_t EXPECTED_SIZE = 4159078400ULL;
constexpr const char* EXPECTED_SHA1 = "28964c33cee578ec3ce476285067044243363d08";
Report verify_iso_fast(const std::string& path); // size only
Report verify_iso(const std::string& path);      // size + sha1
} // namespace RomCheck

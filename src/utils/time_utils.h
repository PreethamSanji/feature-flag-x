#pragma once

#include <string>
#include <chrono>
#include <format>

namespace ffx {

/// Get current UTC time as ISO 8601 string.
inline std::string utc_now_iso8601() {
    auto now = std::chrono::system_clock::now();
    return std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(now));
}

} // namespace ffx

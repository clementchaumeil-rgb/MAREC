#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace marec {

class TimeUtils {
public:
    // Parse a timecode string "HH:MM:SS:FF" to samples given fps and sample rate.
    static std::optional<int64_t> parseTimecode(
        const std::string& tc, int32_t sampleRate, double fps = 25.0);

    // Parse a "MM:SS.mmm" MinSecs string to samples.
    static std::optional<int64_t> parseMinSecs(
        const std::string& ms, int32_t sampleRate);

    // Parse a bars|beats string "BBB|BB|TTTT" — not supported locally, returns nullopt.
    static std::optional<int64_t> parseBarsBeats(const std::string& bb);

    // Try to parse a raw time string as samples (integer), timecode, or min:secs.
    // Returns nullopt if format is unrecognized.
    static std::optional<int64_t> parseAuto(
        const std::string& timeStr, int32_t sampleRate, double fps = 25.0);

    // Format samples to a human-readable string "HH:MM:SS.mmm".
    static std::string formatSamplesToTime(int64_t samples, int32_t sampleRate);
};

} // namespace marec

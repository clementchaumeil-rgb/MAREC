#include "time_utils.h"

#include <charconv>
#include <cmath>
#include <iomanip>
#include <regex>
#include <sstream>

namespace marec {

std::optional<int64_t> TimeUtils::parseTimecode(
    const std::string& tc, int32_t sampleRate, double fps)
{
    // Format: HH:MM:SS:FF or HH:MM:SS;FF (drop-frame uses semicolon)
    std::regex re(R"((\d{1,2}):(\d{2}):(\d{2})[:;](\d{2}))");
    std::smatch match;
    if (!std::regex_match(tc, match, re)) {
        return std::nullopt;
    }

    int hours = std::stoi(match[1].str());
    int minutes = std::stoi(match[2].str());
    int seconds = std::stoi(match[3].str());
    int frames = std::stoi(match[4].str());

    double totalSeconds = hours * 3600.0 + minutes * 60.0 + seconds + frames / fps;
    return static_cast<int64_t>(std::round(totalSeconds * sampleRate));
}

std::optional<int64_t> TimeUtils::parseMinSecs(
    const std::string& ms, int32_t sampleRate)
{
    // Format: MM:SS.mmm or M:SS.mmm
    std::regex re(R"((\d+):(\d{2})\.(\d{3}))");
    std::smatch match;
    if (!std::regex_match(ms, match, re)) {
        return std::nullopt;
    }

    int minutes = std::stoi(match[1].str());
    int seconds = std::stoi(match[2].str());
    int millis = std::stoi(match[3].str());

    double totalSeconds = minutes * 60.0 + seconds + millis / 1000.0;
    return static_cast<int64_t>(std::round(totalSeconds * sampleRate));
}

std::optional<int64_t> TimeUtils::parseBarsBeats(const std::string& /*bb*/)
{
    // Bars|beats requires tempo map info; cannot convert locally.
    return std::nullopt;
}

std::optional<int64_t> TimeUtils::parseAuto(
    const std::string& timeStr, int32_t sampleRate, double fps)
{
    if (timeStr.empty()) {
        return std::nullopt;
    }

    // Try pure integer (samples)
    int64_t samples = 0;
    auto [ptr, ec] = std::from_chars(timeStr.data(), timeStr.data() + timeStr.size(), samples);
    if (ec == std::errc{} && ptr == timeStr.data() + timeStr.size()) {
        return samples;
    }

    // Try timecode HH:MM:SS:FF
    if (auto tc = parseTimecode(timeStr, sampleRate, fps)) {
        return tc;
    }

    // Try min:secs MM:SS.mmm
    if (auto ms = parseMinSecs(timeStr, sampleRate)) {
        return ms;
    }

    return std::nullopt;
}

std::string TimeUtils::formatSamplesToTime(int64_t samples, int32_t sampleRate)
{
    if (sampleRate <= 0) {
        return std::to_string(samples) + " smp";
    }

    double totalSeconds = static_cast<double>(samples) / sampleRate;
    int hours = static_cast<int>(totalSeconds / 3600);
    int minutes = static_cast<int>(std::fmod(totalSeconds, 3600) / 60);
    double secs = std::fmod(totalSeconds, 60.0);

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::fixed << std::setprecision(3) << std::setfill('0') << std::setw(6) << secs;
    return oss.str();
}

} // namespace marec

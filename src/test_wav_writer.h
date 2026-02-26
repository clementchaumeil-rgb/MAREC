#pragma once

#include <cstdint>
#include <string>

namespace marec {

class TestWavWriter {
public:
    /// Generate a minimal mono BWF (Broadcast Wave) file of silence.
    /// Includes a bext chunk with TimeReference so Pro Tools sets original_timestamp_point.
    /// @param path              Output file path (e.g. "/tmp/test/clip.wav")
    /// @param sampleRate        Sample rate in Hz (default 48000)
    /// @param durationSamples   Number of samples (default 48000 = 1 second at 48kHz)
    /// @param bitDepth          Bits per sample (default 24)
    /// @param timeReference     Timeline sample position for BWF timestamp (default 0)
    /// @return true on success
    static bool generate(const std::string& path,
        int32_t sampleRate = 48000,
        int32_t durationSamples = 48000,
        int32_t bitDepth = 24,
        int64_t timeReference = 0);
};

} // namespace marec

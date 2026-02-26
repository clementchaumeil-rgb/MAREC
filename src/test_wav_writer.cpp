#include "test_wav_writer.h"

#include <cstring>
#include <fstream>
#include <vector>

namespace marec {

static void writeLE16(std::ofstream& f, uint16_t val) {
    f.put(static_cast<char>(val & 0xFF));
    f.put(static_cast<char>((val >> 8) & 0xFF));
}

static void writeLE32(std::ofstream& f, uint32_t val) {
    f.put(static_cast<char>(val & 0xFF));
    f.put(static_cast<char>((val >> 8) & 0xFF));
    f.put(static_cast<char>((val >> 16) & 0xFF));
    f.put(static_cast<char>((val >> 24) & 0xFF));
}

bool TestWavWriter::generate(const std::string& path,
    int32_t sampleRate, int32_t durationSamples, int32_t bitDepth,
    int64_t timeReference)
{
    int32_t numChannels = 1;
    int32_t bytesPerSample = bitDepth / 8;
    uint32_t dataSize = static_cast<uint32_t>(durationSamples * numChannels * bytesPerSample);

    // BWF bext chunk: 602 bytes minimum (EBU Tech 3285 v2)
    // We only need up to TimeReference + Version = 348 bytes
    static constexpr uint32_t bextChunkSize = 348;
    // Total RIFF size: 4 (WAVE) + 8+bext + 8+16 (fmt) + 8+data
    uint32_t riffSize = 4 + (8 + bextChunkSize) + (8 + 16) + (8 + dataSize);

    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;

    // ---- RIFF header ----
    f.write("RIFF", 4);
    writeLE32(f, riffSize);
    f.write("WAVE", 4);

    // ---- bext chunk (Broadcast Audio Extension) ----
    f.write("bext", 4);
    writeLE32(f, bextChunkSize);

    // Description: 256 bytes (null-padded)
    char description[256] = {};
    std::strncpy(description, "MAREC test clip", sizeof(description) - 1);
    f.write(description, 256);

    // Originator: 32 bytes
    char originator[32] = {};
    std::strncpy(originator, "MAREC", sizeof(originator) - 1);
    f.write(originator, 32);

    // OriginatorReference: 32 bytes
    char originatorRef[32] = {};
    f.write(originatorRef, 32);

    // OriginationDate: 10 bytes ("2026-01-01")
    f.write("2026-01-01", 10);

    // OriginationTime: 8 bytes ("00:00:00")
    f.write("00:00:00", 8);

    // TimeReferenceLow: lower 32 bits of sample position
    writeLE32(f, static_cast<uint32_t>(timeReference & 0xFFFFFFFF));
    // TimeReferenceHigh: upper 32 bits
    writeLE32(f, static_cast<uint32_t>((timeReference >> 32) & 0xFFFFFFFF));

    // Version: 2 bytes (BWF version 2)
    writeLE16(f, 2);

    // ---- fmt chunk ----
    f.write("fmt ", 4);
    writeLE32(f, 16); // chunk size
    writeLE16(f, 1);  // PCM format
    writeLE16(f, static_cast<uint16_t>(numChannels));
    writeLE32(f, static_cast<uint32_t>(sampleRate));
    writeLE32(f, static_cast<uint32_t>(sampleRate * numChannels * bytesPerSample)); // byte rate
    writeLE16(f, static_cast<uint16_t>(numChannels * bytesPerSample)); // block align
    writeLE16(f, static_cast<uint16_t>(bitDepth));

    // ---- data chunk ----
    f.write("data", 4);
    writeLE32(f, dataSize);

    // Silence (zeros)
    std::vector<char> silence(dataSize, 0);
    f.write(silence.data(), static_cast<std::streamsize>(dataSize));

    return f.good();
}

} // namespace marec

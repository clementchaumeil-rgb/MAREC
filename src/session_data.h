#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace marec {

struct TrackInfo {
    std::string name;
    std::string id;
    int32_t type = 0; // TrackType enum: 2 = Audio
    int32_t index = 0;
};

struct Marker {
    int32_t number = 0;
    std::string name;
    std::string startTime;     // Raw string from PTSL (e.g. "48000" or "00:01:00:00")
    int64_t startSamples = -1; // Normalized to samples (-1 = not yet resolved)
    int32_t timeProperties = 0; // TimeProperties enum: 1 = Marker
};

struct PlaylistElement {
    std::string clipId;
    std::string clipName; // Resolved from clip list
    int64_t startSamples = 0;
    int64_t endSamples = 0;
};

struct ClipInfo {
    std::string clipId;
    std::string clipFullName;
    std::string clipRootName;
    int64_t startSamples = 0;
    int64_t endSamples = 0;
};

struct RenameAction {
    std::string oldName;
    std::string newName;
    int64_t startSamples = 0;
    std::string markerName;
};

struct ExportConfig {
    std::string outputDir;
    std::string fileType = "wav";  // "wav" or "aiff"
    int32_t bitDepth = 24;         // 16, 24, or 32
    bool enabled = false;
};

struct CliOptions {
    bool dryRun = false;
    bool allTracks = false;
    bool renameFile = false;
    bool help = false;
    ExportConfig exportConfig;
};

struct SessionInfo {
    int32_t sampleRate = 48000;
    std::string counterFormat; // e.g. "Samples", "MinSecs", "TimeCode", etc.
};

} // namespace marec

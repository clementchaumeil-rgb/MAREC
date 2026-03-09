#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace marec {

struct TextExportHeader {
    std::string sessionName;
    int32_t sampleRate = 0;
    std::string sessionStartTimecode;
    std::string timecodeFormat;
};

struct TextExportMarker {
    int32_t number = 0;
    std::string location;      // Timecode string (display only)
    int64_t timeReference = 0; // Samples — canonical position
    std::string name;
    std::string comments;
};

struct TextExportResult {
    TextExportHeader header;
    std::vector<TextExportMarker> markers;
    std::vector<std::string> warnings; // Non-fatal parse issues
};

class TextExportParser {
public:
    /// Parse a Pro Tools text export file from disk.
    static TextExportResult parse(const std::string& filePath);

    /// Parse from an in-memory string (for testability).
    static TextExportResult parseString(const std::string& content);
};

} // namespace marec

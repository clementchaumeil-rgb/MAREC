#include "text_export_parser.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace marec {

// ---- Helpers ----

static std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::vector<std::string> splitLines(const std::string& content)
{
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        // Remove trailing \r (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

static std::vector<std::string> splitTabs(const std::string& line)
{
    std::vector<std::string> fields;
    std::istringstream stream(line);
    std::string field;
    while (std::getline(stream, field, '\t')) {
        fields.push_back(trim(field));
    }
    return fields;
}

static std::string stripBOM(const std::string& content)
{
    if (content.size() >= 3 &&
        content[0] == '\xEF' && content[1] == '\xBB' && content[2] == '\xBF') {
        return content.substr(3);
    }
    return content;
}

// ---- Marker section detection ----

static const std::string MARKERS_SECTION_HEADER = "M A R K E R S  L I S T I N G";

static bool isSectionHeader(const std::string& line)
{
    // Pro Tools section headers are spaced-out uppercase letters like
    // "M A R K E R S  L I S T I N G" or "T R A C K  L I S T I N G"
    auto trimmed = trim(line);
    if (trimmed.empty()) return false;

    // Check for the specific markers header or a general pattern:
    // mostly uppercase letters separated by spaces
    if (trimmed == MARKERS_SECTION_HEADER) return true;

    // Other section headers follow the same pattern
    int spaceCount = 0;
    int letterCount = 0;
    for (char c : trimmed) {
        if (c == ' ') spaceCount++;
        else if (std::isupper(c)) letterCount++;
        else return false;
    }
    return letterCount >= 3 && spaceCount >= letterCount - 1;
}

// ---- Parser implementation ----

TextExportResult TextExportParser::parseString(const std::string& content)
{
    auto cleaned = stripBOM(content);

    if (trim(cleaned).empty()) {
        throw std::runtime_error("Empty text export content");
    }

    auto lines = splitLines(cleaned);
    TextExportResult result;

    // Phase 1: Parse header key-value pairs (before marker section)
    size_t lineIdx = 0;
    bool foundMarkerSection = false;

    for (; lineIdx < lines.size(); ++lineIdx) {
        auto trimmed = trim(lines[lineIdx]);

        if (trimmed == MARKERS_SECTION_HEADER) {
            foundMarkerSection = true;
            lineIdx++; // Move past the section header
            break;
        }

        // Parse "KEY:\tVALUE" header lines
        auto tabPos = lines[lineIdx].find('\t');
        if (tabPos != std::string::npos) {
            auto key = trim(lines[lineIdx].substr(0, tabPos));
            auto value = trim(lines[lineIdx].substr(tabPos + 1));

            // Remove trailing colon from key
            if (!key.empty() && key.back() == ':') {
                key.pop_back();
            }
            key = trim(key);

            if (key == "SESSION NAME") {
                result.header.sessionName = value;
            } else if (key == "SAMPLE RATE") {
                // "48000.000000" → 48000
                try {
                    result.header.sampleRate = static_cast<int32_t>(std::stod(value));
                } catch (...) {}
            } else if (key == "SESSION START TIMECODE") {
                result.header.sessionStartTimecode = value;
            } else if (key == "TIMECODE FORMAT") {
                result.header.timecodeFormat = value;
            }
        }
    }

    if (!foundMarkerSection) {
        throw std::runtime_error("No 'M A R K E R S  L I S T I N G' section found in text export");
    }

    // Skip the column header line
    if (lineIdx < lines.size()) {
        lineIdx++; // Skip "#   LOCATION   TIME REFERENCE ..."
    }

    // Phase 2: Parse marker rows
    for (; lineIdx < lines.size(); ++lineIdx) {
        const auto& line = lines[lineIdx];
        auto trimmed = trim(line);

        // Stop at empty line or next section header
        if (trimmed.empty()) break;
        if (isSectionHeader(trimmed)) break;

        auto fields = splitTabs(line);

        // Need at least 5 columns: #, LOCATION, TIME REFERENCE, UNITS, NAME
        if (fields.size() < 5) {
            result.warnings.push_back(
                "Row skipped (too few columns): " + line.substr(0, 60));
            continue;
        }

        TextExportMarker marker;

        // Column 0: marker number
        try {
            marker.number = std::stoi(fields[0]);
        } catch (...) {
            result.warnings.push_back(
                "Row skipped (invalid marker number): " + fields[0]);
            continue;
        }

        // Column 1: location timecode
        marker.location = fields[1];

        // Column 2: time reference (samples)
        try {
            marker.timeReference = std::stoll(fields[2]);
        } catch (...) {
            result.warnings.push_back(
                "Row skipped (invalid time reference): " + fields[2] +
                " for marker #" + std::to_string(marker.number));
            continue;
        }

        // Column 3: units (ignored, always "Samples")
        // Column 4: name
        marker.name = fields[4];

        // Column 7: comments (optional)
        if (fields.size() >= 8) {
            marker.comments = fields[7];
        }

        result.markers.push_back(std::move(marker));
    }

    return result;
}

TextExportResult TextExportParser::parse(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return parseString(ss.str());
}

} // namespace marec

#include "marker_matcher.h"

#include <algorithm>
#include <iomanip>
#include <regex>
#include <sstream>

namespace marec {

// Check if a clip name already matches the expected marker name.
// Matches: exact "MarkerName", or "MarkerName_NN" (MAREC suffix).
// This prevents non-idempotent suffix swaps when clip order varies between runs.
static bool hasCorrectMarkerPrefix(const std::string& clipName, const std::string& markerName)
{
    if (clipName == markerName) return true;
    if (clipName.size() <= markerName.size() + 1) return false;

    // Check prefix + "_" + digits (e.g. "Location 11_02")
    if (clipName.compare(0, markerName.size(), markerName) != 0) return false;
    if (clipName[markerName.size()] != '_') return false;

    for (size_t i = markerName.size() + 1; i < clipName.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(clipName[i]))) return false;
    }
    return true;
}

MatchResult MarkerMatcher::match(
    const std::vector<Marker>& markers,
    const std::vector<PlaylistElement>& elements,
    const std::string& trackName)
{
    MatchResult result;

    if (markers.empty() || elements.empty()) {
        result.skippedBeforeFirstMarker = static_cast<int>(elements.size());
        return result;
    }

    // Sort markers by startSamples (make a copy to avoid mutating input)
    std::vector<Marker> sorted = markers;
    std::sort(sorted.begin(), sorted.end(),
        [](const Marker& a, const Marker& b) {
            return a.startSamples < b.startSamples;
        });

    // Remove markers with unresolved positions
    sorted.erase(
        std::remove_if(sorted.begin(), sorted.end(),
            [](const Marker& m) { return m.startSamples < 0; }),
        sorted.end());

    if (sorted.empty()) {
        result.skippedBeforeFirstMarker = static_cast<int>(elements.size());
        return result;
    }

    // Sort elements by (startSamples, clipName) for deterministic suffix assignment.
    // Using stable_sort + secondary key prevents non-deterministic ordering when
    // multiple clips share the same timeline position.
    std::vector<PlaylistElement> sortedElements = elements;
    std::stable_sort(sortedElements.begin(), sortedElements.end(),
        [](const PlaylistElement& a, const PlaylistElement& b) {
            if (a.startSamples != b.startSamples)
                return a.startSamples < b.startSamples;
            return a.clipName < b.clipName;
        });

    // Group clips by their matching marker for duplicate detection.
    // Key: marker index in sorted array -> list of element indices in sortedElements
    std::map<size_t, std::vector<size_t>> markerToElements;

    for (size_t i = 0; i < sortedElements.size(); ++i) {
        const auto& elem = sortedElements[i];

        // Binary search: find last marker with startSamples <= elem.startSamples
        // upper_bound returns first marker with startSamples > elem.startSamples
        auto it = std::upper_bound(sorted.begin(), sorted.end(), elem.startSamples,
            [](int64_t val, const Marker& m) {
                return val < m.startSamples;
            });

        if (it == sorted.begin()) {
            // Clip is before the first marker
            result.skippedBeforeFirstMarker++;
            continue;
        }

        --it; // Now points to the marker with startSamples <= elem.startSamples
        size_t markerIdx = static_cast<size_t>(std::distance(sorted.begin(), it));
        markerToElements[markerIdx].push_back(i);
    }

    // Generate rename actions with duplicate suffixes
    for (const auto& [markerIdx, elemIndices] : markerToElements) {
        const std::string& markerName = sorted[markerIdx].name;
        std::string fullPrefix = trackName.empty() ? markerName : trackName + " - " + markerName;
        bool hasDuplicates = elemIndices.size() > 1;

        for (size_t dupIdx = 0; dupIdx < elemIndices.size(); ++dupIdx) {
            const auto& elem = sortedElements[elemIndices[dupIdx]];

            std::string newName = fullPrefix;
            if (hasDuplicates) {
                std::ostringstream oss;
                oss << fullPrefix << "_" << std::setfill('0') << std::setw(2) << (dupIdx + 1);
                newName = oss.str();
            }

            // Skip if name is already correct (exact match or same marker with any suffix).
            // The suffix-tolerant check prevents non-idempotent swaps when clip ordering
            // varies between PTSL calls (e.g. Location 11_02 ↔ Location 11_03).
            if (elem.clipName == newName || hasCorrectMarkerPrefix(elem.clipName, fullPrefix)) {
                continue;
            }

            RenameAction action;
            action.oldName = elem.clipName;
            action.newName = newName;
            action.startSamples = elem.startSamples;
            action.markerName = markerName;
            result.actions.push_back(action);
        }
    }

    return result;
}

} // namespace marec

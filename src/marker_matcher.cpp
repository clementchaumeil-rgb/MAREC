#include "marker_matcher.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace marec {

MatchResult MarkerMatcher::match(
    const std::vector<Marker>& markers,
    const std::vector<PlaylistElement>& elements)
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

    // Sort elements by startSamples (copy)
    std::vector<PlaylistElement> sortedElements = elements;
    std::sort(sortedElements.begin(), sortedElements.end(),
        [](const PlaylistElement& a, const PlaylistElement& b) {
            return a.startSamples < b.startSamples;
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
        bool hasDuplicates = elemIndices.size() > 1;

        for (size_t dupIdx = 0; dupIdx < elemIndices.size(); ++dupIdx) {
            const auto& elem = sortedElements[elemIndices[dupIdx]];

            std::string newName = markerName;
            if (hasDuplicates) {
                std::ostringstream oss;
                oss << markerName << "_" << std::setfill('0') << std::setw(2) << (dupIdx + 1);
                newName = oss.str();
            }

            // Skip if name is already correct
            if (elem.clipName == newName) {
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

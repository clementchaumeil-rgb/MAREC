#pragma once

#include "session_data.h"
#include <map>
#include <vector>

namespace marec {

struct MatchResult {
    std::vector<RenameAction> actions;
    int skippedBeforeFirstMarker = 0;
};

class MarkerMatcher {
public:
    // Match clips to markers using binary search.
    // Markers must have startSamples resolved (>= 0).
    // Clips before the first marker are skipped.
    // Duplicate names under the same marker get _01, _02, ... suffixes.
    static MatchResult match(
        const std::vector<Marker>& markers,
        const std::vector<PlaylistElement>& elements);
};

} // namespace marec

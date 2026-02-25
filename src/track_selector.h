#pragma once

#include "session_data.h"
#include <vector>

namespace marec {

class TrackSelector {
public:
    // Display interactive menu and return selected tracks.
    // If allTracks is true, returns all audio tracks without prompting.
    static std::vector<TrackInfo> selectInteractively(
        const std::vector<TrackInfo>& audioTracks, bool allTracks);
};

} // namespace marec

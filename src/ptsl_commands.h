#pragma once

#include "session_data.h"

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace PTSLC_CPP {
class CppPTSLClient;
}

namespace marec {

class PtslCommands {
public:
    explicit PtslCommands(PTSLC_CPP::CppPTSLClient& client);

    // Session info
    SessionInfo getSessionInfo();

    // Track operations
    std::vector<TrackInfo> getTrackList();
    std::vector<TrackInfo> getAudioTracks();

    // Memory locations
    std::vector<Marker> getMemoryLocations();
    std::vector<Marker> getMarkers(); // Filters to TP_Marker only

    // Clip operations (session-level clip list)
    std::vector<ClipInfo> getClipList();

    // Playlist operations (per-track, timeline positions)
    std::vector<std::string> getTrackPlaylists(const std::string& trackName);
    std::vector<PlaylistElement> getPlaylistElements(
        const std::string& playlistName,
        const std::vector<ClipInfo>& clipLookup);

    // Renaming
    bool renameTargetClip(const std::string& oldName, const std::string& newName, bool renameFile);

    // Selection & export
    bool selectAllClipsOnTrack(const std::string& trackName);
    bool exportClipsAsFiles(const ExportConfig& config);

    // Time conversion via PTSL
    std::optional<int64_t> convertTimeToSamples(const std::string& location, const std::string& timeType);

private:
    PTSLC_CPP::CppPTSLClient& m_client;

    // Internal helper: send request and return parsed response body JSON.
    // Throws on failure.
    nlohmann::json sendAndParse(int32_t commandId, const nlohmann::json& requestBody = {});

    // Paginated fetch helper
    nlohmann::json fetchAllPaginated(int32_t commandId, const nlohmann::json& baseRequest,
        const std::string& listKey, int pageSize = 500);
};

} // namespace marec

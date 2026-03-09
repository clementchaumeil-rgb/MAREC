#pragma once

#include "session_data.h"

#include <optional>
#include <set>
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
    std::string getSessionPath();

    // Track operations
    std::vector<TrackInfo> getTrackList();
    std::vector<TrackInfo> getAudioTracks();

    // Memory locations
    std::vector<Marker> getMemoryLocations();
    std::vector<Marker> getMarkers(); // Filters to TP_Marker only
    void clearAllMemoryLocations();

    // Clip operations (session-level clip list)
    std::vector<ClipInfo> getClipList();

    // Playlist operations (per-track, timeline positions)
    std::vector<std::string> getTrackPlaylists(const std::string& trackName);
    std::vector<PlaylistElement> getPlaylistElements(
        const std::string& playlistName,
        const std::vector<ClipInfo>& clipLookup);

    // Renaming
    RenameOutcome renameTargetClip(const std::string& oldName, const std::string& newName, bool renameFile);

    // Selection & export
    bool selectAllClipsOnTrack(const std::string& trackName);
    bool exportClipsAsFiles(const ExportConfig& config);

    // Per-track clip identification (workaround when Private API is unavailable):
    // Selects all clips on the given track, then queries GetFileLocation with
    // SelectedClipsTimeline filter to return the set of file_ids for that track.
    std::set<std::string> getFileIdsForTrack(const std::string& trackName);

    // Time conversion via PTSL
    std::optional<int64_t> convertTimeToSamples(const std::string& location, const std::string& timeType);

    // --- Session management (for test harness) ---
    void createSession(const std::string& name, const std::string& location,
        int32_t sampleRate = 48000, int32_t bitDepth = 24);
    void closeSession(bool saveOnClose = false);
    void saveSession();

    // --- Track / Marker / Clip creation ---
    std::vector<std::string> createNewTracks(const std::string& baseName, int count = 1);
    void createMemoryLocation(int32_t number, const std::string& name, int64_t startSamples);
    /// Bulk-create markers from raw sample positions.
    /// Temporarily sets counter to Samples to avoid format conversion issues,
    /// then restores the original counter format.
    struct MarkerDef {
        int32_t number;
        std::string name;
        int64_t startSamples;
    };
    struct BulkCreateResult {
        int created = 0;
        int errors = 0;
        std::vector<std::pair<int32_t, std::string>> failures; // number → error
    };
    BulkCreateResult createMarkersFromSamples(const std::vector<MarkerDef>& markers);
    // Convert a sample position to the session's current counter format string
    std::string convertSamplesToSessionFormat(int64_t samples);
    void importAudioToClipList(const std::vector<std::string>& filePaths);
    void spotClipByID(const std::string& clipId, const std::string& trackName, int64_t locationSamples);

    // --- Counter format ---
    void setMainCounterFormat(const std::string& format);

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

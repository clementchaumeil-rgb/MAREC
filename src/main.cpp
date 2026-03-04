#include "cli.h"
#include "marker_matcher.h"
#include "ptsl_commands.h"
#include "ptsl_connection.h"
#include "session_data.h"
#include "time_utils.h"
#include "track_selector.h"

#include <filesystem>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>

using namespace marec;
using json = nlohmann::json;

// ---- Helpers shared between interactive and JSON modes ----

static void resolveMarkerTimes(
    std::vector<Marker>& markers, PtslCommands& commands, const SessionInfo& session)
{
    for (auto& marker : markers) {
        if (marker.startTime.empty()) continue;

        // Try local parsing first (handles pure sample numbers, timecode, min:secs)
        // to avoid expensive PTSL gRPC round-trips for each marker.
        auto localSamples = TimeUtils::parseAuto(marker.startTime, session.sampleRate);
        if (localSamples) {
            marker.startSamples = *localSamples;
            continue;
        }

        // Fall back to PTSL conversion for formats we can't parse locally (e.g. bars|beats)
        auto samples = commands.convertTimeToSamples(marker.startTime, "TLType_Samples");
        if (samples) {
            marker.startSamples = *samples;
        } else {
            std::cerr << "  Warning: Cannot resolve time for marker '" << marker.name
                      << "' (time: " << marker.startTime << "), skipping.\n";
        }
    }
}

// Deduplicate stereo L/R channels: same clip name + same start position.
// On stereo tracks, L and R channels share the same root name and timeline position.
static std::vector<PlaylistElement> deduplicateElements(std::vector<PlaylistElement> elements)
{
    std::vector<PlaylistElement> deduped;
    deduped.reserve(elements.size());
    std::set<std::pair<std::string, int64_t>> seen;
    for (auto& elem : elements) {
        auto key = std::make_pair(elem.clipName, elem.startSamples);
        if (seen.insert(key).second) {
            deduped.push_back(std::move(elem));
        }
    }
    return deduped;
}

// Try per-track Private API (GetPlaylistElements). Returns empty if unavailable.
static std::vector<PlaylistElement> getTrackElementsPrivateApi(
    PtslCommands& commands, const TrackInfo& track, const std::vector<ClipInfo>& clipList)
{
    auto playlists = commands.getTrackPlaylists(track.name);
    if (!playlists.empty()) {
        auto elements = commands.getPlaylistElements(playlists[0], clipList);
        return deduplicateElements(std::move(elements));
    }
    return {};
}

// Convert clip list to playlist elements.
// If fileIds is non-empty, only includes clips whose fileId is in the set.
static std::vector<PlaylistElement> clipListToElements(
    const std::vector<ClipInfo>& clipList,
    const std::set<std::string>& fileIds = {})
{
    std::vector<PlaylistElement> elements;
    elements.reserve(clipList.size());
    for (const auto& clip : clipList) {
        if (!fileIds.empty() && fileIds.find(clip.fileId) == fileIds.end()) {
            continue;
        }
        PlaylistElement elem;
        elem.clipId = clip.clipId;
        elem.clipName = clip.clipRootName;
        elem.startSamples = clip.startSamples;
        elem.endSamples = clip.endSamples;
        elements.push_back(elem);
    }
    return deduplicateElements(std::move(elements));
}

// Get per-track elements using file ID cross-referencing (public API workaround).
// SelectAllClipsOnTrack + GetFileLocation(SelectedClipsTimeline) → file_ids → filter clip list.
static std::vector<PlaylistElement> getTrackElementsByFileId(
    PtslCommands& commands, const TrackInfo& track, const std::vector<ClipInfo>& clipList)
{
    auto fileIds = commands.getFileIdsForTrack(track.name);
    if (fileIds.empty()) return {};
    return clipListToElements(clipList, fileIds);
}

// Check if Private API (GetPlaylistElements, CId=158) is available by probing the first track.
static bool isPrivateApiAvailable(
    PtslCommands& commands, const std::vector<TrackInfo>& tracks,
    const std::vector<ClipInfo>& clipList)
{
    if (tracks.empty()) return false;
    try {
        auto playlists = commands.getTrackPlaylists(tracks[0].name);
        if (playlists.empty()) return false;
        // Actually call GetPlaylistElements to test Private API
        commands.getPlaylistElements(playlists[0], clipList);
        return true;
    } catch (...) {
        return false;
    }
}

// Legacy wrapper for interactive mode
static std::vector<PlaylistElement> getTrackElements(
    PtslCommands& commands, const TrackInfo& track, const std::vector<ClipInfo>& clipList)
{
    try {
        return getTrackElementsPrivateApi(commands, track, clipList);
    } catch (const std::exception& e) {
        std::cerr << "  Note: GetPlaylistElements not available for '" << track.name
                  << "', using clip list fallback. (" << e.what() << ")\n";
    }
    return clipListToElements(clipList);
}

// Filter tracks by name list
static std::vector<TrackInfo> filterTracksByNames(
    const std::vector<TrackInfo>& allTracks, const std::vector<std::string>& names)
{
    if (names.empty()) return allTracks;
    std::vector<TrackInfo> result;
    for (const auto& track : allTracks) {
        for (const auto& name : names) {
            if (track.name == name) {
                result.push_back(track);
                break;
            }
        }
    }
    return result;
}

// ---- JSON step functions ----

static json doConnect(const CliOptions& /*opts*/)
{
    PtslConnection connection;
    connection.connect("AudiArt", "MAREC");
    PtslCommands commands(connection.client());

    auto session = commands.getSessionInfo();

    return {
        {"status", "ok"},
        {"session", {
            {"sampleRate", session.sampleRate},
            {"counterFormat", session.counterFormat}
        }}
    };
}

static json doTracks(const CliOptions& /*opts*/)
{
    PtslConnection connection;
    connection.connect("AudiArt", "MAREC");
    PtslCommands commands(connection.client());

    auto audioTracks = commands.getAudioTracks();

    json tracksJson = json::array();
    for (const auto& t : audioTracks) {
        tracksJson.push_back({
            {"name", t.name},
            {"id", t.id},
            {"index", t.index}
        });
    }

    return {{"status", "ok"}, {"tracks", tracksJson}};
}

static json doMarkers(const CliOptions& /*opts*/)
{
    PtslConnection connection;
    connection.connect("AudiArt", "MAREC");
    PtslCommands commands(connection.client());

    auto session = commands.getSessionInfo();
    auto markers = commands.getMarkers();
    resolveMarkerTimes(markers, commands, session);

    json markersJson = json::array();
    for (const auto& m : markers) {
        markersJson.push_back({
            {"number", m.number},
            {"name", m.name},
            {"startTime", m.startTime},
            {"startSamples", m.startSamples}
        });
    }

    return {{"status", "ok"}, {"markers", markersJson}};
}

static json doPreview(const CliOptions& opts)
{
    PtslConnection connection;
    connection.connect("AudiArt", "MAREC");
    PtslCommands commands(connection.client());

    auto session = commands.getSessionInfo();
    auto audioTracks = commands.getAudioTracks();
    auto selectedTracks = filterTracksByNames(audioTracks, opts.trackNames);

    if (selectedTracks.empty() && opts.trackNames.empty()) {
        selectedTracks = audioTracks; // Default: all tracks
    }

    auto markers = commands.getMarkers();
    resolveMarkerTimes(markers, commands, session);

    std::vector<ClipInfo> clipList;
    try { clipList = commands.getClipList(); } catch (...) {}

    json actionsJson = json::array();
    int totalClips = 0;
    int skippedBefore = 0;
    int alreadyCorrect = 0;

    bool privateApi = isPrivateApiAvailable(commands, selectedTracks, clipList);

    // Track claimed clips to avoid duplicate renames when the same file
    // appears on multiple tracks (e.g. Chutier/arrangement tracks).
    std::set<std::string> claimedClips;

    auto addActions = [&](const MatchResult& result, const std::string& trackName) {
        for (const auto& action : result.actions) {
            if (!claimedClips.insert(action.oldName).second) continue; // already claimed
            actionsJson.push_back({
                {"trackName", trackName},
                {"oldName", action.oldName},
                {"newName", action.newName},
                {"startSamples", action.startSamples},
                {"timeFormatted", TimeUtils::formatSamplesToTime(action.startSamples, session.sampleRate)},
                {"markerName", action.markerName}
            });
        }
    };

    // Both paths process per-track with track name prefix.
    for (size_t trackIdx = 0; trackIdx < selectedTracks.size(); ++trackIdx) {
        const auto& track = selectedTracks[trackIdx];
        std::cerr << "PROGRESS: " << (trackIdx + 1) << "/" << selectedTracks.size()
                  << " " << track.name << std::endl;

        auto elements = privateApi
            ? getTrackElementsPrivateApi(commands, track, clipList)
            : getTrackElementsByFileId(commands, track, clipList);
        totalClips += static_cast<int>(elements.size());

        auto result = MarkerMatcher::match(markers, elements, track.name);
        skippedBefore += result.skippedBeforeFirstMarker;
        addActions(result, track.name);
    }

    alreadyCorrect = totalClips - static_cast<int>(actionsJson.size()) - skippedBefore;

    return {
        {"status", "ok"},
        {"actions", actionsJson},
        {"summary", {
            {"totalClips", totalClips},
            {"toRename", actionsJson.size()},
            {"skippedBeforeFirstMarker", skippedBefore},
            {"alreadyCorrect", alreadyCorrect}
        }}
    };
}

static json doRename(const CliOptions& opts)
{
    PtslConnection connection;
    connection.connect("AudiArt", "MAREC");
    PtslCommands commands(connection.client());

    auto session = commands.getSessionInfo();
    auto audioTracks = commands.getAudioTracks();
    auto selectedTracks = filterTracksByNames(audioTracks, opts.trackNames);

    if (selectedTracks.empty() && opts.trackNames.empty()) {
        selectedTracks = audioTracks;
    }

    auto markers = commands.getMarkers();
    resolveMarkerTimes(markers, commands, session);

    std::vector<ClipInfo> clipList;
    try { clipList = commands.getClipList(); } catch (...) {}

    // Gather all rename actions
    bool privateApi = isPrivateApiAvailable(commands, selectedTracks, clipList);
    std::vector<RenameAction> allActions;
    std::set<std::string> claimedClips;

    for (size_t trackIdx = 0; trackIdx < selectedTracks.size(); ++trackIdx) {
        const auto& track = selectedTracks[trackIdx];
        std::cerr << "PROGRESS: " << (trackIdx + 1) << "/" << selectedTracks.size()
                  << " " << track.name << std::endl;

        auto elements = privateApi
            ? getTrackElementsPrivateApi(commands, track, clipList)
            : getTrackElementsByFileId(commands, track, clipList);
        auto result = MarkerMatcher::match(markers, elements, track.name);
        for (auto& action : result.actions) {
            if (claimedClips.insert(action.oldName).second) {
                allActions.push_back(std::move(action));
            }
        }
    }

    // Execute
    json resultsJson = json::array();
    int successCount = 0;
    int errorCount = 0;

    for (size_t i = 0; i < allActions.size(); ++i) {
        const auto& action = allActions[i];
        if (i % 10 == 0) {
            std::cerr << "PROGRESS: renaming " << (i + 1) << "/" << allActions.size() << std::endl;
        }
        auto outcome = commands.renameTargetClip(action.oldName, action.newName, opts.renameFile);
        json r = {
            {"oldName", action.oldName},
            {"newName", action.newName},
            {"success", outcome.success}
        };
        if (!outcome.success) {
            r["error"] = outcome.error.empty() ? "Rename failed" : outcome.error;
            errorCount++;
        } else {
            successCount++;
        }
        resultsJson.push_back(r);
    }

    return {
        {"status", "ok"},
        {"results", resultsJson},
        {"summary", {
            {"successCount", successCount},
            {"errorCount", errorCount},
            {"totalCount", static_cast<int>(allActions.size())}
        }}
    };
}

static json doExport(const CliOptions& opts)
{
    namespace fs = std::filesystem;

    PtslConnection connection;
    connection.connect("AudiArt", "MAREC");
    PtslCommands commands(connection.client());

    // Resolve output directory: use session's parent dir if not specified
    ExportConfig exportConfig = opts.exportConfig;
    if (exportConfig.outputDir.empty()) {
        std::string sessionPath = commands.getSessionPath();
        if (!sessionPath.empty()) {
            fs::path sessionDir = fs::path(sessionPath).parent_path();
            exportConfig.outputDir = (sessionDir / "Bounced Files").string() + "/";
            fs::create_directories(exportConfig.outputDir);
        }
    }

    auto audioTracks = commands.getAudioTracks();
    auto selectedTracks = filterTracksByNames(audioTracks, opts.trackNames);

    if (selectedTracks.empty() && opts.trackNames.empty()) {
        selectedTracks = audioTracks;
    }

    json resultsJson = json::array();
    int successCount = 0;

    for (const auto& track : selectedTracks) {
        bool selected = commands.selectAllClipsOnTrack(track.name);
        bool exported = selected && commands.exportClipsAsFiles(exportConfig);

        resultsJson.push_back({
            {"trackName", track.name},
            {"success", exported}
        });
        if (exported) successCount++;
    }

    return {
        {"status", "ok"},
        {"results", resultsJson},
        {"summary", {
            {"exported", successCount},
            {"total", static_cast<int>(selectedTracks.size())}
        }}
    };
}

// ---- JSON mode dispatcher ----

static int runJsonMode(const CliOptions& opts)
{
    try {
        json result;
        switch (opts.step) {
            case Step::Connect: result = doConnect(opts); break;
            case Step::Tracks:  result = doTracks(opts); break;
            case Step::Markers: result = doMarkers(opts); break;
            case Step::Preview: result = doPreview(opts); break;
            case Step::Rename:  result = doRename(opts); break;
            case Step::Export:  result = doExport(opts); break;
            default:
                result = {{"status", "error"}, {"error", "Missing --step argument"}, {"code", "MISSING_STEP"}};
                break;
        }
        std::cout << result.dump() << std::endl;
        return result.value("status", "") == "ok" ? 0 : 1;
    } catch (const std::exception& e) {
        json err = {
            {"status", "error"},
            {"error", e.what()},
            {"code", "PTSL_ERROR"}
        };
        std::cout << err.dump() << std::endl;
        return 1;
    }
}

// ---- Interactive mode (original) ----

static void printPreview(const std::vector<RenameAction>& actions, const SessionInfo& session)
{
    std::cout << "\n--- Rename Preview ---\n";
    for (const auto& a : actions) {
        std::cout << "  " << a.oldName << "  ->  " << a.newName
                  << "  (at " << TimeUtils::formatSamplesToTime(a.startSamples, session.sampleRate) << ")\n";
    }
    std::cout << "---\n";
}

static int runInteractive(const CliOptions& opts)
{
    // 1. Connect to Pro Tools
    PtslConnection connection;
    try {
        connection.connect("AudiArt", "MAREC");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "Make sure Pro Tools is running.\n";
        return 1;
    }

    PtslCommands commands(connection.client());

    // 2. Get session info
    auto session = commands.getSessionInfo();
    std::cout << "Session: " << session.sampleRate << " Hz\n";

    // 3. Get audio tracks
    auto audioTracks = commands.getAudioTracks();
    if (audioTracks.empty()) {
        std::cerr << "Error: No audio tracks found in session.\n";
        return 1;
    }
    std::cout << "Found " << audioTracks.size() << " audio track(s).\n";

    // 4. Track selection
    auto selectedTracks = TrackSelector::selectInteractively(audioTracks, opts.allTracks);
    if (selectedTracks.empty()) {
        std::cout << "No tracks selected. Exiting.\n";
        return 0;
    }

    // 5. Get markers
    auto markers = commands.getMarkers();
    if (markers.empty()) {
        std::cerr << "Error: No markers found in session.\n";
        return 1;
    }
    std::cout << "Found " << markers.size() << " marker(s).\n";

    // 6. Resolve marker times
    resolveMarkerTimes(markers, commands, session);

    int resolvedCount = 0;
    for (const auto& m : markers) {
        if (m.startSamples >= 0) resolvedCount++;
    }
    if (resolvedCount == 0) {
        std::cerr << "Error: Could not resolve any marker positions.\n";
        return 1;
    }
    std::cout << "Resolved " << resolvedCount << "/" << markers.size() << " marker positions.\n";

    // 7. Get clip list
    std::vector<ClipInfo> clipList;
    try { clipList = commands.getClipList(); }
    catch (const std::exception& e) {
        std::cerr << "  Note: GetClipList not available. (" << e.what() << ")\n";
    }

    // 8. Process each track
    std::vector<RenameAction> allActions;
    std::set<std::string> claimedClips;
    int totalSkipped = 0;
    bool privateApi = isPrivateApiAvailable(commands, selectedTracks, clipList);

    for (const auto& track : selectedTracks) {
        std::cout << "\nProcessing track: " << track.name << "\n";

        auto elements = privateApi
            ? getTrackElementsPrivateApi(commands, track, clipList)
            : getTrackElementsByFileId(commands, track, clipList);
        if (elements.empty()) {
            std::cerr << "  No clips found on track '" << track.name << "'.\n";
            continue;
        }
        std::cout << "  Found " << elements.size() << " clip(s).\n";

        auto result = MarkerMatcher::match(markers, elements, track.name);
        totalSkipped += result.skippedBeforeFirstMarker;

        for (auto& action : result.actions) {
            if (claimedClips.insert(action.oldName).second) {
                allActions.push_back(std::move(action));
            }
        }
    }

    if (allActions.empty()) {
        std::cout << "\nNo clips need renaming.\n";
        return 0;
    }

    // 9. Show preview
    printPreview(allActions, session);
    std::cout << allActions.size() << " clip(s) to rename";
    if (totalSkipped > 0) {
        std::cout << ", " << totalSkipped << " clip(s) before first marker (skipped)";
    }
    std::cout << ".\n";

    // 10. Dry run check
    if (opts.dryRun) {
        std::cout << "\n[DRY RUN] No changes made.\n";
        return 0;
    }

    // 11. Confirm
    std::cout << "\nProceed with renaming? (y/N): ";
    std::string confirm;
    std::getline(std::cin, confirm);
    if (confirm != "y" && confirm != "Y") {
        std::cout << "Cancelled.\n";
        return 0;
    }

    // 12. Execute
    int successCount = 0;
    int errorCount = 0;
    for (const auto& action : allActions) {
        auto outcome = commands.renameTargetClip(action.oldName, action.newName, opts.renameFile);
        if (outcome.success) {
            successCount++;
        } else {
            errorCount++;
        }
    }

    std::cout << "\n" << successCount << "/" << allActions.size() << " clip(s) renamed";
    if (errorCount > 0) {
        std::cout << ", " << errorCount << " error(s)";
    }
    std::cout << ".\n";

    // 13. Export
    if (opts.exportConfig.enabled) {
        std::cout << "\nExporting clips...\n";
        for (const auto& track : selectedTracks) {
            if (commands.selectAllClipsOnTrack(track.name)) {
                if (commands.exportClipsAsFiles(opts.exportConfig)) {
                    std::cout << "  Exported clips from track '" << track.name << "'.\n";
                }
            }
        }
        std::cout << "Export complete: " << opts.exportConfig.fileType
                  << " " << opts.exportConfig.bitDepth << "-bit\n";
    }

    return 0;
}

// ---- Entry point ----

int main(int argc, char* argv[])
{
    auto opts = Cli::parse(argc, argv);
    if (opts.help) {
        Cli::printHelp();
        return 0;
    }

    if (opts.jsonMode) {
        return runJsonMode(opts);
    }

    return runInteractive(opts);
}

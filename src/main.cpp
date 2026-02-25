#include "cli.h"
#include "marker_matcher.h"
#include "ptsl_commands.h"
#include "ptsl_connection.h"
#include "session_data.h"
#include "time_utils.h"
#include "track_selector.h"

#include <iostream>

using namespace marec;

// Resolve marker start times to samples using PTSL time conversion + local fallback.
static void resolveMarkerTimes(
    std::vector<Marker>& markers, PtslCommands& commands, const SessionInfo& session)
{
    for (auto& marker : markers) {
        if (marker.startTime.empty()) {
            continue;
        }

        // Try PTSL GetTimeAsType first (handles all formats including bars|beats)
        auto samples = commands.convertTimeToSamples(marker.startTime, "TLType_Samples");
        if (samples) {
            marker.startSamples = *samples;
            continue;
        }

        // Fallback: local parsing
        auto localSamples = TimeUtils::parseAuto(marker.startTime, session.sampleRate);
        if (localSamples) {
            marker.startSamples = *localSamples;
        } else {
            std::cerr << "  Warning: Cannot resolve time for marker '" << marker.name
                      << "' (time: " << marker.startTime << "), skipping.\n";
        }
    }
}

// Get playlist elements for a track, with fallback to clip list.
static std::vector<PlaylistElement> getTrackElements(
    PtslCommands& commands, const TrackInfo& track, const std::vector<ClipInfo>& clipList)
{
    // Primary: try GetTrackPlaylists + GetPlaylistElements
    try {
        auto playlists = commands.getTrackPlaylists(track.name);
        if (!playlists.empty()) {
            return commands.getPlaylistElements(playlists[0], clipList);
        }
    } catch (const std::exception& e) {
        std::cerr << "  Note: GetPlaylistElements not available for '" << track.name
                  << "', using clip list fallback. (" << e.what() << ")\n";
    }

    // Fallback: use session clip list and match by clip positions
    // Note: GetClipList returns media positions, not timeline positions.
    // This is a limited fallback — positions may not map directly to timeline.
    std::vector<PlaylistElement> elements;
    for (const auto& clip : clipList) {
        PlaylistElement elem;
        elem.clipId = clip.clipId;
        elem.clipName = clip.clipFullName;
        elem.startSamples = clip.startSamples;
        elem.endSamples = clip.endSamples;
        elements.push_back(elem);
    }
    return elements;
}

static void printPreview(const std::vector<RenameAction>& actions, const SessionInfo& session)
{
    std::cout << "\n--- Rename Preview ---\n";
    for (const auto& a : actions) {
        std::cout << "  " << a.oldName << "  ->  " << a.newName
                  << "  (at " << TimeUtils::formatSamplesToTime(a.startSamples, session.sampleRate) << ")\n";
    }
    std::cout << "---\n";
}

int main(int argc, char* argv[])
{
    // 1. Parse CLI args
    auto opts = Cli::parse(argc, argv);
    if (opts.help) {
        Cli::printHelp();
        return 0;
    }

    // 2. Connect to Pro Tools
    PtslConnection connection;
    try {
        connection.connect("AudiArt", "MAREC");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "Make sure Pro Tools is running.\n";
        return 1;
    }

    PtslCommands commands(connection.client());

    // 3. Get session info
    auto session = commands.getSessionInfo();
    std::cout << "Session: " << session.sampleRate << " Hz\n";

    // 4. Get audio tracks
    auto audioTracks = commands.getAudioTracks();
    if (audioTracks.empty()) {
        std::cerr << "Error: No audio tracks found in session.\n";
        return 1;
    }
    std::cout << "Found " << audioTracks.size() << " audio track(s).\n";

    // 5. Track selection
    auto selectedTracks = TrackSelector::selectInteractively(audioTracks, opts.allTracks);
    if (selectedTracks.empty()) {
        std::cout << "No tracks selected. Exiting.\n";
        return 0;
    }

    // 6. Get markers (filtered to TP_Marker)
    auto markers = commands.getMarkers();
    if (markers.empty()) {
        std::cerr << "Error: No markers found in session.\n";
        return 1;
    }
    std::cout << "Found " << markers.size() << " marker(s).\n";

    // 7. Resolve marker times to samples
    resolveMarkerTimes(markers, commands, session);

    // Count resolved
    int resolvedCount = 0;
    for (const auto& m : markers) {
        if (m.startSamples >= 0) resolvedCount++;
    }
    if (resolvedCount == 0) {
        std::cerr << "Error: Could not resolve any marker positions.\n";
        return 1;
    }
    std::cout << "Resolved " << resolvedCount << "/" << markers.size() << " marker positions.\n";

    // 8. Get session clip list (for clip ID resolution)
    std::vector<ClipInfo> clipList;
    try {
        clipList = commands.getClipList();
    } catch (const std::exception& e) {
        std::cerr << "  Note: GetClipList not available. (" << e.what() << ")\n";
    }

    // 9. Process each track
    std::vector<RenameAction> allActions;
    int totalSkipped = 0;

    for (const auto& track : selectedTracks) {
        std::cout << "\nProcessing track: " << track.name << "\n";

        auto elements = getTrackElements(commands, track, clipList);
        if (elements.empty()) {
            std::cerr << "  No clips found on track '" << track.name << "'.\n";
            continue;
        }
        std::cout << "  Found " << elements.size() << " clip(s).\n";

        auto result = MarkerMatcher::match(markers, elements);
        totalSkipped += result.skippedBeforeFirstMarker;

        for (auto& action : result.actions) {
            allActions.push_back(std::move(action));
        }
    }

    if (allActions.empty()) {
        std::cout << "\nNo clips need renaming.\n";
        return 0;
    }

    // 10. Show preview
    printPreview(allActions, session);
    std::cout << allActions.size() << " clip(s) to rename";
    if (totalSkipped > 0) {
        std::cout << ", " << totalSkipped << " clip(s) before first marker (skipped)";
    }
    std::cout << ".\n";

    // 11. Dry run check
    if (opts.dryRun) {
        std::cout << "\n[DRY RUN] No changes made.\n";
        return 0;
    }

    // 12. Confirm with user
    std::cout << "\nProceed with renaming? (y/N): ";
    std::string confirm;
    std::getline(std::cin, confirm);
    if (confirm != "y" && confirm != "Y") {
        std::cout << "Cancelled.\n";
        return 0;
    }

    // 13. Execute renaming
    int successCount = 0;
    int errorCount = 0;

    for (const auto& action : allActions) {
        if (commands.renameTargetClip(action.oldName, action.newName, opts.renameFile)) {
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

    // 14. Export if requested
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

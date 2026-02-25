#include "ptsl_commands.h"
#include "time_utils.h"

#include <iostream>
#include <stdexcept>
#include <unordered_map>

#include "PTSLC_CPP/CppPTSLClient.h"

using namespace PTSLC_CPP;
using json = nlohmann::json;

namespace marec {

PtslCommands::PtslCommands(CppPTSLClient& client)
    : m_client(client)
{}

// ---- Internal helpers ----

json PtslCommands::sendAndParse(int32_t commandId, const json& requestBody)
{
    CppPTSLRequest request(static_cast<CommandId>(commandId), requestBody.dump());

    auto response = m_client.SendRequest(request).get();

    if (response.GetStatus() != CommandStatusType::TStatus_Completed) {
        auto errors = response.GetResponseErrorList();
        std::string errMsg = "PTSL command " + std::to_string(commandId) + " failed";
        if (!errors.errors.empty()) {
            errMsg += ": " + errors.errors[0]->errorMessage;
        }
        throw std::runtime_error(errMsg);
    }

    std::string bodyStr = response.GetResponseBodyJson();
    if (bodyStr.empty()) {
        return json::object();
    }
    return json::parse(bodyStr);
}

json PtslCommands::fetchAllPaginated(int32_t commandId, const json& baseRequest,
    const std::string& listKey, int pageSize)
{
    json allItems = json::array();
    int offset = 0;

    while (true) {
        json req = baseRequest;
        req["pagination_request"] = { {"limit", pageSize}, {"offset", offset} };

        json resp = sendAndParse(commandId, req);

        if (resp.contains(listKey) && resp[listKey].is_array()) {
            for (auto& item : resp[listKey]) {
                allItems.push_back(std::move(item));
            }
        }

        // Check if we got all items
        int total = 0;
        if (resp.contains("pagination_response")) {
            total = resp["pagination_response"].value("total", 0);
        }

        offset += pageSize;
        if (offset >= total || !resp.contains(listKey) || resp[listKey].empty()) {
            break;
        }
    }

    return allItems;
}

// ---- Session info ----

SessionInfo PtslCommands::getSessionInfo()
{
    SessionInfo info;

    // Get sample rate
    try {
        auto resp = sendAndParse(static_cast<int32_t>(CommandId::CId_GetSessionSampleRate));
        // sample_rate is an enum string like "SRate_48000"
        std::string srStr = resp.value("sample_rate", "SRate_48000");
        // Extract the number from the enum name
        auto pos = srStr.rfind('_');
        if (pos != std::string::npos) {
            info.sampleRate = std::stoi(srStr.substr(pos + 1));
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not get sample rate, defaulting to 48000. (" << e.what() << ")\n";
    }

    // Get main counter format
    try {
        auto resp = sendAndParse(static_cast<int32_t>(CommandId::CId_GetMainCounterFormat));
        info.counterFormat = resp.value("current_type", resp.value("current_setting", ""));
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not get counter format. (" << e.what() << ")\n";
    }

    return info;
}

// ---- Track operations ----

std::vector<TrackInfo> PtslCommands::getTrackList()
{
    json allTracks = fetchAllPaginated(
        static_cast<int32_t>(CommandId::CId_GetTrackList), json::object(), "track_list");

    std::vector<TrackInfo> tracks;
    tracks.reserve(allTracks.size());

    for (const auto& t : allTracks) {
        TrackInfo info;
        info.name = t.value("name", "");
        info.id = t.value("id", "");
        info.index = t.value("index", 0);

        // type can be an int or a string enum
        if (t.contains("type")) {
            if (t["type"].is_number()) {
                info.type = t["type"].get<int32_t>();
            } else if (t["type"].is_string()) {
                std::string typeStr = t["type"].get<std::string>();
                if (typeStr == "TType_Audio" || typeStr == "AudioTrack" || typeStr == "TT_Audio") {
                    info.type = 2;
                } else if (typeStr == "TType_Midi" || typeStr == "Midi" || typeStr == "TT_Midi") {
                    info.type = 1;
                } else if (typeStr == "TType_Aux" || typeStr == "Aux" || typeStr == "TT_Aux") {
                    info.type = 3;
                }
                // Other types left as 0
            }
        }
        tracks.push_back(std::move(info));
    }

    return tracks;
}

std::vector<TrackInfo> PtslCommands::getAudioTracks()
{
    auto allTracks = getTrackList();
    std::vector<TrackInfo> audio;
    for (auto& t : allTracks) {
        if (t.type == 2) { // TType_Audio
            audio.push_back(std::move(t));
        }
    }
    return audio;
}

// ---- Memory locations ----

std::vector<Marker> PtslCommands::getMemoryLocations()
{
    json allLocs = fetchAllPaginated(
        static_cast<int32_t>(CommandId::CId_GetMemoryLocations), json::object(), "memory_locations");

    std::vector<Marker> markers;
    markers.reserve(allLocs.size());

    for (const auto& ml : allLocs) {
        Marker m;
        m.number = ml.value("number", 0);
        m.name = ml.value("name", "");
        m.startTime = ml.value("start_time", "");

        // time_properties can be int or string
        if (ml.contains("time_properties")) {
            if (ml["time_properties"].is_number()) {
                m.timeProperties = ml["time_properties"].get<int32_t>();
            } else if (ml["time_properties"].is_string()) {
                std::string tp = ml["time_properties"].get<std::string>();
                if (tp == "TProperties_Marker" || tp == "TP_Marker") {
                    m.timeProperties = 1;
                } else if (tp == "TProperties_Selection" || tp == "TP_Selection") {
                    m.timeProperties = 2;
                }
            }
        }

        markers.push_back(std::move(m));
    }

    return markers;
}

std::vector<Marker> PtslCommands::getMarkers()
{
    auto all = getMemoryLocations();
    std::vector<Marker> markers;
    for (auto& m : all) {
        if (m.timeProperties == 1) { // TP_Marker / TProperties_Marker
            markers.push_back(std::move(m));
        }
    }
    return markers;
}

// ---- Clip operations ----

std::vector<ClipInfo> PtslCommands::getClipList()
{
    json allClips = fetchAllPaginated(
        static_cast<int32_t>(CommandId::CId_GetClipList), json::object(), "clip_list");

    std::vector<ClipInfo> clips;
    clips.reserve(allClips.size());

    for (const auto& c : allClips) {
        ClipInfo info;
        info.clipId = c.value("clip_id", "");
        info.clipFullName = c.value("clip_full_name", "");
        info.clipRootName = c.value("clip_root_name", "");

        // Timeline position is in original_timestamp_point.location (samples as string)
        // start_point/end_point are media-internal positions, not timeline positions.
        int64_t timelineStart = 0;
        if (c.contains("original_timestamp_point") &&
            c["original_timestamp_point"].contains("location")) {
            std::string locStr = c["original_timestamp_point"]["location"].get<std::string>();
            try { timelineStart = std::stoll(locStr); } catch (...) {}
        }

        // Clip length = end_point.position - start_point.position (media positions)
        int64_t mediaStart = 0, mediaEnd = 0;
        if (c.contains("start_point") && c["start_point"].contains("position")) {
            mediaStart = c["start_point"]["position"].get<int64_t>();
        }
        if (c.contains("end_point") && c["end_point"].contains("position")) {
            mediaEnd = c["end_point"]["position"].get<int64_t>();
        }

        info.startSamples = timelineStart;
        info.endSamples = timelineStart + (mediaEnd - mediaStart);

        clips.push_back(std::move(info));
    }

    return clips;
}

// ---- Playlist operations ----

std::vector<std::string> PtslCommands::getTrackPlaylists(const std::string& trackName)
{
    json req;
    req["track_name"] = trackName;

    json allPlaylists = fetchAllPaginated(
        static_cast<int32_t>(CommandId::CId_GetTrackPlaylists), req, "playlists");

    std::vector<std::string> names;
    for (const auto& p : allPlaylists) {
        std::string name = p.value("playlist_name", "");
        bool isTarget = p.value("is_target", false);
        if (isTarget && !name.empty()) {
            // Put the target playlist first
            names.insert(names.begin(), name);
        } else if (!name.empty()) {
            names.push_back(name);
        }
    }
    return names;
}

std::vector<PlaylistElement> PtslCommands::getPlaylistElements(
    const std::string& playlistName,
    const std::vector<ClipInfo>& clipLookup)
{
    // Build clip ID -> name lookup
    std::unordered_map<std::string, std::string> idToName;
    for (const auto& c : clipLookup) {
        idToName[c.clipId] = c.clipFullName;
    }

    json req;
    req["playlist_name"] = playlistName;
    req["time_format"] = "TLType_Samples";

    json allElements = fetchAllPaginated(
        static_cast<int32_t>(CommandId::CId_GetPlaylistElements), req, "elements_list");

    std::vector<PlaylistElement> elements;
    elements.reserve(allElements.size());

    for (const auto& e : allElements) {
        PlaylistElement elem;

        // Parse start_time (timeline location in samples)
        if (e.contains("start_time") && e["start_time"].contains("location")) {
            std::string locStr = e["start_time"]["location"].get<std::string>();
            try {
                elem.startSamples = std::stoll(locStr);
            } catch (...) {
                elem.startSamples = 0;
            }
        }

        // Parse end_time
        if (e.contains("end_time") && e["end_time"].contains("location")) {
            std::string locStr = e["end_time"]["location"].get<std::string>();
            try {
                elem.endSamples = std::stoll(locStr);
            } catch (...) {
                elem.endSamples = 0;
            }
        }

        // Resolve clip ID -> name from channel_clips
        if (e.contains("channel_clips") && e["channel_clips"].is_array() && !e["channel_clips"].empty()) {
            std::string clipId = e["channel_clips"][0].value("clip_id", "");
            elem.clipId = clipId;
            auto it = idToName.find(clipId);
            if (it != idToName.end()) {
                elem.clipName = it->second;
            }
        }

        // Skip fade elements (no clip)
        if (elem.clipId.empty()) {
            continue;
        }

        elements.push_back(std::move(elem));
    }

    return elements;
}

// ---- Renaming ----

bool PtslCommands::renameTargetClip(const std::string& oldName, const std::string& newName, bool renameFile)
{
    json req;
    req["clip_name"] = oldName;
    req["new_name"] = newName;
    req["rename_file"] = renameFile;

    try {
        sendAndParse(static_cast<int32_t>(CommandId::CId_RenameTargetClip), req);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "  Warning: Failed to rename '" << oldName << "' -> '" << newName << "': " << e.what() << "\n";
        return false;
    }
}

// ---- Selection & export ----

bool PtslCommands::selectAllClipsOnTrack(const std::string& trackName)
{
    json req;
    req["track_name"] = trackName;

    try {
        sendAndParse(static_cast<int32_t>(CommandId::CId_SelectAllClipsOnTrack), req);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "  Warning: Failed to select clips on track '" << trackName << "': " << e.what() << "\n";
        return false;
    }
}

bool PtslCommands::exportClipsAsFiles(const ExportConfig& config)
{
    json req;
    req["file_path"] = config.outputDir;

    // Map format string to proto enum
    if (config.fileType == "wav") {
        req["file_type"] = "EFType_WAV";
    } else if (config.fileType == "aiff") {
        req["file_type"] = "EFType_AIFF";
    } else {
        req["file_type"] = "EFType_WAV";
    }

    // Export format: mono by default
    req["format"] = "EFormat_Mono";

    // Bit depth
    if (config.bitDepth == 16) {
        req["bit_depth"] = "BDepth_16";
    } else if (config.bitDepth == 32) {
        req["bit_depth"] = "BDepth_32";
    } else {
        req["bit_depth"] = "BDepth_24";
    }

    try {
        sendAndParse(static_cast<int32_t>(CommandId::CId_ExportClipsAsFiles), req);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "  Error: Export failed: " << e.what() << "\n";
        return false;
    }
}

// ---- Time conversion ----

std::optional<int64_t> PtslCommands::convertTimeToSamples(
    const std::string& location, const std::string& timeType)
{
    json req;
    req["location"] = { {"location", location}, {"time_type", timeType} };
    req["time_type"] = "TLType_Samples";

    try {
        auto resp = sendAndParse(static_cast<int32_t>(CommandId::CId_GetTimeAsType), req);
        if (resp.contains("converted_location") && resp["converted_location"].contains("location")) {
            std::string samplesStr = resp["converted_location"]["location"].get<std::string>();
            return std::stoll(samplesStr);
        }
    } catch (const std::exception& e) {
        std::cerr << "  Warning: Time conversion failed for '" << location << "': " << e.what() << "\n";
    }

    return std::nullopt;
}

} // namespace marec

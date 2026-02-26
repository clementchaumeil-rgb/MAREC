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
        if (resp.contains("pagination_response") &&
            resp["pagination_response"].contains("total") &&
            resp["pagination_response"]["total"].is_number()) {
            total = resp["pagination_response"]["total"].get<int>();
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

std::string PtslCommands::getSessionPath()
{
    auto resp = sendAndParse(static_cast<int32_t>(CommandId::CId_GetSessionPath));
    // Response format: {"session_path": {"info": {...}, "path": "/path/to/Session.ptx"}}
    if (resp.contains("session_path") && resp["session_path"].contains("path")) {
        return resp["session_path"]["path"].get<std::string>();
    }
    return "";
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
        info.name = t.contains("name") && t["name"].is_string() ? t["name"].get<std::string>() : "";
        info.id = t.contains("id") && t["id"].is_string() ? t["id"].get<std::string>() : "";
        info.index = t.contains("index") && t["index"].is_number() ? t["index"].get<int32_t>() : 0;

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
        m.number = ml.contains("number") && ml["number"].is_number() ? ml["number"].get<int32_t>() : 0;
        m.name = ml.contains("name") && ml["name"].is_string() ? ml["name"].get<std::string>() : "";
        m.startTime = ml.contains("start_time") && ml["start_time"].is_string() ? ml["start_time"].get<std::string>() : "";

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

RenameOutcome PtslCommands::renameTargetClip(const std::string& oldName, const std::string& newName, bool renameFile)
{
    json req;
    req["clip_name"] = oldName;
    req["new_name"] = newName;
    req["rename_file"] = renameFile;

    try {
        sendAndParse(static_cast<int32_t>(CommandId::CId_RenameTargetClip), req);
        return {true, ""};
    } catch (const std::exception& e) {
        std::cerr << "  Warning: Failed to rename '" << oldName << "' -> '" << newName << "': " << e.what() << "\n";
        return {false, e.what()};
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

    // Map format string to PTSL enum (use legacy names for compatibility)
    if (config.fileType == "aiff") {
        req["file_type"] = "AIFF";
    } else {
        req["file_type"] = "WAV";
    }

    // Export format: interleaved by default (mono unsupported on some PT versions)
    req["format"] = "EF_Interleaved";

    // Bit depth (use legacy names: Bit16, Bit24, Bit32Float)
    if (config.bitDepth == 16) {
        req["bit_depth"] = "Bit16";
    } else if (config.bitDepth == 32) {
        req["bit_depth"] = "Bit32Float";
    } else {
        req["bit_depth"] = "Bit24";
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

// ---- Session management ----

void PtslCommands::createSession(const std::string& name, const std::string& location,
    int32_t sampleRate, int32_t bitDepth)
{
    json req;
    req["session_name"] = name;
    req["session_location"] = location;
    req["file_type"] = "FT_WAVE";
    req["is_interleaved"] = false;
    req["is_cloud_project"] = false;
    req["create_from_template"] = false;
    req["input_output_settings"] = "IO_Last";

    switch (sampleRate) {
        case 44100:  req["sample_rate"] = "SR_44100";  break;
        case 88200:  req["sample_rate"] = "SR_88200";  break;
        case 96000:  req["sample_rate"] = "SR_96000";  break;
        case 176400: req["sample_rate"] = "SR_176400"; break;
        case 192000: req["sample_rate"] = "SR_192000"; break;
        default:     req["sample_rate"] = "SR_48000";  break;
    }

    switch (bitDepth) {
        case 16: req["bit_depth"] = "Bit16"; break;
        case 32: req["bit_depth"] = "Bit32"; break;
        default: req["bit_depth"] = "Bit24"; break;
    }

    sendAndParse(static_cast<int32_t>(CommandId::CId_CreateSession), req);
}

void PtslCommands::closeSession(bool saveOnClose)
{
    json req;
    req["save_on_close"] = saveOnClose;
    sendAndParse(static_cast<int32_t>(CommandId::CId_CloseSession), req);
}

void PtslCommands::saveSession()
{
    sendAndParse(static_cast<int32_t>(CommandId::CId_SaveSession));
}

// ---- Track / Marker / Clip creation ----

std::vector<std::string> PtslCommands::createNewTracks(const std::string& baseName, int count)
{
    json req;
    req["number_of_tracks"] = count;
    req["track_name"] = baseName;
    req["track_format"] = "TFormat_Mono";
    req["track_type"] = "TType_Audio";
    req["track_timebase"] = "TTimebase_Samples";

    auto resp = sendAndParse(static_cast<int32_t>(CommandId::CId_CreateNewTracks), req);

    std::vector<std::string> names;
    if (resp.contains("created_track_names") && resp["created_track_names"].is_array()) {
        for (const auto& n : resp["created_track_names"]) {
            names.push_back(n.get<std::string>());
        }
    }
    return names;
}

void PtslCommands::createMemoryLocation(int32_t number, const std::string& name,
    int64_t startSamples)
{
    // Convert sample position to the session's current counter format
    std::string timeStr = convertSamplesToSessionFormat(startSamples);

    json req;
    req["number"] = number;
    req["name"] = name;
    req["start_time"] = timeStr;
    req["time_properties"] = "TProperties_Marker";
    req["location"] = "MarkerLocation_MainRuler";

    sendAndParse(static_cast<int32_t>(CommandId::CId_CreateMemoryLocation), req);
}

std::string PtslCommands::convertSamplesToSessionFormat(int64_t samples)
{
    // Use GetTimeAsType to convert from samples to the session's main counter format
    json req;
    req["location"] = {
        {"location", std::to_string(samples)},
        {"time_type", "TLType_Samples"}
    };
    // Convert to the main counter format — pass empty time_type to get session default
    // Actually, we need to know the current counter format. Let's get it first.
    try {
        auto fmtResp = sendAndParse(static_cast<int32_t>(CommandId::CId_GetMainCounterFormat));
        std::string currentFormat = fmtResp.value("current_type",
            fmtResp.value("current_setting", ""));

        // Now convert samples → current format
        json convReq;
        convReq["location"] = {
            {"location", std::to_string(samples)},
            {"time_type", "TLType_Samples"}
        };
        convReq["time_type"] = currentFormat;

        auto convResp = sendAndParse(static_cast<int32_t>(CommandId::CId_GetTimeAsType), convReq);
        if (convResp.contains("converted_location") &&
            convResp["converted_location"].contains("location")) {
            return convResp["converted_location"]["location"].get<std::string>();
        }
    } catch (...) {
        // Fall through to raw samples
    }

    // Fallback: return raw samples string (works if counter is set to samples)
    return std::to_string(samples);
}

void PtslCommands::importAudioToClipList(const std::vector<std::string>& filePaths)
{
    json req;
    req["file_list"] = filePaths;
    req["audio_operations"] = "AOperations_CopyAudio";

    sendAndParse(static_cast<int32_t>(CommandId::CId_ImportAudioToClipList), req);
}

void PtslCommands::spotClipByID(const std::string& clipId, const std::string& trackName,
    int64_t locationSamples)
{
    json req;
    req["src_clips"] = json::array({clipId});
    req["dst_track_name"] = trackName;
    req["dst_location_data"] = {
        {"location_type", "SLType_Start"},
        {"location", {
            {"time_type", "TLType_Samples"},
            {"location", std::to_string(locationSamples)}
        }}
    };

    sendAndParse(static_cast<int32_t>(CommandId::CId_SpotClipsByID), req);
}

// ---- Counter format ----

void PtslCommands::setMainCounterFormat(const std::string& format)
{
    // Try the new location_type field (2025.06+) first, then fall back to deprecated time_scale
    static const std::vector<std::pair<std::string, std::string>> attempts = {
        {"location_type", "TLType_Samples"},
        {"time_scale", "Samples"},
        {"time_scale", "TOOptions_Samples"},
    };

    std::string lastError;
    for (const auto& [field, value] : attempts) {
        try {
            json req;
            req[field] = value;
            sendAndParse(static_cast<int32_t>(CommandId::CId_SetMainCounterFormat), req);
            return; // Success
        } catch (const std::exception& e) {
            lastError = e.what();
        }
    }
    throw std::runtime_error("SetMainCounterFormat failed (all attempts): " + lastError);
}

} // namespace marec

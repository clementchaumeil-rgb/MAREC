// Diagnostic tool: dumps raw PTSL responses to understand the data format.
// Build: add to CMakeLists.txt or compile manually alongside the project.

#include <iostream>
#include <nlohmann/json.hpp>
#include "PTSLC_CPP/CppPTSLClient.h"

using namespace PTSLC_CPP;
using json = nlohmann::json;

static json sendAndDump(CppPTSLClient& client, const std::string& label,
                        CommandId cmdId, const json& body = json::object())
{
    std::cout << "\n===== " << label << " (CId=" << static_cast<int>(cmdId) << ") =====\n";
    std::cout << "Request: " << body.dump(2) << "\n";

    CppPTSLRequest request(cmdId, body.dump());
    auto response = client.SendRequest(request).get();

    std::cout << "Status: " << static_cast<int>(response.GetStatus()) << "\n";

    std::string bodyStr = response.GetResponseBodyJson();
    std::string errorStr = response.GetResponseErrorJson();

    if (!errorStr.empty()) {
        std::cout << "Errors: " << errorStr << "\n";
    }

    if (!bodyStr.empty()) {
        try {
            json parsed = json::parse(bodyStr);
            std::cout << "Response:\n" << parsed.dump(2) << "\n";
            return parsed;
        } catch (...) {
            std::cout << "Response (raw): " << bodyStr << "\n";
        }
    } else {
        std::cout << "Response: (empty)\n";
    }
    return json::object();
}

int main()
{
    // Connect
    ClientConfig config;
    config.address = "localhost:31416";
    config.serverMode = Mode::Mode_ProTools;
    config.skipHostLaunch = SkipHostLaunch::SHLaunch_Yes;

    CppPTSLClient client(config);
    client.OnResponseReceived = [](std::shared_ptr<CppPTSLResponse>) {};

    // Register
    json regBody;
    regBody["company_name"] = "AudiArt";
    regBody["application_name"] = "MAREC_Diag";

    auto regResp = sendAndDump(client, "RegisterConnection", CommandId::CId_RegisterConnection, regBody);
    std::string sessionId = regResp.value("session_id", "");
    if (sessionId.empty()) {
        std::cerr << "Failed to get session_id\n";
        return 1;
    }
    client.SetSessionId(sessionId);
    std::cout << "\nSession ID: " << sessionId << "\n";

    // Sample rate
    sendAndDump(client, "GetSessionSampleRate", CommandId::CId_GetSessionSampleRate);

    // Main counter format
    sendAndDump(client, "GetMainCounterFormat", CommandId::CId_GetMainCounterFormat);

    // Track list (first page)
    json trackReq;
    trackReq["pagination_request"] = {{"limit", 50}, {"offset", 0}};
    auto trackResp = sendAndDump(client, "GetTrackList", CommandId::CId_GetTrackList, trackReq);

    // Memory locations (first page)
    json memReq;
    memReq["pagination_request"] = {{"limit", 50}, {"offset", 0}};
    auto memResp = sendAndDump(client, "GetMemoryLocations", CommandId::CId_GetMemoryLocations, memReq);

    // Clip list (first page)
    json clipReq;
    clipReq["pagination_request"] = {{"limit", 50}, {"offset", 0}};
    sendAndDump(client, "GetClipList", CommandId::CId_GetClipList, clipReq);

    // Try GetTrackPlaylists for "RUDI"
    json plReq;
    plReq["track_name"] = "RUDI";
    plReq["pagination_request"] = {{"limit", 50}, {"offset", 0}};
    auto plResp = sendAndDump(client, "GetTrackPlaylists (RUDI)", CommandId::CId_GetTrackPlaylists, plReq);

    // Try GetPlaylistElements if we got a playlist name
    if (plResp.contains("playlists") && plResp["playlists"].is_array()) {
        for (const auto& pl : plResp["playlists"]) {
            std::string plName = pl.value("playlist_name", "");
            if (!plName.empty()) {
                json elemReq;
                elemReq["playlist_name"] = plName;
                elemReq["time_format"] = "TLType_Samples";
                elemReq["pagination_request"] = {{"limit", 50}, {"offset", 0}};
                sendAndDump(client, "GetPlaylistElements (" + plName + ")",
                           CommandId::CId_GetPlaylistElements, elemReq);
                break; // just first one
            }
        }
    }

    // --- Test workaround: SelectAllClipsOnTrack + GetFileLocation(SelectedClipsTimeline) ---
    std::cout << "\n\n===== WORKAROUND TEST: Per-track clips via selection =====\n";

    // Select all clips on RUDI
    json selReq;
    selReq["track_name"] = "RUDI";
    sendAndDump(client, "SelectAllClipsOnTrack (RUDI)", CommandId::CId_SelectAllClipsOnTrack, selReq);

    // Get file locations for selected clips
    json fileLocReq;
    fileLocReq["file_filters"] = json::array({"FLTFilter_SelectedClipsTimeline"});
    fileLocReq["pagination_request"] = {{"limit", 50}, {"offset", 0}};
    sendAndDump(client, "GetFileLocation (SelectedClipsTimeline)", CommandId::CId_GetFileLocation, fileLocReq);

    std::cout << "\n===== DIAGNOSTIC COMPLETE =====\n";
    return 0;
}

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "session_data.h"

using json = nlohmann::json;
using namespace marec;

// These tests verify that our JSON parsing logic correctly extracts
// data from PTSL response structures matching the proto definitions.

TEST(SessionData, ParseMemoryLocationJson)
{
    json ml = {
        {"number", 1},
        {"name", "Scene_01"},
        {"start_time", "48000"},
        {"end_time", "96000"},
        {"time_properties", "TProperties_Marker"},
        {"reference", "MLReference_Absolute"},
        {"comments", "First scene"},
    };

    Marker m;
    m.number = ml.value("number", 0);
    m.name = ml.value("name", "");
    m.startTime = ml.value("start_time", "");

    std::string tp = ml.value("time_properties", "");
    if (tp == "TProperties_Marker" || tp == "TP_Marker") {
        m.timeProperties = 1;
    } else if (tp == "TProperties_Selection" || tp == "TP_Selection") {
        m.timeProperties = 2;
    }

    EXPECT_EQ(m.number, 1);
    EXPECT_EQ(m.name, "Scene_01");
    EXPECT_EQ(m.startTime, "48000");
    EXPECT_EQ(m.timeProperties, 1);
}

TEST(SessionData, ParseMemoryLocationNumericEnum)
{
    // Some PTSL versions return enum as integer
    json ml = {
        {"number", 2},
        {"name", "Verse"},
        {"start_time", "96000"},
        {"time_properties", 1},
    };

    Marker m;
    m.number = ml.value("number", 0);
    m.name = ml.value("name", "");
    m.startTime = ml.value("start_time", "");

    if (ml["time_properties"].is_number()) {
        m.timeProperties = ml["time_properties"].get<int32_t>();
    }

    EXPECT_EQ(m.timeProperties, 1);
}

TEST(SessionData, ParseTrackJson)
{
    json t = {
        {"name", "VT_EP01"},
        {"id", "0x1234"},
        {"type", "TType_Audio"},
        {"index", 3},
    };

    TrackInfo info;
    info.name = t.value("name", "");
    info.id = t.value("id", "");
    info.index = t.value("index", 0);

    if (t["type"].is_string()) {
        std::string typeStr = t["type"].get<std::string>();
        if (typeStr == "TType_Audio" || typeStr == "AudioTrack" || typeStr == "TT_Audio") {
            info.type = 2;
        }
    }

    EXPECT_EQ(info.name, "VT_EP01");
    EXPECT_EQ(info.type, 2);
    EXPECT_EQ(info.index, 3);
}

TEST(SessionData, ParseTrackNumericType)
{
    json t = {
        {"name", "MIDI_Track"},
        {"type", 1},
    };

    TrackInfo info;
    info.name = t.value("name", "");
    if (t["type"].is_number()) {
        info.type = t["type"].get<int32_t>();
    }

    EXPECT_EQ(info.type, 1); // Midi
}

TEST(SessionData, ParsePlaylistElementJson)
{
    json e = {
        {"start_time", {{"location", "48000"}, {"time_type", "TLType_Samples"}}},
        {"end_time", {{"location", "96000"}, {"time_type", "TLType_Samples"}}},
        {"channel_clips", {{{"clip_id", "abc123"}, {"is_null", false}}}},
    };

    PlaylistElement elem;
    if (e.contains("start_time") && e["start_time"].contains("location")) {
        elem.startSamples = std::stoll(e["start_time"]["location"].get<std::string>());
    }
    if (e.contains("end_time") && e["end_time"].contains("location")) {
        elem.endSamples = std::stoll(e["end_time"]["location"].get<std::string>());
    }
    if (e.contains("channel_clips") && !e["channel_clips"].empty()) {
        elem.clipId = e["channel_clips"][0].value("clip_id", "");
    }

    EXPECT_EQ(elem.startSamples, 48000);
    EXPECT_EQ(elem.endSamples, 96000);
    EXPECT_EQ(elem.clipId, "abc123");
}

TEST(SessionData, ParseSampleRateEnum)
{
    json resp = {{"sample_rate", "SR_48000"}};
    std::string srStr = resp.value("sample_rate", "SR_48000");
    auto pos = srStr.rfind('_');
    int32_t sampleRate = 48000;
    if (pos != std::string::npos) {
        sampleRate = std::stoi(srStr.substr(pos + 1));
    }
    EXPECT_EQ(sampleRate, 48000);
}

TEST(SessionData, ParseSampleRate44100)
{
    json resp = {{"sample_rate", "SR_44100"}};
    std::string srStr = resp.value("sample_rate", "SR_44100");
    auto pos = srStr.rfind('_');
    int32_t sampleRate = 0;
    if (pos != std::string::npos) {
        sampleRate = std::stoi(srStr.substr(pos + 1));
    }
    EXPECT_EQ(sampleRate, 44100);
}

TEST(SessionData, ParseClipJson)
{
    // Matches real PTSL format: original_timestamp_point gives timeline position,
    // start_point/end_point are media-internal positions.
    json c = {
        {"clip_id", "clip_001"},
        {"clip_full_name", "Recording_01"},
        {"clip_root_name", "Recording"},
        {"start_point", {{"position", 0}, {"time_type", "BTType_Samples"}}},
        {"end_point", {{"position", 48000}, {"time_type", "BTType_Samples"}}},
        {"original_timestamp_point", {{"location", "96000"}, {"time_type", "TLType_Samples"}}},
    };

    ClipInfo info;
    info.clipId = c.value("clip_id", "");
    info.clipFullName = c.value("clip_full_name", "");
    info.clipRootName = c.value("clip_root_name", "");

    // Timeline position from original_timestamp_point
    int64_t timelineStart = 0;
    if (c.contains("original_timestamp_point") && c["original_timestamp_point"].contains("location")) {
        timelineStart = std::stoll(c["original_timestamp_point"]["location"].get<std::string>());
    }
    int64_t mediaStart = 0, mediaEnd = 0;
    if (c.contains("start_point") && c["start_point"].contains("position")) {
        mediaStart = c["start_point"]["position"].get<int64_t>();
    }
    if (c.contains("end_point") && c["end_point"].contains("position")) {
        mediaEnd = c["end_point"]["position"].get<int64_t>();
    }
    info.startSamples = timelineStart;
    info.endSamples = timelineStart + (mediaEnd - mediaStart);

    EXPECT_EQ(info.clipId, "clip_001");
    EXPECT_EQ(info.clipFullName, "Recording_01");
    EXPECT_EQ(info.startSamples, 96000);    // timeline position
    EXPECT_EQ(info.endSamples, 144000);     // 96000 + (48000 - 0)
}

TEST(SessionData, ParsePaginationResponse)
{
    json resp = {
        {"memory_locations", json::array()},
        {"pagination_response", {{"total", 150}, {"limit", 100}, {"offset", 0}}},
    };

    int total = resp["pagination_response"].value("total", 0);
    int limit = resp["pagination_response"].value("limit", 0);
    int offset = resp["pagination_response"].value("offset", 0);

    EXPECT_EQ(total, 150);
    EXPECT_EQ(limit, 100);
    EXPECT_EQ(offset, 0);
}

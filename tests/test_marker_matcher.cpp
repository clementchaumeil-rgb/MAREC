#include <gtest/gtest.h>
#include "marker_matcher.h"

using namespace marec;

// Helper to create a marker with name and position
static Marker mkMarker(const std::string& name, int64_t samples)
{
    Marker m;
    m.name = name;
    m.startSamples = samples;
    m.timeProperties = 1;
    return m;
}

// Helper to create a playlist element
static PlaylistElement mkElement(const std::string& clipName, int64_t start, int64_t end = 0)
{
    PlaylistElement e;
    e.clipName = clipName;
    e.clipId = clipName; // Use name as ID for simplicity
    e.startSamples = start;
    e.endSamples = end;
    return e;
}

// --- Nominal case: each clip falls under a different marker ---

TEST(MarkerMatcher, NominalCase)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
        mkMarker("Scene_02", 48000),
        mkMarker("Scene_03", 96000),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("clip_A", 1000),
        mkElement("clip_B", 50000),
        mkElement("clip_C", 100000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    ASSERT_EQ(result.actions.size(), 3u);
    EXPECT_EQ(result.skippedBeforeFirstMarker, 0);

    EXPECT_EQ(result.actions[0].oldName, "clip_A");
    EXPECT_EQ(result.actions[0].newName, "Scene_01");

    EXPECT_EQ(result.actions[1].oldName, "clip_B");
    EXPECT_EQ(result.actions[1].newName, "Scene_02");

    EXPECT_EQ(result.actions[2].oldName, "clip_C");
    EXPECT_EQ(result.actions[2].newName, "Scene_03");
}

// --- Clips before first marker are skipped ---

TEST(MarkerMatcher, ClipsBeforeFirstMarker)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 48000),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("pre_clip_1", 0),
        mkElement("pre_clip_2", 24000),
        mkElement("post_clip", 48000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    EXPECT_EQ(result.skippedBeforeFirstMarker, 2);
    ASSERT_EQ(result.actions.size(), 1u);
    EXPECT_EQ(result.actions[0].oldName, "post_clip");
    EXPECT_EQ(result.actions[0].newName, "Scene_01");
}

// --- Duplicate clips under same marker get _01, _02 suffixes ---

TEST(MarkerMatcher, DuplicateSuffix)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
        mkMarker("Scene_02", 100000),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("clip_A", 1000),
        mkElement("clip_B", 2000),
        mkElement("clip_C", 3000),
        mkElement("clip_D", 100000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    ASSERT_EQ(result.actions.size(), 4u);

    // First 3 clips under Scene_01 → Scene_01_01, Scene_01_02, Scene_01_03
    EXPECT_EQ(result.actions[0].newName, "Scene_01_01");
    EXPECT_EQ(result.actions[1].newName, "Scene_01_02");
    EXPECT_EQ(result.actions[2].newName, "Scene_01_03");

    // Single clip under Scene_02 → no suffix
    EXPECT_EQ(result.actions[3].newName, "Scene_02");
}

// --- Empty markers → all elements skipped ---

TEST(MarkerMatcher, EmptyMarkers)
{
    std::vector<Marker> markers;
    std::vector<PlaylistElement> elements = {
        mkElement("clip_A", 1000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    EXPECT_TRUE(result.actions.empty());
    EXPECT_EQ(result.skippedBeforeFirstMarker, 1);
}

// --- Empty elements → no actions ---

TEST(MarkerMatcher, EmptyElements)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
    };
    std::vector<PlaylistElement> elements;

    auto result = MarkerMatcher::match(markers, elements);

    EXPECT_TRUE(result.actions.empty());
    EXPECT_EQ(result.skippedBeforeFirstMarker, 0);
}

// --- Unresolved markers (startSamples < 0) are filtered out ---

TEST(MarkerMatcher, UnresolvedMarkersIgnored)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
        mkMarker("Bad_Marker", -1), // Unresolved
        mkMarker("Scene_02", 48000),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("clip_A", 1000),
        mkElement("clip_B", 50000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    ASSERT_EQ(result.actions.size(), 2u);
    EXPECT_EQ(result.actions[0].newName, "Scene_01");
    EXPECT_EQ(result.actions[1].newName, "Scene_02");
}

// --- Clip exactly at marker position matches that marker ---

TEST(MarkerMatcher, ClipExactlyAtMarker)
{
    std::vector<Marker> markers = {
        mkMarker("Intro", 0),
        mkMarker("Verse", 48000),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("clip_at_verse", 48000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    ASSERT_EQ(result.actions.size(), 1u);
    EXPECT_EQ(result.actions[0].newName, "Verse");
}

// --- Unsorted markers and elements are handled correctly ---

TEST(MarkerMatcher, UnsortedInput)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_03", 96000),
        mkMarker("Scene_01", 0),
        mkMarker("Scene_02", 48000),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("clip_C", 100000),
        mkElement("clip_A", 1000),
        mkElement("clip_B", 50000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    ASSERT_EQ(result.actions.size(), 3u);

    // Should be sorted by position in output
    EXPECT_EQ(result.actions[0].newName, "Scene_01");
    EXPECT_EQ(result.actions[0].oldName, "clip_A");

    EXPECT_EQ(result.actions[1].newName, "Scene_02");
    EXPECT_EQ(result.actions[1].oldName, "clip_B");

    EXPECT_EQ(result.actions[2].newName, "Scene_03");
    EXPECT_EQ(result.actions[2].oldName, "clip_C");
}

// --- Clip already has the correct name → skipped ---

TEST(MarkerMatcher, AlreadyCorrectNameSkipped)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("Scene_01", 1000), // Already has the correct name
    };

    auto result = MarkerMatcher::match(markers, elements);

    EXPECT_TRUE(result.actions.empty());
    EXPECT_EQ(result.skippedBeforeFirstMarker, 0);
}

// --- All markers unresolved → all clips skipped ---

TEST(MarkerMatcher, AllMarkersUnresolved)
{
    std::vector<Marker> markers = {
        mkMarker("Bad1", -1),
        mkMarker("Bad2", -1),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("clip_A", 1000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    EXPECT_TRUE(result.actions.empty());
    EXPECT_EQ(result.skippedBeforeFirstMarker, 1);
}

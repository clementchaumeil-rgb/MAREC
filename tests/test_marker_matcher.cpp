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

// --- Stereo clips: root names (without .L/.R) produce correct renames ---

TEST(MarkerMatcher, StereoRootNamesMatch)
{
    // Simulates what happens after the idToName fix: elements already use root names
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
        mkMarker("Scene_02", 48000),
    };

    // After dedup, only one element per position with root name
    std::vector<PlaylistElement> elements = {
        mkElement("Audio 1", 1000),
        mkElement("Audio 2", 50000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    ASSERT_EQ(result.actions.size(), 2u);
    EXPECT_EQ(result.actions[0].oldName, "Audio 1");
    EXPECT_EQ(result.actions[0].newName, "Scene_01");
    EXPECT_EQ(result.actions[1].oldName, "Audio 2");
    EXPECT_EQ(result.actions[1].newName, "Scene_02");
}

// --- Stereo duplicate elements (pre-dedup) produce doubled actions ---
// This verifies that without dedup, L+R both generate actions.
// The dedup happens in getTrackElements() before calling match().

TEST(MarkerMatcher, StereoDuplicateElementsProduceDoubledActions)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
    };

    // Two elements with same root name and same position (L+R before dedup)
    std::vector<PlaylistElement> elements = {
        mkElement("Audio 1", 1000),
        mkElement("Audio 1", 1000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    // Without dedup, matcher sees 2 clips under Scene_01 → suffixed as duplicates
    ASSERT_EQ(result.actions.size(), 2u);
    EXPECT_EQ(result.actions[0].newName, "Scene_01_01");
    EXPECT_EQ(result.actions[1].newName, "Scene_01_02");
}

// --- After dedup, single element per position → clean rename ---

TEST(MarkerMatcher, StereoAfterDedupSingleAction)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
    };

    // After dedup: only one element remains
    std::vector<PlaylistElement> elements = {
        mkElement("Audio 1", 1000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    ASSERT_EQ(result.actions.size(), 1u);
    EXPECT_EQ(result.actions[0].oldName, "Audio 1");
    EXPECT_EQ(result.actions[0].newName, "Scene_01");
}

// --- Idempotency: clips already renamed with correct marker + suffix are skipped ---

TEST(MarkerMatcher, IdempotentSuffixSkipped)
{
    std::vector<Marker> markers = {
        mkMarker("Location 11", 0),
    };

    // Clips already renamed from a previous MAREC run, but with different suffix order
    std::vector<PlaylistElement> elements = {
        mkElement("Location 11_02", 1000),
        mkElement("Location 11_01", 2000),
        mkElement("Location 11_03", 3000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    // All 3 clips already have the correct marker prefix → no actions needed
    EXPECT_TRUE(result.actions.empty());
    EXPECT_EQ(result.skippedBeforeFirstMarker, 0);
}

// --- Idempotency: mix of already-renamed and new clips ---

TEST(MarkerMatcher, IdempotentMixedClips)
{
    std::vector<Marker> markers = {
        mkMarker("Location 11", 0),
    };

    // 2 already renamed + 1 new clip
    std::vector<PlaylistElement> elements = {
        mkElement("Location 11_01", 1000),
        mkElement("MATTHEW_01", 2000),
        mkElement("Location 11_02", 3000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    // Only the new clip should generate an action
    ASSERT_EQ(result.actions.size(), 1u);
    EXPECT_EQ(result.actions[0].oldName, "MATTHEW_01");
    EXPECT_EQ(result.actions[0].markerName, "Location 11");
}

// --- Idempotency: single clip with exact marker name (no suffix) ---

TEST(MarkerMatcher, IdempotentSingleClipNoSuffix)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
        mkMarker("Scene_02", 48000),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("Scene_01", 1000),  // Already correct (exact)
        mkElement("Scene_02", 50000), // Already correct (exact)
    };

    auto result = MarkerMatcher::match(markers, elements);
    EXPECT_TRUE(result.actions.empty());
}

// --- Deterministic suffix assignment: clips at same position sorted by name ---

TEST(MarkerMatcher, DeterministicSuffixAtSamePosition)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
    };

    // Two clips at the exact same position — suffix should be deterministic
    // sorted by name: "clip_A" < "clip_B"
    std::vector<PlaylistElement> elements = {
        mkElement("clip_B", 1000),
        mkElement("clip_A", 1000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    ASSERT_EQ(result.actions.size(), 2u);
    // clip_A sorts first → _01, clip_B → _02
    EXPECT_EQ(result.actions[0].oldName, "clip_A");
    EXPECT_EQ(result.actions[0].newName, "Scene_01_01");
    EXPECT_EQ(result.actions[1].oldName, "clip_B");
    EXPECT_EQ(result.actions[1].newName, "Scene_01_02");
}

// --- Marker name that contains underscore+digits should not be confused with suffix ---

TEST(MarkerMatcher, MarkerNameWithUnderscore)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
    };

    // A clip named "Scene_01_03" has the marker prefix with a suffix — should be skipped
    std::vector<PlaylistElement> elements = {
        mkElement("Scene_01_03", 1000),
    };

    auto result = MarkerMatcher::match(markers, elements);
    EXPECT_TRUE(result.actions.empty());
}

// ============================================================
// Track prefix tests (trackName parameter)
// ============================================================

// --- Track prefix: single clip renamed to "TrackName - MarkerName" ---

TEST(MarkerMatcher, TrackPrefixNominal)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("clip_A", 1000),
    };

    auto result = MarkerMatcher::match(markers, elements, "RUDI");

    ASSERT_EQ(result.actions.size(), 1u);
    EXPECT_EQ(result.actions[0].oldName, "clip_A");
    EXPECT_EQ(result.actions[0].newName, "RUDI - Scene_01");
    EXPECT_EQ(result.actions[0].markerName, "Scene_01"); // markerName unchanged
}

// --- Track prefix with duplicates: "RUDI - Scene_01_01", "_02", "_03" ---

TEST(MarkerMatcher, TrackPrefixDuplicates)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("clip_A", 1000),
        mkElement("clip_B", 2000),
        mkElement("clip_C", 3000),
    };

    auto result = MarkerMatcher::match(markers, elements, "RUDI");

    ASSERT_EQ(result.actions.size(), 3u);
    EXPECT_EQ(result.actions[0].newName, "RUDI - Scene_01_01");
    EXPECT_EQ(result.actions[1].newName, "RUDI - Scene_01_02");
    EXPECT_EQ(result.actions[2].newName, "RUDI - Scene_01_03");
}

// --- Track prefix idempotent: clip already named "RUDI - Scene_01" → skipped ---

TEST(MarkerMatcher, TrackPrefixIdempotent)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("RUDI - Scene_01", 1000),
    };

    auto result = MarkerMatcher::match(markers, elements, "RUDI");
    EXPECT_TRUE(result.actions.empty());
}

// --- Track prefix idempotent with suffix: "RUDI - Scene_01_02" → skipped ---

TEST(MarkerMatcher, TrackPrefixIdempotentWithSuffix)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("RUDI - Scene_01_01", 1000),
        mkElement("RUDI - Scene_01_02", 2000),
        mkElement("RUDI - Scene_01_03", 3000),
    };

    auto result = MarkerMatcher::match(markers, elements, "RUDI");
    EXPECT_TRUE(result.actions.empty());
}

// --- Empty trackName → old format (same as existing NominalCase behavior) ---

TEST(MarkerMatcher, TrackPrefixEmptyFallback)
{
    std::vector<Marker> markers = {
        mkMarker("Scene_01", 0),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("clip_A", 1000),
    };

    auto result = MarkerMatcher::match(markers, elements, "");

    ASSERT_EQ(result.actions.size(), 1u);
    EXPECT_EQ(result.actions[0].newName, "Scene_01");
}

// --- Track prefix mixed: some already correct, some need renaming ---

TEST(MarkerMatcher, TrackPrefixMixed)
{
    std::vector<Marker> markers = {
        mkMarker("Location 9", 0),
    };

    std::vector<PlaylistElement> elements = {
        mkElement("RUDI - Location 9_01", 1000),  // already correct
        mkElement("MATTHEW_01", 2000),             // needs rename
        mkElement("RUDI - Location 9_03", 3000),   // already correct
    };

    auto result = MarkerMatcher::match(markers, elements, "RUDI");

    ASSERT_EQ(result.actions.size(), 1u);
    EXPECT_EQ(result.actions[0].oldName, "MATTHEW_01");
    EXPECT_EQ(result.actions[0].markerName, "Location 9");
}

// --- A clip that partially matches the marker name but isn't a real suffix ---

TEST(MarkerMatcher, PartialMatchNotConfusedWithSuffix)
{
    std::vector<Marker> markers = {
        mkMarker("Location 1", 0),
    };

    // "Location 11" starts with "Location 1" but is NOT a suffix match
    // (character after prefix is '1', not '_')
    std::vector<PlaylistElement> elements = {
        mkElement("Location 11", 1000),
    };

    auto result = MarkerMatcher::match(markers, elements);

    ASSERT_EQ(result.actions.size(), 1u);
    EXPECT_EQ(result.actions[0].oldName, "Location 11");
    EXPECT_EQ(result.actions[0].newName, "Location 1");
}

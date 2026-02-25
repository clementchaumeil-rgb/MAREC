#include <gtest/gtest.h>
#include "time_utils.h"

using namespace marec;

// --- parseTimecode ---

TEST(TimeUtils, ParseTimecodeValid)
{
    // 01:00:00:00 at 48000 Hz, 25 fps = 3600 seconds = 172800000 samples
    auto result = TimeUtils::parseTimecode("01:00:00:00", 48000, 25.0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 172800000);
}

TEST(TimeUtils, ParseTimecodeWithFrames)
{
    // 00:00:01:12 at 48000 Hz, 25 fps = 1 + 12/25 = 1.48 seconds = 71040 samples
    auto result = TimeUtils::parseTimecode("00:00:01:12", 48000, 25.0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 71040);
}

TEST(TimeUtils, ParseTimecodeDropFrame)
{
    // 00:00:01;12 (semicolon for drop-frame notation)
    auto result = TimeUtils::parseTimecode("00:00:01;12", 48000, 25.0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 71040);
}

TEST(TimeUtils, ParseTimecodeInvalid)
{
    EXPECT_FALSE(TimeUtils::parseTimecode("not_a_timecode", 48000).has_value());
    EXPECT_FALSE(TimeUtils::parseTimecode("", 48000).has_value());
    EXPECT_FALSE(TimeUtils::parseTimecode("12345", 48000).has_value());
}

// --- parseMinSecs ---

TEST(TimeUtils, ParseMinSecsValid)
{
    // 1:30.000 = 90 seconds at 48000 Hz = 4320000 samples
    auto result = TimeUtils::parseMinSecs("1:30.000", 48000);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 4320000);
}

TEST(TimeUtils, ParseMinSecsWithMillis)
{
    // 0:01.500 = 1.5 seconds = 72000 samples
    auto result = TimeUtils::parseMinSecs("0:01.500", 48000);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 72000);
}

TEST(TimeUtils, ParseMinSecsInvalid)
{
    EXPECT_FALSE(TimeUtils::parseMinSecs("not_a_time", 48000).has_value());
    EXPECT_FALSE(TimeUtils::parseMinSecs("", 48000).has_value());
}

// --- parseBarsBeats ---

TEST(TimeUtils, ParseBarsBeatsAlwaysNullopt)
{
    EXPECT_FALSE(TimeUtils::parseBarsBeats("4|1|000").has_value());
}

// --- parseAuto ---

TEST(TimeUtils, ParseAutoSamples)
{
    auto result = TimeUtils::parseAuto("48000", 48000);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 48000);
}

TEST(TimeUtils, ParseAutoTimecode)
{
    auto result = TimeUtils::parseAuto("00:00:01:00", 48000, 25.0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 48000);
}

TEST(TimeUtils, ParseAutoMinSecs)
{
    auto result = TimeUtils::parseAuto("0:01.000", 48000);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 48000);
}

TEST(TimeUtils, ParseAutoEmpty)
{
    EXPECT_FALSE(TimeUtils::parseAuto("", 48000).has_value());
}

TEST(TimeUtils, ParseAutoInvalid)
{
    EXPECT_FALSE(TimeUtils::parseAuto("garbage", 48000).has_value());
}

// --- formatSamplesToTime ---

TEST(TimeUtils, FormatSamplesToTime)
{
    // 48000 samples at 48000 Hz = 1 second = "00:00:01.000"
    auto str = TimeUtils::formatSamplesToTime(48000, 48000);
    EXPECT_EQ(str, "00:00:01.000");
}

TEST(TimeUtils, FormatSamplesToTimeZero)
{
    auto str = TimeUtils::formatSamplesToTime(0, 48000);
    EXPECT_EQ(str, "00:00:00.000");
}

TEST(TimeUtils, FormatSamplesToTimeOneHour)
{
    // 172800000 samples at 48000 Hz = 3600 seconds = 1 hour
    auto str = TimeUtils::formatSamplesToTime(172800000, 48000);
    EXPECT_EQ(str, "01:00:00.000");
}

TEST(TimeUtils, FormatSamplesToTimeInvalidSampleRate)
{
    auto str = TimeUtils::formatSamplesToTime(48000, 0);
    EXPECT_EQ(str, "48000 smp");
}

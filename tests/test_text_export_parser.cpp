#include "text_export_parser.h"

#include <gtest/gtest.h>
#include <fstream>
#include <sstream>

using namespace marec;

// ---- Header parsing ----

TEST(TextExportParserTest, ParseHeader_NominalCase)
{
    std::string input =
        "SESSION NAME:\tSaddle Club_ep103\n"
        "SAMPLE RATE:\t48000.000000\n"
        "BIT DEPTH:\t24-bit\n"
        "SESSION START TIMECODE:\t00:00:00:00\n"
        "TIMECODE FORMAT:\t24 Frame\n"
        "# OF AUDIO TRACKS:\t24\n"
        "# OF AUDIO CLIPS:\t962\n"
        "# OF AUDIO FILES:\t416\n"
        "\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME                             \tTRACK NAME                       \tTRACK TYPE   \tCOMMENTS\n"
        "1   \t10:00:00:00  \t1728000000        \tSamples  \tLocation 1                       \tMarkers                          \tRuler                            \t\n";

    auto result = TextExportParser::parseString(input);

    EXPECT_EQ(result.header.sessionName, "Saddle Club_ep103");
    EXPECT_EQ(result.header.sampleRate, 48000);
    EXPECT_EQ(result.header.sessionStartTimecode, "00:00:00:00");
    EXPECT_EQ(result.header.timecodeFormat, "24 Frame");
}

TEST(TextExportParserTest, ParseHeader_FloatSampleRate)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\n";

    auto result = TextExportParser::parseString(input);
    EXPECT_EQ(result.header.sampleRate, 48000);
}

// ---- Marker parsing ----

TEST(TextExportParserTest, ParseMarkers_NominalCase)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\n"
        "1   \t10:00:00:00  \t1728000000        \tSamples  \tLocation 1\tMarkers\tRuler\t\n"
        "2   \t10:00:30:00  \t1729440000        \tSamples  \tLocation 2\tMarkers\tRuler\t\n"
        "3   \t10:01:00:00  \t1730880000        \tSamples  \tLocation 3\tMarkers\tRuler\t\n";

    auto result = TextExportParser::parseString(input);

    ASSERT_EQ(result.markers.size(), 3u);
    EXPECT_EQ(result.markers[0].number, 1);
    EXPECT_EQ(result.markers[0].name, "Location 1");
    EXPECT_EQ(result.markers[0].timeReference, 1728000000);
    EXPECT_EQ(result.markers[0].location, "10:00:00:00");

    EXPECT_EQ(result.markers[1].number, 2);
    EXPECT_EQ(result.markers[1].timeReference, 1729440000);

    EXPECT_EQ(result.markers[2].number, 3);
    EXPECT_EQ(result.markers[2].timeReference, 1730880000);
}

TEST(TextExportParserTest, ParseMarkers_NonSequentialNumbers)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\n"
        "77  \t10:38:00:00  \t1837440000        \tSamples  \tLocation 77\tMarkers\tRuler\t\n"
        "201 \t10:38:11:08  \t1837984000        \tSamples  \tbis\tMarkers\tRuler\t\n"
        "78  \t10:38:30:00  \t1838880000        \tSamples  \tLocation 78\tMarkers\tRuler\t\n";

    auto result = TextExportParser::parseString(input);

    ASSERT_EQ(result.markers.size(), 3u);
    EXPECT_EQ(result.markers[0].number, 77);
    EXPECT_EQ(result.markers[1].number, 201);
    EXPECT_EQ(result.markers[1].name, "bis");
    EXPECT_EQ(result.markers[1].timeReference, 1837984000);
    EXPECT_EQ(result.markers[2].number, 78);
}

TEST(TextExportParserTest, ParseMarkers_DuplicateNames)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\n"
        "201 \t10:38:11:08  \t1837984000        \tSamples  \tbis\tMarkers\tRuler\t\n"
        "206 \t10:46:15:13  \t1861226000        \tSamples  \tbis\tMarkers\tRuler\t\n"
        "208 \t10:53:41:00  \t1882608000        \tSamples  \tbis\tMarkers\tRuler\t\n";

    auto result = TextExportParser::parseString(input);

    ASSERT_EQ(result.markers.size(), 3u);
    EXPECT_EQ(result.markers[0].name, "bis");
    EXPECT_EQ(result.markers[1].name, "bis");
    EXPECT_EQ(result.markers[2].name, "bis");
    // All different numbers and positions
    EXPECT_NE(result.markers[0].number, result.markers[1].number);
    EXPECT_NE(result.markers[0].timeReference, result.markers[1].timeReference);
}

TEST(TextExportParserTest, ParseMarkers_TrailingWhitespace)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME                             \tTRACK NAME                       \tTRACK TYPE   \tCOMMENTS\n"
        "1   \t10:00:00:00  \t1728000000        \tSamples  \tLocation 1                       \tMarkers                          \tRuler                            \t\n";

    auto result = TextExportParser::parseString(input);

    ASSERT_EQ(result.markers.size(), 1u);
    EXPECT_EQ(result.markers[0].name, "Location 1");
    EXPECT_EQ(result.markers[0].location, "10:00:00:00");
}

TEST(TextExportParserTest, ParseMarkers_WindowsLineEndings)
{
    std::string input =
        "SESSION NAME:\tTest\r\n"
        "SAMPLE RATE:\t48000.000000\r\n"
        "\r\n"
        "M A R K E R S  L I S T I N G\r\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\r\n"
        "1   \t10:00:00:00  \t1728000000        \tSamples  \tLocation 1\tMarkers\tRuler\t\r\n"
        "2   \t10:00:30:00  \t1729440000        \tSamples  \tLocation 2\tMarkers\tRuler\t\r\n";

    auto result = TextExportParser::parseString(input);

    ASSERT_EQ(result.markers.size(), 2u);
    EXPECT_EQ(result.markers[0].name, "Location 1");
    EXPECT_EQ(result.markers[1].name, "Location 2");
}

TEST(TextExportParserTest, ParseMarkers_UTF8BOM)
{
    std::string input =
        "\xEF\xBB\xBF" // UTF-8 BOM
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\n"
        "1   \t10:00:00:00  \t1728000000        \tSamples  \tLocation 1\tMarkers\tRuler\t\n";

    auto result = TextExportParser::parseString(input);

    EXPECT_EQ(result.header.sessionName, "Test");
    ASSERT_EQ(result.markers.size(), 1u);
    EXPECT_EQ(result.markers[0].name, "Location 1");
}

// ---- Edge cases ----

TEST(TextExportParserTest, ParseMarkers_EmptyFile)
{
    EXPECT_THROW(TextExportParser::parseString(""), std::runtime_error);
}

TEST(TextExportParserTest, ParseMarkers_NoMarkersSection)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "Some other content\n";

    EXPECT_THROW(TextExportParser::parseString(input), std::runtime_error);
}

TEST(TextExportParserTest, ParseMarkers_EmptyMarkersSection)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\n";

    auto result = TextExportParser::parseString(input);
    EXPECT_EQ(result.markers.size(), 0u);
    EXPECT_TRUE(result.warnings.empty());
}

TEST(TextExportParserTest, ParseMarkers_MalformedRow)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\n"
        "1   \t10:00:00:00  \t1728000000        \tSamples  \tLocation 1\tMarkers\tRuler\t\n"
        "this is garbage\n"
        "3   \t10:01:00:00  \t1730880000        \tSamples  \tLocation 3\tMarkers\tRuler\t\n";

    auto result = TextExportParser::parseString(input);

    ASSERT_EQ(result.markers.size(), 2u);
    EXPECT_EQ(result.markers[0].number, 1);
    EXPECT_EQ(result.markers[1].number, 3);
    EXPECT_EQ(result.warnings.size(), 1u);
}

TEST(TextExportParserTest, ParseMarkers_InvalidTimeReference)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\n"
        "1   \t10:00:00:00  \tNOT_A_NUMBER      \tSamples  \tLocation 1\tMarkers\tRuler\t\n"
        "2   \t10:00:30:00  \t1729440000        \tSamples  \tLocation 2\tMarkers\tRuler\t\n";

    auto result = TextExportParser::parseString(input);

    ASSERT_EQ(result.markers.size(), 1u);
    EXPECT_EQ(result.markers[0].number, 2);
    EXPECT_EQ(result.warnings.size(), 1u);
}

TEST(TextExportParserTest, ParseMarkers_StopsAtNextSection)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\n"
        "1   \t10:00:00:00  \t1728000000        \tSamples  \tLocation 1\tMarkers\tRuler\t\n"
        "2   \t10:00:30:00  \t1729440000        \tSamples  \tLocation 2\tMarkers\tRuler\t\n"
        "\n"
        "T R A C K  L I S T I N G\n"
        "TRACK NAME\tCOMMENTS\n"
        "Track 1\t\n";

    auto result = TextExportParser::parseString(input);

    ASSERT_EQ(result.markers.size(), 2u);
    EXPECT_EQ(result.markers[0].number, 1);
    EXPECT_EQ(result.markers[1].number, 2);
}

TEST(TextExportParserTest, ParseMarkers_TooFewColumns)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\n"
        "1   \t10:00:00:00\n"
        "2   \t10:00:30:00  \t1729440000        \tSamples  \tLocation 2\tMarkers\tRuler\t\n";

    auto result = TextExportParser::parseString(input);

    ASSERT_EQ(result.markers.size(), 1u);
    EXPECT_EQ(result.markers[0].number, 2);
    EXPECT_EQ(result.warnings.size(), 1u);
}

// ---- Real file tests ----

TEST(TextExportParserTest, ParseMarkers_RealFile)
{
    // This test uses the actual exported text file from the project root
    const std::string path = "Saddle Club_ep103.txt";

    // Try to find the file relative to the build directory or project root
    std::string filePath;
    for (const auto& candidate : {path, "../" + path, "../../" + path}) {
        std::ifstream f(candidate);
        if (f.good()) {
            filePath = candidate;
            break;
        }
    }

    if (filePath.empty()) {
        GTEST_SKIP() << "Test file not found: " << path;
    }

    auto result = TextExportParser::parse(filePath);

    EXPECT_EQ(result.header.sessionName, "Saddle Club_ep103");
    EXPECT_EQ(result.header.sampleRate, 48000);
    EXPECT_EQ(result.header.timecodeFormat, "24 Frame");
    EXPECT_EQ(result.markers.size(), 209u);
    EXPECT_TRUE(result.warnings.empty());
}

TEST(TextExportParserTest, ParseMarkers_RealFile_SpotCheck)
{
    const std::string path = "Saddle Club_ep103.txt";

    std::string filePath;
    for (const auto& candidate : {path, "../" + path, "../../" + path}) {
        std::ifstream f(candidate);
        if (f.good()) {
            filePath = candidate;
            break;
        }
    }

    if (filePath.empty()) {
        GTEST_SKIP() << "Test file not found: " << path;
    }

    auto result = TextExportParser::parse(filePath);

    // First marker: #1 at 1728000000 "Location 1"
    ASSERT_GE(result.markers.size(), 1u);
    EXPECT_EQ(result.markers[0].number, 1);
    EXPECT_EQ(result.markers[0].name, "Location 1");
    EXPECT_EQ(result.markers[0].timeReference, 1728000000);

    // Non-sequential marker #201 "bis" at 1837984000 (appears between #77 and #78)
    bool found201 = false;
    for (const auto& m : result.markers) {
        if (m.number == 201) {
            EXPECT_EQ(m.name, "bis");
            EXPECT_EQ(m.timeReference, 1837984000);
            found201 = true;
            break;
        }
    }
    EXPECT_TRUE(found201) << "Marker #201 'bis' not found";

    // Last marker: #200 at 2014560000 "Location 200"
    EXPECT_EQ(result.markers.back().number, 200);
    EXPECT_EQ(result.markers.back().name, "Location 200");
    EXPECT_EQ(result.markers.back().timeReference, 2014560000);
}

// ---- Marker with comments ----

TEST(TextExportParserTest, ParseMarkers_WithComments)
{
    std::string input =
        "SESSION NAME:\tTest\n"
        "SAMPLE RATE:\t48000.000000\n"
        "\n"
        "M A R K E R S  L I S T I N G\n"
        "#   \tLOCATION     \tTIME REFERENCE    \tUNITS    \tNAME\tTRACK NAME\tTRACK TYPE\tCOMMENTS\n"
        "1   \t10:00:00:00  \t1728000000        \tSamples  \tScene 1\tMarkers\tRuler\tFirst scene of the episode\n";

    auto result = TextExportParser::parseString(input);

    ASSERT_EQ(result.markers.size(), 1u);
    EXPECT_EQ(result.markers[0].comments, "First scene of the episode");
}

#pragma once

#include "session_data.h"

#include <string>
#include <vector>

namespace marec {

class PtslCommands;

// --- Test scenario data structures ---

struct TestClip {
    std::string name;        // WAV filename (without .wav extension)
    int64_t positionSamples; // Timeline position where clip is spotted
};

struct TestMarker {
    int32_t number;
    std::string name;
    int64_t positionSamples;
};

struct TestTrack {
    std::string name;
    std::vector<TestClip> clips;
};

struct TestScenario {
    std::string name;
    std::string description;
    std::vector<TestMarker> markers;
    std::vector<TestTrack> tracks;
    int32_t sampleRate = 48000;
};

struct ScenarioExpectation {
    int expectedRenames;
    int expectedSkipped;       // Clips before first marker
    int expectedAlreadyCorrect;
    // Optional: expected rename pairs (oldName → newName) for detailed validation
    std::vector<std::pair<std::string, std::string>> renamePairs;
};

// --- Test session builder ---

class TestSessionBuilder {
public:
    explicit TestSessionBuilder(PtslCommands& cmds);

    /// Create a Pro Tools session with tracks, markers, and clips.
    /// @param scenario  The test scenario definition
    /// @param baseDir   Base directory for session and WAV files (e.g. "/tmp/marec_tests")
    /// @return true if setup completed successfully
    bool setup(const TestScenario& scenario, const std::string& baseDir);

    /// Close the session and clean up temp files.
    void teardown();

    /// Get actual track names created by Pro Tools (may differ from requested names)
    const std::vector<std::string>& createdTrackNames() const { return m_createdTrackNames; }

private:
    PtslCommands& m_cmds;
    std::string m_sessionDir;
    std::string m_wavDir;
    std::vector<std::string> m_createdTrackNames;

    bool generateWavFiles(const TestScenario& scenario);
    bool importAndSpotClips(const TestScenario& scenario);
};

} // namespace marec

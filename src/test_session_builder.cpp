#include "test_session_builder.h"
#include "ptsl_commands.h"
#include "test_wav_writer.h"

#include <filesystem>
#include <iostream>
#include <unordered_map>

namespace fs = std::filesystem;

namespace marec {

TestSessionBuilder::TestSessionBuilder(PtslCommands& cmds)
    : m_cmds(cmds)
{}

bool TestSessionBuilder::setup(const TestScenario& scenario, const std::string& baseDir)
{
    m_sessionDir = baseDir + "/" + scenario.name;
    m_wavDir = baseDir + "/wav_" + scenario.name;
    m_createdTrackNames.clear();

    // Clean up any previous test session at this path
    std::error_code ec;
    fs::remove_all(m_sessionDir, ec);
    fs::remove_all(m_wavDir, ec);

    // Create directories
    fs::create_directories(m_sessionDir);
    fs::create_directories(m_wavDir);

    // 1. Close any existing session (ignore errors)
    try { m_cmds.closeSession(false); }
    catch (...) {}

    // 2. Create session
    std::cerr << "  Setup: Creating session '" << scenario.name << "'...";
    try {
        m_cmds.createSession(scenario.name, m_sessionDir, scenario.sampleRate);
        std::cerr << " OK\n";
    } catch (const std::exception& e) {
        std::cerr << " FAILED: " << e.what() << "\n";
        return false;
    }

    // 3. Set counter format to samples
    try {
        m_cmds.setMainCounterFormat("TLFormat_Samples");
    } catch (const std::exception& e) {
        std::cerr << "  Setup: Warning — could not set counter format: " << e.what() << "\n";
        // Continue anyway — marker times might still work
    }

    // 4. Create tracks
    for (const auto& track : scenario.tracks) {
        std::cerr << "  Setup: Creating track '" << track.name << "'...";
        try {
            auto names = m_cmds.createNewTracks(track.name, 1);
            std::string actualName = names.empty() ? track.name : names[0];
            m_createdTrackNames.push_back(actualName);
            std::cerr << " OK";
            if (actualName != track.name) {
                std::cerr << " (actual: '" << actualName << "')";
            }
            std::cerr << "\n";
        } catch (const std::exception& e) {
            std::cerr << " FAILED: " << e.what() << "\n";
            return false;
        }
    }

    // 5. Create markers
    if (!scenario.markers.empty()) {
        std::cerr << "  Setup: Creating " << scenario.markers.size() << " marker(s)...";
        for (const auto& marker : scenario.markers) {
            try {
                m_cmds.createMemoryLocation(marker.number, marker.name, marker.positionSamples);
            } catch (const std::exception& e) {
                std::cerr << " FAILED on '" << marker.name << "': " << e.what() << "\n";
                return false;
            }
        }
        std::cerr << " OK\n";
    }

    // 6. Generate WAV files, import, and spot clips
    if (!generateWavFiles(scenario)) return false;

    // Count total clips
    int totalClips = 0;
    for (const auto& t : scenario.tracks) totalClips += static_cast<int>(t.clips.size());

    if (totalClips > 0) {
        if (!importAndSpotClips(scenario)) return false;
    }

    // 7. Save session
    std::cerr << "  Setup: Saving session...";
    try {
        m_cmds.saveSession();
        std::cerr << " OK\n";
    } catch (const std::exception& e) {
        std::cerr << " FAILED: " << e.what() << "\n";
        return false;
    }

    return true;
}

void TestSessionBuilder::teardown()
{
    // Close session (don't save — we don't need the test data)
    try { m_cmds.closeSession(false); }
    catch (...) {}

    // Remove temp directories
    std::error_code ec;
    if (!m_wavDir.empty()) {
        fs::remove_all(m_wavDir, ec);
    }
    // Session dir contains the Pro Tools session — remove it too
    if (!m_sessionDir.empty()) {
        fs::remove_all(m_sessionDir, ec);
    }
}

bool TestSessionBuilder::generateWavFiles(const TestScenario& scenario)
{
    int totalClips = 0;
    for (const auto& t : scenario.tracks) totalClips += static_cast<int>(t.clips.size());
    if (totalClips == 0) return true;

    std::cerr << "  Setup: Generating " << totalClips << " WAV file(s)...";

    for (const auto& track : scenario.tracks) {
        for (const auto& clip : track.clips) {
            std::string wavPath = m_wavDir + "/" + clip.name + ".wav";
            // 1-second clips at the scenario's sample rate, with BWF timestamp at clip position
            if (!TestWavWriter::generate(wavPath, scenario.sampleRate, scenario.sampleRate, 24,
                    clip.positionSamples)) {
                std::cerr << " FAILED on '" << clip.name << "'\n";
                return false;
            }
        }
    }
    std::cerr << " OK\n";
    return true;
}

bool TestSessionBuilder::importAndSpotClips(const TestScenario& scenario)
{
    // Collect all WAV paths for import
    std::vector<std::string> wavPaths;
    for (const auto& track : scenario.tracks) {
        for (const auto& clip : track.clips) {
            wavPaths.push_back(m_wavDir + "/" + clip.name + ".wav");
        }
    }

    // Import all audio files at once
    std::cerr << "  Setup: Importing " << wavPaths.size() << " audio file(s)...";
    try {
        m_cmds.importAudioToClipList(wavPaths);
        std::cerr << " OK\n";
    } catch (const std::exception& e) {
        std::cerr << " FAILED: " << e.what() << "\n";
        return false;
    }

    // Get clip list to find clip IDs by name
    std::cerr << "  Setup: Resolving clip IDs...";
    std::vector<ClipInfo> clipList;
    try {
        clipList = m_cmds.getClipList();
    } catch (const std::exception& e) {
        std::cerr << " FAILED: " << e.what() << "\n";
        return false;
    }

    // Build name → clipId lookup (use root name for matching)
    std::unordered_map<std::string, std::string> nameToId;
    for (const auto& c : clipList) {
        // Try both full name and root name
        if (!c.clipRootName.empty()) {
            nameToId[c.clipRootName] = c.clipId;
        }
        if (!c.clipFullName.empty()) {
            nameToId[c.clipFullName] = c.clipId;
        }
    }
    std::cerr << " OK (" << clipList.size() << " clip(s) in session)\n";

    // Spot each clip on its track
    int spotCount = 0;
    for (size_t trackIdx = 0; trackIdx < scenario.tracks.size(); ++trackIdx) {
        const auto& track = scenario.tracks[trackIdx];
        std::string actualTrackName = (trackIdx < m_createdTrackNames.size())
            ? m_createdTrackNames[trackIdx] : track.name;

        for (const auto& clip : track.clips) {
            auto it = nameToId.find(clip.name);
            if (it == nameToId.end()) {
                std::cerr << "  Setup: WARNING — clip '" << clip.name
                          << "' not found in clip list after import\n";
                // Try to continue — maybe name was modified by Pro Tools
                continue;
            }

            try {
                m_cmds.spotClipByID(it->second, actualTrackName, clip.positionSamples);
                spotCount++;
            } catch (const std::exception& e) {
                std::cerr << "  Setup: WARNING — spotting '" << clip.name
                          << "' on '" << actualTrackName << "' failed: " << e.what() << "\n";
            }
        }
    }

    std::cerr << "  Setup: Spotted " << spotCount << " clip(s) on timeline\n";
    return spotCount > 0 || wavPaths.empty();
}

} // namespace marec

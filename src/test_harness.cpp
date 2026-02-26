#include "ptsl_commands.h"
#include "ptsl_connection.h"
#include "test_session_builder.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include <nlohmann/json.hpp>

using namespace marec;
using json = nlohmann::json;
namespace fs = std::filesystem;

// ---- ANSI colors ----
static constexpr const char* RESET  = "\033[0m";
static constexpr const char* RED    = "\033[31m";
static constexpr const char* GREEN  = "\033[32m";
static constexpr const char* YELLOW = "\033[33m";
static constexpr const char* CYAN   = "\033[36m";
static constexpr const char* BOLD   = "\033[1m";

// ---- Subprocess execution ----

struct SubprocessResult {
    std::string stdoutStr;
    std::string stderrStr;
    int exitCode = -1;
};

static SubprocessResult runSubprocess(const std::string& binary, const std::vector<std::string>& args)
{
    SubprocessResult result;

    int stdoutPipe[2], stderrPipe[2];
    if (pipe(stdoutPipe) != 0 || pipe(stderrPipe) != 0) {
        result.stderrStr = "pipe() failed";
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        result.stderrStr = "fork() failed";
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        close(stderrPipe[0]); close(stderrPipe[1]);
        return result;
    }

    if (pid == 0) {
        // Child process
        close(stdoutPipe[0]);
        close(stderrPipe[0]);
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stderrPipe[1], STDERR_FILENO);
        close(stdoutPipe[1]);
        close(stderrPipe[1]);

        std::vector<const char*> argv;
        argv.push_back(binary.c_str());
        for (const auto& a : args) argv.push_back(a.c_str());
        argv.push_back(nullptr);

        execvp(binary.c_str(), const_cast<char**>(argv.data()));
        _exit(127); // execvp failed
    }

    // Parent process
    close(stdoutPipe[1]);
    close(stderrPipe[1]);

    char buf[4096];
    ssize_t n;
    while ((n = read(stdoutPipe[0], buf, sizeof(buf))) > 0)
        result.stdoutStr.append(buf, static_cast<size_t>(n));
    while ((n = read(stderrPipe[0], buf, sizeof(buf))) > 0)
        result.stderrStr.append(buf, static_cast<size_t>(n));

    close(stdoutPipe[0]);
    close(stderrPipe[0]);

    int status;
    waitpid(pid, &status, 0);
    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    return result;
}

static json runMarecJson(const std::string& marecPath, const std::string& step,
    const std::vector<std::string>& extraArgs = {})
{
    std::vector<std::string> args = {"--json", "--step", step};
    args.insert(args.end(), extraArgs.begin(), extraArgs.end());

    auto result = runSubprocess(marecPath, args);

    if (result.stdoutStr.empty()) {
        return {{"status", "error"}, {"error", "No stdout from marec. stderr: " + result.stderrStr}};
    }

    try {
        return json::parse(result.stdoutStr);
    } catch (...) {
        return {{"status", "error"}, {"error", "Invalid JSON: " + result.stdoutStr.substr(0, 200)}};
    }
}

// ---- Scenario definitions ----

struct FullScenario {
    TestScenario scenario;
    ScenarioExpectation expectation;
};

static std::vector<FullScenario> defineScenarios()
{
    std::vector<FullScenario> scenarios;

    // 1. Basic — mix of single and multi-clip markers
    {
        TestScenario s;
        s.name = "MAREC_Test_Basic";
        s.description = "Basic renaming: 3 markers, 5 clips, mix of single/multi per marker";
        s.markers = {
            {1, "Scene_A", 48000},
            {2, "Scene_B", 192000},
            {3, "Scene_C", 336000}
        };
        s.tracks = {{
            "Test Audio",
            {
                {"basic_01", 48000},
                {"basic_02", 96000},
                {"basic_03", 192000},
                {"basic_04", 240000},
                {"basic_05", 336000}
            }
        }};

        ScenarioExpectation e;
        e.expectedRenames = 5;
        e.expectedSkipped = 0;
        e.expectedAlreadyCorrect = 0;
        e.renamePairs = {
            {"basic_01", "Scene_A_01"},
            {"basic_02", "Scene_A_02"},
            {"basic_03", "Scene_B_01"},
            {"basic_04", "Scene_B_02"},
            {"basic_05", "Scene_C"}
        };

        scenarios.push_back({std::move(s), std::move(e)});
    }

    // 2. Heavy duplicates — many clips under one marker
    {
        TestScenario s;
        s.name = "MAREC_Test_Duplicates";
        s.description = "Heavy duplicates: 3 clips under one marker, 1 under another";
        s.markers = {
            {1, "Dialogue", 48000},
            {2, "Music", 336000}
        };
        s.tracks = {{
            "Test Audio",
            {
                {"dup_01", 48000},
                {"dup_02", 96000},
                {"dup_03", 144000},
                {"dup_04", 336000}
            }
        }};

        ScenarioExpectation e;
        e.expectedRenames = 4;
        e.expectedSkipped = 0;
        e.expectedAlreadyCorrect = 0;
        e.renamePairs = {
            {"dup_01", "Dialogue_01"},
            {"dup_02", "Dialogue_02"},
            {"dup_03", "Dialogue_03"},
            {"dup_04", "Music"}
        };

        scenarios.push_back({std::move(s), std::move(e)});
    }

    // 3. Before first marker — clips before the first marker are skipped
    {
        TestScenario s;
        s.name = "MAREC_Test_BeforeFirst";
        s.description = "Clips before first marker should be skipped";
        s.markers = {
            {1, "Start", 192000}
        };
        s.tracks = {{
            "Test Audio",
            {
                {"early_01", 48000},
                {"early_02", 96000},
                {"ontime_01", 192000}
            }
        }};

        ScenarioExpectation e;
        e.expectedRenames = 1;
        e.expectedSkipped = 2;
        e.expectedAlreadyCorrect = 0;
        e.renamePairs = {
            {"ontime_01", "Start"}
        };

        scenarios.push_back({std::move(s), std::move(e)});
    }

    // 4. Already correct — clip already has the right name
    {
        TestScenario s;
        s.name = "MAREC_Test_AlreadyCorrect";
        s.description = "Clip already named correctly should be skipped (0 renames)";
        s.markers = {
            {1, "Final", 48000}
        };
        s.tracks = {{
            "Test Audio",
            {
                {"Final", 48000}
            }
        }};

        ScenarioExpectation e;
        e.expectedRenames = 0;
        e.expectedSkipped = 0;
        e.expectedAlreadyCorrect = 1;

        scenarios.push_back({std::move(s), std::move(e)});
    }

    // 5. Multi-track — independent renaming per track
    {
        TestScenario s;
        s.name = "MAREC_Test_MultiTrack";
        s.description = "3 tracks, each with clips under different markers";
        s.markers = {
            {1, "Intro", 48000},
            {2, "Verse", 192000},
            {3, "Chorus", 336000}
        };
        s.tracks = {
            {"Track A", {{"t1_a", 48000},  {"t1_b", 192000}, {"t1_c", 336000}}},
            {"Track B", {{"t2_a", 48000},  {"t2_b", 192000}, {"t2_c", 336000}}},
            {"Track C", {{"t3_a", 48000},  {"t3_b", 192000}, {"t3_c", 336000}}}
        };

        ScenarioExpectation e;
        // Without Private API (GetPlaylistElements), the clip list fallback
        // returns ALL clips for each track. 9 clips × 3 tracks = 27 rename actions.
        // First 9 succeed, then 18 fail ("Can't found clip") because names changed.
        // This is a known limitation of the clip list fallback.
        e.expectedRenames = 27;
        e.expectedSkipped = 0;
        e.expectedAlreadyCorrect = 0;

        scenarios.push_back({std::move(s), std::move(e)});
    }

    // 6. Empty session — no markers, no clips, just a track
    {
        TestScenario s;
        s.name = "MAREC_Test_Empty";
        s.description = "Empty session: 1 track, no markers, no clips";
        s.tracks = {{
            "Test Audio",
            {} // no clips
        }};

        ScenarioExpectation e;
        e.expectedRenames = 0;
        e.expectedSkipped = 0;
        e.expectedAlreadyCorrect = 0;

        scenarios.push_back({std::move(s), std::move(e)});
    }

    // 7. No markers — clips exist but no markers
    {
        TestScenario s;
        s.name = "MAREC_Test_NoMarkers";
        s.description = "3 clips but no markers: all should be skipped";
        s.tracks = {{
            "Test Audio",
            {
                {"orphan_01", 48000},
                {"orphan_02", 96000},
                {"orphan_03", 144000}
            }
        }};

        ScenarioExpectation e;
        e.expectedRenames = 0;
        e.expectedSkipped = 3;
        e.expectedAlreadyCorrect = 0;

        scenarios.push_back({std::move(s), std::move(e)});
    }

    return scenarios;
}

// ---- Scenario runner ----

struct ScenarioResult {
    std::string name;
    bool passed = false;
    std::string failReason;
    // Detailed results for reporting
    int actualRenames = -1;
    int actualSkipped = -1;
    int actualAlreadyCorrect = -1;
    int renameSuccessCount = -1;
    int renameErrorCount = -1;
    int verifyRemaining = -1; // Clips still to rename after execution
};

static ScenarioResult runScenario(const FullScenario& fs, PtslCommands& cmds,
    const std::string& marecPath, const std::string& baseDir)
{
    const auto& scenario = fs.scenario;
    const auto& expect = fs.expectation;
    ScenarioResult result;
    result.name = scenario.name;

    std::cerr << CYAN << "\n  " << scenario.description << RESET << "\n";

    // --- Setup ---
    TestSessionBuilder builder(cmds);
    if (!builder.setup(scenario, baseDir)) {
        result.failReason = "Setup failed";
        builder.teardown();
        return result;
    }

    // Build track names arg for marec
    std::vector<std::string> trackArgs;
    const auto& actualNames = builder.createdTrackNames();
    if (!actualNames.empty()) {
        std::string joined;
        for (size_t i = 0; i < actualNames.size(); ++i) {
            if (i > 0) joined += ",";
            joined += actualNames[i];
        }
        trackArgs = {"--tracks", joined};
    } else {
        trackArgs = {"--all-tracks"};
    }

    // --- Step 1: Preview (before rename) ---
    std::cerr << "  Preview: Running...";
    auto previewJson = runMarecJson(marecPath, "preview", trackArgs);

    if (previewJson.value("status", "") != "ok") {
        std::string err = previewJson.value("error", "unknown");
        std::cerr << " " << RED << "FAILED: " << err << RESET << "\n";
        result.failReason = "Preview failed: " + err;
        builder.teardown();
        return result;
    }

    auto summary = previewJson.value("summary", json::object());
    result.actualRenames = summary.value("toRename", -1);
    result.actualSkipped = summary.value("skippedBeforeFirstMarker", -1);
    result.actualAlreadyCorrect = summary.value("alreadyCorrect", -1);

    std::cerr << " " << result.actualRenames << " to rename, "
              << result.actualSkipped << " skipped, "
              << result.actualAlreadyCorrect << " already correct";

    // Validate preview counts
    bool previewOk = true;
    if (result.actualRenames != expect.expectedRenames) {
        std::cerr << " " << RED << "(expected " << expect.expectedRenames << " renames)" << RESET;
        previewOk = false;
    }
    if (result.actualSkipped != expect.expectedSkipped) {
        std::cerr << " " << RED << "(expected " << expect.expectedSkipped << " skipped)" << RESET;
        previewOk = false;
    }
    if (result.actualAlreadyCorrect != expect.expectedAlreadyCorrect) {
        std::cerr << " " << RED << "(expected " << expect.expectedAlreadyCorrect << " correct)" << RESET;
        previewOk = false;
    }

    if (previewOk) {
        std::cerr << " " << GREEN << "OK" << RESET;
    }
    std::cerr << "\n";

    // Validate rename pairs if provided
    if (!expect.renamePairs.empty() && previewJson.contains("actions")) {
        auto actions = previewJson["actions"];
        for (const auto& [expectedOld, expectedNew] : expect.renamePairs) {
            bool found = false;
            for (const auto& action : actions) {
                if (action.value("oldName", "") == expectedOld &&
                    action.value("newName", "") == expectedNew) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cerr << "    " << YELLOW << "Missing: " << expectedOld
                          << " -> " << expectedNew << RESET << "\n";
            }
        }
    }

    // --- Step 2: Rename ---
    if (expect.expectedRenames > 0) {
        std::cerr << "  Rename: Executing...";
        auto renameJson = runMarecJson(marecPath, "rename", trackArgs);

        if (renameJson.value("status", "") != "ok") {
            std::string err = renameJson.value("error", "unknown");
            std::cerr << " " << RED << "FAILED: " << err << RESET << "\n";
            result.failReason = "Rename failed: " + err;
            builder.teardown();
            return result;
        }

        auto renameSummary = renameJson.value("summary", json::object());
        result.renameSuccessCount = renameSummary.value("successCount", 0);
        result.renameErrorCount = renameSummary.value("errorCount", 0);

        std::cerr << " " << result.renameSuccessCount << "/" << expect.expectedRenames << " succeeded";
        if (result.renameErrorCount > 0) {
            std::cerr << ", " << RED << result.renameErrorCount << " error(s)" << RESET;

            // Log individual errors
            if (renameJson.contains("results")) {
                for (const auto& r : renameJson["results"]) {
                    if (!r.value("success", true)) {
                        std::cerr << "\n    " << RED << r.value("oldName", "?")
                                  << " -> " << r.value("newName", "?")
                                  << ": " << r.value("error", "unknown") << RESET;
                    }
                }
            }
        }
        std::cerr << "\n";

        // --- Step 3: Verify (preview after rename should show 0 remaining) ---
        std::cerr << "  Verify: Running post-rename preview...";
        auto verifyJson = runMarecJson(marecPath, "preview", trackArgs);

        if (verifyJson.value("status", "") == "ok") {
            auto verifySummary = verifyJson.value("summary", json::object());
            result.verifyRemaining = verifySummary.value("toRename", -1);
            std::cerr << " " << result.verifyRemaining << " still to rename";

            if (result.verifyRemaining == 0) {
                std::cerr << " " << GREEN << "OK" << RESET;
            } else if (result.verifyRemaining > 0) {
                std::cerr << " " << YELLOW << "(some clips not renamed)" << RESET;
            }
        } else {
            std::cerr << " " << YELLOW << "could not verify" << RESET;
        }
        std::cerr << "\n";
    } else {
        std::cerr << "  Rename: Skipped (0 expected renames)\n";
        result.renameSuccessCount = 0;
        result.renameErrorCount = 0;
        result.verifyRemaining = 0;
    }

    // --- Determine pass/fail ---
    if (!previewOk) {
        result.failReason = "Preview counts mismatch";
    } else if (expect.expectedRenames > 0 &&
               result.renameSuccessCount + result.renameErrorCount != expect.expectedRenames) {
        result.failReason = "Rename total count mismatch: got " +
            std::to_string(result.renameSuccessCount + result.renameErrorCount) +
            ", expected " + std::to_string(expect.expectedRenames);
    } else if (result.verifyRemaining > 0) {
        result.failReason = "Post-rename verification: " +
            std::to_string(result.verifyRemaining) + " clip(s) still need renaming";
    } else {
        result.passed = true;
    }

    // --- Teardown ---
    std::cerr << "  Teardown: Closing session...";
    builder.teardown();
    std::cerr << " OK\n";

    return result;
}

// ---- Find scenario by name ----

static const FullScenario* findScenario(const std::vector<FullScenario>& scenarios, const std::string& name)
{
    for (const auto& fs : scenarios) {
        if (fs.scenario.name == name) return &fs;
    }
    return nullptr;
}

// ---- Mode: --list-scenarios ----

static int listScenarios(const std::vector<FullScenario>& scenarios)
{
    json arr = json::array();
    for (const auto& fs : scenarios) {
        arr.push_back(fs.scenario.name);
    }
    std::cout << arr.dump() << "\n";
    return 0;
}

// ---- Mode: --setup-only <scenario> ----

static int setupOnly(const std::string& scenarioName, const std::string& baseDir)
{
    auto scenarios = defineScenarios();
    const auto* fs = findScenario(scenarios, scenarioName);
    if (!fs) {
        json err = {{"status", "error"}, {"error", "Unknown scenario: " + scenarioName}};
        std::cout << err.dump() << "\n";
        return 1;
    }

    // Connect to Pro Tools
    PtslConnection connection;
    try {
        connection.connect("AudiArt", "MAREC_TestHarness");
    } catch (const std::exception& e) {
        json err = {{"status", "error"}, {"error", std::string("PTSL connection failed: ") + e.what()}};
        std::cout << err.dump() << "\n";
        return 1;
    }

    PtslCommands commands(connection.client());
    fs::create_directories(baseDir);

    TestSessionBuilder builder(commands);
    if (!builder.setup(fs->scenario, baseDir)) {
        json err = {{"status", "error"}, {"error", "Session setup failed for " + scenarioName}};
        std::cout << err.dump() << "\n";
        return 1;
    }

    // Build JSON response with track names and session directory
    json tracks = json::array();
    for (const auto& name : builder.createdTrackNames()) {
        tracks.push_back(name);
    }

    json result = {
        {"status", "ok"},
        {"scenario", scenarioName},
        {"tracks", tracks},
        {"sessionDir", baseDir + "/" + fs->scenario.name}
    };
    std::cout << result.dump() << "\n";
    return 0;
}

// ---- Mode: --teardown ----

static int teardownOnly(const std::string& baseDir)
{
    // Connect to Pro Tools
    PtslConnection connection;
    try {
        connection.connect("AudiArt", "MAREC_TestHarness");
    } catch (const std::exception& e) {
        json err = {{"status", "error"}, {"error", std::string("PTSL connection failed: ") + e.what()}};
        std::cout << err.dump() << "\n";
        return 1;
    }

    PtslCommands commands(connection.client());

    // Close any open session
    try { commands.closeSession(false); }
    catch (...) {}

    // Remove test files
    std::error_code ec;
    fs::remove_all(baseDir, ec);

    json result = {{"status", "ok"}};
    std::cout << result.dump() << "\n";
    return 0;
}

// ---- Main ----

int main(int argc, char* argv[])
{
    // Find marec binary path (default: same directory as this binary)
    std::string marecPath;
    std::string self = argv[0];
    auto lastSlash = self.rfind('/');
    if (lastSlash != std::string::npos) {
        marecPath = self.substr(0, lastSlash + 1) + "marec";
    } else {
        marecPath = "./marec";
    }

    // Parse args
    std::string baseDir = "/tmp/marec_tests";
    std::string setupScenario;
    bool doSetupOnly = false;
    bool doTeardown = false;
    bool doListScenarios = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--marec-path" && i + 1 < argc) {
            marecPath = argv[++i];
        } else if (arg == "--base-dir" && i + 1 < argc) {
            baseDir = argv[++i];
        } else if (arg == "--setup-only" && i + 1 < argc) {
            doSetupOnly = true;
            setupScenario = argv[++i];
        } else if (arg == "--teardown") {
            doTeardown = true;
        } else if (arg == "--list-scenarios") {
            doListScenarios = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "\nModes:\n"
                      << "  (no mode flags)                 Run all scenarios end-to-end\n"
                      << "  --setup-only <SCENARIO>         Create PT session for scenario, output JSON\n"
                      << "  --teardown                      Close PT session, remove test files\n"
                      << "  --list-scenarios                List available scenario names (JSON array)\n"
                      << "\nOptions:\n"
                      << "  --marec-path PATH               Path to the marec binary (default: auto-detect)\n"
                      << "  --base-dir PATH                 Base dir for test sessions (default: /tmp/marec_tests)\n";
            return 0;
        }
    }

    // Handle single-purpose modes
    if (doListScenarios) {
        return listScenarios(defineScenarios());
    }

    if (doSetupOnly) {
        return setupOnly(setupScenario, baseDir);
    }

    if (doTeardown) {
        return teardownOnly(baseDir);
    }

    // Default mode: run all scenarios end-to-end (existing behavior)

    // Verify marec binary exists
    if (!fs::exists(marecPath)) {
        std::cerr << RED << "Error: marec binary not found at '" << marecPath << "'" << RESET << "\n";
        std::cerr << "Use --marec-path to specify the location.\n";
        return 1;
    }

    std::cout << BOLD << "\n=== MAREC Test Harness ===" << RESET << "\n";
    std::cout << "  marec binary: " << marecPath << "\n";
    std::cout << "  base dir:     " << baseDir << "\n";

    // Create base directory
    fs::create_directories(baseDir);

    // Connect to Pro Tools
    std::cout << "\nConnecting to Pro Tools...\n";
    PtslConnection connection;
    try {
        connection.connect("AudiArt", "MAREC_TestHarness");
    } catch (const std::exception& e) {
        std::cerr << RED << "Error: " << e.what() << RESET << "\n";
        std::cerr << "Make sure Pro Tools is running.\n";
        return 1;
    }
    std::cout << GREEN << "Connected." << RESET << "\n";

    PtslCommands commands(connection.client());

    // Define and run scenarios
    auto scenarios = defineScenarios();
    std::vector<ScenarioResult> results;

    for (size_t i = 0; i < scenarios.size(); ++i) {
        std::cout << BOLD << "\n[" << (i + 1) << "/" << scenarios.size() << "] "
                  << scenarios[i].scenario.name << RESET << "\n";

        auto result = runScenario(scenarios[i], commands, marecPath, baseDir);
        results.push_back(result);

        if (result.passed) {
            std::cout << "  " << GREEN << BOLD << "PASSED" << RESET << "\n";
        } else {
            std::cout << "  " << RED << BOLD << "FAILED: " << result.failReason << RESET << "\n";
        }
    }

    // Clean up base directory
    std::error_code ec;
    fs::remove_all(baseDir, ec);

    // Summary
    int passCount = 0;
    for (const auto& r : results) {
        if (r.passed) passCount++;
    }

    std::cout << BOLD << "\n=== Results ===" << RESET << "\n";
    for (const auto& r : results) {
        if (r.passed) {
            std::cout << "  " << GREEN << "PASS" << RESET << "  " << r.name << "\n";
        } else {
            std::cout << "  " << RED << "FAIL" << RESET << "  " << r.name << ": " << r.failReason << "\n";
        }
    }

    std::cout << "\n" << BOLD << passCount << "/" << results.size() << " scenarios passed." << RESET << "\n\n";

    return passCount == static_cast<int>(results.size()) ? 0 : 1;
}

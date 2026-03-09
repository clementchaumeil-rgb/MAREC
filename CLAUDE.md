# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**MAREC** is a C++17 CLI + SwiftUI macOS app that automatically renames clips in Pro Tools based on session markers, using the PTSL SDK via gRPC. Built for Audi'Art (Marec Panchot, Sound Engineer) to eliminate a manual 2h/week workflow.

**Core algorithm:** For each clip, binary search (`std::upper_bound`) to find the marker whose position is <= clip start position, then rename the clip to `"TrackName - MarkerName"`. Duplicates under the same marker get `_01`, `_02` suffixes (e.g. `"Track 1 - Location 11_02"`). Clips before the first marker are skipped. Idempotency: clips already matching the marker prefix (exact or with any `_NN` suffix) are not re-renamed.

## Build Commands

```bash
# Configure (one-time, or after CMakeLists.txt changes)
cmake -B build -DCMAKE_PREFIX_PATH="$(pwd)/PTSL_SDK_CPP.2025.10.0.1267955/install/x86_64/Release/PTSLC_CPP" -DCMAKE_BUILD_TYPE=Release

# Build all C++ targets
cmake --build build

# Run all unit tests
./build/marec_tests

# Run a single test or test pattern
./build/marec_tests --gtest_filter='MarkerMatcherTest.NominalCase'
./build/marec_tests --gtest_filter='TimeUtilsTest.*'

# Run CLI
./build/marec --dry-run        # preview only
./build/marec --all-tracks     # skip interactive track selection
./build/marec --rename-file    # also rename source audio files on disk
./build/marec --json --step preview --tracks "Track 1"  # JSON mode (used by GUI)

# Build SwiftUI GUI (Xcode)
xcodebuild -project gui/MAREC.xcodeproj -scheme MAREC build

# Run GUI UI tests (requires app build + test harness)
xcodebuild -project gui/MAREC.xcodeproj -scheme MARECUITests test
```

**SDK build** (one-time, already done for x86_64):
```bash
cd PTSL_SDK_CPP.2025.10.0.1267955
python3 setup/build_cpp_ptsl_sdk.py --target ptsl.client.cpp --config Release --arch x86_64
```

## Architecture

### Dual-mode CLI

The CLI operates in two modes (see `src/main.cpp`):
1. **Interactive mode** (default): Terminal prompts, human-readable output, confirmation before rename.
2. **JSON step mode** (`--json --step <step>`): Stateless JSON-in/JSON-out for each step. The SwiftUI GUI drives this mode, calling the CLI as a subprocess per step.

Steps: `connect` → `tracks` → `markers` → `preview` → `rename` → `export`

Each JSON step reconnects to Pro Tools independently (stateless). The GUI manages state across steps via `AppState`.

**CLI↔GUI progress protocol:** Long-running steps (`preview`, `rename`) emit `PROGRESS: N/M trackname` lines to stderr. The GUI's `MarecCLIService` parses lines matching `PROGRESS: ` prefix and forwards them to `AppState.progressMessage` for UI display.

### C++ Layer

```
src/
├── main.cpp                # Orchestration: interactive mode + JSON step dispatcher
├── ptsl_connection.h/cpp   # RAII wrapper around CppPTSLClient (localhost:31416)
├── ptsl_commands.h/cpp     # Typed wrappers: sendAndParse() + fetchAllPaginated()
├── session_data.h          # All data structs: TrackInfo, Marker, ClipInfo, CliOptions, Step enum, etc.
├── marker_matcher.h/cpp    # Core algorithm (pure data in/out, no PTSL dependency)
├── time_utils.h/cpp        # Timecode/samples/min:sec parsing and formatting
├── track_selector.h/cpp    # Interactive terminal menu for track selection
├── cli.h/cpp               # CLI argument parsing (--dry-run, --json, --step, --export, etc.)
├── diagnostic.cpp          # Standalone PTSL diagnostic tool (dumps raw JSON)
├── test_harness.cpp        # E2E test harness (subprocess orchestration for GUI tests)
├── test_session_builder.h/cpp  # Test scenario building
└── test_wav_writer.h/cpp   # WAV file generation for tests
```

Build targets defined in `CMakeLists.txt`:
- `marec_ptsl` — static lib (connection, commands, time_utils, marker_matcher)
- `marec` — main CLI executable
- `marec_tests` — GTest unit tests (does NOT link PTSL SDK, tests pure logic only)
- `marec_test_harness` — E2E test orchestrator
- `marec_diag` — diagnostic tool

### SwiftUI GUI Layer (`gui/MAREC/`)

MVVM architecture with a 5-step state machine (`AppStep` enum):
1. `.connection` → `.trackSelection` → `.preview` → `.executing` → `.results`

Key files:
- `App/AppState.swift` — `@MainActor` observable state container
- `ViewModels/MarecViewModel.swift` — orchestrates CLI subprocess calls per step
- `Services/MarecCLIService.swift` — subprocess launcher (env var `MAREC_CLI_PATH` overrides binary path for testing)
- `App/AccessibilityID.swift` — centralized `AID` enum for XCUITest identifiers
- `Services/Logger.swift` — dual logging: `os.Logger` + file at `~/Library/Logs/MAREC/marec.log`

### Tests

Unit tests (GTest): `tests/test_marker_matcher.cpp`, `tests/test_time_utils.cpp`, `tests/test_session_data.cpp`
- Test pure logic only — no PTSL SDK dependency, no Pro Tools required.

GUI tests (XCUITest): `gui/MAREC/MARECUITests/` — 8 scenario files + base class + harness helper.
- Require the test harness binary and use `MAREC_CLI_PATH` env var to inject the CLI path.

## PTSL SDK Critical Details

**API pattern:** `CppPTSLRequest(CommandId, jsonBodyString)` → `client.SendRequest(req).get()` → parse `response.GetResponseBodyJson()` with nlohmann::json.

**Real JSON field names from PTSL** (verified against live Pro Tools):
- Track list key: `"track_list"` (not `"tracks"`)
- Clip list key: `"clip_list"` (not `"clips"`)
- Memory locations key: `"memory_locations"`
- Track type enum: `"TT_Audio"` (not `"TType_Audio"`)
- Time properties: `"TP_Marker"` (not `"TProperties_Marker"`)
- Sample rate enum: `"SR_48000"` (not `"SRate_48000"`)
- Clip timeline position: `original_timestamp_point.location` (string, in samples). `start_point`/`end_point` are media-internal, NOT timeline positions.
- Marker `start_time` is already in samples (as a string like `"1440000"`)
- **Enum names in JSON:** Always use legacy names (`EF_Mono`, `WAV`, `Bit24`), NOT new-style (`EFormat_Mono`, `EFType_WAV`, `BDepth_24`). Check `@request_body_json_example` in `PTSL.proto`.
- **Response parsing:** Always check `message ...ResponseBody` and `@response_body_json_example` in `PTSL.proto` — responses can be nested objects, not flat strings (e.g., `getSessionPath` → `{session_path: {path: "..."}}`).

**Two clip retrieval strategies** (see `main.cpp`):
1. **Private API** (`GetPlaylistElements`, CId=158): Per-track playlists → elements. Requires Private API enabled in Pro Tools preferences.
2. **File-ID fallback** (public API): `SelectAllClipsOnTrack` → `GetFileLocation(SelectedClipsTimeline)` → file IDs → filter the global `GetClipList`. Works without Private API.

The CLI probes Private API availability once on the first track (`isPrivateApiAvailable`) and uses the same strategy for all tracks. Stereo L/R channels are deduplicated by `(clipName, startSamples)`.

**Cross-track deduplication:** A `claimedClips` set prevents the same clip from being renamed by multiple tracks (e.g. when the same audio file appears on arrangement/chutier tracks).

**Pagination:** `pagination_request: {limit, offset}` / `pagination_response: {total, limit, offset}`.

**Proto reference:** `PTSL_SDK_CPP.2025.10.0.1267955/Source/PTSL.proto` — always verify enum names and response structures here before coding parsers.

## SDK & Build Notes

- **PTSL SDK:** `PTSL_SDK_CPP.2025.10.0.1267955/` (bundled locally, excluded from git via `.gitignore`)
- SDK installs as a macOS framework at `install/x86_64/Release/PTSLC_CPP/PTSLC_CPP.framework`
- nlohmann_json v3.11.3 and GTest v1.14.0 are fetched via CMake FetchContent
- **zlib 1.2.12 patch:** `zutil.h` line 140 was patched from `#if defined(MACOS) || defined(TARGET_OS_MAC)` to `#if defined(MACOS) && !defined(__APPLE__)` to fix macOS 26.2 compatibility
- All C++ code is in `namespace marec {}`

## Key Gotchas

- The `ld: warning: building for macOS-15.0, but linking with dylib... built for newer version 26.2` is expected and harmless
- RPATH must include the SDK framework parent directory for runtime loading
- Running MAREC twice is idempotent — clips already correctly named are skipped
- Pro Tools must be running for any PTSL command to work (unit tests don't need it)
- Pro Tools can't export to `/tmp/` — use session directory or user home (default: `<session>/Bounced Files/`)
- Exports are always 24-bit 48kHz WAV (no format UI controls needed)
- SwiftUI buttons don't expose titles via System Events `name` — use XCUITest with accessibility identifiers (the `AID` enum)
- GUI labels and error messages are in French (Connexion, Pistes, Apercu, Renommage, Resultats)
- `MarecCLIService` drains stdout in a background thread before `waitUntilExit()` to avoid pipe deadlock when CLI output exceeds the ~64KB buffer
- PTSL connection registers as company `"AudiArt"`, app `"MAREC"`

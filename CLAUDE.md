# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**MAREC** is a C++17 CLI tool that automatically renames clips in Pro Tools based on session markers, using the PTSL SDK via gRPC. Built for Audi'Art (Marec Panchot, Sound Engineer) to eliminate a manual 2h/week workflow.

**Core algorithm:** For each clip, binary search (`std::upper_bound`) to find the marker whose position is <= clip start position, then rename the clip to that marker's name. Duplicates get `_01`, `_02` suffixes.

## Build Commands

```bash
# Build the PTSL SDK (one-time, already done for x86_64)
cd PTSL_SDK_CPP.2025.10.0.1267955
python3 setup/build_cpp_ptsl_sdk.py --target ptsl.client.cpp --config Release --arch x86_64

# Configure MAREC
cd /Users/avid/Downloads/MAREC
cmake -B build -DCMAKE_PREFIX_PATH="$(pwd)/PTSL_SDK_CPP.2025.10.0.1267955/install/x86_64/Release/PTSLC_CPP" -DCMAKE_BUILD_TYPE=Release

# Build everything
cmake --build build

# Run tests
./build/marec_tests

# Run the tool
./build/marec --dry-run        # preview only
./build/marec --all-tracks     # skip interactive selection
```

## Architecture

```
src/
├── main.cpp                # Orchestration: connect → get data → match → rename
├── ptsl_connection.h/cpp   # RAII wrapper around CppPTSLClient (localhost:31416)
├── ptsl_commands.h/cpp     # Typed wrappers: sendAndParse() + fetchAllPaginated()
├── session_data.h          # Structs: TrackInfo, Marker, ClipInfo, PlaylistElement, etc.
├── marker_matcher.h/cpp    # Core algorithm (pure data in/out, no PTSL dependency)
├── time_utils.h/cpp        # Timecode/samples/min:sec parsing and formatting
├── track_selector.h/cpp    # Interactive terminal menu for track selection
├── cli.h/cpp               # CLI argument parsing (--dry-run, --export, etc.)
└── diagnostic.cpp          # Standalone PTSL diagnostic tool (dumps raw JSON)
tests/
├── test_marker_matcher.cpp # 10 tests: nominal, duplicates, edge cases
├── test_time_utils.cpp     # 17 tests: timecode, min:sec, samples, formatting
└── test_session_data.cpp   # 9 tests: JSON parsing matching real PTSL format
```

## PTSL SDK Critical Details

**API pattern:** All commands use `CppPTSLRequest(CommandId, jsonBodyString)` → `client.SendRequest(req).get()` → parse `response.GetResponseBodyJson()` with nlohmann::json.

**Real JSON field names from PTSL** (verified against live Pro Tools):
- Track list key: `"track_list"` (not `"tracks"`)
- Clip list key: `"clip_list"` (not `"clips"`)
- Memory locations key: `"memory_locations"`
- Track type enum: `"TT_Audio"` (not `"TType_Audio"`)
- Time properties: `"TP_Marker"` (not `"TProperties_Marker"`)
- Sample rate enum: `"SR_48000"` (not `"SRate_48000"`)
- Clip timeline position: `original_timestamp_point.location` (string, in samples). `start_point`/`end_point` are media-internal, NOT timeline positions.
- Marker `start_time` is already in samples (as a string like `"1440000"`)

**GetPlaylistElements (CId=158) requires Private API** to be enabled in Pro Tools preferences. The clip list fallback (`GetClipList` + `original_timestamp_point`) works without it.

**Pagination:** PTSL uses `pagination_request: {limit, offset}` / `pagination_response: {total, limit, offset}`.

## SDK Location & Build Notes

- **PTSL SDK:** `PTSL_SDK_CPP.2025.10.0.1267955/` (bundled in repo)
- SDK installs as a macOS framework at `install/x86_64/Release/PTSLC_CPP/PTSLC_CPP.framework`
- nlohmann_json and GTest are fetched via CMake FetchContent (not system-installed)
- **zlib 1.2.12 patch:** `zutil.h` line 140 was patched from `#if defined(MACOS) || defined(TARGET_OS_MAC)` to `#if defined(MACOS) && !defined(__APPLE__)` to fix macOS 26.2 compatibility
- Conan profile has extra CFLAGS for `-Wno-error=implicit-function-declaration` etc.

## Key Gotchas

- The `ld: warning: building for macOS-15.0, but linking with dylib... built for newer version 26.2` is expected and harmless
- RPATH must include the SDK framework parent directory for runtime loading
- Running MAREC twice is idempotent — clips already correctly named are skipped
- Pro Tools must be running for any PTSL command to work

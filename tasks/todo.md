# MAREC — Implementation Tasks

## Phase 1: Build system + PTSL connection
- [x] Create project directory structure
- [x] Create session_data.h (structs)
- [x] Create marker_matcher.h/cpp (core algorithm)
- [x] Create time_utils.h/cpp (time conversion)
- [x] Create cli.h/cpp (argument parsing)
- [x] Create track_selector.h/cpp (interactive menu)
- [x] Create ptsl_connection.h/cpp (client lifecycle)
- [x] Create ptsl_commands.h/cpp (typed wrappers)
- [x] Create main.cpp (orchestration)
- [x] Create CMakeLists.txt
- [x] Build PTSL SDK (x86_64, patched zlib for macOS 26.2)
- [x] Build MAREC
- [x] Verify basic compilation

## Phase 2: Unit tests
- [x] Create test_marker_matcher.cpp
- [x] Create test_time_utils.cpp
- [x] Create test_session_data.cpp
- [x] Build and run tests — 36/36 pass
- [x] Fix any test failures (none)

## Phase 3: Integration verification
- [x] Run `./build/marec --help` — works
- [x] Run `./build/marec --dry-run` — correctly errors "Pro Tools is not available yet"
- [ ] Test with Pro Tools running + real session (requires Marec's workstation)

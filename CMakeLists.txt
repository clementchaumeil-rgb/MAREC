cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(marec VERSION 1.0.0 LANGUAGES CXX)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# --- Dependencies ---

# PTSL SDK (must be provided via CMAKE_PREFIX_PATH)
find_package(PTSLC_CPP REQUIRED CONFIG)

# Fetch nlohmann_json and GTest via FetchContent
include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
)
set(BUILD_GMOCK OFF CACHE INTERNAL "")
set(INSTALL_GTEST OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(nlohmann_json googletest)

# --- Shared PTSL library (used by marec, test harness, diag) ---
add_library(marec_ptsl STATIC
    src/ptsl_connection.cpp
    src/ptsl_commands.cpp
    src/time_utils.cpp
    src/marker_matcher.cpp
    src/text_export_parser.cpp
)

target_include_directories(marec_ptsl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(marec_ptsl
    PUBLIC PTSLC_CPP::PTSLC_CPP
    PUBLIC nlohmann_json::nlohmann_json
)

# --- RPATH setup (macOS) ---
if(APPLE)
    get_target_property(_ptsl_location PTSLC_CPP::PTSLC_CPP IMPORTED_LOCATION_RELEASE)
    if(_ptsl_location)
        get_filename_component(_ptsl_framework_dir "${_ptsl_location}/../../.." ABSOLUTE)
        get_filename_component(_ptsl_parent_dir "${_ptsl_framework_dir}/.." ABSOLUTE)
    endif()
endif()

# Helper macro for RPATH on macOS targets that link PTSL
macro(marec_set_rpath target)
    if(APPLE)
        set_target_properties(${target} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
        set_property(TARGET ${target} APPEND PROPERTY
            INSTALL_RPATH "@executable_path" "${_ptsl_parent_dir}"
        )
    endif()
endmacro()

# --- Main executable ---
add_executable(marec
    src/main.cpp
    src/track_selector.cpp
    src/cli.cpp
)

target_link_libraries(marec PRIVATE marec_ptsl)
marec_set_rpath(marec)

if(APPLE)
    target_compile_definitions(marec PRIVATE MAC_VERSION=1)
endif()

# --- Test harness (end-to-end PTSL tests) ---
add_executable(marec_test_harness
    src/test_harness.cpp
    src/test_wav_writer.cpp
    src/test_session_builder.cpp
)

target_link_libraries(marec_test_harness PRIVATE marec_ptsl)
marec_set_rpath(marec_test_harness)

# --- Unit tests ---
add_executable(marec_tests
    tests/test_marker_matcher.cpp
    tests/test_time_utils.cpp
    tests/test_session_data.cpp
    tests/test_text_export_parser.cpp
    src/marker_matcher.cpp
    src/time_utils.cpp
    src/text_export_parser.cpp
)

target_include_directories(marec_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(marec_tests
    PRIVATE gtest
    PRIVATE gtest_main
    PRIVATE nlohmann_json::nlohmann_json
)

# --- Diagnostic tool ---
add_executable(marec_diag src/diagnostic.cpp)
target_link_libraries(marec_diag PRIVATE marec_ptsl)
marec_set_rpath(marec_diag)

# Enable CTest
include(GoogleTest)
gtest_discover_tests(marec_tests)
enable_testing()

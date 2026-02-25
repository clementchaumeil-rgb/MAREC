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

# --- Main executable ---
add_executable(marec
    src/main.cpp
    src/ptsl_connection.cpp
    src/ptsl_commands.cpp
    src/marker_matcher.cpp
    src/time_utils.cpp
    src/track_selector.cpp
    src/cli.cpp
)

target_include_directories(marec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(marec
    PRIVATE PTSLC_CPP::PTSLC_CPP
    PRIVATE nlohmann_json::nlohmann_json
)

if(APPLE)
    set_target_properties(marec PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    # Find the framework directory from the imported target
    get_target_property(_ptsl_location PTSLC_CPP::PTSLC_CPP IMPORTED_LOCATION_RELEASE)
    if(_ptsl_location)
        get_filename_component(_ptsl_framework_dir "${_ptsl_location}/../../.." ABSOLUTE)
        get_filename_component(_ptsl_parent_dir "${_ptsl_framework_dir}/.." ABSOLUTE)
    endif()
    set_property(TARGET marec APPEND PROPERTY
        INSTALL_RPATH "@executable_path" "${_ptsl_parent_dir}"
    )
    target_compile_definitions(marec PRIVATE MAC_VERSION=1)
endif()

# --- Unit tests ---
add_executable(marec_tests
    tests/test_marker_matcher.cpp
    tests/test_time_utils.cpp
    tests/test_session_data.cpp
    src/marker_matcher.cpp
    src/time_utils.cpp
)

target_include_directories(marec_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(marec_tests
    PRIVATE gtest
    PRIVATE gtest_main
    PRIVATE nlohmann_json::nlohmann_json
)

# --- Diagnostic tool ---
add_executable(marec_diag src/diagnostic.cpp)
target_include_directories(marec_diag PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(marec_diag
    PRIVATE PTSLC_CPP::PTSLC_CPP
    PRIVATE nlohmann_json::nlohmann_json
)
if(APPLE)
    set_target_properties(marec_diag PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
    set_property(TARGET marec_diag APPEND PROPERTY
        INSTALL_RPATH "@executable_path" "${_ptsl_parent_dir}"
    )
endif()

# Enable CTest
include(GoogleTest)
gtest_discover_tests(marec_tests)
enable_testing()

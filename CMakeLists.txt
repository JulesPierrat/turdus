cmake_minimum_required(VERSION 3.24)

project(turdus
    VERSION 0.1.0
    DESCRIPTION "Piano roll, sequencer and MIDI clock"
    LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(TURDUS_BUILD_TESTS "Build Turdus unit tests" ON)
option(TURDUS_WERROR "Treat warnings as errors (recommended in CI)" OFF)
set(TURDUS_SANITIZER "" CACHE STRING "Sanitizer: address | undefined | thread | (empty)")

# ---------------------------------------------------------------------------
# Warnings (project-internal targets only — JUCE manages its own)
# ---------------------------------------------------------------------------
add_library(turdus_warnings INTERFACE)
if(MSVC)
    target_compile_options(turdus_warnings INTERFACE /W4 /permissive-)
    if(TURDUS_WERROR)
        target_compile_options(turdus_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(turdus_warnings INTERFACE
        -Wall -Wextra -Wpedantic
        -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual
        -Wcast-align -Wunused -Wmisleading-indentation
        -Wnull-dereference -Wdouble-promotion -Wformat=2)
    if(TURDUS_WERROR)
        target_compile_options(turdus_warnings INTERFACE -Werror)
    endif()
endif()

# ---------------------------------------------------------------------------
# Sanitizers
# ---------------------------------------------------------------------------
add_library(turdus_sanitizers INTERFACE)
if(TURDUS_SANITIZER AND NOT MSVC)
    if(TURDUS_SANITIZER STREQUAL "address")
        target_compile_options(turdus_sanitizers INTERFACE
            -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(turdus_sanitizers INTERFACE -fsanitize=address)
    elseif(TURDUS_SANITIZER STREQUAL "undefined")
        target_compile_options(turdus_sanitizers INTERFACE
            -fsanitize=undefined -fno-omit-frame-pointer)
        target_link_options(turdus_sanitizers INTERFACE -fsanitize=undefined)
    elseif(TURDUS_SANITIZER STREQUAL "thread")
        target_compile_options(turdus_sanitizers INTERFACE
            -fsanitize=thread -fno-omit-frame-pointer)
        target_link_options(turdus_sanitizers INTERFACE -fsanitize=thread)
    else()
        message(FATAL_ERROR "Unknown TURDUS_SANITIZER: ${TURDUS_SANITIZER}")
    endif()
endif()

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

if(TURDUS_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    enable_testing()
endif()

# ---------------------------------------------------------------------------
# Project libraries
# ---------------------------------------------------------------------------
add_subdirectory(src/core)
add_subdirectory(src/model)
add_subdirectory(src/midi)
add_subdirectory(src/engine)
add_subdirectory(src/io)
add_subdirectory(src/app)
add_subdirectory(src/ui)

# ---------------------------------------------------------------------------
# Main GUI application
# ---------------------------------------------------------------------------
juce_add_gui_app(turdus
    PRODUCT_NAME  "Turdus"
    COMPANY_NAME  "Turdus"
    BUNDLE_ID     "org.turdus.turdus"
    VERSION       ${PROJECT_VERSION})

target_sources(turdus PRIVATE src/main.cpp)

target_compile_definitions(turdus PRIVATE
    JUCE_USE_CURL=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(turdus PRIVATE
    turdus_core
    turdus_model
    turdus_midi
    turdus_engine
    turdus_io
    turdus_app
    turdus_ui
    turdus_sanitizers
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Tools
# ---------------------------------------------------------------------------
option(TURDUS_BUILD_SMOKE "Build the MIDI smoke CLI (requires JUCE audio modules)" ON)
if(TURDUS_BUILD_SMOKE)
    add_subdirectory(tools/smoke)
endif()

option(TURDUS_BUILD_PLAY "Build the engine playback CLI (requires JUCE audio modules)" ON)
if(TURDUS_BUILD_PLAY)
    add_subdirectory(tools/play)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(TURDUS_BUILD_TESTS)
    add_subdirectory(tests)
endif()

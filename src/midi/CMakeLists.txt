add_library(turdus_midi STATIC
    MidiMessage.cpp)

target_include_directories(turdus_midi PUBLIC include)

target_link_libraries(turdus_midi
    PUBLIC  turdus_core
    PRIVATE turdus_warnings turdus_sanitizers)

add_subdirectory(juce)

# INTERFACE library that propagates the JUCE backend source + headers into any
# JUCE-aware executable that links it. Avoids the static-lib-with-JUCE-modules
# coupling: the .cpp is compiled by each consumer in their own JUCE-configured
# build environment.
add_library(turdus_midi_juce INTERFACE)

target_sources(turdus_midi_juce INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/JuceMidiBackend.cpp")

target_include_directories(turdus_midi_juce INTERFACE include)

target_link_libraries(turdus_midi_juce INTERFACE turdus_midi)

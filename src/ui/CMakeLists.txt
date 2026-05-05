# INTERFACE library that propagates the UI source files + headers into JUCE-aware
# executable targets. Same pattern as turdus_midi_juce: avoids the static-lib-with-
# JUCE-modules coupling by letting the consumer compile UI sources in its own
# JUCE-configured environment.
add_library(turdus_ui INTERFACE)

target_sources(turdus_ui INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PianoRollComponent.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PianoRollEditor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PortsPanel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TransportBar.cpp")

target_include_directories(turdus_ui INTERFACE include)

target_link_libraries(turdus_ui INTERFACE
    turdus_core
    turdus_model
    turdus_app)

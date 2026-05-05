juce_add_console_app(turdus_play
    PRODUCT_NAME "Turdus Play")

target_sources(turdus_play PRIVATE main.cpp)

target_compile_definitions(turdus_play PRIVATE
    JUCE_USE_CURL=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(turdus_play PRIVATE
    turdus_core
    turdus_engine
    turdus_io
    turdus_midi
    turdus_midi_juce
    turdus_sanitizers
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

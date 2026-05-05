juce_add_console_app(turdus_smoke
    PRODUCT_NAME "Turdus Smoke")

target_sources(turdus_smoke PRIVATE main.cpp)

target_compile_definitions(turdus_smoke PRIVATE
    JUCE_USE_CURL=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(turdus_smoke PRIVATE
    turdus_core
    turdus_midi
    turdus_midi_juce
    turdus_sanitizers
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

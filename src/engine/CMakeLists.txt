add_library(turdus_engine STATIC
    Clock.cpp
    Engine.cpp
    MidiClockEmitter.cpp)

target_include_directories(turdus_engine PUBLIC include)

target_link_libraries(turdus_engine
    PUBLIC  turdus_core turdus_model turdus_midi
    PRIVATE turdus_warnings turdus_sanitizers)

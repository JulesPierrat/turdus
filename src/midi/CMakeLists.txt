add_library(turdus_midi STATIC
    placeholder.cpp)

target_link_libraries(turdus_midi
    PUBLIC  turdus_core
    PRIVATE turdus_warnings turdus_sanitizers)

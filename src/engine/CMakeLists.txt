add_library(turdus_engine STATIC
    placeholder.cpp)

target_link_libraries(turdus_engine
    PUBLIC  turdus_core turdus_model turdus_midi
    PRIVATE turdus_warnings turdus_sanitizers)

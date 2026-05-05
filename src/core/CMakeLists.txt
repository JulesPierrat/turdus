add_library(turdus_core STATIC
    placeholder.cpp)

target_link_libraries(turdus_core
    PRIVATE turdus_warnings turdus_sanitizers)

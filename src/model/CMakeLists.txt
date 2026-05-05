add_library(turdus_model STATIC
    placeholder.cpp)

target_link_libraries(turdus_model
    PUBLIC  turdus_core
    PRIVATE turdus_warnings turdus_sanitizers)

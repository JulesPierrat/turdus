add_library(turdus_io STATIC
    placeholder.cpp)

target_link_libraries(turdus_io
    PUBLIC  turdus_core turdus_model
    PRIVATE turdus_warnings turdus_sanitizers)

add_library(turdus_app STATIC
    placeholder.cpp)

target_link_libraries(turdus_app
    PUBLIC  turdus_core turdus_model turdus_engine turdus_io
    PRIVATE turdus_warnings turdus_sanitizers)

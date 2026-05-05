add_library(turdus_ui STATIC
    placeholder.cpp)

target_link_libraries(turdus_ui
    PUBLIC  turdus_core turdus_model turdus_app
    PRIVATE turdus_warnings turdus_sanitizers)

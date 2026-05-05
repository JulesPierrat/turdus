add_library(turdus_app STATIC
    AppController.cpp)

target_include_directories(turdus_app PUBLIC include)

target_link_libraries(turdus_app
    PUBLIC  turdus_core turdus_model turdus_engine turdus_io turdus_midi
    PRIVATE turdus_warnings turdus_sanitizers)

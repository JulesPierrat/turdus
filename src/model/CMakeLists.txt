add_library(turdus_model STATIC
    Pattern.cpp
    Project.cpp
    TimeSignature.cpp
    Track.cpp)

target_include_directories(turdus_model PUBLIC include)

target_link_libraries(turdus_model
    PUBLIC  turdus_core
    PRIVATE turdus_warnings turdus_sanitizers)

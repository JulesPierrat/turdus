add_library(turdus_io STATIC
    ProjectIO.cpp
    Serialization.cpp)

target_include_directories(turdus_io PUBLIC include)

target_link_libraries(turdus_io
    PUBLIC  turdus_core turdus_model nlohmann_json::nlohmann_json
    PRIVATE turdus_warnings turdus_sanitizers)

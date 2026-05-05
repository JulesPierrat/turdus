add_executable(turdus_tests
    smoke_test.cpp)

target_link_libraries(turdus_tests PRIVATE
    turdus_core
    turdus_model
    turdus_midi
    turdus_engine
    turdus_io
    turdus_app
    turdus_warnings
    turdus_sanitizers
    Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(turdus_tests)

add_executable(turdus_tests
    smoke_test.cpp
    core/value_types_tests.cpp
    core/id_tests.cpp
    model/note_tests.cpp
    model/time_signature_tests.cpp
    model/pattern_tests.cpp
    model/track_tests.cpp
    model/project_tests.cpp
    io/serialization_tests.cpp
    io/project_io_tests.cpp
    midi/midi_message_tests.cpp
    midi/fake_port_tests.cpp
    engine/transport_tests.cpp
    engine/engine_tests.cpp
    engine/clock_tests.cpp
    engine/midi_clock_tests.cpp)

target_compile_definitions(turdus_tests PRIVATE
    TURDUS_TESTS_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

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

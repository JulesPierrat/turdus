#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

#include <turdus/io/ProjectIO.hpp>
#include <turdus/model/Project.hpp>
#include <turdus/model/Track.hpp>

using namespace turdus::core;
using namespace turdus::model;
using namespace turdus::io;

#ifndef TURDUS_TESTS_FIXTURES_DIR
#error "TURDUS_TESTS_FIXTURES_DIR must be defined by the build system"
#endif

namespace {

Project make_test_project() {
    Project p;
    p.set_name("test");
    p.set_tempo(Bpm{132.0});
    p.set_time_signature(TimeSignature{6, 8});
    auto tid = p.add_track(Track{"Lead", "Synth", Channel{0}});
    p.add_placement(PatternPlacement{tid, PatternId::from_raw(1), Tick{0}});
    p.add_port_mapping(MidiPortMapping{"Synth", "IAC Bus 1", false});
    return p;
}

std::filesystem::path scratch_path(std::string_view name) {
    auto dir = std::filesystem::temp_directory_path() / "turdus_tests";
    std::filesystem::create_directories(dir);
    return dir / name;
}

std::filesystem::path fixtures_dir() {
    return std::filesystem::path{TURDUS_TESTS_FIXTURES_DIR};
}

}  // namespace

TEST_CASE("Save then load round-trips", "[io][project_io]") {
    Project original = make_test_project();
    auto path = scratch_path("rtsave.turdus");

    auto save = ProjectIO::save(original, path);
    REQUIRE(save.ok());

    auto load = ProjectIO::load(path);
    REQUIRE(load.ok());
    REQUIRE(load.project.has_value());
    REQUIRE(*load.project == original);
}

TEST_CASE("Round-trip is byte-identical across two save passes", "[io][project_io]") {
    Project original = make_test_project();
    auto first = ProjectIO::to_json_string(original);

    auto loaded = ProjectIO::from_json_string(first);
    REQUIRE(loaded.ok());
    auto second = ProjectIO::to_json_string(*loaded.project);

    REQUIRE(first == second);
}

TEST_CASE("Loading non-existent file returns FileNotFound", "[io][project_io]") {
    auto result = ProjectIO::load("/definitely/does/not/exist.turdus");
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.error == ProjectIOError::FileNotFound);
}

TEST_CASE("Loading malformed JSON returns InvalidJson", "[io][project_io]") {
    auto result = ProjectIO::from_json_string("{ this is not valid json");
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.error == ProjectIOError::InvalidJson);
    REQUIRE_FALSE(result.detail.empty());
}

TEST_CASE("Loading non-object returns MalformedData", "[io][project_io]") {
    auto result = ProjectIO::from_json_string("[1, 2, 3]");
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.error == ProjectIOError::MalformedData);
}

TEST_CASE("Loading newer schema version returns UnsupportedSchemaVersion",
          "[io][project_io]") {
    auto result = ProjectIO::from_json_string(R"({"schema_version": 9999})");
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.error == ProjectIOError::UnsupportedSchemaVersion);
}

TEST_CASE("Loading data missing required fields returns MalformedData",
          "[io][project_io]") {
    auto result = ProjectIO::from_json_string(R"({"schema_version": 1})");
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.error == ProjectIOError::MalformedData);
}

TEST_CASE("Loading the hand-written fixture works", "[io][project_io][fixture]") {
    auto path = fixtures_dir() / "sample.turdus";
    auto result = ProjectIO::load(path);
    REQUIRE(result.ok());
    REQUIRE(result.project.has_value());

    const auto& p = *result.project;
    REQUIRE(p.name() == "Sample fixture");
    REQUIRE(p.tempo() == Bpm{120.0});
    REQUIRE(p.time_signature() == TimeSignature{4, 4});
    REQUIRE(p.tracks().size() == 2);
    REQUIRE(p.arrangement().size() == 3);
    REQUIRE(p.port_mappings().size() == 2);

    const auto& lead = p.tracks().front();
    REQUIRE(lead.id == TrackId::from_raw(1));
    REQUIRE(lead.track.name() == "Lead");
    REQUIRE(lead.track.patterns().size() == 1);
    REQUIRE(lead.track.patterns().front().pattern.notes().size() == 3);
}

TEST_CASE("Fixture round-trips byte-identically through load/save", "[io][project_io][fixture]") {
    auto path = fixtures_dir() / "sample.turdus";
    auto loaded_once = ProjectIO::load(path);
    REQUIRE(loaded_once.ok());

    auto rendered_once = ProjectIO::to_json_string(*loaded_once.project);
    auto loaded_twice = ProjectIO::from_json_string(rendered_once);
    REQUIRE(loaded_twice.ok());
    auto rendered_twice = ProjectIO::to_json_string(*loaded_twice.project);

    REQUIRE(rendered_once == rendered_twice);
}

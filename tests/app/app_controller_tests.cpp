#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <memory>

#include <turdus/app/AppController.hpp>
#include <turdus/io/ProjectIO.hpp>
#include <turdus/core/Channel.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/midi/FakeMidiBackend.hpp>
#include <turdus/model/Note.hpp>
#include <turdus/model/Pattern.hpp>
#include <turdus/model/Track.hpp>

using namespace turdus::core;
using namespace turdus::model;
using namespace turdus::midi;
using namespace turdus::app;

namespace {

// Build a controller seeded with a project containing one track and one pattern.
struct Fixture {
    std::unique_ptr<AppController> controller;
    TrackId track_id;
    PatternId pattern_id;
};

Fixture make_fixture() {
    auto backend = std::make_unique<FakeMidiBackend>();
    auto controller = std::make_unique<AppController>(std::move(backend));

    // The controller starts with an empty project. We can't add tracks via its
    // public API yet (that's Phase 9 / project editing), so we go through
    // open_project with a tiny in-memory JSON. Easiest path: construct a
    // project, save it to a temp file, load it.
    Project p;
    p.set_name("test");
    auto track_id = p.add_track(Track{"Lead", "Synth", Channel{0}});
    auto* track = p.find_track(track_id);
    REQUIRE(track != nullptr);
    track->add_pattern(Pattern{"Verse", Tick{1920}, Channel{0}});

    // Persist + load via the controller so the project lives inside it.
    auto tmp = std::filesystem::temp_directory_path() / "turdus_app_ctrl_tests"
               / "fixture.turdus";
    std::filesystem::create_directories(tmp.parent_path());
    {
        // Use ProjectIO directly to write the seed file.
        REQUIRE(turdus::io::ProjectIO::save(p, tmp).ok());
    }
    REQUIRE(controller->open_project(tmp));

    // Recover the (potentially renumbered) ids from the controller's project.
    REQUIRE(!controller->project().tracks().empty());
    const auto& te = controller->project().tracks().front();
    REQUIRE(!te.track.patterns().empty());
    const auto& pe = te.track.patterns().front();

    return Fixture{std::move(controller), te.id, pe.id};
}

Note make_note(int pitch, int start, int length, int vel = 100) {
    return Note{Pitch{pitch}, Tick{start}, Tick{length}, Velocity{vel}};
}

}  // namespace

TEST_CASE("AppController::add_note inserts and returns a valid id",
          "[app][controller][edits]") {
    auto f = make_fixture();
    const auto id = f.controller->add_note(f.track_id, f.pattern_id,
                                           make_note(60, 0, 240));
    REQUIRE(id.is_valid());
    const auto* track = f.controller->project().find_track(f.track_id);
    REQUIRE(track != nullptr);
    const auto* pattern = track->find_pattern(f.pattern_id);
    REQUIRE(pattern != nullptr);
    REQUIRE(pattern->size() == 1);
    REQUIRE(pattern->find_note(id).has_value());
}

TEST_CASE("AppController::add_note rejects unknown track/pattern",
          "[app][controller][edits]") {
    auto f = make_fixture();
    const auto bad_track = TrackId::from_raw(999999);
    const auto id = f.controller->add_note(bad_track, f.pattern_id,
                                           make_note(60, 0, 240));
    REQUIRE_FALSE(id.is_valid());
}

TEST_CASE("AppController::remove_note", "[app][controller][edits]") {
    auto f = make_fixture();
    const auto id = f.controller->add_note(f.track_id, f.pattern_id,
                                           make_note(60, 0, 240));
    REQUIRE(id.is_valid());

    REQUIRE(f.controller->remove_note(f.track_id, f.pattern_id, id));
    const auto* track = f.controller->project().find_track(f.track_id);
    const auto* pattern = track->find_pattern(f.pattern_id);
    REQUIRE(pattern->empty());

    REQUIRE_FALSE(f.controller->remove_note(f.track_id, f.pattern_id, id));
}

TEST_CASE("AppController::move_note changes start and pitch",
          "[app][controller][edits]") {
    auto f = make_fixture();
    const auto id = f.controller->add_note(f.track_id, f.pattern_id,
                                           make_note(60, 0, 240));
    REQUIRE(f.controller->move_note(f.track_id, f.pattern_id, id,
                                    Tick{480}, Pitch{64}));

    const auto* pattern = f.controller->project().find_track(f.track_id)
                              ->find_pattern(f.pattern_id);
    auto note = pattern->find_note(id);
    REQUIRE(note.has_value());
    REQUIRE(note->start == Tick{480});
    REQUIRE(note->pitch == Pitch{64});
    REQUIRE(note->length == Tick{240});  // length preserved
}

TEST_CASE("AppController::move_note clamps negative start to 0",
          "[app][controller][edits]") {
    auto f = make_fixture();
    const auto id = f.controller->add_note(f.track_id, f.pattern_id,
                                           make_note(60, 100, 240));
    REQUIRE(f.controller->move_note(f.track_id, f.pattern_id, id,
                                    Tick{-50}, Pitch{60}));
    const auto* pattern = f.controller->project().find_track(f.track_id)
                              ->find_pattern(f.pattern_id);
    REQUIRE(pattern->find_note(id)->start == Tick{0});
}

TEST_CASE("AppController::resize_note clamps to >= 1", "[app][controller][edits]") {
    auto f = make_fixture();
    const auto id = f.controller->add_note(f.track_id, f.pattern_id,
                                           make_note(60, 0, 240));

    REQUIRE(f.controller->resize_note(f.track_id, f.pattern_id, id, Tick{480}));
    REQUIRE(f.controller->project().find_track(f.track_id)
                ->find_pattern(f.pattern_id)
                ->find_note(id)
                ->length
            == Tick{480});

    REQUIRE(f.controller->resize_note(f.track_id, f.pattern_id, id, Tick{0}));
    REQUIRE(f.controller->project().find_track(f.track_id)
                ->find_pattern(f.pattern_id)
                ->find_note(id)
                ->length
            == Tick{1});
}

TEST_CASE("AppController::set_note_velocity", "[app][controller][edits]") {
    auto f = make_fixture();
    const auto id = f.controller->add_note(f.track_id, f.pattern_id,
                                           make_note(60, 0, 240, 100));
    REQUIRE(f.controller->set_note_velocity(f.track_id, f.pattern_id, id,
                                            Velocity{42}));
    REQUIRE(f.controller->project().find_track(f.track_id)
                ->find_pattern(f.pattern_id)
                ->find_note(id)
                ->velocity
            == Velocity{42});
}

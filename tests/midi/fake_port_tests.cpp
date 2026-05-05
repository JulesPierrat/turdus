#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/midi/FakeMidiBackend.hpp>
#include <turdus/midi/FakeMidiPort.hpp>
#include <turdus/midi/MidiMessage.hpp>

using namespace turdus::core;
using namespace turdus::midi;

TEST_CASE("FakeMidiPort starts closed and toggles via open/close", "[midi][fake]") {
    FakeMidiPort port{"test"};
    REQUIRE(port.name() == "test");
    REQUIRE_FALSE(port.is_open());
    REQUIRE(port.open());
    REQUIRE(port.is_open());
    port.close();
    REQUIRE_FALSE(port.is_open());
}

TEST_CASE("FakeMidiPort records sent messages with their deadlines", "[midi][fake]") {
    FakeMidiPort port{"test"};
    port.open();

    auto m1 = MidiMessage::note_on(Channel{0}, Pitch{60}, Velocity{100});
    auto m2 = MidiMessage::note_off(Channel{0}, Pitch{60});
    port.send(m1, Tick{0});
    port.send(m2, Tick{240});

    REQUIRE(port.sent().size() == 2);
    REQUIRE(port.sent()[0].msg == m1);
    REQUIRE(port.sent()[0].deadline == Tick{0});
    REQUIRE(port.sent()[1].msg == m2);
    REQUIRE(port.sent()[1].deadline == Tick{240});
}

TEST_CASE("FakeMidiPort drops messages when closed", "[midi][fake]") {
    FakeMidiPort port{"test"};
    port.send(MidiMessage::clock());  // closed → dropped
    REQUIRE(port.sent().empty());

    port.open();
    port.send(MidiMessage::clock());
    REQUIRE(port.sent().size() == 1);

    port.close();
    port.send(MidiMessage::stop());
    REQUIRE(port.sent().size() == 1);
}

TEST_CASE("FakeMidiPort::clear empties the recording", "[midi][fake]") {
    FakeMidiPort port{"test"};
    port.open();
    port.send(MidiMessage::clock());
    port.send(MidiMessage::clock());
    REQUIRE(port.sent().size() == 2);
    port.clear();
    REQUIRE(port.sent().empty());
}

TEST_CASE("FakeMidiBackend lists configured ports and creates Fake instances",
          "[midi][fake]") {
    FakeMidiBackend backend;
    backend.set_available_ports({"Port A", "Port B"});

    REQUIRE(backend.list_output_ports() == std::vector<std::string>{"Port A", "Port B"});

    auto port = backend.create_output_port("Port A");
    REQUIRE(port != nullptr);
    REQUIRE(port->name() == "Port A");
    REQUIRE_FALSE(port->is_open());
}

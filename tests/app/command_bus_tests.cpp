#include <catch2/catch_test_macros.hpp>

#include <variant>

#include <turdus/app/Command.hpp>
#include <turdus/app/CommandBus.hpp>
#include <turdus/core/Bpm.hpp>

using namespace turdus::app;

TEST_CASE("CommandBus push/pop preserves command type and value", "[app][commandbus]") {
    CommandBus bus{8};
    REQUIRE(bus.empty());

    REQUIRE(bus.push(StartTransport{}));
    REQUIRE(bus.push(SetTempo{turdus::core::Bpm{132.0}}));
    REQUIRE(bus.push(SetClockEnabled{true}));
    REQUIRE(bus.push(StopTransport{}));

    Command cmd;

    REQUIRE(bus.try_pop(cmd));
    REQUIRE(std::holds_alternative<StartTransport>(cmd));

    REQUIRE(bus.try_pop(cmd));
    REQUIRE(std::holds_alternative<SetTempo>(cmd));
    REQUIRE(std::get<SetTempo>(cmd).tempo == turdus::core::Bpm{132.0});

    REQUIRE(bus.try_pop(cmd));
    REQUIRE(std::holds_alternative<SetClockEnabled>(cmd));
    REQUIRE(std::get<SetClockEnabled>(cmd).enabled);

    REQUIRE(bus.try_pop(cmd));
    REQUIRE(std::holds_alternative<StopTransport>(cmd));

    REQUIRE(bus.empty());
    REQUIRE_FALSE(bus.try_pop(cmd));
}

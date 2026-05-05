#include <catch2/catch_test_macros.hpp>

#include <turdus/core/Id.hpp>

using namespace turdus::core;

namespace {
struct TagA;
struct TagB;
using IdA = Id<TagA>;
using IdB = Id<TagB>;
}  // namespace

TEST_CASE("Id default state", "[core][id]") {
    IdA id;
    REQUIRE_FALSE(id.is_valid());
    REQUIRE(id.raw() == 0);
}

TEST_CASE("Id::next yields strictly increasing, valid ids", "[core][id]") {
    auto a = IdA::next();
    auto b = IdA::next();
    auto c = IdA::next();
    REQUIRE(a.is_valid());
    REQUIRE(b.is_valid());
    REQUIRE(c.is_valid());
    REQUIRE(a < b);
    REQUIRE(b < c);
}

TEST_CASE("Id counters are independent across tags", "[core][id]") {
    // The two tags share no counter; ids of different tags don't compare.
    auto a = IdA::next();
    auto b = IdB::next();
    REQUIRE(a.is_valid());
    REQUIRE(b.is_valid());
    // Just to exercise raw() on each tag — values can overlap, that's fine.
    REQUIRE(a.raw() > 0);
    REQUIRE(b.raw() > 0);
}

TEST_CASE("Id::from_raw preserves the value", "[core][id]") {
    auto id = IdA::from_raw(42);
    REQUIRE(id.raw() == 42);
    REQUIRE(id.is_valid());

    auto null_id = IdA::from_raw(0);
    REQUIRE_FALSE(null_id.is_valid());
}

TEST_CASE("Id equality", "[core][id]") {
    REQUIRE(IdA::from_raw(7) == IdA::from_raw(7));
    REQUIRE(IdA::from_raw(7) != IdA::from_raw(8));
}

TEST_CASE("Id::ensure_next_at_least bumps the counter", "[core][id]") {
    // Use a tag local to this test so the counter state doesn't leak elsewhere.
    struct LocalTag;
    using LocalId = Id<LocalTag>;

    LocalId::ensure_next_at_least(1000);
    auto first = LocalId::next();
    REQUIRE(first.raw() >= 1000);

    // Lower values are no-ops.
    LocalId::ensure_next_at_least(5);
    auto second = LocalId::next();
    REQUIRE(second > first);
}

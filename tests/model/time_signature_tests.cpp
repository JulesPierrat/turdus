#include <catch2/catch_test_macros.hpp>

#include <turdus/model/TimeSignature.hpp>

using namespace turdus::model;

TEST_CASE("TimeSignature default is 4/4", "[model][timesig]") {
    TimeSignature ts;
    REQUIRE(ts.numerator() == 4);
    REQUIRE(ts.denominator() == 4);
}

TEST_CASE("TimeSignature accepts valid power-of-two denominators", "[model][timesig]") {
    for (int d : {1, 2, 4, 8, 16, 32}) {
        TimeSignature ts(3, d);
        REQUIRE(ts.numerator() == 3);
        REQUIRE(ts.denominator() == d);
    }
}

TEST_CASE("TimeSignature rejects invalid denominators (falls back to 4)", "[model][timesig]") {
    REQUIRE(TimeSignature(4, 3).denominator() == 4);
    REQUIRE(TimeSignature(4, 5).denominator() == 4);
    REQUIRE(TimeSignature(4, 64).denominator() == 4);
    REQUIRE(TimeSignature(4, 0).denominator() == 4);
    REQUIRE(TimeSignature(4, -8).denominator() == 4);
}

TEST_CASE("TimeSignature clamps numerator to [1, 32]", "[model][timesig]") {
    REQUIRE(TimeSignature(0, 4).numerator() == 1);
    REQUIRE(TimeSignature(-5, 4).numerator() == 1);
    REQUIRE(TimeSignature(33, 4).numerator() == 32);
    REQUIRE(TimeSignature(100, 4).numerator() == 32);
}

TEST_CASE("TimeSignature equality", "[model][timesig]") {
    REQUIRE(TimeSignature(3, 4) == TimeSignature(3, 4));
    REQUIRE(TimeSignature(3, 4) != TimeSignature(4, 4));
    REQUIRE(TimeSignature(3, 4) != TimeSignature(3, 8));
}

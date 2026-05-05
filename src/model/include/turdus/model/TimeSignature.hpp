#pragma once

#include <compare>

namespace turdus::model {

class TimeSignature {
public:
    static constexpr int kMinNumerator = 1;
    static constexpr int kMaxNumerator = 32;

    constexpr TimeSignature() noexcept = default;

    // Numerator clamped to [1, 32]; denominator must be a power of two in {1, 2, 4, 8, 16, 32}
    // — invalid denominators fall back to 4.
    TimeSignature(int numerator, int denominator);

    constexpr int numerator() const noexcept { return numerator_; }
    constexpr int denominator() const noexcept { return denominator_; }

    constexpr auto operator<=>(const TimeSignature&) const noexcept = default;

    static bool is_valid_denominator(int d) noexcept;

private:
    int numerator_{4};
    int denominator_{4};
};

}  // namespace turdus::model

#pragma once

#include <algorithm>
#include <compare>

namespace turdus::core {

// Tempo in beats per minute. Values are silently clamped to a musically sane range
// on construction; see kMin / kMax.
class Bpm {
public:
    using value_type = double;

    static constexpr value_type kMin = 20.0;
    static constexpr value_type kMax = 999.0;
    static constexpr value_type kDefault = 120.0;

    constexpr Bpm() noexcept : value_(kDefault) {}
    explicit constexpr Bpm(value_type v) noexcept : value_(std::clamp(v, kMin, kMax)) {}

    constexpr value_type value() const noexcept { return value_; }

    constexpr auto operator<=>(const Bpm&) const noexcept = default;

private:
    value_type value_;
};

}  // namespace turdus::core

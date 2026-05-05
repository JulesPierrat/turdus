#pragma once

#include <algorithm>
#include <compare>
#include <cstdint>

namespace turdus::core {

// MIDI velocity, [0, 127]. Out-of-range values are clamped on construction.
class Velocity {
public:
    using value_type = std::uint8_t;

    static constexpr int kMin = 0;
    static constexpr int kMax = 127;
    static constexpr int kDefault = 100;

    constexpr Velocity() noexcept : value_(static_cast<value_type>(kDefault)) {}
    explicit constexpr Velocity(int v) noexcept
        : value_(static_cast<value_type>(std::clamp(v, kMin, kMax))) {}

    constexpr value_type value() const noexcept { return value_; }

    constexpr auto operator<=>(const Velocity&) const noexcept = default;

private:
    value_type value_;
};

}  // namespace turdus::core

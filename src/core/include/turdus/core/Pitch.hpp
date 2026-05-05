#pragma once

#include <algorithm>
#include <compare>
#include <cstdint>

namespace turdus::core {

// MIDI note number, [0, 127]. Out-of-range values are clamped on construction.
class Pitch {
public:
    using value_type = std::uint8_t;

    static constexpr int kMin = 0;
    static constexpr int kMax = 127;

    constexpr Pitch() noexcept = default;
    explicit constexpr Pitch(int v) noexcept
        : value_(static_cast<value_type>(std::clamp(v, kMin, kMax))) {}

    constexpr value_type value() const noexcept { return value_; }

    constexpr auto operator<=>(const Pitch&) const noexcept = default;

private:
    value_type value_{0};
};

}  // namespace turdus::core

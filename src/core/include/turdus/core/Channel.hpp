#pragma once

#include <algorithm>
#include <compare>
#include <cstdint>

namespace turdus::core {

// MIDI channel, internally [0, 15] (displayed [1, 16]). Clamped on construction.
class Channel {
public:
    using value_type = std::uint8_t;

    static constexpr int kMin = 0;
    static constexpr int kMax = 15;

    constexpr Channel() noexcept = default;
    explicit constexpr Channel(int v) noexcept
        : value_(static_cast<value_type>(std::clamp(v, kMin, kMax))) {}

    constexpr value_type value() const noexcept { return value_; }

    // 1-indexed display value (matches what users see in DAWs).
    constexpr int display() const noexcept { return static_cast<int>(value_) + 1; }

    constexpr auto operator<=>(const Channel&) const noexcept = default;

private:
    value_type value_{0};
};

}  // namespace turdus::core

#pragma once

#include <cmath>

#include <turdus/core/Beats.hpp>
#include <turdus/core/Tick.hpp>

namespace turdus::core {

// Engine internal resolution: 960 ticks per quarter note. Divides cleanly by
// 2, 3, 4, 5, 6, 8 — covers every musically common subdivision.
inline constexpr Tick::value_type kPpq = 960;

// Convert beats to ticks, rounding to the nearest tick.
inline Tick beats_to_ticks(Beats b) noexcept {
    return Tick{static_cast<Tick::value_type>(std::llround(b.value() * static_cast<Beats::value_type>(kPpq)))};
}

// Convert ticks to beats. Round-trip is exact when starting from an integer tick value.
inline constexpr Beats ticks_to_beats(Tick t) noexcept {
    return Beats{static_cast<Beats::value_type>(t.value()) / static_cast<Beats::value_type>(kPpq)};
}

}  // namespace turdus::core

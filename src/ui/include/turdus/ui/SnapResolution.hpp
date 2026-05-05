#pragma once

#include <cstdint>

#include <turdus/core/Tick.hpp>

namespace turdus::ui {

// Snap-to-grid resolutions for the piano roll editor. Values are tick counts at
// the engine's PPQ=960; they are exact integer divisors of one quarter note.
enum class SnapResolution : std::int64_t {
    QuarterNote      = 960,  // 1/4
    EighthNote       = 480,  // 1/8
    SixteenthNote    = 240,  // 1/16
    ThirtySecondNote = 120,  // 1/32
};

inline core::Tick snap_floor(core::Tick t, SnapResolution res) noexcept {
    const auto step = static_cast<core::Tick::value_type>(res);
    if (step <= 0) {
        return t;
    }
    auto v = t.value();
    // Floor toward negative infinity (so negative ticks snap correctly too).
    if (v < 0) {
        v -= step - 1;
    }
    return core::Tick{(v / step) * step};
}

inline core::Tick::value_type snap_ticks(SnapResolution res) noexcept {
    return static_cast<core::Tick::value_type>(res);
}

}  // namespace turdus::ui

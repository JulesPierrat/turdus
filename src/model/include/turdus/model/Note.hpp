#pragma once

#include <compare>

#include <turdus/core/Pitch.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/core/Velocity.hpp>

namespace turdus::model {

struct Note {
    core::Pitch pitch;
    core::Tick start;
    core::Tick length;
    core::Velocity velocity;

    constexpr core::Tick end() const noexcept { return start + length; }

    constexpr auto operator<=>(const Note&) const noexcept = default;
};

}  // namespace turdus::model

#pragma once

#include <compare>
#include <cstdint>

namespace turdus::core {

class Tick {
public:
    using value_type = std::int64_t;

    constexpr Tick() noexcept = default;
    explicit constexpr Tick(value_type v) noexcept : value_(v) {}

    constexpr value_type value() const noexcept { return value_; }

    constexpr auto operator<=>(const Tick&) const noexcept = default;

    constexpr Tick& operator+=(Tick rhs) noexcept {
        value_ += rhs.value_;
        return *this;
    }
    constexpr Tick& operator-=(Tick rhs) noexcept {
        value_ -= rhs.value_;
        return *this;
    }

    friend constexpr Tick operator+(Tick a, Tick b) noexcept { return Tick{a.value_ + b.value_}; }
    friend constexpr Tick operator-(Tick a, Tick b) noexcept { return Tick{a.value_ - b.value_}; }

private:
    value_type value_{0};
};

}  // namespace turdus::core

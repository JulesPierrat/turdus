#pragma once

#include <compare>

namespace turdus::core {

class Beats {
public:
    using value_type = double;

    constexpr Beats() noexcept = default;
    explicit constexpr Beats(value_type v) noexcept : value_(v) {}

    constexpr value_type value() const noexcept { return value_; }

    constexpr auto operator<=>(const Beats&) const noexcept = default;

    constexpr Beats& operator+=(Beats rhs) noexcept {
        value_ += rhs.value_;
        return *this;
    }
    constexpr Beats& operator-=(Beats rhs) noexcept {
        value_ -= rhs.value_;
        return *this;
    }

    friend constexpr Beats operator+(Beats a, Beats b) noexcept { return Beats{a.value_ + b.value_}; }
    friend constexpr Beats operator-(Beats a, Beats b) noexcept { return Beats{a.value_ - b.value_}; }

private:
    value_type value_{0.0};
};

}  // namespace turdus::core

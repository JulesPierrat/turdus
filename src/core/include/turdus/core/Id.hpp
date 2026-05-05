#pragma once

#include <atomic>
#include <compare>
#include <cstdint>

namespace turdus::core {

// Strongly-typed, monotonic 64-bit identifier templated by an empty tag struct.
// A default-constructed Id is the null/invalid value (raw == 0); valid ids start at 1.
//
// Use Id<Tag>::next() to allocate a fresh id at runtime. Use Id<Tag>::from_raw(...)
// to reconstruct an id from a serialized representation. Use ensure_next_at_least
// after deserializing to guarantee that future next() calls don't collide with
// loaded ids.
template <typename Tag>
class Id {
public:
    using value_type = std::uint64_t;

    constexpr Id() noexcept = default;

    static constexpr Id from_raw(value_type v) noexcept { return Id{v}; }

    static Id next() noexcept {
        return Id{counter_.fetch_add(1, std::memory_order_relaxed) + 1};
    }

    // After this call, next() is guaranteed to return an id with raw() >= v.
    // Cheap and idempotent: callable repeatedly from any thread.
    static void ensure_next_at_least(value_type v) noexcept {
        if (v == 0) {
            return;
        }
        const value_type target = v - 1;  // counter == target → next() yields target+1 == v
        auto current = counter_.load(std::memory_order_relaxed);
        while (current < target
               && !counter_.compare_exchange_weak(current, target, std::memory_order_relaxed)) {
            // CAS failed — `current` was refreshed, retry while still below target.
        }
    }

    constexpr value_type raw() const noexcept { return value_; }
    constexpr bool is_valid() const noexcept { return value_ != 0; }

    constexpr auto operator<=>(const Id&) const noexcept = default;

private:
    explicit constexpr Id(value_type v) noexcept : value_(v) {}

    value_type value_{0};

    inline static std::atomic<value_type> counter_{0};
};

}  // namespace turdus::core

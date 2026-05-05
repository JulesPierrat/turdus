#pragma once

#include <atomic>
#include <cstddef>
#include <utility>
#include <vector>

namespace turdus::app {

// Single-producer / single-consumer lock-free queue. The classic ring-buffer with
// atomic head/tail indices and one sentinel slot.
//
// Memory ordering: store-release on the index the producer/consumer mutates,
// load-acquire on the other side's index — pairs to publish the buffer slot writes.
//
// Constraints:
//   - exactly one thread calls push, exactly one calls try_pop (otherwise UB).
//   - T must be moveable (or copyable). Allocations on push happen only if T's
//     move/copy assigns allocate; for "RT-safe" use, choose POD or pre-reserved T.
template <typename T>
class SpscQueue {
public:
    explicit SpscQueue(std::size_t capacity)
        : buffer_(capacity + 1),  // +1 for the sentinel slot we keep empty
          slots_(capacity + 1) {}

    // Capacity in user-visible items (= internal size - 1).
    std::size_t capacity() const noexcept { return slots_ - 1; }

    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire)
               == tail_.load(std::memory_order_acquire);
    }

    // Producer side. Returns false if full.
    bool push(T item) {
        const auto t = tail_.load(std::memory_order_relaxed);
        const auto next = (t + 1) % slots_;
        if (next == head_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[t] = std::move(item);
        tail_.store(next, std::memory_order_release);
        return true;
    }

    // Consumer side. Returns false if empty.
    bool try_pop(T& out) {
        const auto h = head_.load(std::memory_order_relaxed);
        if (h == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        out = std::move(buffer_[h]);
        head_.store((h + 1) % slots_, std::memory_order_release);
        return true;
    }

private:
    std::vector<T> buffer_;
    std::size_t slots_;
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};

}  // namespace turdus::app

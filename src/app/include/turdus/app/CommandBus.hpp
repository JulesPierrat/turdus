#pragma once

#include <cstddef>

#include <turdus/app/Command.hpp>
#include <turdus/app/SpscQueue.hpp>

namespace turdus::app {

// One-way SPSC channel from the UI thread to the engine thread.
class CommandBus {
public:
    explicit CommandBus(std::size_t capacity = 256) : queue_(capacity) {}

    // UI thread.
    bool push(Command cmd) { return queue_.push(std::move(cmd)); }

    // Engine thread.
    bool try_pop(Command& out) { return queue_.try_pop(out); }

    bool empty() const noexcept { return queue_.empty(); }

private:
    SpscQueue<Command> queue_;
};

}  // namespace turdus::app

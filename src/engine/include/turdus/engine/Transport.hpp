#pragma once

#include <atomic>
#include <cstdint>

#include <turdus/core/Bpm.hpp>
#include <turdus/core/Tick.hpp>

namespace turdus::engine {

// Transport state shared between the UI thread (writes via play/stop/seek) and the
// engine thread (writes via advance, reads everything). Each field is its own atomic;
// fields have no inter-invariant so independent loads are correct.
class Transport {
public:
    enum class State : std::int32_t { Stopped = 0, Playing = 1 };

    struct Snapshot {
        State state;
        core::Tick position;
        core::Bpm tempo;

        bool operator==(const Snapshot&) const noexcept = default;
    };

    Transport() noexcept = default;

    // Reads — safe from any thread.
    State state() const noexcept { return state_.load(std::memory_order_acquire); }
    core::Tick position() const noexcept {
        return core::Tick{position_.load(std::memory_order_acquire)};
    }
    core::Bpm tempo() const noexcept { return core::Bpm{tempo_.load(std::memory_order_acquire)}; }
    Snapshot snapshot() const noexcept { return {state(), position(), tempo()}; }
    bool is_playing() const noexcept { return state() == State::Playing; }

    // UI-thread writes.
    void play() noexcept { state_.store(State::Playing, std::memory_order_release); }
    void pause() noexcept { state_.store(State::Stopped, std::memory_order_release); }
    void stop() noexcept {
        state_.store(State::Stopped, std::memory_order_release);
        position_.store(0, std::memory_order_release);
    }
    void seek(core::Tick t) noexcept { position_.store(t.value(), std::memory_order_release); }
    void set_tempo(core::Bpm bpm) noexcept {
        tempo_.store(bpm.value(), std::memory_order_release);
    }

    // Engine-thread write: advance position by `delta` ticks.
    void advance(core::Tick delta) noexcept {
        position_.fetch_add(delta.value(), std::memory_order_acq_rel);
    }

private:
    std::atomic<State> state_{State::Stopped};
    std::atomic<std::int64_t> position_{0};
    std::atomic<double> tempo_{core::Bpm::kDefault};
};

}  // namespace turdus::engine

#pragma once

#include <variant>

#include <turdus/core/Bpm.hpp>

namespace turdus::app {

// Engine-targeted commands. Each command is small and POD-like to keep the SPSC
// queue cheap and avoid allocation on the engine side. UI-thread-only operations
// (load project, save project, port assignment) live directly on AppController
// and don't go through this bus.

struct StartTransport {};
struct StopTransport {};

struct SetTempo {
    core::Bpm tempo;
};

struct SetClockEnabled {
    bool enabled;
};

using Command = std::variant<StartTransport, StopTransport, SetTempo, SetClockEnabled>;

}  // namespace turdus::app

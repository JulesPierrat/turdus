# Turdus — Software Structure

This document describes the proposed architecture of Turdus, a C++ MIDI sequencer combining a piano roll editor, a pattern sequencer and a master MIDI clock. Its purpose is to serve as a blueprint for the initial implementation. It is intentionally prescriptive on boundaries and responsibilities, but leaves a few technology choices open (flagged as **Decision**) where multiple reasonable options exist.

---

## 1. Goals and non-goals

### Goals
- Author and play back MIDI patterns from a graphical piano roll.
- Chain patterns into songs/scenes via a sequencer.
- Emit a stable, jitter-bounded MIDI clock to drive external gear.
- Send MIDI Out to one or more ports (hardware or virtual).
- Save and reload projects deterministically.
- Run on Linux, macOS and Windows.

### Non-goals (initial scope)
- Audio synthesis or audio I/O.
- VST / AU / LV2 hosting.
- MIDI recording from external input (planned later as a separate concern; the architecture leaves room but does not implement it in v1).
- Network / RTP-MIDI.

---

## 2. High-level architecture

Turdus is organized as a layered architecture with a strict separation between the **engine** (real-time, thread-safe, no UI dependency) and the **UI** (non real-time, talks to the engine through a thread-safe API).

```
+----------------------------------------------------------+
|                          UI                              |
|   piano roll  |  sequencer view  |  transport  |  ports  |
+------------------------+---------------------------------+
                         |  (commands / observed state)
                         v
+----------------------------------------------------------+
|                     Application                          |
|   project lifecycle, undo/redo, persistence, settings    |
+------------------------+---------------------------------+
                         |
                         v
+----------------------------------------------------------+
|                       Engine                             |
|   transport · scheduler · clock · sequencer · midi out   |
+------------------------+---------------------------------+
                         |
                         v
+----------------------------------------------------------+
|                  Platform / MIDI backend                 |
|        RtMidi (or JUCE / ALSA / CoreMIDI / WinMM)        |
+----------------------------------------------------------+
```

The engine never calls into the UI. The UI observes engine state through snapshots and a lock-free message queue (engine → UI), and submits changes through a command queue (UI → engine). This is the standard pattern for any DAW-like real-time application and avoids locking the audio/MIDI thread.

---

## 3. Directory layout

```
turdus/
├── CMakeLists.txt
├── LICENSE
├── README.md
├── doc/
│   └── STRUCTURE.md
├── src/
│   ├── core/             # value types, units, ids — header-only where possible
│   ├── model/            # domain model (Note, Pattern, Track, Project)
│   ├── engine/           # real-time engine (clock, transport, scheduler, sequencer)
│   ├── midi/             # MIDI message types + backend abstraction
│   ├── io/               # project serialization (load/save)
│   ├── app/              # application layer: command bus, undo, settings
│   ├── ui/               # GUI (piano roll, sequencer view, transport)
│   └── main.cpp
├── tests/
│   ├── model/
│   ├── engine/
│   ├── midi/
│   └── io/
├── third_party/          # vendored deps (or fetched via CMake FetchContent)
└── resources/            # icons, default project, fonts
```

Each top-level `src/` subdirectory is a CMake target (static library) with explicit public headers. `ui` depends on `app`, `app` depends on `engine` and `model`, `engine` depends on `midi` and `model`, `model` depends only on `core`. Cycles are forbidden.

---

## 4. Core types (`src/core`)

Header-only utility types that describe time, pitch and identifiers. These are passed by value across the codebase.

- `Tick` — strongly-typed integer representing one PPQ (pulses-per-quarter-note) tick. PPQ is fixed at compile time (see §6).
- `Beats` — fractional beats, used for UI display and tempo math.
- `Bpm` — tempo as `double`, clamped to a sane range (e.g. `[20, 999]`).
- `Pitch` — MIDI note number `[0, 127]`.
- `Velocity` — `[0, 127]`.
- `Channel` — MIDI channel `[0, 15]` (displayed `[1, 16]`).
- `Id<T>` — opaque, monotonic 64-bit identifier templated by entity tag (`NoteId`, `PatternId`, `TrackId`). IDs are stable across edits and are how the UI references engine objects.

Why strong types: prevents mixing ticks with beats or note numbers with channels — a cheap, compile-time guarantee that pays off across thousands of call sites.

---

## 5. Domain model (`src/model`)

Pure data, no real-time concerns. Owned by the application layer; the engine sees an immutable snapshot.

- `Note { Pitch pitch; Tick start; Tick length; Velocity velocity; }`
- `Pattern` — a named, fixed-length container of notes plus metadata (length in ticks, default channel, color). Notes are stored sorted by `start` for fast iteration; insertion uses a sorted insert.
- `Track` — references one MIDI output port + channel, owns a list of `Pattern`s, plus per-track settings (mute, solo, transpose).
- `Project` — top-level container: tempo, time signature, list of tracks, song arrangement (sequence of pattern triggers per track over time), MIDI port mappings.

Mutation rule: the UI never mutates a model object that the engine is currently reading. Edits go through the command bus (§9), which produces a new immutable snapshot the engine swaps in atomically.

---

## 6. Engine (`src/engine`)

The real-time core. Runs on a dedicated high-priority thread, allocates nothing in its hot path, never takes locks, never calls the UI directly.

### 6.1 Time base

- **PPQ**: `960` ticks per quarter note. This is the internal resolution for all event scheduling. It is high enough for musical precision and divides cleanly by 2, 3, 4, 5, 6, 8.
- **MIDI clock**: 24 PPQN per spec — emitted as a derived stream at every `PPQ / 24 = 40` engine ticks.

### 6.2 Components

- `Transport` — playing/stopped/paused state, current song position (in ticks), loop region. Exposes thread-safe getters that return atomic snapshots.
- `Clock` — wall-clock driver. On each tick of a high-resolution timer, it advances `Transport` and asks the `Scheduler` to emit any events due in the current slice. The clock runs on its own thread with elevated priority.
  - **Decision**: implement the timer using `std::chrono::steady_clock` + `std::this_thread::sleep_until` for v1, with a measured sleep-correction loop. If jitter is unacceptable on a target platform, switch to platform-specific timers (`timerfd` on Linux, `mach_absolute_time` on macOS, `CreateWaitableTimerEx` on Windows). Encapsulate behind a `HighResTimer` interface so the choice is local to one file.
- `Scheduler` — given the current tick range `[t, t+slice)`, walks the active pattern(s) on each track and produces ordered MIDI events. Output is fed to `MidiOut`.
- `Sequencer` — interprets the song arrangement: which pattern is active on each track at the current position, including loop and pattern transitions.
- `MidiClockEmitter` — emits MIDI Real-Time messages: `Start` (0xFA), `Continue` (0xFB), `Stop` (0xFC), and `Clock` (0xF8) at 24 PPQN. Targets are configurable per port.

### 6.3 Look-ahead scheduling

The engine uses a **look-ahead window** (e.g. 5–10 ms): on each clock tick it generates all events that will fire within the next window and timestamps them. Backends that support timestamped delivery (CoreMIDI, ALSA seq) honor those timestamps; backends that don't, send-on-due. This bounds jitter to the window size and decouples musical timing from OS scheduling jitter.

### 6.4 Threading model

- **MIDI/clock thread** — high priority, real-time. Owns the engine, runs the scheduler loop. Pulls commands from the UI→engine queue (lock-free SPSC) and pushes state updates / outgoing-event echoes onto the engine→UI queue.
- **UI thread** — runs the GUI event loop, reads engine snapshots, posts commands.
- **Worker thread(s)** — file I/O (project save/load), settings persistence. Never touches engine state directly.

Communication: lock-free queues (e.g. `moodycamel::ReaderWriterQueue` for SPSC, or a custom ring buffer). Snapshots are exchanged via atomic pointer swap of immutable `Project` instances (RCU-style). The engine retires old snapshots on a worker thread to avoid deallocation in the RT thread.

---

## 7. MIDI layer (`src/midi`)

- `MidiMessage` — small POD `{ status, data1, data2, timestamp }`. Helpers for note on/off, CC, program change, real-time bytes.
- `MidiPort` — abstract output port, identified by name + backend handle. Methods: `open`, `close`, `send(MidiMessage, Tick deadline)`.
- `MidiBackend` — enumerates available ports and instantiates `MidiPort` objects.

**Decision — backend library**:
- *RtMidi* — minimal, cross-platform, BSD-style license, no GUI dependency. Recommended for v1.
- *JUCE* — heavier, GPL/commercial dual-licensed; pulls in a GUI framework we may also use (§8).
- *Native per-platform* (ALSA seq, CoreMIDI, WinMM/UWP) — most control, most code.

If the GUI uses JUCE, reuse JUCE's MIDI module. Otherwise pick RtMidi. The `MidiBackend` abstraction makes this swappable.

---

## 8. UI (`src/ui`)

The UI is the most opinionated part; the architecture lets us swap toolkits without touching the engine.

**Decision — GUI toolkit**:
- *Qt 6* (LGPL) — mature, native look-and-feel, good for desktop apps. Familiar to most C++ devs. License obligations are manageable for GPL projects.
- *JUCE* (GPL/commercial) — purpose-built for audio software, ships its own widget set, integrates with its MIDI module. Best fit for piano-roll-style UIs, but the framework opinionates the whole app.
- *Dear ImGui* — immediate mode, very fast to prototype, less polished for a finished product. Good for an early prototype, less ideal long-term.

Recommended path: **JUCE** if we are willing to GPL the project (we already are — the LICENSE is GPL v3), since it gives us MIDI + UI in one coherent stack. **Qt 6** otherwise.

### 8.1 Views

- **Transport bar** — play/stop/record, tempo, time signature, position, master clock toggle per port.
- **Piano roll editor** — note grid with horizontal time axis and vertical pitch axis. Tools: select, draw, erase, resize. Velocity lane below. Snap-to-grid. Zoom/pan. Multi-note selection and drag.
- **Sequencer / song view** — tracks as rows, time as columns, pattern blocks as colored regions. Drag to extend, double-click to open the pattern in the piano roll.
- **MIDI ports panel** — list available output ports, assign to tracks, configure which ports receive the master MIDI clock.
- **Project settings** — tempo, time signature, default channel, PPQ display options.

### 8.2 UI ↔ engine interaction

The UI never reads or writes engine state directly. It:
1. Issues commands (`AddNote`, `MoveNote`, `StartTransport`, `SetTempo`, …) through the command bus.
2. Receives state updates and observed events (note-on echoes for visual feedback, position pointer) from the engine→UI queue.
3. Renders from a UI-side cached snapshot, not from the engine's live data.

---

## 9. Application layer (`src/app`)

Glues UI and engine together. Responsibilities:

- **Command bus** — central dispatch for user actions. Each command is a small struct; applying it produces a new project snapshot and an undo entry.
- **Undo / redo** — command-based, with snapshot coalescing for continuous gestures (e.g. dragging a note records one undo entry, not 60).
- **Project lifecycle** — new / open / save / save-as.
- **Settings** — per-user preferences (last project, MIDI port assignments, key bindings) persisted to a platform-appropriate path.

---

## 10. Persistence (`src/io`)

- File format: **JSON** for v1 (human-readable, easy to diff, easy to evolve). Schema versioned via a `schema_version` integer; loader applies migrations.
- File extension: `.turdus` (a JSON document under the hood).
- **Decision**: if file size becomes a concern (long projects, many notes), introduce an optional binary side-format later. Not v1.
- Library: `nlohmann/json` (header-only, MIT). Vendored or fetched by CMake.

Round-trip determinism is a hard requirement: loading and re-saving a project must produce a byte-identical file (modulo formatting), so projects are diffable in git.

---

## 11. Build system

- **CMake** ≥ 3.24, C++20.
- Targets: one static library per `src/` subdir + a single `turdus` executable + a `turdus_tests` binary.
- Dependencies fetched via `FetchContent` or vendored under `third_party/`:
  - `nlohmann/json`
  - `Catch2` (or `doctest`) for tests
  - `RtMidi` *or* `JUCE` (per §7 / §8)
  - A SPSC queue (`moodycamel::ReaderWriterQueue` if not using JUCE's own)
- Warnings-as-errors in CI (`-Wall -Wextra -Wpedantic` / `/W4`).
- Sanitizer builds (ASan, UBSan, TSan) wired as CMake presets.

---

## 12. Testing strategy

- **Unit tests** for everything in `core`, `model`, `midi` (message encoding), `io` (round-trip).
- **Engine tests** drive the scheduler with a fake clock and a fake `MidiPort`, asserting the exact sequence of `MidiMessage`s for given inputs. The engine must be testable without any real MIDI device — this is why `MidiPort` is an interface.
- **Property tests** for tick/beat conversions and pattern looping math.
- **Smoke test** that boots the app, opens an embedded sample project and shuts down, run in CI.

---

## 13. Implementation roadmap

A suggested order — each step ends in a working, tested artifact:

1. **Skeleton**: CMake, directory layout, empty libs, hello-world `main`, CI green.
2. **Core + model**: types, `Note`, `Pattern`, `Project`, JSON round-trip, unit tests.
3. **MIDI layer**: backend abstraction + RtMidi/JUCE implementation, port enumeration, send a hard-coded note in a CLI smoke test.
4. **Engine v0**: transport + clock + scheduler playing a single hard-coded pattern, no UI.
5. **MIDI clock emitter**: 24 PPQN out, start/stop/continue, validated against an external slave (a hardware sequencer or a software like a DAW set to MIDI sync).
6. **UI shell**: transport bar, port assignment panel — no editor yet.
7. **Piano roll**: read-only first (renders a pattern), then editing.
8. **Sequencer view + song arrangement**.
9. **Undo/redo**, project save/load wired into the UI.
10. **Polish**: settings persistence, keyboard shortcuts, packaging.

---

## 14. Open decisions

These are choices that must be made before or during step 1 of the roadmap. Each has a recommended default but should be explicitly confirmed.

| # | Decision                | Default                      | Alternatives                          |
|---|-------------------------|------------------------------|---------------------------------------|
| 1 | MIDI backend            | RtMidi                       | JUCE, native per-platform             |
| 2 | GUI toolkit             | JUCE (GPL)                   | Qt 6 LGPL, Dear ImGui                 |
| 3 | High-res timer          | `std::chrono` + sleep_until  | platform-specific (timerfd, etc.)     |
| 4 | Test framework          | Catch2                       | doctest, GoogleTest                   |
| 5 | Project file format     | JSON (`nlohmann/json`)       | binary (CBOR/MessagePack), SQLite     |
| 6 | Lock-free queue         | `moodycamel::ReaderWriterQueue` | hand-rolled ring buffer            |
| 7 | Minimum C++ standard    | C++20                        | C++17                                 |

---

## 15. Glossary

- **PPQ / PPQN** — pulses (ticks) per quarter note. PPQ is the engine's internal resolution; PPQN (24) is the MIDI clock standard.
- **Pattern** — a fixed-length, named bag of notes; the unit edited in the piano roll.
- **Track** — one row in the song view; binds patterns to a MIDI output port + channel.
- **Song / arrangement** — the timeline that says which patterns play on which tracks at which times.
- **Real-time thread** — a high-priority thread that must never block, allocate, or take contended locks.

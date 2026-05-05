# Turdus — Development Plan

Concrete, ordered plan for building Turdus from an empty repository to a usable v1. Read alongside [STRUCTURE.md](STRUCTURE.md), which describes the *what*; this document describes the *when* and *in what order*.

## Locked decisions

These supersede the "Open decisions" table in STRUCTURE.md:

| Decision           | Choice                                           |
|--------------------|--------------------------------------------------|
| GUI toolkit        | **JUCE** (GPL v3 license, matches our LICENSE)   |
| MIDI backend       | **JUCE `juce_audio_devices`** (built into JUCE)  |
| Build system       | CMake ≥ 3.24, C++20                              |
| Test framework     | Catch2 v3                                        |
| Project file       | JSON via `nlohmann/json`                         |
| Lock-free queue    | JUCE `juce::AbstractFifo` for SPSC               |

Picking JUCE collapses the two biggest forks (UI + MIDI) into one stack, gives us a portable high-resolution timer (`juce::HighResolutionTimer`), and ships a tested ring-buffer primitive. License obligations are already satisfied since the project is GPL v3.

---

## How to use this plan

- Phases are sequential. Tasks within a phase can sometimes parallelize — noted where applicable.
- Every phase ends with a **Done when** checklist. Don't move on until it's all green.
- Cross-cutting tasks (testing, CI, lint) are not repeated per phase; see §Cross-cutting at the bottom.
- Sizes: **S** ≈ half-day, **M** ≈ 1–2 days, **L** ≈ 3–5 days, **XL** ≈ a week+. Estimates assume one developer familiar with C++ and JUCE.

---

## Phase 0 — Bootstrap

**Goal:** a buildable, CI-green skeleton that links JUCE and runs a "Hello Turdus" window.

- [ ] Add top-level `CMakeLists.txt`, C++20, warnings-as-errors. (S)
- [ ] Vendor or `FetchContent` JUCE at a pinned tag. (S)
- [ ] Create empty static-lib targets for `core`, `model`, `engine`, `midi`, `io`, `app`, `ui`. (S)
- [ ] Create `turdus` executable with a JUCE `MainWindow` showing the app name. (S)
- [ ] Add Catch2 v3, create `turdus_tests` target with one passing test. (S)
- [ ] Add GitHub Actions CI: build + test on Linux/macOS/Windows. (M)
- [ ] Add CMake presets for Debug, Release, ASan, UBSan, TSan. (S)
- [ ] Add `.clang-format` and `.editorconfig`. (S)
- [ ] Add `.gitignore` (build dirs, IDE files, JUCE artifacts). (S)

**Done when:** `cmake --build` succeeds on all three OSes in CI, `turdus_tests` runs green, the empty window opens locally on one OS.

---

## Phase 1 — Core types and domain model

**Goal:** all value types and the immutable project model exist, with full unit-test coverage. No MIDI, no UI.

- [ ] Implement strong types in `core/`: `Tick`, `Beats`, `Bpm`, `Pitch`, `Velocity`, `Channel`, `Id<T>`. (M)
- [ ] Tick/beat conversion helpers; property tests. (S)
- [ ] `Note`, `Pattern` (sorted-by-start insertion, length, default channel). (M)
- [ ] `Track` (port assignment, channel, mute, solo, transpose). (S)
- [ ] `Project` (tempo, time signature, tracks, song arrangement, port mappings). (M)
- [ ] Snapshot semantics: `Project` is value-copyable; mutations return a new instance. (S)
- [ ] Unit tests for all of the above. (M)

**Done when:** `turdus_tests` covers every public method in `model/`, including round-trip equality on copies.

---

## Phase 2 — Persistence (early)

**Goal:** save and load `Project` to disk before the engine exists, so future tests can use sample project files.

Pulled forward from later in the structure doc because every subsequent phase benefits from being able to load a fixture.

- [ ] Add `nlohmann/json` via FetchContent. (S)
- [ ] Implement `to_json` / `from_json` for every model type. (M)
- [ ] `schema_version` field, with a migration stub. (S)
- [ ] `ProjectIO::save` / `load` with explicit error type (no exceptions across the boundary). (M)
- [ ] Round-trip determinism test: load → save produces byte-identical output (modulo whitespace). (M)
- [ ] Commit one fixture file under `tests/fixtures/` for use by later phases. (S)

**Done when:** a hand-written `.turdus` file loads, plays no role yet, and re-serializes byte-identically.

---

## Phase 3 — MIDI layer

**Goal:** enumerate ports, open one, send a hard-coded note from a CLI smoke test.

- [ ] `MidiMessage` POD + helpers (note on/off, CC, program change, real-time bytes). (S)
- [ ] `MidiPort` interface: `open`, `close`, `send(MidiMessage, Tick deadline)`. (S)
- [ ] `MidiBackend` interface: list output devices, open by name. (S)
- [ ] JUCE-backed implementation using `juce::MidiOutput`. (M)
- [ ] Fake `MidiPort` for tests that records every sent message with timestamp. (S)
- [ ] CLI smoke test (separate small executable or `--smoke` flag): list ports, send a C major chord to a chosen port, exit. (M)
- [ ] Unit tests for message encoding (every helper produces correct status/data bytes). (S)

**Done when:** running the smoke test against a real or virtual MIDI port produces audible/visible MIDI output, and the fake port lets engine tests run with zero real devices.

---

## Phase 4 — Engine v0 (single-pattern playback)

**Goal:** play one hard-coded pattern on one track, in real time, to a real MIDI port. No UI yet — driven from `main` or the CLI.

- [ ] `Transport` with state (stopped/playing), position in ticks, atomic snapshot getters. (M)
- [ ] `HighResTimer` wrapper around `juce::HighResolutionTimer`. (S)
- [ ] `Clock` thread: ticks at a fixed slice (e.g. 1 ms), advances `Transport`, calls `Scheduler`. (M)
- [ ] `Scheduler` with look-ahead window: given `[t, t+slice)`, walks one pattern and emits ordered MIDI events. (L)
- [ ] Note-off bookkeeping: every note-on schedules its matching note-off at `start + length`. (M)
- [ ] Engine integration test using fake `MidiPort` and a deterministic fake clock — assert exact event sequence for a known pattern. (M)
- [ ] CLI playback mode: load a `.turdus` fixture, play it on a chosen port, stop on Ctrl+C. (M)

**Done when:** the engine plays a fixture project to a real port with audible/observable correctness, and the integration test is fully deterministic (no real timer).

---

## Phase 5 — MIDI clock and transport messages

**Goal:** Turdus can drive an external slave (DAW or hardware) over MIDI clock.

- [ ] `MidiClockEmitter` producing `Clock` (0xF8) at 24 PPQN. (M)
- [ ] Emit `Start` (0xFA) on transport start from position 0, `Continue` (0xFB) on resume, `Stop` (0xFC) on stop. (S)
- [ ] Per-port toggle: which output ports receive clock. Stored in `Project`. (S)
- [ ] Tests asserting the clock-message sequence over one bar at several tempos. (M)
- [ ] Validation against an external slave (a DAW set to external MIDI sync, or hardware). Document the procedure in `doc/`. (M)

**Done when:** an external slave locks to Turdus's clock, follows tempo changes, and respects start/stop/continue.

---

## Phase 6 — UI shell

**Goal:** a real window with transport controls and port assignment, talking to the engine through the command bus. No editing yet.

- [ ] Application layer (`app/`): command bus interface, command types (`StartTransport`, `StopTransport`, `SetTempo`, `AssignTrackPort`, `ToggleClockOnPort`). (M)
- [ ] Engine consumes commands from a JUCE `AbstractFifo`-backed SPSC queue; UI publishes to it. (M)
- [ ] Engine-to-UI snapshot: atomic pointer swap of immutable `Project`; UI polls on a timer. (M)
- [ ] Engine-to-UI event queue for live position updates. (S)
- [ ] Transport bar component: play/stop, tempo spin, time signature, position display. (M)
- [ ] MIDI ports panel: list ports, assign to tracks, clock toggle per port. (M)
- [ ] Project menu: New / Open / Save / Save As wired to `ProjectIO`. (M)

**Done when:** the user can open a fixture project, hit play, and hear it; can change tempo live; can reassign output ports without restarting.

---

## Phase 7 — Piano roll (read-only first)

**Goal:** a pattern from the project renders correctly in a piano roll component. No editing yet.

- [ ] `PianoRollComponent` with horizontal time axis, vertical pitch axis, snap grid. (L)
- [ ] Zoom and pan (mouse wheel + modifiers, scrollbars). (M)
- [ ] Render notes as colored rectangles, sized by length, positioned by start. (M)
- [ ] Velocity lane below the grid. (M)
- [ ] Position pointer following the engine during playback. (S)
- [ ] Pattern picker: choose which pattern from which track to display. (S)

**Done when:** loading a fixture with a multi-pattern project shows the right notes at the right places, velocity lane matches, the playhead moves smoothly during playback.

---

## Phase 8 — Piano roll (editing)

**Goal:** the piano roll becomes a proper editor.

- [ ] Tools: select, draw, erase, resize. Keyboard modifiers per JUCE conventions. (L)
- [ ] Note creation issues an `AddNote` command; deletion issues `RemoveNote`; drag issues `MoveNote`/`ResizeNote`. (M)
- [ ] Multi-select (rubber band, shift-click). (M)
- [ ] Drag-edit a multi-selection as a group. (M)
- [ ] Velocity edit in the lane (drag bars). (M)
- [ ] Snap-to-grid with configurable resolution (1/4, 1/8, 1/16, 1/32, triplets). (M)

**Done when:** a user can build a 4-bar pattern from scratch, hear it play, save the project, reload it, and see the same notes.

---

## Phase 9 — Song / sequencer view

**Goal:** chain patterns into a song.

- [ ] `SongViewComponent`: tracks as rows, time as columns. (L)
- [ ] Pattern blocks rendered as colored regions; click to place, drag to extend, double-click to open in piano roll. (L)
- [ ] Loop region selector. (M)
- [ ] `Sequencer` engine component interprets the arrangement: which pattern plays on which track at the current position, with correct loop/transition behavior. (L)
- [ ] Tests: scheduler over a multi-pattern, multi-track arrangement produces the expected event sequence. (M)

**Done when:** a song with at least three patterns chained on two tracks plays back correctly, looping is honored, double-click navigation works.

---

## Phase 10 — Undo / redo

**Goal:** every user-facing edit is reversible.

- [ ] Command-based undo stack in `app/`. Each applied command pushes an inverse onto the stack. (M)
- [ ] Coalescing for continuous gestures (drag = one undo entry, not 60). (M)
- [ ] Standard shortcuts wired (Ctrl/Cmd+Z, Shift+Ctrl/Cmd+Z). (S)
- [ ] Tests covering undo/redo on every command type. (M)

**Done when:** every edit reachable from the UI can be undone and redone, including coalesced drags.

---

## Phase 11 — Polish

**Goal:** v1-ready quality.

- [ ] Settings persistence (last project path, MIDI port assignments, window geometry, key bindings). (M)
- [ ] Configurable keyboard shortcuts. (M)
- [ ] Default project shipped under `resources/` and offered on first launch. (S)
- [ ] In-app About dialog (version, license, JUCE attribution per JUCE license terms). (S)
- [ ] Error handling pass: surface `ProjectIO` errors and MIDI port failures with non-modal toasts. (M)
- [ ] Performance pass: profile the engine under a 100-track stress project, fix any allocation in the RT thread. (L)
- [ ] Manual QA pass on all three OSes. (M)

**Done when:** no known crashes, no regressions, RT thread allocations confirmed zero under stress, all three OSes tested.

---

## Phase 12 — Packaging and release

**Goal:** users can install it.

- [ ] Linux: `.deb` and AppImage. (M)
- [ ] macOS: signed `.app` bundle in a DMG. (M — depends on Apple Developer account)
- [ ] Windows: signed installer (NSIS or WiX). (M)
- [ ] Release workflow in CI: tag → build artifacts on all three OSes → attach to GitHub Release. (M)
- [ ] User-facing CHANGELOG. (S)
- [ ] Tag v1.0.0. (S)

**Done when:** a user on each OS can download the artifact and run Turdus without installing a toolchain.

---

## Cross-cutting (every phase)

These are not separate phases but standing rules during all of the above:

- Every new public type or function ships with a unit test in the same commit.
- CI must stay green; a red main is the team's top priority.
- No `new`/`malloc`/locks/blocking calls in the engine's RT thread — TSan and a no-allocate audit catch this.
- Every command added to the bus also gets an undo path (from Phase 10 onward).
- Every user-visible string goes through a single `Strings` table — cheap insurance for future i18n.
- Every architectural deviation from STRUCTURE.md is recorded as a new section there before merging.

---

## Risk register

Known risky areas to revisit if they bite:

| Risk                                              | Mitigation                                                       |
|---------------------------------------------------|------------------------------------------------------------------|
| MIDI clock jitter unacceptable on Windows         | Phase 5 includes external-slave validation; fall back to platform-specific timers if needed (see STRUCTURE §6.2) |
| JUCE GPL forces all dependencies to be GPL-compatible | Already GPL v3, no new constraint                            |
| Piano roll perf on large patterns (10k+ notes)    | Phase 11 profiling pass; introduce spatial index if needed       |
| RT thread allocation regressions                  | TSan + no-allocate audit in CI; gated under CMake preset         |
| Project file format churn                         | `schema_version` + migration stubs from Phase 2                  |

---

## Quick milestone view

- **M1 — "It plays"** (end of Phase 4): CLI plays a fixture out of a MIDI port.
- **M2 — "It syncs"** (end of Phase 5): external slaves lock to Turdus.
- **M3 — "It edits"** (end of Phase 8): you can author a pattern in the GUI.
- **M4 — "It's a song"** (end of Phase 9): you can arrange patterns into a song.
- **M5 — "It's v1"** (end of Phase 12): packaged, signed, released.

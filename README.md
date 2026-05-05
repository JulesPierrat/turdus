# Turdus

Turdus is a MIDI sequencer written in C++ that combines a **piano roll**, a **step sequencer** and a **MIDI clock** in a single tool, dedicated to generating MIDI Out toward external hardware or third-party software.

The name *Turdus* refers to the thrush genus (in particular *Turdus philomelos*, the song thrush), known for the richness and structure of its song.

## Planned features

- **Piano roll** for editing MIDI patterns (notes, velocities, durations).
- **Sequencer** for chaining and looping patterns.
- **Master MIDI clock** to synchronize external devices (synthesizers, drum machines, other sequencers).
- **MIDI output** to one or several hardware / virtual ports.

## Tech stack

- Language: C++
- Target: MIDI Out (hardware and virtual)

> The project is in its bootstrap phase — the codebase will land progressively. Build instructions, dependencies and usage details will be documented here as they stabilize.

## Documentation

- [doc/STRUCTURE.md](doc/STRUCTURE.md) — software architecture and module breakdown, used as the basis for implementation.

## Build

To be documented once the project skeleton is in place (CMake, MIDI dependencies, supported platforms).

## License

Distributed under the [GNU GPL v3](LICENSE) license.

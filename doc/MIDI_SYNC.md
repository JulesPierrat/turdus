# MIDI Clock Sync — Validation Procedure

This document describes how to verify Turdus's MIDI clock emission against an external slave device. Required by Phase 5 ("Done when an external slave locks to Turdus's clock").

## What Turdus emits

When `--clock` is passed to `turdus_play` (or `Engine::set_clock_enabled(true)` in code):

| Byte | When                                                | Meaning                                         |
|------|-----------------------------------------------------|-------------------------------------------------|
| `0xFA` Start    | transport play() from position 0                | "begin from the start of the song"            |
| `0xFB` Continue | transport play() from non-zero position         | "resume from current song position"           |
| `0xFC` Stop     | transport stop() or pause()                     | "stop"                                        |
| `0xF8` Clock    | every 40 internal ticks while playing           | tempo pulse — 24 per quarter note (24 PPQN)   |

The pulse cadence is fixed at 24 PPQN regardless of tempo — the wall-clock interval between pulses changes with tempo, the per-beat count does not.

## Test rig

You need:

- A MIDI source from Turdus to the slave (one of):
  - On native Linux: an ALSA virtual port (`snd-virmidi` module) or a hardware port.
  - On native macOS: a virtual IAC bus (Audio MIDI Setup → MIDI Studio → IAC Driver).
  - On native Windows: [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) or a hardware port.
  - Across WSL2 ↔ Windows: [`rtpmidid`](https://github.com/davidmoreno/rtpmidid) on Linux paired with [rtpMIDI](https://www.tobias-erichsen.de/software/rtpmidi.html) on Windows. Note that WSL2's default kernel does NOT include the ALSA sequencer modules, so this path requires a custom kernel.

- A slave that displays incoming MIDI clock — either a DAW (Ableton Live, Logic, Reaper, etc.) configured for external MIDI sync, or hardware (drum machine, sequencer, synth with arp).

## Procedure

1. **Prepare a test fixture** (or use [tests/fixtures/sample.turdus](../tests/fixtures/sample.turdus)). The pattern itself is irrelevant — clock fires regardless of notes.

2. **Configure the slave** to follow external MIDI clock from the port Turdus will use. In Ableton: Preferences → Link/Tempo/MIDI → enable "Sync" on the input port. In Reaper: Options → Preferences → Audio → MIDI Devices → enable input + "Receive MIDI clock". Hardware varies — check the manual for "external sync" or "MIDI sync".

3. **Run Turdus** with the clock flag:
   ```bash
   turdus_play <project.turdus> [port_index] --clock
   ```

4. **Check on the slave** — three things to verify in order:

   a. **Tempo locks.** The slave's displayed BPM should match the project's tempo (within 0.1 BPM). Tempo display in DAWs usually has a "synced to external" indicator.

   b. **Start works.** The slave's playhead should start moving the moment Turdus's transport starts. In Ableton, the play button on the slave will start blinking in sync.

   c. **Stop works.** Ctrl+C on Turdus stops the slave. The slave's playhead halts.

5. **Tempo change** (optional): change `tempo` in the project file, reload, restart Turdus. The slave should pick up the new tempo immediately on Start. Live tempo change is Phase 6 (UI commands) — not validated here.

6. **Continue from a non-zero position** (optional): edit the project to use a non-zero starting position, or seek before play in code. The slave should receive `0xFB Continue` (visible in MIDI monitors as a status byte) and resume from its current position rather than the start.

## MIDI monitor sanity check

If the slave doesn't lock and you want to confirm Turdus actually emits the bytes, use a MIDI monitor:

- **macOS**: `mmonitor` (CLI) or [Snoize MIDI Monitor](https://www.snoize.com/MIDIMonitor/) (GUI).
- **Linux**: `aseqdump -p <port>` shows ALSA sequencer events. For raw bytes, `amidi --dump --port hw:X,Y`.
- **Windows**: [MIDI-OX](http://www.midiox.com/).

Expected wire output for one beat at the start of playback (port pointing at the monitor instead of the slave):

```
FA            -- Start
F8            -- Clock pulse 1 of 24 (at tick 0)
F8            -- Clock pulse 2 (tick 40)
...
F8            -- Clock pulse 24 (tick 920)
[note bytes, interleaved]
```

If you see the pulses but the slave doesn't lock, the issue is on the slave side (sync configuration). If you don't see pulses at all, the issue is in Turdus or in the routing.

## Known limitations (v0)

- **Send-on-due**, no timestamped delivery — pulse jitter is bounded by the engine slice (default 1ms). Most slaves tolerate this comfortably; tight hardware sequencers may want sub-ms accuracy, which is on the roadmap (STRUCTURE §6.3).
- **Single port**: Phase 5 emits clock only on the engine's primary output port. Multi-port clock fan-out (per the project's `port_mappings` `send_clock` flag) lands in Phase 6 alongside the UI.
- **No tempo ramp**: tempo changes apply at the next clock pulse boundary, not interpolated.

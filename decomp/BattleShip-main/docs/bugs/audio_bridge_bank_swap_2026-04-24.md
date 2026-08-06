# Audio bridge swapped sSYAudioSequenceBank1/2 sources

**Date:** 2026-04-24
**Status:** Fix shipped to `port/bridge/audio_bridge.cpp`. Awaiting in-game verification of audible BGM.
**Class:** Port-side asset wiring mismatch — bridge loaded the right CTL files but routed each into the wrong global slot.

## Symptom

After the audio buildout reached "fully plumbed but silent" (see `docs/audio_buildout_2026-04-24.md`), every NoteOn in BGM 33 missed in `__n_lookupSoundQuick`. Diagnostic dumps showed the music bank parsed as **1 instrument with 322 sounds, sampleRate=44100**, with 8-byte degenerate keymaps that only matched MIDI keys 0/1/2 — never the keys 84..105 the sequence actually played.

## Root cause

The original game's `SYAudioSettings` (in `src/sys/audio.c`) declares:

```c
(uintptr_t)&B1_sounds2_ctl_ROM_START,      // bank1_start
(uintptr_t)&B1_sounds1_ctl_ROM_START,      // bank2_start
```

`syAudioInit()` then loads:

```c
sSYAudioSequenceBank2 = bnkf->bankArray[0];  // from bank2_start = sounds1_ctl
sSYAudioSequenceBank1 = bnkf->bankArray[0];  // from bank1_start = sounds2_ctl
```

…and uses those globals as:

```c
n_alCSPSetBank(gSYAudioCSPlayers[i], sSYAudioSequenceBank2);                      // music
audio_config.inst_sound_array = sSYAudioSequenceBank1->instArray[0]->soundArray;  // SFX direct-index
```

So the original wiring is:

| File          | Global                  | Used by              | Shape                                        |
|---------------|-------------------------|----------------------|----------------------------------------------|
| `sounds1_ctl` | `sSYAudioSequenceBank2` | music CSP players    | multi-instrument bank, normal keymap ranges  |
| `sounds2_ctl` | `sSYAudioSequenceBank1` | SFX direct-index     | one instrument, 322 single-key sound effects |

The port-side `audio_bridge.cpp` did the **opposite** assignment:

```cpp
sSYAudioSequenceBank2 ← sounds2_ctl   // the SFX file
sSYAudioSequenceBank1 ← sounds1_ctl   // the music file
```

The decomp comment `// load sfx bank` immediately above the `sSYAudioSequenceBank2` assignment in `sys/audio.c` is misleading — the slot named "Bank2" is actually used for music. The `audio_bridge` author followed that label rather than tracing the consumers, and the bridge's own comment block doubled down on the swap (`bank2 (sounds2_ctl/tbl) → sSYAudioSequenceBank2 (music)`).

Effect: the music CSP players were handed the SFX bank's 322-sound table. `__n_lookupSoundQuick` did a binary search over keymaps that all encoded `keyMin == keyMax == sound_index/128`, so MIDI keys outside `{0, 1, 2}` could never match → every NoteOn returned NULL → synthesizer emitted only zeros.

## Fix

`port/bridge/audio_bridge.cpp` — swap the two parser blocks so that:

```cpp
sounds1_ctl → sSYAudioSequenceBank2   // music CSP
sounds2_ctl → sSYAudioSequenceBank1   // SFX
```

Each bank still uses its paired TBL (`sounds1_tbl` with `sounds1_ctl`, etc.) because wavetable `base` pointers index into the TBL the parser was given.

## Why the SFX-fallback workaround was added

`syAudioInit` (`src/sys/audio.c:970`) has a PORT-only fallback:

```c
ALInstrument *inst0 = sSYAudioSequenceBank1->instArray[0];
if (inst0 == NULL && sSYAudioSequenceBank1->instCount > 1) inst0 = sSYAudioSequenceBank1->instArray[1];
```

That was added before this swap was identified, working around the symptom that — under the swapped mapping — `sSYAudioSequenceBank1` was the multi-instrument music bank whose `instArray[0]` is a NULL slot (the original `alBnkfNew` would turn ROM offset 0 into a non-NULL garbage pointer; our parser correctly leaves it NULL). With the swap fix, `sSYAudioSequenceBank1` is the single-instrument SFX bank whose `instArray[0]` is the real (non-NULL) entry, so the fallback never trips. Left in for defensive value; can be removed once SFX has been observed audible.

## Fingerprint for next time

If a port-side parser/bridge consumes ROM data and exposes globals to decomp code, **trace each global's consumer in the original `sys/`/`*_init` paths before naming.** Decomp comments adjacent to assignment statements are often inaccurate; the only authoritative source is "what does the game actually do with this pointer downstream?"

This is a sibling of the `LUS-vs-decomp typename shadowing` and `Token vs raw pointer in PORT u32 fields` classes: shape mismatch between port-side glue and the decomp's expectations of identically-named state.

## Open follow-up

Verify in-game: BGM 33 should now produce audible output, and the new audio_bridge log line should report `sounds1_ctl → sSYAudioSequenceBank2 (music): N instruments, rate=32000` (multi-instrument, 32 kHz). If still silent post-swap, return to `docs/audio_buildout_2026-04-24.md` and resume from "voice alloc / ADPCM / mixer" downstream investigation.

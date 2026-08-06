# Audio buildout — state as of 2026-04-24

## One-line status

Full audio pipeline is live end-to-end. Asset parsing, thread loop, BGM state machine, MIDI event dispatch, and PCM submission to LUS all work. Synthesis emits **silence** because every NoteOn in BGM 33 misses in `__n_lookupSoundQuick` — the music bank's keymap data does not look like standard `ALKeyMap`. This handoff captures everything we proved works, the one concrete LP64 bug that was fixed along the way, and the specific open question blocking audible output.

## UPDATE 2026-04-24 (later same day)

Identified root cause: `audio_bridge.cpp` had **`sSYAudioSequenceBank1` and `sSYAudioSequenceBank2` swapped** — it loaded `sounds2_ctl` (the 322-sound SFX bank) into the music CSP slot and `sounds1_ctl` (the multi-instrument music bank) into the SFX slot. The original `SYAudioSettings` assigns `bank2_start = sounds1_ctl` and `bank1_start = sounds2_ctl`, and `n_alCSPSetBank(player, sSYAudioSequenceBank2)` is what hands that slot to the music player. The misleading `// load sfx bank` comment in `sys/audio.c:806` (next to the `sSYAudioSequenceBank2` assignment) led the bridge author astray. Bridge fix: swap the two `BankParser` blocks in `portAudioLoadAssets`. **Verified in-game** — log now reads `sounds1_ctl → sSYAudioSequenceBank2 (music): 43 instruments, rate=32000` and NoteOns successfully look up sounds. See `docs/bugs/audio_bridge_bank_swap_2026-04-24.md`. The "music bank keymap shape" investigation below was the wrong-bank symptom and is no longer the open issue.

## UPDATE 2026-04-24 (yet later)

Bank swap exposed a second LP64 bug: SIGSEGV in `n_alEnvmixerPull+100` at `fault_addr=0x7f0d`. Root cause was the variant-overlay class: `ALParam` allocator returns 40-byte slots, but `ALStartParamAlt` grew to 48 bytes on LP64 (`wave *` widened, alignment pad pushed `unk1C/unk1D` past offset 40). Writes overran the next freelist param's `next` pointer — `unk1C=0x05, unk1D=0x7F` from chan-state values produced exactly the `0x7f05` corrupt pointer we observed. **Fix shipped:** padded `ALParam` to 48 bytes under PORT plus `_Static_assert(sizeof(variant) <= sizeof(ALParam))` for ALStartParamAlt/ALStartParam/ALFreeParam. Verified: 30 s smoke test runs without crash. See `docs/bugs/audio_alparam_variant_overrun_2026-04-24.md`.

## RESOLVED 2026-04-24 (final session)

Two more fixes after the variant-overrun:

1. **`dc_memin (s32) truncations** at `n_env.c:566/628/683` — wrapped in `#ifdef PORT (uintptr_t) #else (s32) #endif` to match the existing pattern at line 732. Latent LP64 fix; didn't unblock audio alone.

2. **The actual unblock — mixer macro clobber.** `abi.h` auto-included `port/audio/mixer.h` at its tail, BEFORE `n_abi.h` was pulled in via `n_synthInternals.h`. Mixer.h `#undef`-ed and redefined the standard `aXxx` family (worked) and the N_MICRO `n_aXxx` family (silently no-op'd: those macros didn't exist yet because n_abi.h hadn't run). When n_abi.h later ran, it defined the original Acmd-write `n_aLoadBuffer`/`n_aADPCMdec`/etc., **silently overriding** mixer.h's CPU-impl versions. The pull chain wrote Acmd command words that nothing consumed. Fix: removed the auto-include from abi.h; n_env.c already includes mixer.h *after* the n_audio headers, so the override now sticks. See `docs/bugs/audio_mixer_macro_clobber_2026-04-24.md`.

**Verified:** `SSB64 Audio: first non-zero sample detected (idx=0 v=17)` fires within seconds of `syAudioPlayBGM` being called. 30 s smoke test stable. BGM 33 audible end-to-end on the attract demo. Diagnostic instrumentation removed.

The full sequence of fixes that turned silent audio into audible BGM:
1. `audio_bridge_bank_swap` — swapped sounds1/sounds2 → sSYAudioSequenceBank2/1 mapping (let lookup find real instruments).
2. `audio_alparam_variant_overrun` — padded ALParam to 48 bytes on PORT (let envmixer walk the param list without corruption).
3. `audio_cseqplayer_altbank_lp64` — widened `unknown0/1` to `ALBank *` (latent CC22 alt-bank fix; not load-bearing for BGM 33).
4. `dc_memin (s32) → (uintptr_t)` at n_env.c:566/628/683 — latent LP64 fix.
5. `audio_mixer_macro_clobber` — fixed include order so mixer.h's CPU-impl macros aren't silently re-overridden by n_abi.h. **This is the one that made BGM actually emit non-zero PCM samples.**

## What works

| Step | Evidence |
|------|----------|
| Audio assets load from `ssb64.o2r` | `audio_bridge: parsed bank2 (music): 1 instruments, rate=44100` etc. |
| Audio thread enters synthesis loop | `SSB64 Audio: thread entering synthesis loop (freq=552)` |
| `n_alAudioFrame` runs each tick | `first n_alAudioFrame (tic=1 samples=552)` |
| PCM submitted to LUS each tick | `first synth frame submitted (sampleCount=552 bytes=2208)` |
| VRETRACE ticks wake the audio coroutine | Frame counter advances while audio stays responsive |
| BGM status machine walks 0→1→2→3 | `BGM[0] status 0→1→2→3 (bgm=33)` on scene 28 |
| Sequence file parses correctly | `n_alCSeqNew validTracks=0xf97f division=480 qnpt=0.00208333 t0off=68 t0delta=0` |
| MIDI event stream dispatches | `MIDI evt status=0xc2 chan=2 b1=7 b2=0 state=1` plus many control changes |
| NoteOn reaches `__n_lookupSoundQuick` | Multiple attempts observed for `ch=2 key=86..105 vel=41..53` |

## What's broken: the music-bank keymap shape

For every NoteOn, `__n_lookupSoundQuick` returns NULL. The music bank's instrument has:

- `soundCount = 322`
- Every sound has its own `ALKeyMap`, packed 8 bytes each starting at CTL offset `0x540`
- Each keymap's bytes look like `[N%128, N%128, N/128, N/128, 0, 0, ??, ??]`

Parsed as standard `ALKeyMap {u8 velMin, velMax, keyMin, keyMax, keyBase, s8 detune}`, every sound declares `keyMin == keyMax == (N / 128)` — so only MIDI keys 0, 1, 2 are covered. None of the actual notes in BGM 33 (keys 84–105) can match. Meanwhile the **SFX bank** (`sounds1_ctl`, parsed into `sSYAudioSequenceBank1`) has perfectly standard 6-byte `ALKeyMap` data (`velMin=0 velMax=127 keyMin=0 keyMax=37 keyBase=37` for its first sound). So the parser works for SFX — the music bank is structurally different.

Contrast in raw bytes (bank 2 music vs bank 1 SFX, sound[0]'s keymap):

```
bank2 music sound[0] km@0x540: 00 00 00 00 00 00 00 04
bank1 sfx   sound[0] km@0x1a10: 00 7f 00 25 25 00 00 00   ← standard ALKeyMap
```

The music bank's keymap is 8 bytes (not 6) — HAL padded it, and the values look velocity-indexed, not range-indexed. SSB64's music instrument is not a "playable" pitched instrument in the SGI sense; it's closer to a dense drum kit with one sample per (velocity-bucket, key-group) pair. Binary search over `{keyMin, keyMax}` cannot match any MIDI key outside `{0, 1, 2}`.

### Hypotheses for what's really going on

1. **HAL uses a different lookup function for music.** There may be a `__n_lookupSoundQuick` variant, or `__n_lookupSound` (the slow version) in SSB64 that indexes sounds directly by `(key, velocity)` rather than walking keymap ranges. Only `__n_lookupSoundQuick` is wired today — worth grepping the ROM for the slow variant and whether a dispatch chooses between them.
2. **The instrument layout differs.** Maybe `ALInstrument` on the music bank has extra leading fields, pushing `soundCount` and `soundArray` off by some bytes. The header bytes at offset 14 really do decode cleanly to 322, and the soundArray offsets (`0xe780, 0xe790, 0xe7a0, ...`) march at an even 16-byte ALSound stride consistent with 322 entries, so this is unlikely. But worth auditing with `rabbitizer` against the IDO reader (same technique as `docs/debug_ido_bitfield_layout.md`).
3. **The music bank is only usable after a CC-driven inst switch we're dropping.** The sequence sends `AL_MIDI_FX_CTRL_4` (CC22) with value=5. The handler gates on `if (byte2 > 2) break;` so the switch silently fails and the channel stays on `bank[0]` (= the 322-sound instrument). If HAL moved that gate — say `byte2 > 7` to allow more banks — the game might route channels to a different bank that the port has never loaded. `n_alCSPSetBank_Alt` is never called anywhere in the codebase today, so even the existing `byte2 ≤ 2` gates can only point at NULL.

None of these are settled. Option 1 is the most promising starting point — look for a second lookup routine that's direct-indexed.

## Diagnostics infrastructure left in place

Low-volume runtime logs that confirm the pipeline is alive (remove when the remaining bug is closed):

- `port/audio/audio_playback.cpp` — first-submit and first-non-zero-sample one-shot logs
- `src/sys/audio.c` — first `syAudioPlayBGM` / first `syAudioPlayFGM` / first `n_alAudioFrame` / audio thread-enter logs

All are one-shot `if (!logged) { logged = true; port_log(...); }` — no per-frame spam.

The verbose bank/instrument dump in `port/bridge/audio_bridge.cpp` was removed after characterizing the keymap shape; revive it (it was under `if (sParseLogged < 2)`) if you need to re-check the raw ROM bytes.

## Fixed along the way

- **LP64 alt-bank pointer-array stride** in `N_ALCSPlayer` / `N_ALSeqPlayer`. See `docs/bugs/audio_cseqplayer_altbank_lp64_2026-04-24.md`. Widened `unknown0`/`unknown1` from `s32` to `ALBank *` under `#ifdef PORT` so that `(&seqp->bank)[idx]` walks three adjacent pointer slots cleanly. Latent-bug fix — does not affect BGM 33 since nothing registers alt-banks.

## Suggested next step

Run the game in a scene that triggers **bank 1 (SFX)** — any scene with sound effects, e.g. menu navigation that plays a cursor blip. Since bank 1 has standard keymap data, `__n_lookupSoundQuick` should succeed there. If SFX sample _do_ play but BGM doesn't, that's definitive: the music bank needs a different lookup, not a different parser. `syAudioPlayFGM` is the entry point; we already log its first call.

If SFX also fails to produce audible output, the problem is further downstream (voice allocation, ADPCM decode, envelope, mixer buffer routing) and the investigation should move into `mixer.c` and `n_synallocvoice`.

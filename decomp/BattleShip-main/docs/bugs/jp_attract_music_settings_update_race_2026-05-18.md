# JP build: attract/intro music silent — PORT audio settings-update race

**Date:** 2026-05-18
**Area:** `decomp/src/sc/scmanager.c` (`scManagerRunLoop`, `#ifdef PORT`
branch) ↔ `decomp/src/sys/audio.c` audio-thread settings-update restart
**Symptom (JP only):** attract/intro/title BGM silent. SFX works, stage
music works, JP text/assets fine. US plays the same sequence correctly.

## Root cause

`scManagerRunLoop` calls `syAudioSetSettingsUpdated()` then, on N64
(`#else`), spin-waits `while (syAudioGetSettingsUpdated()) ;` and
`while (syAudioGetRestarting()) ;` so the audio thread's
settings-update restart completes before scene dispatch. The `#ifdef
PORT` branch **skipped both waits** — old comment "Audio is stubbed on
PC — skip the spin-waits that would hang." That comment is obsolete: the
audio pipeline has run for real since 2026-04-24, so the restart now
executes asynchronously to scene dispatch.

The audio-thread settings-update path (`audio.c` ~1273–1311): it sums
`port_cmdLen` over sound players + non-`AL_STOPPED` CSPlayers. If
`port_cmdLen == 0` it does the full restart (`n_alClose` →
`portAudioLoadAssets` → `syAudioMakeBGMPlayers`) and **clears**
`dSYAudioIsSettingsUpdated`. Otherwise it calls `syAudioStopBGMAll()`
and leaves the flag set.

**US masks the race:** first scene is `nSCKindStartup` (~3 s N64 logo;
`scmanager.c:498–504` REGION split). During those 3 s all CSPlayers are
`AL_STOPPED` → `port_cmdLen == 0` → restart completes, flag cleared,
long before any BGM call.

**JP breaks:** first scene is `nSCKindOpeningRoom` immediately.
`mvOpeningRoomFuncStart` calls `syAudioPlayBGM(0, nSYAudioBGMOpening)`
(`mvopeningroom.c`, no REGION guard) while the flag is still TRUE.
`syAudioPlayBGM` sets `sSYAudioCSPlayerStatuses[0] = AL_PLAYING`, so the
audio thread now sees `port_cmdLen != 0`, takes the
`syAudioStopBGMAll()` branch (wipes the queued BGM) and does **not**
clear the flag — the attract BGM never starts. SFX and stage music are
entered ≫3 s later, after the flag has cleared, so they work.

Ruled out: JP vs US `B1_sounds1_ctl` / `S1_music_sbk` are byte-for-byte
identical (BankParser / ctl→bank mapping not the cause); the
`mvopeningroom.c` BGM trigger has no `#if REGION_US` guard. (Parallel
sub-agent investigation; mechanism then verified directly in
`scmanager.c` + `audio.c`.)

## Fix

The pre-audio-era scaffolding here was a divergent `#ifdef PORT` block
that *skipped* the waits (stale "audio is stubbed" comment) opposite a
full N64 `#else`. Now that audio is real, the two paths are **converged**
to one shared sequence — the only genuine PORT differences remain:

- the spin-wait body injects `port_coroutine_yield()` under `#ifdef
  PORT` (cooperative scheduler) vs the N64 `continue;` busy-spin — the
  exact established idiom already used by `scautodemo.c` /
  `scvsbattle.c` for their `syAudioCheckBGMPlaying` waits (which proves
  it terminates on PORT: the audio restart runs independently of the
  game coroutine);
- the framebuffer clear and its `framebuffer`/`end` locals are
  `#ifndef PORT` (no physical N64 framebuffers on PC).

No iteration cap / diagnostic log: the established idiom has none, the
restart reliably clears the flag (measured: 16 yields), and the 3 s
hang watchdog is the global safety net — a bespoke cap here would be
exactly the kind of legacy-feeling complexity this change removes.
`extern void port_coroutine_yield(void);` added to the file's existing
PORT extern block (same pattern as sc1pgame.c / scvsbattle.c etc.).

## Verification

First fix (with cap+log) confirmed the mechanism: JP log
`settings-update cleared after 16 yields`, full attract loop
(27→28→29→30→33→31→34→36→32→35→37 incl. mvOpeningRoom), no
hang/crash/watchdog; **user-confirmed JP attract music now audible**.
The converged refactor is behaviourally identical for the waits (same
two waits, same yield idiom; only the cap/log removed) and was
re-verified: JP reaches scene dispatch (scene 27 → 28 → 29 → 30 → 33,
i.e. the audio waits cleared — scene dispatch is unreachable if they
hang) and runs the opening/attract with no hang/crash/watchdog. US
port regression (shared path): rebuilt + attract re-run, clean.

## Notes

`scmanager.c` is decomp source (`decomp` submodule, branch
`jp-rom-support`/`port-patches`). Pure PORT-branch change — N64/US-ROM
matching builds (`PORT` undefined) are unaffected; US **port** build now
runs the same waits as N64 here (more correct than skipping; US first
scene is still `nSCKindStartup` so it clears immediately too). Submodule
pointer bump required when this lands (see CLAUDE.md submodule workflow).
Related JP bring-up: `jp_submotion_figatree_misclassified_2026-05-18`,
`jp_us_only_funcs_called_unconditionally_2026-05-18`.

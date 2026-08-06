# JP runtime triage — automated test session 2026-05-18

Build: `cmake -DSSB64_VERSION=jp` (decomp `caae085e1`, libultraship
`d6587f8`), `baserom.jp.z64`. Ran `/tmp/cfg-jp/BattleShip` under `xvfb-run`
(env has `DISPLAY=:0`; xvfb used for determinism). o2r = complete JP
archive (2134 entries: 2107 reloc + 8 audio + 18 particles + version).

## Works ✅

- Boots. All audio assets load ("all audio assets loaded successfully").
- **SFX works** (user-confirmed interactive).
- Reaches the render loop / attract; runs ~70 s before the texture
  breakage starts.

## Issue A — particle/texture breakage → bad DL → stale-DL crash (task #7)

Symptom: sustained `GfxDpLoadBlock: missing texture image for tile 7` +
`ImportTexture: null texture address for tile 0 (both TMEM slots empty)`,
then (headless run, ~frame 4555) the watchdog's `GFX STALE-DL DIAG`
fires — diag classifies the bad command as `scene_arena+0x…` with a long
run of `seg[0xe] … (MovewordF3dex2/G_MW_SEGMENT)` writes. Interactive run
crashed (SIGSEGV) at the same point. This is the **known stale-DL crash
family** (stability-fixes PR #182), per user.

**Not a JP data/extraction bug.** All 18 JP particle bank sizes match US
byte-for-byte (`particles_unk*`, `efcommon`, `itmanager`, `grpupupu`,
`grhyrule`, `gryoster`, `mntitle` scb/txb); offsets come from the
authoritative `decomp origin/main:smashbrothers.jp.yaml` splat segment
list. Particle data is region-identical and correctly extracted.

**Decomp/LUS fixes ARE present.** `caae085e1` contains the stale-DL
structural fixes `b7d9291d2` / `f6d2e1965`; US is stable with them. So
JP triggers a variant/trigger the US-developed guards don't cover, on a
region-conditional path that feeds garbage into segment `0x0E` / the
scene-arena DL. This is genuine port-side debugging on the JP scene /
particle DL setup — likely a `#if REGION_*` path or a JP scene-arena
layout difference, not the YAMLs.

Next: capture which scene/effect is active when the first
`missing texture image` fires (the headless first occurrence is ~70 s
in, well past title); bisect against the stability-fixes variant list;
check JP-conditional code feeding `gGCCommonDLLinks[]` / segment 0x0E /
particle bank pointers (cf. `docs/bugs/segment_0e_gdl_*`,
`dl_link_stale_pointer_guard_*`, `linux_stale_scene_data_family_*`,
`npikachu_particlebankid_lp64_overrun_*`).

## Issue B — JP attract/intro music silent — FIXED 2026-05-18 (audible confirm pending)

**Root cause:** PORT audio settings-update race in
`decomp/src/sc/scmanager.c` `scManagerRunLoop`. The `#ifdef PORT` branch
skipped the N64 `#else` spin-waits (old comment: "audio is stubbed on
PC" — long obsolete). The audio thread's settings-update restart
(`audio.c`: `n_alClose` → `portAudioLoadAssets` →
`syAudioMakeBGMPlayers` → clear `dSYAudioIsSettingsUpdated`) therefore
ran asynchronously to scene dispatch. **US masks it**: first scene is
`nSCKindStartup` (~3 s N64 logo) so the restart finishes first. **JP**'s
first scene is `nSCKindOpeningRoom` immediately (scmanager.c:498–504
REGION split), so `mvOpeningRoomFuncStart` calls
`syAudioPlayBGM(0, nSYAudioBGMOpening)` while the flag is still TRUE;
`status[0]=AL_PLAYING` makes the audio thread's `port_cmdLen != 0`, it
takes the `syAudioStopBGMAll()` branch (audio.c ~1307) which wipes the
queued BGM and does **not** clear the flag → silent. SFX/stage music
work (entered ≫3 s later). Binary-identical JP/US audio assets ruled out
a bank/CTL-parse cause; `mvopeningroom.c` BGM trigger has no REGION
guard. (Investigation: parallel sub-agent + verified in source.)

**Fix:** restore the settings-update / restarting waits in the PORT
branch, yielding via `port_coroutine_yield()` (the established
scautodemo/scvsbattle PORT-audio-wait idiom — proven to terminate on
PORT because the audio restart runs independently) instead of the N64
busy-spin. Iteration cap + diagnostic log as a hang-proof backstop
(addresses the old comment's documented fear; degrades to the pre-fix
skip rather than hang). N64-only framebuffer clear stays in `#else`.

**Verified:** JP runtime log —
`scManagerRunLoop — past audio/FB setup (settings-update cleared after
16 yields)` then game proceeds to scene dispatch and runs the **entire
attract loop** (scenes 27→28→29→30→33→31→34→36→32→35→37 incl.
mvOpeningRoom) with **no hang, no crash, watchdog never fired**. The
documented wipe path is structurally eliminated (flag clears in
`scManagerRunLoop`, ≫ before any scene's `syAudioPlayBGM`). Audible
confirmation (actual sound) pending user — logs cannot verify audio
output. US regression: shared PORT branch, US rebuild + attract re-run
in progress.

### (historical) original triage — superseded by the above

## Issue B (historical) — JP attract/intro music silent (task #8) — OPEN

Clarified (user, 2026-05-18): the JP attract/intro music is **still not
playing**. The JP ROM is *supposed* to play the **exact same music
sequence as US** during the intro/attract — this is NOT a JP design
difference. US plays it correctly, so a JP-specific bug breaks playback
of the *same* sequence. JP intro visuals + title VO render, and DK's
fighter-intro name is the correct JP "Donkey Kong" (so JP text/assets
load fine). Stage music works; SFX works. Only the intro/attract BGM is
silent, on JP only.

Working theory space: (a) the attract/title BGM trigger sits on a
region-conditional code path that doesn't fire (or fires wrong) on JP;
(b) the BGM/sequence id resolves differently in the JP sequence bank;
(c) `S1_music_sbk` content differs despite matching size (extraction
offset). Next: runtime trace of `syAudioPlayBGM` / audio_bridge during
the title→attract phase to see whether the call fires on JP and with
what id, vs US.

### (historical) original observation

SFX works, music does not. audio_bridge log:
`parsed sounds1_ctl → sSYAudioSequenceBank2 (music): 43 instruments`,
`parsed sounds2_ctl → sSYAudioSequenceBank1 (SFX): 1 instruments` (the
"1 instruments" is implausible). JP `B1_sounds2_ctl/tbl` legitimately
differ in size from US (0xFC40 vs 0xFBA0; 0x2B05E0 vs 0x2DC1E0) — these
are correct per the JP splat boundaries (verified). So the extraction is
right; the **audio bridge's CTL parse / ctl→bank mapping is US-tuned and
mishandles the JP sound-bank-2 layout**. Investigate
`port/audio/*` (the `audio_bridge: parsed … ctl → sSYAudioSequenceBank*`
path) for region assumptions about bank ordering / instrument counts.

## Repro

```
cmake -S . -B <dir> -DSSB64_VERSION=jp -GNinja
cmake --build <dir> --target ssb64 -j4
rm -f <dir>/extracted/BattleShip.o2r   # force re-extract if yamls changed
cmake --build <dir> --target ExtractAssets -j4
cd <dir> && xvfb-run -a ./BattleShip    # or run on a real display
```

(Stale repo-root `BattleShip.o2r` moved to
`BattleShip.o2r.stale-noaudio.bak` so CWD-relative loads from repo root
don't pick up the old no-audio archive. Run from the build dir.)

## Issue C — CSS / opening fighter T-pose (Mario, DK, Fox) — OPEN

User report 2026-05-18: in the JP build, Mario/DK/Fox T-pose on the VS
character-select screen and Fox T-poses through the opening "master hand
pulls fighter from box" sequence (no FigurePulled/Stance motion). **All
fighters animate correctly in actual battle.** US build is correct.

### Code path (verified)

- Battle uses `fp->data->mainmotion` → `dFT<F>MotionDescs[]`; CSS/opening
  use `fp->data->submotion` → `dFT<F>SubMotionDescs[]`
  (`scsubsysdata{mario,fox,donkey}.c`). The port build compiles the
  `#ifdef PORT` branch, which references the **numeric** `ll_<N>_FileID`
  symbols (e.g. `dFTMarioSubMotionDescs`: `ll_357..361`, `ll_362`
  FigurePulled idx8, `ll_365` Stance idx12). The non-PORT branch's pretty
  aliases (`llFTMarioAnimSelectedFileID` …) are `/* STUBBED */ 0` in
  `reloc_data.jp.h` but are **not compiled** in a port build.
- Status→submotion-index resolution in `ftmain.c` (`ftMainSetStatus` →
  `lbRelocGetForceExternHeapFile`). `mnplayersvs.c` maps Mario→Win3
  (0x10003), Fox→Win4 (0x10004), DK→Win1 (0x10001);
  `mvopeningroom.c`/`mvopening{mario,fox,donkey}.c` use FigurePulled
  (0x10008) / Stance (0x1000C).

### RESOLVED 2026-05-18 — it WAS the reloc-name wart (not cosmetic)

The "cosmetic" conclusion below was **wrong** and is kept only to mark
the dead reasoning. Correct root cause + fix + verification:
`docs/bugs/jp_submotion_figatree_misclassified_2026-05-18.md`. Summary:
`generate_yamls.py` keyed its name tables by US ordinals, so JP file_ids
332–356 became `reloc_misc/file_0NNN`;
`portRelocIsFighterFigatreeFile()` classifies figatree files by the
`reloc_submotions/FT` / `reloc_animations/FT` **path prefix**, so those
ids were misclassified non-figatree → the AObj halfswap fixup + range
registration were skipped → streams stayed BE-halfswapped → T-pose. The
flaw in the reasoning below: figatree fixups are **not** access-time
content-driven; they are gated at load by path-prefix classification.
Fixed by rebasing the name tables through the header's `ll_<N>_FileID`
US-ordinal map (identity for US ⇒ US byte-identical). Runtime-verified:
JP submotion ids 332–356 now load `fig=1` with `reloc_submotions/FT`
paths.

### (dead reasoning) RULED OUT — JP reloc-name wart is cosmetic, NOT this bug

There is a real JP-only naming wart (recorded here so it is not
re-chased as the cause): `tools/generate_yamls.py` names non-header
files via **hardcoded US ordinals** (`SUBMOTION_NAMES` 357–498,
`MAIN_MOTION_RANGES` 499–2131, `SUPPLEMENTAL_NAMES`). JP file IDs shift
(US 2132 vs JP 2107). So in `yamls/jp` + `RelocFileTable.jp.cpp`, JP id
332 → `reloc_misc/file_0332` (placeholder) and the name
`reloc_submotions/FTMarioSubMotionAppearR` is parked at id 357 (a
different JP file). 25 such misplaced/duplicated entries (== 2132−2107).

**But the bytes the game actually loads are correct.** Authoritative
cross-check: upstream `relocFileDescriptions.jp.txt` lists `-332: _357_`
and `reloc_data_symbols.jp.txt` has `ll_357_FileID = 0x14c (332)` — the
symbol, the upstream description token, and the physical reloc table all
agree (US 357 / JP 332). Torch's `SSB64:RELOC` factory reads ROM reloc
table[file_id] (per the landed `jp_torch_relocfactory_*` fix, JP table
@0x1ACAF0 / 2107), so `reloc_misc/file_0332` resolves to ROM reloc[332]
= the correct Mario submotion data. Chain: source `ll_357_FileID` → JP
332 → `RelocFileTable.jp[332]`="reloc_misc/file_0332" → ROM reloc[332]
bytes = correct. Figatree halfswap/AObj fixups are applied at
data-access time, not keyed on resource path/category, so the ugly name
does not corrupt the figatree. Conclusion: the wart wastes 25 slots and
should be fixed for hygiene (rebase the hardcoded ordinal tables through
the version header's `ll_<usord>_FileID`→id map — identity for US so US
output stays byte-identical), but it is **not** the T-pose root cause.

### Live hypothesis / next step

T-pose = figatree present-or-empty but joints not driven, JP-only,
SubMotion-only, battle MainMotion fine. Most likely a `#if REGION_US`
(no JP/`#else`/`PORT`) block in the SubMotion *application* path
(`ft/`, `objanim`, figatree/AObj setup, `scsubsys`) that the JP bring-up
excluded or approximated — same class as the documented
`func_ovl0_800C9F70` / `ftParamGetGroundHazardKnockback` relaxations
(`jp_only_funcs_called_unconditionally_2026-05-18.md`), but on an
animation function. Next: (a) runtime-instrument the CSS/opening
figatree load+apply on JP (requested FileID, returned byte length,
figatree parse result / joint count) vs US; (b) audit `#if REGION_US`
blocks lacking a JP/`PORT` branch in the SubMotion application path.

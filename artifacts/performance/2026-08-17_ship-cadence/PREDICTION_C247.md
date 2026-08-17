# Prediction for `c247-pubgate-packon`, written BEFORE the run

**Date:** 2026-08-17 · **Branch:** `codex/r2-runtime2` · **HEAD `798007f30d8`**
**Nothing had been built or run on the flipped defaults when this file was
written.** Root ROM at the time of writing:
`smash64ds-battle-playable-hwtri.nds` SHA-256
`5F3D1FE3C78720CF666E5F5C8131BCC19158CF615413D8389568292B29D2D20C`
(12,538,880 B, built 14:10 on the flipped defaults),
`smash64ds.nds` `54C07FAC80C50418949908701F7C2BDBF27512C5F96AC09086FABBB0DF6AC68A`.

## The arm

`build-c247-pubgate-packon` = `TARGET=smash64ds-battle-playable-hwtri`
(the published target, all its own overrides) with `NDS_R2_BOTH_CPU=1` and
nothing else, on a tree where `NDS_R2_BATTLEPACK ?= 1` and
`NDS_R2_BATTLEPACK_KEEP_CACHE ?= 1`. It is `c245-pubgate` plus the two
battlepack flags, and nothing else at all.

## The number predicted

`c245` (same target, same stress arm, pack **off**) measured **1,945 / 2,043 =
95.20%** two-VBlank. `SHIP_CADENCE.md` §2.1 isolated the pack at **+13 frames**
(`c241` 1,955 with the pack minus `c242` 1,942 without, on the proof ROM, one
flag pair wide), and the tick arm found the *same* +13 independently as its
over-gate delta (§4).

```text
PREDICTED  two-VBlank frames        1,958  of 2,043   =  95.84%
           plausible range          1,950 - 1,966     =  95.45% - 96.23%
           presented (denominator)  2,043 exactly
           viol (interval < 2)      0
           max interval             18
           margin over the >=95% bar (1,941 frames)   +17 frames
```

The range is wider on the low side than the isolation's own precision because
the +13 was measured on the *proof* ROM (`NDS_SHIP_TELEMETRY 1`) and is being
applied to the *published* ROM (`NDS_SHIP_TELEMETRY 0`); the two differ by
+3 frames of telemetry work, and the pack's benefit need not be exactly
additive across that step.

**A result outside 1,950-1,966 falsifies the additive isolation model in
`SHIP_CADENCE.md` §2.1 and will be reported as such rather than explained
away.**

## The determinism control predicted

After the `c247` run, `make TARGET=smash64ds-battle-playable-hwtri` with no
overrides (`BUILD=build`, the same directory that produced the 14:10 link) must
regenerate the root pair. `NDS_TASK10_GIT_SHORT` is `798007f` in
`builds/build/nds_build_config.h` and HEAD has not moved and will not move in
this cycle (this agent does not commit), so the relink is from identical
objects at an identical stamp.

```text
PREDICTED  root smash64ds-battle-playable-hwtri.nds returns to
           5F3D1FE3C78720CF666E5F5C8131BCC19158CF615413D8389568292B29D2D20C
           root smash64ds.nds unchanged at 54C07FAC...C68A (never rebuilt)
```

Note the mechanism this control needs: the `c247` build writes a *newer* root
`.elf` and `.nds` than `builds/build`'s object files, so a plain re-`make` would
find the root pair up to date and relink nothing — leaving `c247`'s ROM in place
while reporting success. The root `.elf`/`.nds` are therefore deleted first
(both halves, never one), forcing the link to run from `builds/build`'s objects.

## The soak predicted

Three completed successive matches in one emulator session, `NO-FREEZE`,
`gNdsVSResultsStartCount` **3**, `gNdsSCVSBattleSuddenDeathPrepareCount`
**>= 1**, `gNdsTaskmanArenaChosenSize` **1,548,288** (the pack's arena, from
`ARENA_PRICE.md`), `AllocFail`/`ReserveFail`/`Rejects`/`SyMallocOverflow` all
**0**, general-heap low-water around **52,400-53,136** against the 32,768 floor
(`ARENA_PRICE.md` measured 52,400; `c246`/`c239` measured 53,136).

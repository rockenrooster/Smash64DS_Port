# P2-2 compact battle-core capacity recovery — 2026-09-11

## Scope and conclusion

This is the natural Donkey/Samus/Link/Kirby follow-up required by the
ShieldPose recovery evidence.  The compact resident Main/Model/closure path
removes the former frame-0 fourth-fighter allocation failure.  The same frozen
ROM stayed in the four-CPU match through the verifier's one-minute coverage
window with all four compact cores and all four compact ShieldPose blobs live.

**P2-2 Main-RAM capacity recovery is GREEN for this argmax lifetime.**  The
wide verifier still returns RED because it exposed an independent native-output
failure in source `EFCommonEffects1` DamageSlash (`asset 83`, root `0x75A0`,
`NDS_NATIVE_FAILURE_NO_PROGRAM`).  That output failure blocks dependent full
acceptance/promotion; it does not reopen the measured Main-RAM capacity result.
P2-2p8 CPU optimization remains owner-deferred and was not attempted here.

## Provenance

- Immutable repository baseline: `5e733919023f9d109773df89d7913589039b79fa`.
- The compact battle-core implementation was already present in the dirty Main
  integration tree when this cycle began.  Its files also contain unrelated
  1P/CSS work, so this cycle did not stage or overwrite those existing edits.
- Generator discriminator:
  `python scripts/fighters/generate_battle_core_packs.py --output-dir builds/p2-2-battle-core-check --kinds donkey,samus,link,kirby`.
- Generated resident bytes: Donkey 21,996; Samus 20,856; Link 19,964; Kirby
  29,384; total **92,200 B**.  Generated Main extern patches: **18**.
- Frozen lab build:
  `make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-battle-core`.
- ROM SHA-256:
  `CC148C03815DF695EB11E911E5148785E22BA30B0054F6C8EE55D031D0D032A2`.
- ELF SHA-256:
  `566D0319EB4F842182F3AE4331DA4FE969DD7205655503F44DBDE0C6FF970240`.
- Wide verifier:
  `pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify-p2-four-fighter-stress.ps1 -NoBuild -Build build-p2-battle-core`.

The verifier wrote its ledger before rejecting the native-output failure.  Raw
verification telemetry is rotating; hashes below pin the exact sampled inputs:

- tick HUD JSON SHA-256 `A9DFF3D1DFD16E27811CD24AFB803EFB745E9B8077A80766436670A8E29AAD44`
- memory JSON SHA-256 `DEA658A9C6C1245B8BAD5A04370270A963EFD6177A097D213F2E7D31924E22A5`
- coverage JSON SHA-256 `048E3EB503900B2442863E9655E4B5A2980341FE8C2E6EFDA43E9D0678401EBC`

## Natural lifetime and capacity

- source identity: 0 human / 4 CPU / 4 fighters / active mask `0xF`
- observed roster: Donkey / Samus / Link / Kirby
- match clock: 60 -> 1 seconds; frames 1 -> 1973
- timing window: frames 2 -> 1973, **1,972 samples**
- presented delta: 1,972; logic delta: 3,944
- chosen taskman arena: **1,396,736 B**
- general heap free minimum: **94,076 B**
- required safety floor: **25,600 B**
- margin above floor: **68,476 B**
- `syTaskmanMalloc` overflow: 0
- object-manager panic: 0
- graphics heap: 1,536 B capacity, 232 B peak, 0 overflow/no-room
- DObj max: 241
- effect pool: 38 capacity, 16 free minimum, 22 active maximum
- particle structs: 53/112; generators 15/24; transforms 27/80; rejects 0
- animation cache: 11,024 B reserved; 79 hits; 704 fills; 166 raw recycles;
  0 failures

The old failure was frame 0 in `ftManagerSetupFilesMainKind(Kirby)`, where a
201,456 B raw Main/closure request met only 47,980 B free.  This run passes that
point and keeps **94,076 B** free at its observed minimum through the same
four-kind natural lifetime.

## Replacement engagement

- compact fighter core loads: **4/4**
- compact fighter resident bytes: **92,200 B**
- compact fighter load failure: 0
- Main extern patches: **18/18**
- source extern assets loaded by the compact closure: **12**
- Main extern failure: 0
- compact ShieldPose loads: **4/4**
- compact ShieldPose resident bytes: **11,799 B**
- ShieldPose native fixups: **36/36**
- ShieldPose load/fixup/decode failures: 0/0/0
- native fighter draw-slot mask: `0xF`

These are positive engagement counters from the same run, so the capacity
result is not inferred from an absent OOM or from an unused compact decoder.

## Independent native-output blocker

The verifier's final native-only guard records **19,708** failures.  Its first
failure is:

- domain: 2 (`NDS_NATIVE_FAILURE_STAGE`)
- scene: 22 (`nSCKindVSBattle`)
- status/stage kind: 6 (`nGRKindPupupu`, Dream Land)
- identity: `0x03F30053`; low 16 bits `0x53` = reloc asset **83**
  (`EFCommonEffects1`)
- source root: **`0x75A0`** = `dEFCommonEffects1_DL_0x75A0`
- reason: 1 (`NDS_NATIVE_FAILURE_NO_PROGRAM`)

BattleShip maps that root through
`dEFManagerDamageSlashEffectDesc` / `dEFCommonEffects1_DamageSlash`.  The
source maker preserves live translate/rotate/size scaling and the source effect
uses its two source display lists/material animation.  A procedural stand-in is
not acceptance for this source-backed path.  Native output coverage is already
owned by the existing P2-3/P2-4/P2-5 output packages; no new queue ID is needed.

## Timing recorded, not promoted

The failing native-output run is useful attribution only and is **not** a final
performance acceptance sample:

- ALL: mean 6,482,771; P50 6,719,744; P95 8,960,512; max 19,604,032 ticks
- WORK: mean 6,201,798; P50 6,539,904; P95 8,754,368; max 19,586,944 ticks
- WORK-H: mean 6,046,695; P50 6,368,192; P95 8,476,416; max 19,144,576 ticks
- VBlank intervals: 2 = 84, 3 = 81, 4 = 12, 5+ = 1,796; max = 35;
  total = 1,973

Per the execution board, P2-2p8 optimization stays deferred until the owner
reopens it.  The capacity conclusion above depends on resident lifetime and
allocator/resource floors, not on accepting these cadence figures.

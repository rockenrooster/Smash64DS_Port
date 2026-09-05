# Kirby — P2-3 fighter 10 (last: copy needs everyone)

Status: gameplay (incl. all ten Copy specials), native owner, shell surfaces and audio bank admitted behind `NDS_P2_KIRBY` (roster-close slice, not yet smoked) · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/`

## Role

Scheduled last because **Copy requires every other fighter's neutral-B to
exist**. Everything else about Kirby is ordinary multi-jump lightweight
(machinery already landed with Jigglypuff).

## Moveset uniques

- **Inhale (B)**: vacuum windbox → swallow (two-body state, reuse the
  DK/Yoshi seam) → choice: spit as star projectile, or **Copy**.
- **Copy**: gains the victim's neutral-B (11 copyable specials + the matching
  hat model per character; loses on damage threshold/taunt per source).
  Implementation = per-fighter copy entries produced by the P2-3 pipeline as
  each fighter lands (each fighter's neutral-B must be callable from Kirby's
  state context — plan for that in the pipeline from Luigi onward, so
  Kirby-day is assembly, not surgery).
- **Final Cutter (Up-B)**: rise/fall slash with a landing shockwave
  projectile.
- **Stone (Down-B)**: transform, invincible? (heavy armor — verify exact 64
  rules), can cancel; hat/copy kept through Stone.
- Five puffs (multi-jump), lightest-class, crouch is nearly flat.

## Assets & audio

Round model + **12 hat variants** (one per copyable fighter incl. himself for
mirror matches) — an asset batch, budget it; 4+ costumes; Kirby voice + the
copied-B voice line variants where the original had them; announcer clip.

## DS notes / risks

- Copy is a code-size and asset bomb if done as forks — it must be "call the
  existing per-fighter special through the seam", which is why the pipeline
  carries a copy-entry requirement from fighter 1.
- Copy × item-hold × multi-jump state interactions: run the full inventory
  sweep per copied power (12 sweeps, scripted).
- Hat attach uses the item/hand attach transform path where possible.

## Admission — 2026-09-02 (roster-close slice, `admit_fighter.py`)

- Manifest: core files `llKirbyMainFileID` 0xe5, MainMotion 0xe4, Model
  0x148 (120,948 B, the heaviest model file), ShieldPose 0x149, Special2
  0x15c; 188 motion files (`FTKirbyAnim` 0x4eb..0x5df, 2 event32, 19 item),
  198 nitrofs files, `attributes_offset` 0x808.
- Gameplay TUs behind `NDS_P2_KIRBY`: `battleship_kirby.c` (ftkirbyspecialn/
  hi/lw + ftkirbythrowf), `battleship_kirby_copy.c` (the ten copy TUs: each
  fighter's neutral-B under Kirby's state context), `battleship_kirby_weapons.c`
  (wpkirbycutter, KirbyMain token 0x08) and `battleship_ftcommon_capturekirby.c`
  (victim half: CaptureKirby / CaptureWait / ThrownKirbyStar / ThrownCopyStar,
  US constants). `battleship_kirby_common.h` carries every FTKIRBY_* REGION_US
  constant and includes the decomp's `ftkirbyfunctions.h`. Status table
  promoted; kind 8. The verbatim `ftKirbySpecialNApplyCaptureDamage` copy in
  Yoshi's capture TU is now `#if !NDS_P2_KIRBY`.
- Native owner slot **11**, image slot **9**: KirbyModel (0x148), JointTree
  High 0x1448 / Low 0x2cd0 (28 descriptors), setup parts 0xef7cffc0, 23
  parts, 7 drawable roots; hierarchy 1/4/4 (the same skeleton shape as
  Purin), 4 cross bindings (slots 16..19), 36 restores; census High
  11/23/27/256 / 768 corners, Low 11/23/16/182 / 546.
- Shell: HUD stock LUTs (texture 0x1D4B8, sprite 0x1D5E0), CSS bake fkind 8,
  Selected demo clip 418; entry effect `efManagerKirbyEntryStarMakeEffect`
  on the entry seam; effect descs VulcanJab / CutterUp / CutterDown /
  CutterDraw / CutterTrail / EntryStar on the roster list.
- Audio: 42 own cues + 11 shared; 207/222 dropped as duplicates of Ness's;
  397 FuraSleep is a 16 kHz body like Yoshi's 596.
  **ACCEPTED DELTA (visual, temporary):** the spit-out and lose-copy star
  sprites draw from ITCommonData, which the port does not load yet; both
  effect calls resolve to NULL until board row P2-3f48 (the same residency
  the Pikachu/Purin Master Ball article needs).
  **ACCEPTED DELTA (audio):** 203 KirbySpecialNStart (the Inhale vacuum) is
  an infinite sequencer held while B is held; it ships as a 300-tick (5 s)
  16 kHz prefix (`LOOP_PREFIX_CUES`) and goes quiet on a longer hold.

## Lab smoke -- 2026-09-04 (RED, board row P2-3f47): out of arena, not a fixup

- `build-kirby-cpu` (ten P2 fighter flags, proof fighter 8,
  `NDS_R2_BOTH_CPU=1`) never presents a frame. The ARM9 halts in
  `ndsSyMallocOverflowHalt`, reached from
  `syMallocSet(bp=gSYTaskmanGeneralHeap, size=115440, alignment=16)` in
  `ftManagerSetupFilesMainKind` for **fkind=1, Fox** -- the SECOND fighter
  set up, after Kirby. Progress line: `presented=0 openfail=0 streamfail=0
  arena=1318912 allocfail=100`. Capture:
  `artifacts/verification/2026-09-04_kirby-battle-progress.txt`.
- **This is not Jigglypuff's defect.** `openfail=0` and `streamfail=0`, and
  the halt is an allocation refusal with a sound heap pointer, not a data
  abort on a raw chain word. The external-fixup path is not reached.
- **It is not binary growth either.** `arm-none-eabi-size` gives 2,671,188
  total for the ten-flag ELF against 2,675,016 for
  `build-battle-playable-proof-hwtri-harness`, so the ten-flag ELF is
  slightly SMALLER. The ten fighter flags add generated payload rows and
  tables, not resident code, so the usual arena-versus-binary trade does not
  explain it.
- **The arena is not the difference, and the first version of this entry said
  it was.** Comparing 1,318,912 against the `chosen=1,597,440` a Jigglypuff
  lab printed on 2026-09-02 looked like a 278,528-byte loss, but that figure
  is two days and many commits old and is not a control. The control built
  from *this* tree -- Mario versus Fox, `build-battle-playable-proof-hwtri-
  harness` -- reads `MEMARENA ... 1319008` and reaches frame 212 of live
  gameplay with render, HUD and audio counters all populated. Same arena,
  same allocation, different outcome.
- So the difference is **what Kirby's own setup consumes** before Fox asks
  for its 115,440. Kirby's `ftManagerSetupFilesMainKind` succeeds; Fox's is
  refused. Mario's leaves room and Kirby's does not.
- A second wrong turn worth recording: the run that first showed this also
  showed the *Mario/Fox* ROM halting identically, which read as a main-line
  regression. It was not. That ELF had been built while the shared generated
  headers still held the ten-flag Kirby configuration -- the generators write
  outside `$(BUILD)`, so the last build's flags decide their content.
  Rebuilding it after a plain `make` restored it to green. Any cross-
  configuration comparison has to rebuild both arms, in order, or it is
  comparing one config's code against another config's tables.
- **What that setup allocates, measured 2026-09-04.**
  `ftManagerSetupFilesMainKind` makes exactly one general-heap allocation per
  kind, `syTaskmanMalloc(lbRelocGetFileSize(file_main_id), 0x10)` (decomp
  `ft/ftmanager.c:285`). `lbRelocGetFileSize` is not one file: decomp
  `lb/lbreloc.c:208-273` walks the whole extern closure, and
  `lbRelocGetExternBytesNum` returns **0** for any id already in the status
  buffer or already counted, so a fighter pays only for the part of its
  closure that is not resident yet. The census column of
  `include/nds/generated/nds_fighter_production.generated.h:9-20` is that
  figure per kind: Mario 54,048, Fox 119,040, **Kirby 204,208**.
- **It is not a port sizing defect.** `ndsRelocExternTreeAllocSize`
  (`src/port/reloc_backend_assets.c:7725-7769`) implements the same two
  exclusions -- a `seen` set and a `sNdsRelocStatusBuffer` lookup that returns
  0 for an already-resident asset -- so the port dedups exactly as the source
  does. It is working: with Kirby already resident, Fox's request came in at
  115,440 against his 119,040 census. The dedup found only 3,600 bytes to
  remove, because Kirby's closure and Fox's barely overlap.
- **It is not copy-ability donor data either**, which was the obvious guess.
  His neutral-B dispatch is a function-pointer list
  (`ftcommonspecialn.c:10-38`), his hats come from the native image slot at
  `ftManagerMakeFighter`, and `dFTKirbyData` leaves special1/3/4 null, so this
  call loads zero donor bytes. The 204,208 is his own closure: **144 extern
  ids against Mario's 48**, and the single largest member is KirbyModel
  (`0x148`) at **120,864** -- the heaviest model in the roster.
- So the cost is located, and the levers are: reclaim ARM9 image bytes (the
  arena is one `calloc` from the same heap, so image and arena trade one for
  one); stop holding every fighter's closure resident at once; or reduce
  KirbyModel itself. The third is a visual-fidelity compromise and
  `PROJECT_GOAL.md`'s sacrifice order makes a permanent one an owner decision,
  so it is not taken here. Sized as a P2-2/P2-3 shared blocker in
  `docs/p2/P2-2-four-fighters.md`.

## Copy abilities -- source survey, 2026-09-05 (probe, LOW confidence, cited)

The copy set is already carried: ten TUs behind `NDS_P2_KIRBY`
(`battleship_kirby_copy.c` and siblings; Luigi's copy rides Mario's TU, and
`ft/ftchar/ftkirby/` has no luigi file). Inhale, swallow, spit and copy live in
`ftkirbyspecialn.c`; the ability is selected through the shared
`dFTKirbySpecialNStatusList` (`ftcommon/ftcommonspecialn.c:10-39`) and its air
twin, not through Kirby-specific code.

**The hat is not a model graft.** It is a modelpart display-list swap on joint 6:
`ftParamSetModelPartDefaultID` sets `modelpart_id_base` and
`ftParamResetModelPartAll` re-seats the joint's `dl` from the container
(`ft/ftparam.c:748-909`); the hat id per donor is the `FTKirbyCopy` table
`dKirbyMainMotion_0x0000[27]` (`relocData/228_KirbyMainMotion.c:132-160`:
Mario 12, Fox 7, Donkey 4, Samus 8, Luigi 11, Link 10, Yoshi 5, Captain 9,
Kirby 0, Pikachu 6, Purin 3, Ness 13). So the native owner export can carry the
hats as data; no new mechanism is needed.

**Each copied special needs its donor's build flag**, because the copy TU has no
per-ability `#if`: Mario and Fox fireball/laser are baseline imports, but Samus,
Donkey, Link, Yoshi, Captain, Pikachu, Purin and Ness copies each pull a maker or
a shared status function from that fighter's TU behind `NDS_P2_<NAME>`.

**Landing order** (probe's, and it isolates the P2-3f47 heap floor from copy
content): Kirby plus Mario only, exercising inhale and spit without a copy;
then the Mario copy (its fireball maker is baseline-resident, so zero new reloc
files); then Fox. Per-hat byte cost is UNVERIFIED.

`llKirbySpecial1FileID` (0xe6, o2r `MiscData230`) is the source's own name, used
at `sc/sc1pmode/sc1pgame.c:2081`: the Kirby Team stage loads it so a copied PK
Fire has its graphics. The symbols dump spells the same file `ll_230_FileID`.

## Acceptance

- [ ] Move inventory sweep vs `ftkirby` data.
- [ ] All 11 copy powers + hats verified (scripted per-power sweep, incl.
      loss rules).
- [ ] Inhale two-body matrix (spit star, copy, escape, edge/KO cases).
- [ ] Budgets (hat batch included) + stress measurement banked; CSS live;
      owner feel pass.

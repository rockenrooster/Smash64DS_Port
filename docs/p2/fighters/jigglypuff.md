# Jigglypuff — P2-3 fighter 9

Status: gameplay, native owner, shell surfaces and audio bank admitted behind `NDS_P2_PURIN` (roster-close slice, not yet smoked) · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftpurin/`
(`purin` = Jigglypuff's Japanese name — the decomp uses it throughout.)

## Role

Multi-jump aerial drift specialist; mechanically small after Ness/Yoshi, so a
quick close-out before Kirby. Unlockable (gating P2-7).

## Moveset uniques

- **Five midair jumps** — shares the multi-jump machinery Kirby will reuse;
  land it here first.
- **Pound (B)**: horizontal drift attack, air-stall/recovery tool.
- **Sing (Up-B)**: radius sleep on grounded opponents, no recovery
  properties; sleep state (opponent-side) lands here and is reused by items
  later if applicable.
- **Rest (Down-B)**: instant sleep with a 1-frame-active point-blank KO
  hitbox — frame-1 exactness matters; this move is a community benchmark.
- Lightest fighter, slowest fall, strongest air drift; shield-break launch
  (instant upward KO on shield break — verify the special-case in source).

## Assets & audio

Tiny model (cheapest in roster), 4 costumes (flower/bow/hat), Jigglypuff
voice samples, sleep VFX ("Zzz"), announcer clip.

## DS notes / risks

- Rest's frame-1 hitbox and Sing's wake rules are exactness-sensitive; take
  them from frame data directly.
- Sleep (opponent state) interacts with damage/mash-wake scaling — shared
  seam, will be reused (Mr. Saturn? no — but Sing + future items), keep it
  in `ftcommon` equivalent.

## Admission — 2026-09-02 (roster-close slice, `admit_fighter.py`)

- Manifest: core files `llPurinMainFileID` 0xe9, MainMotion 0xe8, Model
  0x14a, ShieldPose 0x14b, Special2 0x15f (Sing effect); 144 motion files
  (0x4ef..0x5e7, 2 event32, 19 item), 150 nitrofs files,
  `attributes_offset` 0x474. **The corpus labels her animations
  `FTKirbyCopyAnim000..058` (1445..1503) then `FTPurinAnim000..007`
  (1504..1511)**: the manifest gained two-segment stems (`O2R_ANIM_STEMS`,
  `_ANIM_SPLIT_ID` / `_ANIM_PATH_STEM2`) and the reloc anim arm follows them.
- Gameplay TU `battleship_purin.c` (ftpurinspecialn/hi/lw: Pound, Sing,
  Rest) behind `NDS_P2_PURIN`; status table promoted; kind 10. The victim
  half of Sing is the shared `ftCommonSleepProc*` (inactive stub now
  `#if !NDS_P2_PURIN`).
- Native owner slot **10**, image slot **8**: PurinModel (0x14a), JointTree
  High 0x2028 / Low 0x40a0 (27 descriptors), setup parts 0xeff9ff80, 23
  parts, 7 drawable roots; hierarchy 1/4/4, 4 cross bindings (slots 16..19,
  both details), 38 per-corner restores; census High 21/58/44/319 / 957
  corners, Low 21/44/31/200 / 600.
- Shell: HUD stock LUTs (texture 0x7A88, sprite 0x7BB0, five palettes),
  CSS bake fkind 10 (portrait, Pokémon emblem, name text
  `llMNPlayersCommonJigglypuffTextSprite`), Selected demo clip 470.
- Audio: 23 own cues + 5 shared (91/102/233/234/638); 569 FuraSleep is a
  16 kHz body like Yoshi's 596.

## Lab smoke — 2026-09-02 (RED, board row P2-3f50)

- `build-purin-cpu` (`NDS_P2_PURIN=1` alone, proof fighter 10,
  `NDS_R2_BOTH_CPU=1`) never reaches a presented frame. The new
  `probe-battle-progress.ps1` finds `presented=0` with the ARM9 parked in
  calico's `__excpt_entry`, `cpsr=0x90000097` (abort mode), and the banked
  registers name the caller: `lr_usr=lbCommonSetupFighterPartsDObjs+86`,
  `r7=0xeff9ff80` (his own setup-parts mask, read correctly) and
  `r0=r3=0x4f640000`. Capture:
  `artifacts/verification/2026-09-02_purin-battle-progress.txt`.
- It is not a resource failure: arena `chosen=1,597,440`, `allocfail=32`,
  **`openfail=0`, `streamfail=0`**. The suspect is a pointer fixup on
  PurinMain/PurinModel that feeds the common-parts DObjDesc array.
- Disassembly of the return address narrows it further. `+86` is the `bl`
  to `memset(sp+48, 0, 72)` that clears the function's local
  `array_dobjs[18]`, so the ARM9 aborted **inside that memset** with a
  running pointer of 0x4f640000 while the destination (`r1`) was a sound
  stack address, 0x2255b90. A stack clear cannot fault on its own, so the
  frame was already corrupt when it ran. The mechanism that fits: the
  loop below writes `array_dobjs[dobjdesc->id & 0xFFF]` with no bound
  against the array's 18 entries, exactly as the source does, so a garbage
  `dobjdesc` writes far outside the frame and the next fighter's setup
  faults. 0x4f640000 byte-swaps to 0x644f, a plausible offset inside the
  32,304-byte PurinModel, which is why an unrelocated pointer is the
  leading suspect. A counted bound check at that index would turn the
  stack smash into an attributable rejection; it would be a guard, not the
  fix.
- **First run of this lab failed differently** and that failure is fixed: his
  animations are stored under three corpus stems and 77 of them live under
  `FTKirbyAnim`, so before commit 1fa52c906f9 they could only resolve when
  `NDS_P2_KIRBY` happened to be in the same build. The generated
  `NDS_P2_PURIN_ANIM_SEGMENTS` rows now carry all three segments.
- **Control (this is his, not the tree's):** a Mario-only lab built from
  the same commit runs **965 presented frames** in 30 s with the same
  harness (`2026-09-02_mario-control-progress.txt`), and its own trace
  shows three consecutive fighter setups. Purin's trace stops after the
  **first** `lbCommonSetupFighterPartsDObjs`, which in his lab is Purin
  himself (`2026-09-02_purin-trace.txt` against `..._mario-trace.txt`).
- Eliminated so far: arena size (1,597,440, larger than the ten-flag ROM),
  asset resolution (`openfail=0`; Mario's baseline is 104), the descriptor
  index (the new bound guard never fires), the joint array (25 mask steps
  into 33 slots from `nFTPartsJointCommonStart`), thread provisioning (no
  create or provision failures) and the tree itself (the control above).
- What remains is inside that one call: the entry `memset` of its local
  `array_dobjs` takes a data abort although the container argument, the
  detail index and the output array all read sound at entry. Next step is a
  single-step or watchpoint session across that prologue, or a build with
  `NDS_TASK20_STACK_PROFILE=1` to settle whether the coroutine stack is the
  memory being written.
- **Root cause located 2026-09-02.** The fighter's `FTCommonPartContainer`
  holds unrelocated words when his parts setup dereferences them. Dumped at
  the entry of the failing call, Purin's container reads
  `0x37080a 0x380000 0x3a0934 0 0x3b1028 0x3c0a2c 0xaa1154 0`, and the
  abort's `r4` is that first word verbatim: the loop loads `dobjdesc->id`
  from 0x37080a and takes the data abort
  (`2026-09-02_purin-container.txt`).
- **The control shows the same words becoming pointers.** Mario's container
  reads `0x5a0880 0x5b0000 0x5d09a0 0` at his first call and
  `0x22fcc30 0x22faa30 0x22fd0b0 0` at every later one, while his second
  detail block is already `0x22fefc0 0x22fd420 0x22ff440` at the first
  (`2026-09-02_mario-container.txt`). So these are tokens a fixup pass
  rewrites in place; Mario's lands in time and both of Purin's blocks are
  still raw when the loop runs. The ids they name (0x37, 0x38, 0x3a, 0x3b,
  0x3c, 0xaa; Mario 0x5a, 0x5b, 0x5d) are not file ids in the production
  manifest, so the encoding has to be read from the fixup pass itself.
- **The container is built in RAM, not loaded.** None of the observed words
  (0x37080a, 0x3a0934, 0x3b1028, 0xaa1154) appears in any packed reloc
  file, in either byte order, and PurinModel's own internal fixup chain
  walks cleanly for all 308 of its slots. So the words are not an
  unrelocated file image: they are what the container holds before
  something populates it. Mario's identical-looking words at his first call
  become real pointers by his second, so that population is a runtime step
  that runs for him and never runs for Purin.
- **CAUSE (2026-09-02): PurinMain's external pointer fixups fail.**
  `gNdsRelocExternalFixupFailCount=1`, first and last failing asset
  **233 (0xe9, PurinMain)**, with the dependency id unset -- so the pass
  bails before resolving a single cross-file pointer, and
  `attr->commonparts_container` keeps the raw chain word the loop then
  dereferences (`2026-09-02_purin-externfail.txt`). His internal chains are
  fine: PurinModel 308 fixups, ShieldPose 144, MainMotion 37, Special2 38,
  Main 13, over 23 files in all.
- **This was invisible by construction.** `ndsRelocRecordExternalFixupFail`
  only counted Pupupu-stage and Mario/Fox assets, so every other fighter
  could fail every cross-file pointer in silence. It now records a generic
  count with the first failing asset and dependency, which is what turned a
  three-frames-away data abort into a named file.
- **Exact failing line, with its call chain** (`2026-09-02_purin-failsite.txt`):
  `ndsRelocApplyExternalPointerFixups` bails at the `extern_count == 0`
  guard for asset 233, reached from `ndsRelocFinalizeLoadedFile` <-
  `ndsRelocLoadExternTreeAsset(233)` <- `lbRelocGetExternHeapFile
  (llPurinMainFileID)` <- `ftManagerSetupFilesMainKind` <-
  `ftManagerSetupFilesAllKind(fkind=10)`. So the file's extern chain head
  is present (the pass got past the 0xffff early-out) while its extern id
  table is empty, and every cross-file pointer stays raw.
- The packed data is not at fault: PurinMain declares 41 extern ids and a
  1,984-byte payload, and the generated payload-size rows match every one
  of his five core files exactly (checked against the file headers).
  `ndsRelocAssetLoadDataAndExternIDs` also returns the count correctly.
- So the registration that left `extern_count` at 0 is an EARLIER load of
  the same file: `ndsRelocLoadExternTreeAsset` returns early when the asset
  is already loaded, and only finalizes it. Next: break on
  `ndsRelocRegisterLoadedFileImpl` for asset 233 and find which path
  registers PurinMain first -- the ones using the `ndsRelocRegisterLoadedFile`
  wrapper pass no known extern table at all.

- **Measurement caveat:** the tree at build time carried another agent's
  uncommitted P2-3f47 work (owner-image-size arms for the five new owners, and
  a block of Ness admission-witness globals that says it is chasing an
  unassigned admission failure). Isolate on a clean checkout of `1fa52c906f9`
  before attributing the abort to Purin.

## Acceptance

- [ ] Move inventory sweep vs `ftpurin` data.
- [ ] Rest frame-1 KO reproduces (replay-verified); Sing radius/duration
      equivalent.
- [ ] Multi-jump count/decay equivalent.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.

## Static evidence exhausted 2026-09-04 -- the next step is the live read

The chain above ends at "an EARLIER load registered asset 233 with
`extern_count` 0". A full static pass over that premise ruled out every
mechanism it could name, so it is recorded here rather than attempted again.

- **The count arithmetic holds and narrows it to one path.**
  `ndsRelocRegisterLoadedFileImpl` leaves `extern_count` at 0 four ways, and
  three of them record a fixup failure (`reloc_backend_assets.c:3947`, `:3955`,
  `:3970`). Only `header->extern_file_ids_num == 0` skips the block silently
  (`:3942`). The guard bail itself records (`:5381-5383`) and leaves the
  dependency id unset, which matches the capture. Registration failure plus
  guard bail would be 2; the observed count is 1. So the registering header
  must have declared zero externs. `ndsRelocRecordExternalFixupFail`
  (`:1746-1780`) increments unconditionally, so the count can be taken at face
  value.
- **No caller can produce that header.** All nine call sites of
  `ndsRelocRegisterLoadedFile`/`Impl` were enumerated. Every main-capable path
  feeds a header parsed by the single shared reader
  (`nds_reloc_assets.c:541-599`), and the only structurally-zero header
  (`:10613-10617`, force-anim stream) is behind `ndsRelocIsFighterAnimID`,
  whose Purin range is `0x5a5..0x5e7` -- 233 is `0xe9`, far below it.
- **The file on disk is not the problem.** PurinMain's own header declares
  `n_ext=41`, `reloc_intern=0x00b0`, `reloc_extern=0x0000`, `data_size=1984`.
  Mario, Fox, Ness, Yoshi and Kirby all carry `reloc_extern=0x0000` too, so
  the chain head is not anomalous.
- **The `seen`-set capacity is not the problem either**, which was worth
  checking because `ndsRelocAddSeenAsset` (`:7715`) records a fixup failure on
  overflow and would have been a second candidate for the single count.
  Walking the extern graph offline over all 2,132 o2r files gives closures of
  6 to 12 **unique** nodes -- KirbyMain's 144 declared ids resolve to 10 files,
  because the id list repeats heavily -- against a 144-entry `seen` array. The
  walk reproduces the generated census exactly (Mario 54,048, Fox 119,040,
  Purin 72,368, Kirby 204,208, Ness 79,216, Yoshi 146,928), so both the sizing
  and the dedup are right.

**Next step, and it needs no new instrumentation.**
`ndsRelocRecordExternalFixupFail` already publishes
`gNdsRelocExternalFixupFailFirstLR` and `...LastLR` -- the return address of
whoever recorded the failure, flushed with `DC_FlushAll` so a stub read is
sound. Read those two on a booted Purin ROM and resolve them against the ELF.
If they name `:5382` the guard fired and the silent registration really
happened, and only then is a breakpoint on `ndsRelocRegisterLoadedFileImpl`
filtered to asset 233 worth the run. If they name something else, the premise
above is what is wrong.

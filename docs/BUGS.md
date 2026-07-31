AI Agent should mark fixed items with FIXED prefix.
These bugs should be fixed for P1 delivery.
-FIXED Pressing START on the Results screen does not restart the P1 match.
  Fixed 2026-07-30, exactly as this row prescribed: the manual
  `syControllerReadDeviceData(); syControllerUpdateGlobalData();` pair was
  removed from the Results branch of `src/port/taskman_seam.c`
  (`NDS_SEAM_CONTROLLER_PAIR` -> 0), `ndsPlatformReadInput()` kept, and the
  original `syTaskmanCommonTaskUpdate -> syControllerFuncRead` path left to
  sample and publish once. Nothing synthesises START and
  `mnVSResultsCheckExit` is untouched.
  A second defect had to be fixed first for that to work: the port bypassed the
  source's once-per-read publish interlock from nine call sites, so
  `sSYControllerIsUpdateData` was never consulted and 1,388 dead second-publishes
  ran per Results screen, zeroing `button_tap`. Restored in
  `src/import/battleship_sys_controller.c`.
  Proof obtained on the canonical mode-163 ROM: `InputTapMask & 0x1000`,
  `RematchCount == 1`, and the restarted match reaches gameplay at 28.9 FPS.
  One held press does not fire twice.
  NOT COVERED BY THIS ROW, tracked below: the restarted match is drawn wrong.
  The input path and the redirect are correct; what follows them is a separate
  second-scene-entry defect.
  Original research (2026-07-30):
  - Source contract: after the normal-result wait reaches 410 Results ticks,
    `mnVSResultsCheckExit` accepts only a `START_BUTTON` rising edge from
    `gSYControllerDevices[].button_tap`. The existing
    `ndsMNVSResultsSetLoadScene` redirect already reseeds canonical mode 163 and
    sends that exit back to `nSCKindVSBattle`; do not replace it.
  - Root cause: Results runs two controller pipelines. `syTaskmanRunTask` first
    calls `syControllerReadDeviceData` + `syControllerUpdateGlobalData`, whose
    publish exposes START and drains the edge accumulator. Its subsequent
    `task_update` calls BattleShip's live `syControllerFuncRead`; the controller
    thread reads the same held key and publishes the now-empty accumulator, so
    `button_tap` becomes zero before `mnVSResultsCheckExit` while `button_hold`
    correctly remains `0x1000`. This also explains why the earlier seam-pair
    experiment was inconclusive: it ran before the public publish interlock and
    another duplicate publish was still active.
  - Fix at the owning seam: remove the manual
    `syControllerReadDeviceData(); syControllerUpdateGlobalData();` pair from
    the Results branch of `src/port/taskman_seam.c`. Keep
    `ndsPlatformReadInput()` so libnds latches the keypad, then let the original
    `syTaskmanCommonTaskUpdate -> syControllerFuncRead` path sample/publish once
    immediately before the scene update. Do not synthesize START, change
    `mnVSResultsCheckExit`, or add another input bridge.
  - The known rematch-lifetime cache hazard is already covered:
    `ndsDevSceneHarnessApply` restores the canonical one-minute
    Mario-vs-level-3-Fox state, and the animation cache invalidates itself when
    the taskman heap rewinds (`ndsR2AnimCacheArenaStillOwned`).
  - Required proof: on the exact canonical mode-163 ROM, finish a natural match,
    wait past tick 410, press and release START once for about 500 ms, and prove
    `InputTapMask & 0x1000`, `RematchCount == 1`, and a reset match reaches
    countdown/GO. Complete the second match without hang/corruption, confirm one
    held press does not trigger twice, run Boundary, then remove the temporary
    `gcRunAll`/controller telemetry. Status: FIX IDENTIFIED, NOT YET QUALIFIED.
-The rematched match is drawn wrong: duplicated fighters over corrupted stage
  and background geometry. Split out of the START row above 2026-07-30, because
  that row's input fix is done and this is a different defect.
  Symptom: START on Results restarts the match, and the second match runs at a
  healthy 28.9 FPS with FTR 385,728 -- so this is VISUAL, not performance. Do
  not re-open it as a slow/re-warm problem; that line is measured dead.
  Shape: it is not rematch-specific. It is any SECOND entry into a scene, which
  is why Sudden Death shows the same thing (`scVSBattleStartSuddenDeath`
  re-enters nSCKindVSBattle on the same scene kind).
  Refuted, with counters, so nobody repeats them: heap not rewound
  (`AdapterCount` 2 proves it is); recursion through `scVSBattleStartScene`
  (`scmanager.c:870` is a flat loop); same-kind reloc cache staleness
  (`gNdsRelocSceneReentryEvictCount` stayed 0 -- rematch is VSResults->VSBattle,
  different kinds, so the old kind-only guard already evicted); and three
  earlier hypotheses withdrawn in e46340eda for comparing per-frame tick-HUD
  brackets against a run average.
  Landed and verified to fire, but NOT shown to fix the picture:
  `ndsIFCommonNativeOamDiscardTextures()`. `ndsIFCommonNativeOamInit()` runs once
  at boot and was the only thing clearing `sNdsIFCommonCloudTextureNames[]` /
  `sNdsIFCommonTrafficTextureName`, while
  `ndsRendererHardwareDiscardBattleStaticTextures()` runs on every scene change
  and drops the VRAM behind them, so `ndsIFCommonNativeOamPrepareClouds()`
  early-returned on stale names and never re-uploaded.
  `gNdsIFCommonNativeOamTextureDiscardCount` reads 1. Same class as the prepare
  latch beside it; look for further boot-scoped state guarding scene-scoped
  resources.
  Do NOT retry the reboot workaround. Restarting the ARM9 instead of
  re-entering the scene was implemented and measured dead: calico links no
  `svcSoftReset`, bare BIOS `swi #0` is a no-op here (a soak left
  `gNdsVSResultsRematchCount` at 1, which a real .bss re-zero would have
  cleared), and `crt0Startup` needs three loader-supplied arguments that no
  longer exist by Results time. Reviving it is a crt0 change, not a scene one.
  Status: OPEN.
-"Time Up" VFX and SFX after match countdown finished.
  Research (2026-07-30, Sol Max match-end/audio):
  - Source contract: `ifcommon.c` creates six blue mixed-width letter sprites
    for `TIME UP`, keeps them for 90 ticks, and queues announcer FGM 527
    (`decomp/BattleShip-main/decomp/src/if/ifcommon.c:1965-1979,
    2244-2252,3262-3285`).
  - Root cause: the live non-HUD SObj compositor is already the right path, but
    its mixed-width normalization manifest
    (`src/port/reloc_backend_assets.c:836-873`) stops after countdown/GO and
    omits all six Time Up images. The source offsets and shapes are T
    `0xE4A8` 36x56/3 bitmaps, I `0xF740` 17x57/2, M `0x127E0` 50x56/4, E
    `0x144E0` 32x56/2, U `0x16EB8` 41x58/3, and P `0x18FE8` 36x56/3.
    FGM 527 is also absent from the selector/allowlist in
    `src/nds/nds_audio_fgm.c:182-252`.
  - Proposed fix: add those six exact descriptors to the existing manifest and
    pin them in `scripts/check_ifcommon_hybrid_oam.py`; add source cue 527 to
    the existing FGM selector, pack, allowlist, and checker. No new renderer or
    audio path is needed.
  - Required proof: let canonical mode 163 expire naturally; show all six
    letters for the source 90-tick lifetime, prove cue 527 reaches ARM7/a
    channel, retain a synchronized screenshot, and obtain owner visual/listen
    approval plus Boundary verification. Status: OPEN.
  - Verified 2026-07-30, so the next attempt does not re-derive it:
    * The letters are `llIFCommonGameStatusBlueLetter{T,I,M,E,U,P}Sprite` from
      `gGMCommonFiles[1]` (`ifcommon.c:143-151, 2244-2252`), which IS
      `NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS` -- the same asset the countdown
      and GO descriptors already use, so no new asset is involved.
    * All six offsets are ALREADY declared in `include/reloc_data.h:274-281`
      and match this row's research exactly (T 0xe4a8, I 0xf740, M 0x127e0,
      E 0x144e0, P 0x18fe8, U 0x16eb8). Nothing needs adding there.
    * `sobj->sprite.attr = SP_TEXSHUF | SP_TRANSPARENT` for all of them
      (`ifcommon.c:1974`), set by the shared `ifCommonAnnounceSetAttr`.
    * BONUS, and it changes the sizing of the Results row below: GAME SET draws
      from the SAME blue-letter set (`dIFCommonAnnounceGameSetSpriteData`,
      `ifcommon.c:155+`, letters G/A/M/E/S/...), and A 0x1de68, G 0x20788 and
      S 0x1b5f8 are declared alongside. M and E are shared with TIME UP, so
      normalizing this family serves both announcements. Do them together.
    * STILL MISSING and the actual blocker: the `bmfmt`/`bmsiz`/bitmap-count
      fields for each descriptor in `sNdsBattleInterfaceSpriteDescs`
      (`src/port/reloc_backend_assets.c:841+`). This row's widths, heights and
      bitmap counts are recorded but not the format, and it CANNOT be inferred
      from the offset deltas -- the letters are not contiguous in the asset
      (I->M spans 12,448 bytes for a 17x57 two-bitmap sprite). Read the real
      `Sprite` records out of the loaded asset rather than guessing; a wrong
      format normalizes to corrupt pixels rather than to an error.
  - Reading those formats was ATTEMPTED 2026-07-30 and is not as simple as it
    looks. Recorded so the next attempt starts past the two dead ends:
    * The method itself is sound. `soak-freeze-watch.ps1`'s field list takes
      arbitrary GDB expressions, and
      `((Sprite *)((char *)gGMCommonFiles[1] + 0xe4a8))->bmfmt` resolves --
      the `Sprite` type is in the debug info, no new tooling needed.
    * WRONG ROM, first attempt. On `smash64ds-results-lab-hwtri` every field
      came back garbage (`nbitmaps` 8260, `height` -8062, `bmfmt` 60).
      `gGMCommonFiles[1]` is not the IFCommonGameStatus asset on a
      Results-only boot; the index is battle-context. Do not read these off
      the Results lab ROM.
    * WHOLE READ LOST, second attempt. On the battle tick-HUD ROM the run
      produced no `CLEAN=` line at all -- one bad expression aborts the entire
      printf, which that harness documents as deliberate ("one missing symbol
      fails its whole command"). So a speculative typed read costs the run's
      other sixty counters too.
    * Next attempt: dump raw words (`x/8xw`) at the offsets instead of typed
      field access, or read them in a dedicated one-shot GDB script rather
      than inside the shared counter read, and confirm `gGMCommonFiles[1]` is
      non-NULL before dereferencing it.
-Results screen. VFX and SFX/BGM/FGM.
  Research (2026-07-30, Sol Max match-end/audio):
  - Source contract: the Results sequence queues PublicWin 621 at scene start,
    WinnerIs 534 at tick 81, Mario 499 or Fox 486 at tick 210, and
    PublicExcited 626 at tick 270. It starts the winner fanfare, transitions to
    looping Results BGM, and creates two confetti instances from particle
    script 112 at tick 120
    (`decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c`).
  - Current split: the core Results scene and Mario/Fox fanfare plus Results
    loop entries already exist (`src/nds/nds_audio_bgm.c:41-85`). FGM 626 is
    packed, but 621/534/499/486 are absent from
    `src/nds/nds_audio_fgm.c:182-252`. Confetti cannot appear while
    `lbParticleMakeScriptID` returns NULL
    (`src/port/reloc_backend_compat_shims.c:12963-12972`).
  - Preserved WIP commits
    `17e6cd0b301fe77402f352ec0c96cd6fc6ce19b7` (generator/checker) and
    `ef5d25268678070a6f1dd452ecbb0e1ad9194519` (runtime) are useful review
    inputs, not fixes: neither has been built or runtime-qualified, and the
    generator does not include Results script 112.
  - Proposed fix: complete the shared particle work with script 112 and its
    exact texture closure; add only source cues 621/534/499/486 to the existing
    FGM path and retain 626. Leave the working BGM seam alone unless a measured
    natural Results run exposes a BGM defect.
  - Required proof: natural match end with source-order cue/channel evidence,
    both tick-120 confetti instances, fanfare-to-loop transition, screenshots,
    owner visual/listen approval, and pacing after the missing content is live.
    Status: OPEN.
-Crowd noise is missing from results and match gameplay.
  Research (2026-07-30, Sol Max match-end/audio):
  - Source contract: `decomp/BattleShip-main/decomp/src/ft/ftpublic.c` owns the
    crowd actor, thresholds, cooldowns, repeat limits, and event queue. The
    reachable Mario/Fox set includes chants 609/605, reactions 615-625, Results
    win cue 621, and PublicExcited 626.
  - Root cause: only the isolated 626 path is currently packed. Reactive crowd
    behavior is structurally absent: `ftPublicCommonCheck` is diagnostic-only
    (`src/port/reloc_backend_compat_shims.c:6530-6548`),
    `ftPublicMakeActor` only marks bits (`:12768-12772`), the battle queue is a
    no-op (`src/port/battle_playable_compat_stubs.c:216-219`), and the original
    `ftpublic.c` is not in the forced source list (`Makefile:1150-1163`).
  - Proposed fix: import the source actor/state machine, or the smallest
    mechanically equivalent DS translation, at that shared seam. Preserve its
    thresholds/cooldowns/repeats/queue ordering and add only reachable source
    cues 605, 609, 615-625, and 621 to the existing FGM backend; retain 626.
  - Required proof: natural gameplay reactions and Results win events with
    event-to-ID-to-ARM7/channel traces, source timing/queue guards, and owner
    listen approval. A marker or packed cue alone is not completion. Status:
    OPEN.
-Sudden Death has FPS, freezing, and animation issues. (Enable Mario CPU input
  for a CPU-vs-CPU tie and natural Sudden Death transition.)
  Current diagnosis (2026-07-30):
  - Source contract: a tie starts a complete second battle scene with stock
    rules, 300% damage, no entry sequence, the Sudden Death interface,
    announcer cue 514, and GO
    (`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:404-501`).
  - FIXED IN SOURCE, not yet qualified or published: commit `e72fad988` adds the
    missing `scVSBattleStartSuddenDeath` DS wrapper and remaps the second
    `func_start` through it (`src/import/battleship_scvsbattle.c:93-116,
    210-235`). It restores static battle textures, native OAM clouds, and the R2
    warm-list cursor; `gNdsSCVSBattleSuddenDeathPrepareCount` proves that path.
    The current root battle ROM predates the commit: it was built at 05:18 with
    SHA-256 `5B6E82A8B8CA8D8AC903EBE30FFACF0F8E60BFD3A4A94C60A0C9182B3B1D0CCD`,
    while `e72fad988` landed at 13:14. The latest linked both-CPU diagnostic ROM
    found during this audit was also older (13:07). No examined runnable ROM
    contains this source fix.
  - The wrapper is only half the runtime fix. Every battle entry calls
    `syTaskmanStartTask`, which reinitializes `gSYTaskmanGeneralHeap` over the
    same arena (`decomp/BattleShip-main/decomp/src/sys/taskman.c:1240-1295`).
    The R2 animation cache keeps its arena pointer and payload entries across
    that rewind. `ndsR2AnimCacheArenaStillOwned` considers the old block valid
    whenever its end is at or below the CURRENT heap cursor
    (`src/port/reloc_backend_assets.c:5854-5871`). New-scene allocations can
    advance the cursor past the reclaimed block before the first cache lookup,
    so this test can false-positive and hand back pointers into reused memory.
  - Correction to the earlier refutation: putting that predicate on the read
    path does not prove stale hits impossible. Sudden Death's base start creates
    fighters and selects Wait/Fall status before the wrapper returns
    (`ftmanager.c:867-899` -> `mpcommon.c:870-879` -> `ftMainSetStatus`). That
    path reaches `ndsR2AnimCacheFind` before the wrapper calls
    `ndsR2AnimCachePreloadMatch`, and a hit copies the retained payload directly
    into the fighter figatree heap
    (`src/port/reloc_backend_assets.c:6200-6225`). The current preload function
    only rewinds a warm-list cursor; it does not invalidate cache ownership.
  - Runtime evidence matches that failure. The exact `cb62fa2` both-CPU capture
    `artifacts/verification/freeze-soak/2026-07-30_125758-FROZEN-PICTURE.txt`
    shows visibly corrupted Mario/scene geometry, then a confirmed self-branch
    in `syTaskmanCheckBufferLengths` at BattleShip `taskman.c:338`: a display-list
    buffer overflow. Counters were `ANIMARENA=87824,0`,
    `TASKARENA=1273856,25`, and `MALLOCOVF=0`. This is not the historical
    `syMallocSet` heap-exhaustion freeze and not the older finite renderer `cmp`
    sample.
  - START-to-rematch widens the bug to every SECOND battle entry. The measured
    run had match 1 at 2,043 presented frames / ~15 FPS / 3,728 arena bytes and
    match 2 at 423 frames / ~4.7 FPS / 87,824 arena bytes; match two never
    reached Results. Static battle textures are refuted for that run
    (`PrepareCount=2`, `ViolationCount=0`), and
    `gNdsSCVSBattleLifecycleArenaAdapterCount=2` proves the second heap
    initialization occurred. Those facts do not refute the cursor
    false-positive above.
  - The reproduction lane had an independent configuration bug, now FIXED
    (2026-07-30). `scripts/soak-freeze-watch.ps1` rebuilt the default
    `build-r2-bothcpu` without passing `NDS_R2_BOTH_CPU=1`, and the Makefile
    default is 0, so the directory said both-CPU while its generated
    `nds_build_config.h` said `NDS_R2_BOTH_CPU 0`. Every soak run that way had a
    human player on P1, never created the CPU-vs-CPU tie, and therefore never
    entered Sudden Death -- while looking exactly like a clean run. The script
    now takes `-BothCpu` (default ON, matching the default build), passes it to
    `make`, and then READS THE GENERATED HEADER BACK and throws if the ROM is not
    the configuration that was asked for. Naming a build directory after a flag
    is not the same as setting it, and only the header knows which happened.
  - RUN 2026-07-30 with the fix, and it changes what is blocking. The soak built
    both-CPU (header readback passed), published no controller input at all
    (`gNdsControllerReadCount 0`, `gNdsControllerPublishSuppressedCount 1516`),
    ran a full CPU-vs-CPU minute, and reached Results --
    `gNdsVSResultsStartCount 1`. But `gNdsSCVSBattleSuddenDeathPrepareCount`
    is still **0**: the match ended DECISIVELY, so there was no tie and no Sudden
    Death. The flag fix was necessary and is not sufficient. **A passive soak
    cannot force a tie**, so Sudden Death has no deterministic entry today and
    every "Sudden Death soak" before this one was doubly incapable of reaching
    it.
  - **REPRODUCED ON DEMAND, 2026-07-30, and the seeded-mode plan below was
    WITHDRAWN as the wrong shape.** `scripts/capture-sudden-death-entry.ps1`
    reaches Sudden Death every run in about ninety seconds, with **no harness
    mode, no new flag and no source change**. Sudden Death is not a distinct
    scene kind: `scVSBattleStartScene` (`scvsbattle.c:513`) runs the match to
    completion in a BLOCKING `scManagerFuncUpdate`, asks
    `scVSBattleSetScoreCheckSuddenDeath` (`:228`), and only on TRUE calls
    `scManagerFuncUpdate` a second time with `func_start` remapped. A mode that
    "boots into Sudden Death" would therefore have to fork that control-flow
    function to skip the first match -- and then the thing being reproduced is
    the fork, not the bug.
    What the lane does instead: the decision is `tko = score - falls`, so **a
    match in which nobody scores is a 0-0 tie**, the most ordinary tie the game
    has. The lane runs the canonical mode-163 match with the Fox CPU LIVE, drives
    no input, and writes `gSCManagerBattleState->time_remain` once mid-match to
    end it in two seconds -- too short for either fighter to score. The source
    then declares its own tie. Scores are never touched; every run so far reports
    `0-0 vs 0-0`. Do NOT try to guarantee the tie by pausing the Fox CPU:
    `gNdsBattlePlayableFoxCpuEnabled = 0` is a fast-iteration switch that also
    holds `sIFCommonTimerIsStarted = FALSE` and freezes the tic source
    (`battleship_ifcommon.c:95-134`), so the clock never runs out and no tie is
    ever evaluated.
  - **MEASURED at the reproduction (2026-07-30), and the heap is implicated.**
    The arena IS rewound for Sudden Death. Its setup pass consumes **1,226,768
    bytes and leaves only 42,992 free**.
    `gNdsTaskmanArenaAllocFailCount` reads 26 on both sides of
    the setup, so those failures belong to match one, not to Sudden Death.
  - **THE "119 KB MORE THAN MATCH ONE" CLAIM IS WITHDRAWN (2026-07-31).** It was
    `1,226,768 - 1,107,392`, and those are not the same quantity. 1,226,768 is a
    measured high-water on this build; 1,107,392 came from the arena-SIZING work
    on the board (`P1_EXECUTION_BOARD.md:198`), where it is "990,640 used plus a
    116,752 request" -- a peak-demand figure, computed for a stress config that
    that same paragraph describes as **192 KiB poorer**. A measured high-water
    minus a derived peak demand from a different configuration is not a delta.
  - **What the allocation ledger actually measured** (item 3, per-caller by
    caller LR, `NDS_R2_SECOND_ENTRY_DIAG=1`, run `2026-07-31_010341`). Both
    entries start their setup from the **identical rewound baseline of 319,968
    bytes** -- measured directly at `scVSBattleStartBattle` and at
    `scVSBattleStartSuddenDeath`, not inferred. From there:
    - match one, setup plus a full minute of runtime: **925,816 B** -> 1,245,784
    - Sudden Death setup: **906,568 B** -> 1,226,768
    So Sudden Death's arena footprint is **19,248 bytes LOWER** than match one's,
    and there is no excess to explain. Per caller it is stronger than that: 31
    callers, overflow 0, no new caller sites, and 22 of them allocate a
    byte-identical amount on both entries. The remaining rows differ only by
    match one's *runtime* allocations, which Sudden Death has not made yet.
  - **METHOD, and it cost two runs.** `capture-sudden-death-entry.ps1` rebuilds
    the ROM on every invocation, and its build line did not pass
    `NDS_R2_SECOND_ENTRY_DIAG=1`. So a hand-built diag ELF was silently
    overwritten by the next run, the ledger symbols stopped existing, and the
    gdb script kept reading them. **A gdb command file aborts on the first
    command that errors -- silently, leaving a bare `(gdb) ` prompt** -- so one
    dead symbol destroyed every proof after it and two consecutive runs reached
    `battle-start` and printed nothing. It reads exactly like an emulator hang.
    Fixed at the seam: the harness now takes `-SecondEntryDiag`, puts the flag on
    the make line, verifies it in the generated config header the same way
    `NDS_R2_BOTH_CPU` is verified, and **strips every diag-only read from the
    script when the flag is off**, so a printf can never outlive its symbol.
    If a lane ever prints nothing after an early stage, check the ELF exports
    before suspecting the emulator: `arm-none-eabi-nm <elf> | grep <symbol>`.
  - **The ledger's blind spot is now measured, not assumed.** taskman.c's
    intra-TU `syTaskmanMalloc` calls bypass the wrapper. Heap consumed across the
    Sudden Death setup is `1,226,768 - 319,968 = 906,800`; the ledger saw
    906,568. The gap is **232 bytes**, so the wrapper sees essentially
    everything, and the delta above is safe to quote. Do not quote the ledger as
    an absolute total regardless -- see `battleship_sys_taskman.c`.
  - **`M1BASE` also killed a cumulative-window trap before it published.** The
    ledger is cumulative from boot, so "every caller exactly doubled" only means
    `sd = boot + m1`, which equals `m1` alone if boot is zero. It is:
    `SD-LEDGER-M1BASE-TOTAL=0`. Measuring that was the difference between a
    proof and a coincidence, and the same care is owed to any other counter read
    at two points.
  - **ROOT-CAUSE CANDIDATE, first hard one: `frame_draw_last` is `0xFF` on
    every live stage GObj on the second entry (2026-07-31).** Log
    `2026-07-31_020601`. Eight words read from each of the five unreplayed
    segments at the **same** stop on both entries (`scVSBattleFuncUpdate`):
    ```
    entry 1   0x01000401 0x01000401 0x01000401 0x01001001 0x01000602
    entry 2   0x01ff0401 0x01ff0401 0x01ff0401 0x01ff1001 0x01ff0602
    ```
    `struct GObj` (`decomp/src/sys/objtypes.h:188`) puts four `u8` at +0x0C:
    `link_id`, `dl_link_id`, `frame_draw_last` ("Last frame drawn?"),
    `obj_kind`. Decoding little-endian, `link_id`, `dl_link_id` and `obj_kind`
    are **identical** on both entries, as are both link pointers and both
    trailing pointers. The single differing field, on **all five** segments, is
    **`frame_draw_last`: `0x00` -> `0xFF`**.
    **The sampling confound was killed before this was written.** The first read
    took entry one at `ifCommonTimerFuncRun` and entry two at
    `scVSBattleFuncUpdate`; `frame_draw_last` legitimately varies *within* a
    frame, so that pair could not distinguish a scene-entry difference from a
    stop-point difference. Re-read at the same function, the difference stands.
    **What is NOT yet established:** the causal chain from `0xFF` to the visible
    corruption and to `STG` 2.21x. `0xFF` is a plausible unset/-1 sentinel and
    the field gates per-frame draw admission, so a stale one could plausibly
    admit or reject segments wrongly — but that is a hypothesis, not a
    measurement, and this bug has already retired three causes that were written
    the moment they looked obvious.
  - **The field's own code, from `decomp/` (read-only reference).**
    - init, `sys/objman.c:1892` in the display-add path:
      `gobj->frame_draw_last = dSYTaskmanFrameCount - 1;`
    - per-draw write, `sys/objdisplay.c:3188`:
      `current_gobj->frame_draw_last = dSYTaskmanFrameCount;`
    - reads, `sys/objdisplay.c:403`, `:452`, and — the important one — `:1161`:
      `proc = (frame_draw_last != (u8)dSYTaskmanFrameCount) ? proc_diff : proc_same;`
    So the field **selects between two different matrix procedures**, which is a
    mechanism that can produce both wrong transforms and a different cost. That
    makes it a credible owner of "scrambled stage" *and* of `STG` 2.21x, which
    no previous candidate managed to explain together.
    **MEASURED, and it refuted my own arithmetic (log `2026-07-31_021151`).**
    The prediction was that the counter would read **1** at the first setup and
    **0** at the second. It reads **0 at BOTH** (`dSYTaskmanUpdateCount` 0 at
    both as well). So the "different phase at creation" story is **WRONG** and
    is withdrawn. What the same reading establishes instead is stronger:
    - `0 - 1 = 0xFF`, so **both** entries create these GObjs with `0xFF`. Entry
      one's `0x00` is therefore NOT the creation value — it is written later, by
      the per-draw writer at `objdisplay.c:3188` (`frame_draw_last =
      dSYTaskmanFrameCount`, with the counter 0). My earlier note that these
      GObjs "never reach `:3188`" was right for entry two and **wrong for entry
      one**; corrected here.
    - The counter reads 0 at `battle-start` and **still 0 at `entered`**, an
      entire match later. The port increments `dSYTaskmanFrameCount` at four
      sites in `taskman_seam.c` and never resets it, so those sites do not run
      in the battle loop: **the counter is 0 throughout battle gameplay.**
    **The comparison therefore collapses to `frame_draw_last != 0`**, and the
    two entries land on opposite sides of it:
    ```
    entry 1  field 0x00  ==  counter 0   ->  proc_same
    entry 2  field 0xFF  !=  counter 0   ->  proc_diff
    ```
    That is a complete mechanism from the measured field value to a different
    matrix procedure for the whole stage, and it is consistent with every
    measurement in this row. **Still not measured:** that `proc_diff` is what
    the screen shows. The open question is now narrow and concrete — **what runs
    on the first entry that drives these five GObjs through `objdisplay.c:3188`,
    and why the second entry never does.** Note the decomp's `- 1` seed assumes
    a counter that advances; in a port where it never does, `0xFF` is
    permanently "different frame" and only a real generic-path draw corrects it.
  - **The five unreplayed segments are all PRESENT on both entries
    (2026-07-31).** Read at two known-good stops in the Sudden Death lane,
    `timer-live` (entry one) and `running` (entry two), log
    `2026-07-31_015935`:
    ```
    entry 1  map_gobj = 0x2369410, 0x23697d8, 0x2369e10, 0x236a470  layer1 = 0x2368658
    entry 2  map_gobj = 0x2369410, 0x23697d8, 0x2369e10, 0x236a470  layer1 = 0x2368658
    ```
    **Read this correctly.** The identical ADDRESSES prove nothing — the taskman
    heap is a bump allocator rewound between scenes, so the same allocation
    order necessarily yields the same addresses. What the reading does establish
    is the **null/non-null pattern**: all five segments exist on entry two, none
    is missing and none has been replaced by NULL. So the corruption is not a
    lost or unbuilt segment; it is in the DATA those five reach. The next probe
    must dereference — DObj chain, vertex pointers, matrices — and compare
    CONTENT. Deref commands belong at the `running` stage only: it is last, so a
    faulting one cannot abort the command file before the rest is read.
  - **MECHANISM 10 ELIMINATED — the stage direct path does NOT drop out on the
    second entry (2026-07-31).** A third explanation for the 2.21x was that the
    stage's direct path stops engaging on re-entry and the generic renderer
    takes over, which would raise cost AND change what is drawn. **Refuted by
    measurement, no build spent**, on a rematch soak whose match two was
    confirmed corrupt in the same run (`STG` **378,880**, `ALL` 1,679,936,
    `artifacts/verification/freeze-soak/2026-07-31_015424-NO-FREEZE.png` — the
    signature reproduces to the same tick bucket as the earlier pair):
    - `gNdsR2StagePrepareBuildCount` **2** — exactly one build per scene entry
    - `gNdsR2StagePrepareReuseCount` **2240** — reuse dominates 1120:1
    - `gNdsR2StagePreflightElideCount` **11200** — exactly 5x reuse, which
      R2-02 E8's own comment defines as the elision working ("five per frame is
      the elision working, zero is a flag that compiled but never fired")
    - `gNdsRendererTask36ReplayArenaStaleCount` **2242** — read its definition
      before alarm: it counts frames the RELAXED guard admits that the legacy
      STRICT guard would have blocked, i.e. "proof that the fix is doing its
      job" (`nds_renderer.c:4461`). ~Once per frame is the DESIGNED healthy
      value, not an anomaly.
    So the fast path is fully engaged while the stage renders wrong. The defect
    is inside the direct path's data, not in whether that path runs.
  - **NEXT HYPOTHESIS, and the probe that did NOT settle it (2026-07-31).**
    `STG` is **2.21x**, and a doubling has two very different explanations: the
    stage SUBMITTED TWICE (duplicate GObjs surviving teardown, which would also
    explain overlapping scrambled geometry) versus the same geometry drawn with
    wrong bindings. Nine binding-side mechanisms already measured clean, so
    duplication is the better-supported reading and is what to test first.
    **Attempted and REMOVED:** reading the geometry engine's own population
    registers (`0x04000604` polygon, `0x04000606` vertex) in the matched-capture
    arms. Both read **0** — at the camera-proc stop AND at `ndsPlatformEndFrame`
    — alongside `STG=0`, so neither point samples a frame whose geometry is
    still resident (presumably cleared by the buffer swap). Removed rather than
    left in as an unverified diagnostic. **A working version needs a stop proven
    to be inside a drawn frame before the swap; validate it by first getting a
    NON-zero `STG` there**, since `STG` failed together with the registers and
    is the cheaper canary.
  - **REMATCH SECOND ENTRY REPRODUCED WITH A MATCHED SCREENSHOT PAIR
    (2026-07-31), and the fixes so far do NOT close it.** Two soak runs on the
    same ROM, same build, same configuration, differing only in when START was
    pressed. Both frames carry the tick HUD, so the picture and the counters
    come from the same instant.
    - `artifacts/visibility/2026-07-31_second-entry-A-match1-clean.png` --
      match one. `STG` **171,328**, `ALL` **1,119,808**, 29.9 FPS, coherent
      Dream Land.
    - `artifacts/visibility/2026-07-31_second-entry-B-rematch-corrupt.png` --
      match two, reached through the Results START redirect. `STG` **378,880**,
      `ALL` **1,679,936**, 20.0 FPS, stage geometry visibly scrambled.
    `STG` is **2.21x** across the pair, which is the signature already recorded
    below (173,568 -> 383,296) and means this needs no screenshot to detect.
    **What match two gets RIGHT**, so these are not the defect: "GO!" is
    reached, `TIME 01:00` is fresh, both fighters exist, and `P1 [MARIO] DMG 0%
    STOCK x1` / `CPU L3 [FOX] DMG 0% STOCK x1` are correct. The heap generation
    and OAM discard fixes both engaged on this run
    (`gNdsIFCommonNativeOamTextureDiscardCount=2`,
    `gNdsRelocSceneReentryEvictCount=1`,
    `gNdsRendererBattleStaticTexturePrepareCount=2` with **0** violations).
    So the stale-texture-name class is fixed and is NOT what this frame shows.
  - **A second, SEPARATE failure on that run: the ARM9 took an ABORT.**
    `cpsr=0x400000b7` -> mode 0x17, ABORT. `pc=0x2000e1e` executing `movs r0,r0`
    with a garbage backtrace (`#1 0x00000e9a`), so control reached a near-null
    address. `MALLOCOVF=0`, so this is NOT the arena-overflow class and not the
    `while (TRUE);` spin class -- the soak's own PC check correctly downgraded
    it. The last valid user-mode link register is
    `lr_usr=0x208d319 = ifCommonBattlePauseMakeInterface+68`, which the
    disassembly puts as the return address of the `bl gcAddGObjDisplay` at
    `0x208d314`. START had been pressed four times, so a press after the rematch
    landed inside match two and opened the pause interface.
    **NOT YET ESTABLISHED, do not repeat it as fact:** that this is
    second-entry-specific. The match-one arm (one press at t+45 s) came back
    NO-FREEZE, but its final frame shows `TIME 00:34` and `DMG 41%` -- the clock
    advanced, so that press did not demonstrably build the pause interface at
    all. The discriminator has to prove the match-one pause interface was
    CONSTRUCTED before its clean result means anything. Until then this is one
    observation, not an attribution.
    Every file-scope static holding a taskman-heap pointer or a VRAM handle was
    enumerated and each of the five named categories was checked for whether it
    is scene-scoped AND stale. Nothing was blanket-reset.
    - animation cache (`sNdsR2AnimCacheArena`): WAS stale, fixed by the heap
      GENERATION contract (item 1). Engaged on the run above:
      `SD-HEAP-GEN=2`, `SD-CACHE-GEN-MISMATCH=1`.
    - native OAM texture names (`sNdsIFCommonCloudTextureNames`,
      `sNdsIFCommonTrafficTextureName`): WAS stale -- cleared only in
      `ndsIFCommonNativeOamInit`, which runs once at boot, while the VRAM behind
      them is released on every scene change. Fixed by
      `ndsIFCommonNativeOamDiscardTextures`, wired into the scene-cache eviction
      (`reloc_backend_assets.c:2162`). `sNdsTask39HitSparkGfx` is a raw
      `SPRITE_GFX + cursor` pointer with the same boot-only clear, but it is
      re-assigned by `ndsTask39PrepareHitSparks` (`nds_ifcommon_oam.c:2031`),
      which sits INSIDE the function the cloud-name early-return guards -- so
      zeroing the names re-runs it. Covered transitively; a separate reset would
      be exactly the blanket reset to avoid.
    - static-texture latch (`sNdsRendererBattleStaticTexturePrepared` / `Armed`):
      already correct, cleared by `ndsRendererHardwareDiscardBattleStaticTextures`
      (`nds_renderer.c:10640`).
    - prepared-owner state (`sNdsNativeStageValidationCache`): already correct,
      keyed on `topology_generation` + `topology_stamp` (`nds_renderer.c:21509`),
      so it self-invalidates.
    - renderer caches (`sNdsRendererTask36ReplayOwner`): already correct, reset
      by `ndsRendererTask36ReplayReset` on the same generation/stamp mismatch.
    The stall itself: Sudden Death presents exactly **two** frames
    (`gNdsFrameCounter` 311 -> 312) and then presents no more, so the picture
    freezes with a live CPU. An async interrupt during the stall landed in
    `ndsRendererAdapterBuildNativeMaterialSnapshot`
    (`reloc_backend_renderer_dl.c`, via inlined `ndsRendererAdapterMaterialFlags`)
    -- one sample, not yet proof of a loop. Evidence:
    `artifacts/verification/sudden-death/2026-07-30_21*`.
  - **CONFIRMED 2026-07-30 (owner): Sudden Death ASSIGNS THE WRONG TEXTURES,
    and it is a SECOND-ENTRY defect.** Owner: *"it kinda looks to me that it
    assigns the wrong textures to each character/element/object."* Verified with
    a same-run pair, so nothing about build, config or seed differs between the
    two frames:
    - `2026-07-30_230425-sudden-death-entry.png` — the last frame of **match 1**:
      Dream Land correct. Bark-textured Whispy trunk, three wooden platforms,
      both fighters visible and correctly textured.
    - `2026-07-30_230425-sudden-death-watch.png` — **Sudden Death**, 45 s later,
      same run: the trunk is a green blocky column wearing a foliage/grass
      texture, both side platforms wear an ornate gold frame texture that
      belongs to neither, and no fighter is visible.
    The bindings are **shifted between objects** rather than the atlas being
    corrupt — which is a different failure from a bad texture cache and points
    at whatever maps object to material on a re-entered scene.
    **Measured, and it narrows the field:**
    - **Static battle textures are HEALTHY on the second entry.**
      `PrepareCount=2` (so Sudden Death re-prepared rather than early-returning),
      `PreparedCount=24`, `ViolationCount=0`, `PrepareFailCount=0`. The
      early-return on `sNdsRendererBattleStaticTexturePrepared` was the obvious
      suspect -- `Discard` only runs after the whole scene
      (`battleship_scvsbattle.c:301`, past the blocking base start) -- and the
      counters refute it.
    - **The native OAM prepare DOES early-return**: `PrepareCount=1` across both
      entries, cloud textures 2, fails 0. That path is sprites (HUD, clouds),
      not stage geometry, so it does not explain the stage bindings, but it is a
      confirmed instance of exactly the persistent-state class item 4 names.
    - The MObj chain is clean throughout (91,482 probes, 0 invalid), so this is
      not list corruption either.
    **The stale-workspace suspect is REFUTED too (2026-07-30).**
    `gNdsR2StageTopologyRebuildCount` reads **2** across a run that reaches
    Sudden Death -- one full rebuild per scene entry -- against
    `gNdsR2StageSteadyAdmitCount` 301 for the ordinary in-match frames. So
    `binding_dobjs[]` is **freshly collected from the live tree on the second
    entry**, not re-admitted from match 1. The Task 44 steady path
    (`reloc_backend_renderer_dl.c:7520`), which reuses the whole workspace on
    nothing but `sNdsRelocStageAssetMutation`, was the obvious hazard and it is
    not firing across the boundary.
    **AND THE BINDINGS ARE CORRECT TOO -- fifth refutation, and it re-frames
    the symptom.** `gNdsR2StageMaterialRejectCount` is **0** and its latched
    index stays at its "never rejected" sentinel across a run that reaches
    Sudden Death. `ndsRendererAdapterPrepareNativeStageMaterials` checks each of
    the four fixed bindings against a fixed expected material flag word and
    returns FALSE on any mismatch; it never does. So on the second entry the
    indices `{20,22,31,32}` resolve to objects whose materials match exactly
    what the first entry saw. The tree-order hypothesis below is refuted with
    the rest.
    **Five mechanisms are now eliminated by measurement** -- static textures,
    MObj chain corruption, scene-cache eviction, stale workspace admission, and
    stage material binding -- while the picture is still visibly different. That
    weight of negative evidence argues the original framing is wrong: **this may
    not be a texture defect at all.**
    **THE MATCHED PAIR SETTLES IT: THE STAGE GEOMETRY IS CORRUPT ON THE SECOND
    ENTRY, AND THE FIGHTER IS NOT.** `-MatchedCapture` shoots both entries at
    the same camera-proc call count, so both are taken at a converged and
    comparable distance -- `target_dist` **3996.20** in match 1 against
    **3703.11** in Sudden Death -- which removes the "same stage, further away"
    confound that wasted two hypotheses. Evidence pair:
    `2026-07-30_235643-matched-match1.png` /
    `2026-07-30_235643-matched-suddendeath.png`.
    - Match 1: Dream Land clean. Bark trunks, wooden platforms, grass, flowers,
      Mario correct.
    - Sudden Death: stage geometry **corrupt** -- floating green slabs where
      platforms belong, a melted green mass at top left, multicoloured noise
      bands, untextured white quads. **Mario renders correctly in the same
      frame.**
    So the corruption is **stage-only, with the fighter path intact**, which is
    also why every fighter-side hypothesis measured clean. The owner's original
    report was right and my "it's the camera" detour was not.
    **It has a quantitative signature too:** `STG` **173,568 -> 383,296** (2.2x)
    and `ALL` 1,119,808 -> 1,679,936 across the matched pair. The stage owner is
    not merely drawing wrong, it is doing more than twice the work to do it --
    so whatever it is, it is visible in the stage bucket and does not need a
    screenshot to detect. Use that as the regression signal.
    **Partitioning that STG doubling is IN PROGRESS, and the instrument has a
    trap worth recording first.** The four phases already exist behind
    `NDS_TASK103_STAGE_RUN_PHASE` (Prepare / Traversal / Display / Finish) and
    sum to the STG bucket, so no new code is needed -- but **do NOT read
    `gNdsTickHudStageTicks` itself at a breakpoint.** It is zeroed every frame
    by the battle presentation loop, so a mid-frame stop reads a partial frame;
    the first attempt printed `0` for it and the number means nothing. The
    Task 103 counters are cumulative and are the ones to diff. Same shape as the
    Results buckets free-running (see HANDOFF): know whether a counter is
    per-frame or cumulative before quoting it.
    Arm A (match 1, 120 camera-proc calls) reads, per call:
    Prepare **77,378** (120 calls), Traversal **330,360** (8), Display
    **5,213** (3,288), Finish **504** (119). Arm B did not complete -- a
    Task 103 build is slow enough that 240 gdb-evaluated breakpoint stops
    exceed the 180 s cap, so re-run with `-MatchedCameraCalls 120`, which is
    still past convergence.
    **ARM B RAN, AND IT REFUTES THE PREDICTION BELOW WHILE ANSWERING THE
    QUESTION: SUDDEN DEATH NEVER RE-TRAVERSES THE STAGE.**

        phase       arm A (match 1)        arm B (after SD)       delta
        Traversal   2,642,880 /  8 calls   2,642,880 /  8 calls   0 / 0
        Prepare     3,088,064 / 30         28,247,680 / 333       303 frames
        Display     4,347,264 / 783        50,712,896 / 9,232     8,449 calls
        Finish         17,280 / 29            166,272 / 332

    **`gNdsTask103TraversalCount` is 8 at both stops.** The counter is cumulative
    from boot, so that single fact is airtight regardless of window: across the
    remainder of match 1, the scene transition, and the whole of Sudden Death,
    the eight stage segments are **never traversed again**. I predicted the
    opposite -- more traversals per frame -- and it is zero. Display runs at a
    normal rate throughout (26.1/frame in arm A, 27.9/frame in the delta) and
    its per-call cost is unchanged (5,552 vs 5,487), so the extra work is not a
    costlier or more frequent commit.
    **WITHDRAWN, same session: the "draws from match 1's capture" inference is
    REFUTED by the Task 36 owner state.** Measured at both matched stops:

        match 1        state 2 (READY)  topology_generation 2  stamp 0xe3202299
        Sudden Death   state 2 (READY)  topology_generation 3  stamp 0x73745b9a

    `ndsRendererTask36ReplayReset` memsets the owner to `UNSEEDED` (0), and the
    only route from there to `READY` (2) is through `CAPTURING` (1). So on the
    second entry the replay **reset, re-captured, and re-keyed** to a new
    generation and a new stamp. It is not serving match 1's geometry.
    The Traversal count of 8/8 is still a true fact, but it does not mean what I
    said it meant: the Task 103 Traversal bracket wraps
    `Begin/EndStageTraversal` in the DObj-draw callback, which is **not** the
    replay's capture mechanism -- the replay records inside command execution
    (`NDS_TASK36_REPLAY_RECORD`). An unchanged traversal count therefore says
    nothing about whether the replay re-captured.
    **That is the THIRD causal claim withdrawn on this symptom** (textures, the
    camera, and now the stale replay), each from reading one counter as a cause.
    The standing correction in this row stands and is now doubly earned: state
    the mechanism a counter actually brackets before inferring from it.

    **PAYLOAD COMPARED, and the replay is indistinguishable between entries.**
    Following the row's own instruction to compare data rather than guess again:

        field                    match 1     Sudden Death
        word_count               3916        3916
        captured_segment_mask    0xa1        0xa1
        capture_fault            0           0

    Same recorded stream size, same segments captured, no capture faults. Equal
    `word_count` is not proof of equal CONTENT -- state that plainly rather than
    overclaim -- but same size, same segment set and no fault is every field the
    owner exposes, and all of them match.
    **Word CONTENT sampled, and it matches too.** Eight fixed indices across the
    3916-word stream, read at both stops:

        index      0            1     100   500      1000         2000  3000  3915
        match 1    0x40181110   0x2   0     0x2217   0x18111210   0     0     0xd10
        sudden     0x40181110   0x2   0     0x2217   0x18111210   0     0     0xd10

    Identical at every index, including the first and last live words. Honest
    limits: this is 8 samples of 3916, and three of them (100, 2000, 3000) are
    zero and therefore weak evidence. Five non-trivial words plus equal size,
    equal segment mask and zero capture fault is strong but not proof of a
    byte-identical stream. A full dump would settle it -- note that
    `dump binary memory` was tried and silently terminated the gdb script, so
    whoever retries it must check the generated script rather than assume the
    command took.
    **`0xa1` is segments 0, 5 and 7 -- three of eight.** So the Task 36 replay
    only ever covers three segments and the other five are drawn by the ordinary
    path every frame, on both entries. The replayed third is identical; the
    difference therefore has to live in the five that are NOT replayed, or in
    the content of those 3916 words. That is a real narrowing and it is where a
    payload/word-level diff should be pointed next -- dump `owner->words[0..3916]`
    at both stops and compare, which is the one comparison that can distinguish
    "same size, same content" from "same size, different content".

    **The five unreplayed segments have names, and a pointer probe on them is a
    NULL RESULT -- with a lesson attached.** `ndsRendererAdapterNativeStageSegmentGObj`
    maps the mask: replayed 0/5/7 are `gGRCommonLayerGObjs[0]/[2]/[3]`, and the
    unreplayed 1/2/3/6 are `gGRCommonStruct.pupupu.map_gobj[0..3]` -- **the Dream
    Land map objects** -- plus `gGRCommonLayerGObjs[1]` at 4. That is exactly the
    geometry that looks wrong, which is a satisfying alignment.
    Probing those GObj pointers on both sides returns them **identical**
    (MAP0 0x2369a10, MAP1 0x2369dd8, MAP2 0x236a410, MAP3 0x236aa70,
    LAYER0 0x23677a8, LAYER1 0x2368c58).
    **That proves nothing, and must not be read as reuse.** The taskman heap is
    a bump allocator that the scene entry rewinds, so the same allocation
    sequence necessarily lands at the same addresses -- a freshly rebuilt stage
    and a stale one are pointer-identical by construction. **Standing
    consequence: on this heap, pointer identity is never evidence of object
    identity across a scene boundary.** Any "is it stale?" test built on
    comparing addresses is answering a different question; use the heap
    generation (which is why that contract exists) or compare contents.

    **Honest current state.** The stage geometry is corrupt on the second entry
    and the fighter is not -- that much is proven by the matched pair and is not
    in question. Everything measured so far is clean or accounted for: static
    textures, MObj chains, scene-cache eviction (now firing 2/2), stale
    workspace admission, stage material bindings, camera convergence, and the
    Task 36 replay identity. **The cause is not yet known**, and the next step
    should be a data comparison rather than another mechanism guess -- diff the
    captured replay payload or the emitted segment commands between the two
    entries, since the pixels differ while every piece of identifying state
    matches.

    **(Withdrawn) the second entry draws the stage from match 1's captured
    traversal:**
    That fits the corruption better than any cost story: the topology IS rebuilt
    on the second entry (`gNdsR2StageTopologyRebuildCount` 2, measured earlier),
    but the per-segment traversal that produces the draw data is not re-run, so
    freshly rebuilt bindings are being replayed against capture data belonging
    to the previous scene instance. Rebuilt pointers, stale geometry.
    **The scene-cache fix did NOT cause this, and the timeline settles it.**
    The obvious worry is that making the eviction actually fire (2/2, commit
    `368a037d4e` at 23:30) freed the reloc files the one-time traversal capture
    points into, so evicting is what broke the replay. It is not: the corrupt
    Sudden Death capture `2026-07-30_230425-sudden-death-watch.png` was written
    at **23:06**, and the owner's report came before that. The defect predates
    the fix. Keep the fix.
    **Where to fix:** whatever should re-arm the segment traversal after a
    scene generation bump. Note `ndsStageGCDrawAllLoopNativeStageArmed` is
    re-evaluated every frame from `ndsRendererAdapterPrepareNativeStageOwner`
    (Prepare ran 333 times, once per frame), so the OWNER is re-armed fine --
    it is specifically the one-time per-segment traversal capture that is not
    re-run, and `ndsRendererAdapterCommitNativeStageDisplay` then serves every
    frame from it. Note the connection to the scene-cache fix in this
    row -- `topology_generation` now advances correctly on the second entry
    (2/2), so the replay is being invalidated; nothing appears to rebuild it.

    **METHOD FLAW in the numbers above, stated so nobody over-reads them.** The
    Task 103 counters are cumulative from boot and arm A is snapshotted early in
    match 1, so the B-A delta spans the rest of match 1, GAME SET and the
    transition as well as Sudden Death. **Only the Traversal result is clean**,
    because an unchanged cumulative count cannot hide activity anywhere in that
    span. Prepare and Display deltas are NOT attributable to Sudden Death alone
    and must not be quoted as such. Fix the harness before using them: take a
    third snapshot at the `running` stage so the Sudden-Death-only delta is
    (shot-sd - running).

    **(Refuted) the arm-A numbers already name the likely culprit:** Traversal costs **330,360 ticks
    per call** -- on its own that is ~1.9x an entire match-1 stage frame
    (STG 173,568) -- and match 1 takes it only **8 times across 120 frames**,
    about once every fifteen. Display, by contrast, is 5,213 x 3,288 calls, and
    Prepare 77,378 x 120 (once per frame). So the STG doubling does not need a
    new phase or a costlier one: **a Traversal that fires on frames where match 1
    skips it accounts for the whole 173,568 -> 383,296 by count alone.**
    Falsifiable prediction for arm B: its Traversal COUNT per frame is higher
    than one-in-fifteen, while Display and Prepare per-call costs are
    unchanged. If instead Display's count jumps, the extra work is extra
    segments committed and the traversal reading is a coincidence. Either way
    the diff decides it; do not adopt the prediction until it is run.
    Traversal lives in `ndsStageGCDrawAllLoopRecordDObjDraw`
    (`reloc_backend_movement.c:13299`), gated per classified stage GObj by
    `sNdsStageGCDrawAllLoopHardwareSubmitActive`, and **8 is not an arbitrary
    number: it is the segment count.** `ndsRendererAdapterNativeStageSegmentGObj`
    enumerates exactly eight segments (cases 0..7). So match 1 traverses each
    segment ONCE and the Task 36 replay serves every frame after that -- which
    is the R2-02 architecture working as designed, and why Traversal is 8 calls
    in 120 frames rather than 8 per frame.
    **That sharpens the hypothesis into something structural:** if the second
    entry does not re-establish the replay, the stage falls back to full
    traversal, and 8 x 330,360 per frame instead of once is far more than enough
    to explain STG 173,568 -> 383,296. It would also explain the corruption
    rather than merely the cost, because the fallback draws by a different route
    than the replay it is standing in for.
    There is a direct connection to the scene-cache fix landed earlier in this
    row: the board records `topology_generation` -- which comes from
    `owner_generation`, which comes from `sNdsRelocSceneGeneration` -- as **the
    only thing that resets the Task 36 replay**. That generation now advances
    correctly on a second entry (2/2, where the old cursor test caught 0/2), so
    the replay IS being reset where before it silently was not. **The open
    question is whether anything REBUILDS it afterwards**, or whether the second
    entry simply runs un-replayed from then on. Check
    `gNdsTask103TraversalCount` per frame in arm B first; if it is 8 per frame
    rather than 8 per entry, that is the answer and the fix belongs at whatever
    should re-arm the replay after a generation bump.

    **(Withdrawn, kept for the record) THE CAMERA CONVERGES NORMALLY.** `gmCameraDefaultFuncCamera` runs in Sudden Death, and
    `target_dist` reads **10000.0 on the first frame and 3996.67 after 120 proc
    calls** -- essentially match 1's 3937.42. The easing moves 7.5% of the
    remaining gap per call, so two seconds of proc closes it. The "10000.0
    exactly" below was sampled at the `running` stage, which is Sudden Death's
    FIRST frame, before any easing had happened. It was the creation value
    because nothing had converged *yet*, not because nothing ever would.
    That is the second wrong causal claim on this symptom in a row -- first
    textures, then the camera -- both from over-reading a single measurement.
    **Standing correction for this row: stop proposing causes from the
    screenshots.** The visible difference is real and reproducible; every
    mechanism measured so far is clean. What it needs is a properly MATCHED
    comparison -- same converged camera distance and same scene tick on both
    entries, the way `compare-capture-pair.ps1` matches Results arms -- not
    another hypothesis. The watch captures are taken 30-45 s in, long after
    convergence, so whatever they show is not a camera-distance artifact either.

    **(Withdrawn reasoning below, kept because it is how the camera was
    measured and because the 10000.0/3937.42 pair is still a useful fact.)**
    Measured on both sides of the boundary in one run:

        match 1        target_dist 3937.42   status 0  fovy 38  vp 300x220
        Sudden Death   target_dist 10000.00  status 0  fovy 38  vp 300x220

    Everything matches except the distance, and Sudden Death's is **exactly**
    `10000.0` -- the literal `gGMCameraStruct.target_dist = 10000.0F` that
    `gmCameraMakeDefaultCamera` writes at creation
    (`decomp/.../gm/gmcamera.c:1157`). The battle camera eases that value toward
    a computed distance every frame (`:593-601`), which is how match 1 reaches
    3937. **In Sudden Death it holds the creation value, so the convergence
    never runs at all** -- not "converges to the wrong place", never runs.
    The stage is therefore drawn from **2.54x** the intended distance. That is
    the whole visible defect: the "wrong textures" are Dream Land's own geometry
    seen from far enough out to be unrecognisable, which is why five separate
    texture and binding hypotheses all measured clean. The status word is 0
    (`nGMCameraStatusDefault`) on BOTH sides, so the mode is not the difference
    and the camera object does exist.
    **Next: why the per-frame convergence does not execute for the second
    entry.** `scVSBattleStartSuddenDeath` calls `gmCameraMakeBattleCamera` just
    as `scVSBattleStartBattle` does, so the object is created; the question is
    whether its proc is scheduled and whether the fighter list it reads is
    populated at that moment. `FTR` 232,896 against match 1's 394,816 says the
    fighters are drawn but at 59% of the cost, which is consistent with a camera
    that never converges on them.

    **(Superseded reasoning, kept because it is how the camera was found.)**
    Re-read the pair with that in mind. The Sudden Death frame is shot from much
    FURTHER OUT and higher than the match-1 frame; the whole stage is small and
    centred, and no fighter is visible. Geometry seen from an unfamiliar
    distance and angle is exactly what would read as "wrong textures" while
    every binding check passes. `FTR` is 232,896 in Sudden Death against 394,816
    in match 1 -- fighters are still being drawn, at 59% of the cost, not
    absent. **So the next target is the CAMERA, not the material path:** why is
    the Sudden Death view wide and untracked, and is the fighter draw reduced
    because the camera never converges on them? Do not spend another experiment
    on textures until that is answered.
    **(Superseded) the mis-binding is in the CONTENT of a fresh rebuild:** `ndsRendererAdapterCollectNativeStageTopology` fills
    `binding_dobjs[]` in whatever order the live DObj tree presents, and
    `ndsRendererAdapterPrepareNativeStageMaterials` then indexes it by the fixed
    constants `{20, 22, 31, 32}`. If Sudden Death builds that tree in a
    different ORDER -- genuinely different, not stale -- the fresh collection
    still hands each index another object's material. That is now the leading
    hypothesis and it is falsifiable: dump `binding_dobjs[]` on both entries and
    compare the DObj identities at those four indices.
    Four hypotheses are eliminated by measurement so far: static textures
    (2 prepares, 0 violations), MObj chain corruption (91,482 probes, 0 invalid),
    scene-cache eviction (now 2/2 and the symptom is unchanged), and stale
    workspace admission (2 rebuilds).

    **The freeze is INTERMITTENT, not gone.** An unchanged ROM that had given
    6/6 distinct frames froze on a later run of the same lane -- 1 distinct
    frame in 6 samples over 30 s, 245 distinct colours so the capture was live
    -- and then ran clean again. That is direct evidence the fault is racy
    rather than deterministic, and it weakens the earlier "the guard displaced
    it" reasoning as well: the pre-guard runs that froze four times in a row may
    have been an unlucky streak. Treat any single freeze-free Sudden Death run
    as uninformative; this needs repetition counts, not one observation.

    **Superseded suspect (kept for the record):** the native stage owner's
    prepared workspace.
    `ndsRendererAdapterPrepareNativeStageMaterials`
    (`reloc_backend_renderer_dl.c:7398`) indexes `workspace->binding_dobjs[]` by
    FIXED indices `{20, 22, 31, 32}` with fixed expected flags. If the DObj tree
    is rebuilt in a different order on a second entry, those indices no longer
    name the same objects and each one gets another's material -- which is
    precisely the observed symptom. There is a stamp guard at `:6995` that
    rejects a workspace whose `binding_dobjs[i]->dv` no longer matches its
    recorded display list; find out whether it fires on the second entry, and if
    it does not, why the indices still resolve.

  - **BOTH KNOWN FREEZE MECHANISMS ARE REFUTED FOR THIS FREEZE.** BattleShip
    never fails an overflow, it STOPS: eleven `while (TRUE);` sites across
    `sys/taskman.c` and `sys/malloc.c`, which is why this bug class always
    presents as a frozen picture rather than a crash. Every one of the eleven is
    immediately preceded by a `syDebugPrintf`, and the port links a real (empty)
    one at `boot_stubs.c:91`. **It never fires**, which rules out the
    `syTaskmanCheckBufferLengths` display-list overflow and the other nine
    taskman give-up sites.
    **CORRECTION: `syDebugPrintf` is NOT a total detector for this class.** The
    port replaced malloc's overflow arm with `ndsSyMallocOverflowHalt()`
    (`battleship_sys_malloc.c:99`), which prints nothing and spins in its own
    named symbol -- deliberately, so the PC identifies it. Heap exhaustion is
    therefore detected by `gNdsSyMallocOverflowCount` or a breakpoint on that
    halt, never by the printf. The conclusion that this freeze is not the
    `syMallocSet` spin still holds -- the arena had 42,992 bytes free and the
    range/generation counters are clean -- but it rests on those, not on the
    printf being silent. The CPU is live and
    executing ordinary code that never completes a frame.
  - **DEFECT FOUND BY INSPECTION, and it is on the sampled path:**
    `ndsRendererAdapterPrepareNativeMaterials`
    (`src/port/reloc_backend_renderer_dl.c:7825`) walks `dobj->mobj` TWICE. Pass
    one (`:7841`) counts and is bounded -- it returns FALSE once `count >
    capacity`. **Pass two (`:7850`) has no bound and no capacity check at all**,
    and writes `materials[count]` with `count` free-running. `capacity` here is
    `NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX`, which is **4**. Pass one's bound
    only protects pass two if the list is identical across both walks -- and pass
    two MUTATES every node as it goes, because it calls
    `ndsRendererAdapterBuildNativeMaterial`, which is the
    `advance_texture_ids = TRUE` wrapper that writes `mobj->texture_id_curr` and
    `->texture_id_next` (`:6669-6673`). A list that becomes cyclic or is
    corrupted mid-walk runs away and overruns a four-element array. That matches
    the interrupt sample's arguments exactly (`advance_texture_ids=1`,
    `out_curr=NULL` -- the signature of the `:7852` call site, NOT the stage path
    at `:7412`, which passes FALSE and non-NULL), and it explains why a
    breakpoint on the out-of-line `...BuildNativeMaterialSnapshot` never fires
    during the stall: at -O2 the copy executing inside that loop is inlined.
    This is an unbounded write loop into a 4-entry array and is worth fixing on
    its own merits, whether or not it proves to be the whole freeze.
  - **PLAN (2026-07-30), in order, each step falsifiable on its own:**
    0. **CORRECTION 2026-07-30, later the same day: the claim below that the
       bound FIXED the freeze is WITHDRAWN, and the MObj-corruption hypothesis
       is REFUTED.** Two measurements, both on the owner-requested second-entry
       diagnostics:
       - **The guard never fires.** `gNdsR2MaterialWalkBoundHits` is **0** on
         every run, including runs that reach Sudden Death. A same-binary A/B --
         `gNdsR2MaterialWalkBoundEnabled` cleared from the debugger, so code
         layout, inlining and the ROM are identical between arms -- gives
         **9 distinct frames of 9 samples in BOTH arms**. Guard on and guard off
         are indistinguishable. Whatever stopped the freeze, it was not this
         branch executing.
       - **The chain is never invalid.** The validator ran **91,482** probes
         across a full run including Sudden Death and found **zero**: no
         overlong chain, no cycle, nothing outside the taskman arena, at either
         probe point. The chains are **one node long** against a capacity of
         four, which is also why the guard cannot fire.
       So the freeze did stop when the guard commit landed, and the guard is not
       the reason. The remaining explanation is that adding it moved code --
       this campaign has repeatedly measured placement changing results on its
       own (E11 removed real work and P95 still rose 15,744). **That makes the
       Sudden Death freeze a LATENT, layout-sensitive fault that has been
       displaced, not repaired**, and it says the real cause is still live and
       will move again on the next unrelated edit.
       **Keep the guard** -- an unbounded write into a four-entry array is a
       defect whether or not it is this one, and it costs one compare on a
       one-node list. But do not carry it as the fix.
       **Next, and it is a reproduction problem before it is a diagnosis one:**
       revert the guard in a scratch build and confirm the freeze returns with
       chains still clean. If it does, the fault is elsewhere and triggered by
       placement; the instruments to reach for are the ones that survive a
       layout change -- `gNdsSyMallocOverflowCount` /
       `ndsSyMallocOverflowHalt` (NOT `syDebugPrintf`; see the corrected note
       above), stack headroom, and the taskman DL buffers.
    1. **Bound pass two. DONE 2026-07-30 -- ~~THE FREEZE IS FIXED~~ SEE THE
       CORRECTION ABOVE.** The write walk
       now returns FALSE once `count >= capacity`, handing the caller its
       existing generic fallback instead of running off a four-entry array.
       Measured on the lane, same ROM configuration, 60-second picture watch:
       **before 12 samples / 1 distinct frame (frozen); after 12 samples / 12
       distinct frames**, with the tick-HUD VBlank histogram accumulating again.
       `verify-all -Profile Latest` passes, `battle_playable_realtime` included.
       The guard is a pure safety net and cannot alter a healthy frame: pass one
       already rejects any list longer than `capacity`, so for a well-formed list
       the new test is never reached. That is also why this is a fix and not a
       workaround at the wrong seam -- it only fires when the list changed
       between the two walks, which is itself the defect.
       **It stops the damage; it does not explain why the list goes bad.** Steps
       2 and 3 stay open.
       **And with it in, Sudden Death genuinely RUNS.** Measured the same way:
       `gSCManagerBattleState->game_status` is `nSCBattleGameStatusWait` (0) at
       entry, as `ifCommonSuddenDeathMakeInterface` leaves it, and reads
       `nSCBattleGameStatusGo` (1) 240 presents later at frame 553, with
       `time_remain` re-armed to 3600. So `ifCommonSuddenDeathThread` -- which
       sleeps 90 tics and then calls `ifCommonAnnounceGoSetStatus` -- resumes
       correctly across the coroutine seam, the announce fires, and the second
       match starts. The freeze was the whole blocker, not a symptom of a
       deeper stall.
       Still to look at, and NOT yet diagnosed: the end-of-watch capture
       (`artifacts/verification/sudden-death/2026-07-30_214748-sudden-death-watch.png`)
       shows the stage from a wide angle with neither fighter visible, and P1's
       stock reads `--` while the CPU's reads `x1`. One screenshot is not a
       diagnosis -- the run reaches Go, so the match is live behind it. Check
       whether that is a camera-tracking defect, a fighter-draw fallback taken
       because the new guard now returns FALSE for a corrupt DObj, or simply the
       frame the capture happened to land on.
    2. **Find who corrupts the list.** The standing suspect is already in this
       row: `ndsR2AnimCacheArenaStillOwned` (`reloc_backend_assets.c:5854-5871`)
       can false-positive after `syTaskmanStartTask` rewinds
       `gSYTaskmanGeneralHeap` over the same arena, handing back payload pointers
       into reused memory on a SECOND scene entry -- which is exactly what Sudden
       Death is. Test it directly rather than by inference: invalidate the R2
       animation cache at battle-scene entry, before `scManagerFuncUpdate` can
       reuse the heap, and re-run the lane.
    3. **Explain the 119 KB.** Sudden Death's setup takes ~119 KB more arena than
       match one for the same stage and fighters, leaving 42,992 free. Even if
       steps 1-2 clear the freeze, that gap is unexplained and is the margin the
       next mid-match allocation spends. Census the second setup pass against the
       first rather than guessing.
    Do NOT enlarge any buffer or arena to make this go away: every overflow site
    here is a symptom of corrupted second-entry state, and a bigger buffer only
    moves the freeze later into the match.
  - Also seen in that run, unrelated to Sudden Death and unowned:
    `gNdsR2AnimCacheArenaOverflows 109` with `gNdsR2AnimCacheRejects 109`, and
    `gNdsTaskmanArenaAllocFailCount 26`.
  - FGM 514 remains absent from the DS FGM selector/pack. That explains the
    missing announcer voice but cannot cause the render corruption or freeze.
  - Proposed owning-seam fix: explicitly invalidate the R2 animation cache at
    battle-scene entry BEFORE `scManagerFuncUpdate` / `syTaskmanStartTask` can
    reuse its heap, then keep the new Sudden Death wrapper and stepped warm
    preload. Do not enlarge the display-list buffer; its overflow is downstream
    of corrupted second-entry state. Make the soak's both-CPU build flag
    explicit and add cue 514 through the existing FGM path.
  - Required proof: build a ROM that contains the current commit and explicit
    both-CPU flag; reach a natural tie; prove one second-entry invalidation and
    fresh 92,160-byte reservation before any cache hit, nonzero
    `SuddenDeathPrepareCount`, no stale payload/read or DL overflow, and complete
    the Sudden Death/rematch battle through Results. Report the 2/3/4/5+ VBlank
    histogram and max interval, DLDI reads, exact ROM/ELF identity, and owner
    visual/listen acceptance. Status: PARTLY FIXED IN SOURCE; CACHE LIFETIME,
    ROM PUBLICATION, REPRODUCTION LANE, AND FGM 514 remain OPEN.


-Wind hazard not working, (SFX, VFX, gameplay effects)  [gameplay+SFX FIXED]
  Gameplay FIXED: ftParamSetVelPush was a counter-only stub that dropped the
  push vector on the floor, so Whispy's gust had no effect at all. It now does
  the source assignment (ftparam.c:526-531); the consumer chain was already
  live (mpprocess.c:450 adds coll_data.vel_push to the translation,
  ftmain.c:1571 clears it each frame).
  SFX FIXED: nSYAudioFGMPupupuWhispyWind (285) was never packed. Added, and
  then it still puffed once and stopped -- the sample is 0.88 s but the blow
  is 470 ticks x 5750 us = 2.70 s, so two thirds of the gust was silence.
  (An earlier note here said 7.8 s; that used the wrong tick rate.)
  Root cause, and it was not the runtime:
  - The DS side was already complete. ndsAudioFgmValidateCachedEntry accepts
    flags & ~1u == 0, so a looping entry already passed validation, and the
    play path already passed the loop bit and loop point through to
    soundPlaySample (nds_audio_fgm.c:1158-1162), with the duration clock
    releasing the handle at 470 ticks.
  - The generator never emitted one. All four branches in
    render-audio-fgm-phase-pack.py hardcoded `flags = 0` and
    `loop_point_words = 0`. The `"loop": True, loop_start: 48,
    loop_end: 13348` on cue 285's selector was descriptive source metadata
    that nothing acted on. My earlier note claiming it "ships with the loop
    live" was wrong.
  Fixed as a DS hardware loop, not an AOT render. The DS ADPCM channel
  latches predictor/index when it reaches PNT and restores them on every
  repeat, so putting PNT at the first word after the IMA header makes the
  latched state the header state and every cycle decodes bit-identically by
  construction. Cost is zero extra bytes -- the pack got 28 B smaller.
  AOT was the other option and would have fit (~20 KB for 2.70 s), but it
  buys nothing over a loop the hardware does for free, and 626 only renders
  AOT because its loop feeds a volume ramp that a hardware repeat cannot.
  Shape: body = pcm[48:13352], 13304 samples = 1663 words after a 1-word
  PNT; the 4 samples past loop_end are the source's own tail, taken as
  alignment debt so the seam is real audio rather than synthetic guard
  nibbles. 6656 B, SNR 34.6 dB, ~3.07 cycles per blow.
  The unused ima_encode_loop_body / ima_ds_repeat_cycles / ima_repeat_oracle
  machinery is now live and proves it: three restored cycles hash identically,
  and all three negative controls fire (carried decoder state diverges, PNT
  at the header rejected, LEN counting the whole buffer rejected).
  check-audio-fgm-phase-pack.ps1 pins the flag, the PNT/LEN geometry and the
  three proofs so the loop cannot be dropped again silently.
  Latest profile green. Needs an ear check.
  VFX still open: the same particle-bank gap as the other VFX rows below --
  lbParticleMakeScriptID is a skipped stub.
  Research (2026-07-30, Sol Max wind/particles):
  - Source contract: Whispy uses Dream Land's separate Pupupu particle bank,
    with script 0 for leaves and script 1 for dust
    (`decomp/BattleShip-main/decomp/src/gr/grcommon/grpupupu.c`,
    `particles/grpupupu_scb.c`, and `particles/grpupupu_txb.c`). Its source
    bank is 4,896 bytes; that is source representation size, not a promised
    final DS residency cost.
  - Remaining root cause: the particle constructor above is skipped and the
    draw seam counts particles without drawing textured quads. The two
    preserved WIP commits are incomplete together:
    `17e6cd0b301fe77402f352ec0c96cd6fc6ce19b7` generates only the common EF
    bank, while `ef5d25268678070a6f1dd452ecbb0e1ad9194519` registers every
    non-common bank, including Pupupu, as empty and has no live draw
    (`src/import/battleship_lbparticle.c:507-560,669-677`).
  - Proposed fix: reconcile those WIPs into explicit common-plus-Pupupu pack
    contracts, pin script/texture closure and bytes in one checker, complete
    only the required interpreter operations, and submit one DS textured quad
    through the existing renderer texture fencing. Measure RAM, VRAM, pool
    high-water, and drops; keep the current fallback until natural proof.
  - Required proof: a natural Whispy cycle showing source leaves/dust alongside
    the already-live push and cue 285, zero unexplained particle drops,
    synchronized screenshots plus owner visual/ear approval, and an eight-frame
    A/B with the 2/3/4/5+ VBlank histogram and max interval. Status: gameplay
    FIXED; SFX implementation landed but acoustic acceptance is OPEN; VFX OPEN.

-some VFX are wrong/don't look right, running foot dust VFX, fireball hit VFX, fox down B, hard landing vfx
  Root cause, measured: every named effect does spawn -- the verifier reports
  178/178 Mario/Fox motion calls with bounded DS presentation -- but the
  original particle scripts never run. lbParticleMakeScriptID is a stub
  (reloc_backend_compat_shims.c:12963) and the common particle script/texture
  banks are not resident, so nothing here is textured. In their place,
  13 NDSVisualEffectKind values (nds_effects.h:9-25) share a small set of
  template slots (battleship_efmanager.c) built from only FOUR untextured
  16-vertex primitives: BuildDust, BuildStar, BuildRing, BuildDisc.
  This splits the row in two:
  (a) A real defect -- before this pass, five kinds rendered as a *different
      effect*: Coin->Sparkle, Catch->ImpactWave, Slash->HitNormal,
      Rebirth->Death, and Reflector->Shield. "fox down B" is
      that last one: efManagerFoxReflectorMakeEffect asks for
      nNDSVisualEffectReflector and gets the shield disc, white->red -- P1
      Mario's shield colors on Fox's reflector. The port also never reads
      effect_vars.reflector.status, which ftfoxspeciallw.c:29 sets every frame,
      so the reflector cannot react to its own state either.
      FIXED for the two collapses a Mario-vs-Fox match actually shows:
      Reflector and Rebirth now have their own template slots. Fox's down B
      draws a blue barrier disc instead of Mario's red shield. The hues reuse
      pairs already in the file, so this is an approximation and the owner's
      eye is the gate. Coin/Catch/Slash still share slots -- coins need items
      (off for P1) and neither of the other two was reported, so they stay
      collapsed rather than getting invented colors.
  (b) Running foot dust, fireball hit, and hard landing each DO have their own
      template (Dust, Fire, Dust). They look wrong because a recolored
      16-vertex primitive is standing in for a textured particle script.
      This is P1, not P2 -- the contract is that this exact match is identical
      to the full game with these settings, so the real scripts have to run.
      Sized 2026-07-27 and it fits: efmanager reaches 26 of 119 efcommon
      scripts naming 18 of 47 textures = 129,768 B, plus 4,896 B for Dream
      Land's grpupupu bank, against 210,320 B measured arena headroom. The
      banks are position-independent so no relocData tooling is needed.
      OPEN: port lb/lbparticle.c (2,961 lines), ef/efparticle.c,
      ef/efdisplay.c, add a DS pack step, and draw textured quads.
      See KNOWN_ISSUES.md for the full measurement.
  Research (2026-07-30, Sol Max wind/particles):
  - Exact common-particle targets are run/expiry dust scripts 0x55/0x56, hard
    landing/double scripts 0x58/0x59, fireball bounce 0x0B, fire damage 0x4D,
    and fireball-hit sparkle 0x73
    (`decomp/BattleShip-main/decomp/src/ef/efmanager.c` and
    `wp/wpmario/wpmariofireball.c`). The preserved generator's reachable list
    currently omits 0x58/0x59, so it cannot fix hard landing as-is.
  - Fox Reflector is not part of that particle bank. It is an animated DObj
    from Fox's special2 assets with four status animations
    (`decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxspeciallw.c`);
    `src/import/battleship_efmanager.c:901-905` currently maps it to a generic
    disc. Treating Reflector as another particle script would fix the wrong
    seam.
  - Proposed fix: finish one shared, bounded particle interpreter/pack and one
    textured-quad draw path for the exact reachable scripts above. Restore
    Reflector separately through the existing fighter DObj/model-animation
    path. Retire each primitive fallback only after its natural replacement is
    verifier- and owner-approved; do not add a second effect architecture.
  - Required proof: natural run/expiry dust, both landing variants, distinct
    fireball bounce/damage/hit events, and all four Reflector statuses, with
    exact-ROM screenshots, owner visual approval, pool/drop guards, and
    synchronized eight-frame pacing evidence. Status: OPEN except for the
    explicitly documented primitive-category separations.

-Upwards KO boundary:  VFX and SFX never play for fighters
  SFX implementation landed; natural qualification remains OPEN. The star-KO
  path asks for nSYAudioFGMDeadUpStar (12) and the per-fighter deadup_sfx (Fox
  360 / Mario 433); all three are now packed and allowlisted.
  VFX half: same root cause as the VFX row above. efManagerSparkleWhiteDead
  spawns nNDSVisualEffectSparkle at scale 5.0 and efManagerDeadExplode spawns
  nNDSVisualEffectDeath, so something does draw -- a white star and a red ring.
  The original star-KO is a textured particle script that is not resident.
  Research (2026-07-30, Sol Max Sudden Death/KO):
  - Source contract: crossing the top boundary has a 1-in-6 falling-KO branch
    and otherwise enters the 180-tick star-KO sequence. The star branch plays
    fighter voice 433/360 immediately, FGM 12 at the source point, and sparkle
    script 0x5C; the falling branch intentionally omits the star cue
    (`decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondead.c`).
  - Current weak effect code
    (`src/port/battle_playable_compat_stubs.c:130-138`) substitutes a generic
    untextured sparkle and returns NULL. That establishes a missing source
    effect seam, but the report that VFX/SFX “never play” still needs one
    natural trace to distinguish wrong branch selection, effect create/drop,
    draw visibility, and speaker delivery.
  - Proposed fix: first trace the natural branch/status, audio requests,
    particle create/drop, and draw submission. Fix the measured owner: retain
    the already-packed audio if it reaches the speaker, and restore script 0x5C
    through the shared particle work rather than adding a KO-only renderer.
  - Required proof: natural Mario and Fox star KOs plus the 1-in-6 falling
    branch, with trigger-to-speaker and trigger-to-pixel evidence and owner
    listen/visual approval. Status: OPEN.
-KO VFX wrong.
  Partly FIXED. nNDSVisualEffectDeath and nNDSVisualEffectRebirth shared one
  template, a red->white ring, so the KO burst and the respawn flash were the
  same effect. Rebirth now has its own white/cyan ring and Death keeps the red
  one. What remains is the untextured-primitive gap: the original KO burst is a
  particle script, and a ring is the stand-in. That remaining half is P1.
  Research (2026-07-30, Sol Max Sudden Death/KO):
  - Source contract: down/left/right deaths select player- and type-specific
    DeathExplode generators, orientation, DObj/material animation, and player
    colors (`decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondead.c` and
    `decomp/BattleShip-main/decomp/src/ef/efmanager.c`).
  - Root cause: the weak fallback
    (`src/port/battle_playable_compat_stubs.c:118-128`) discards the player
    argument and maps every type to one untextured ring, changing only scale.
    Separating Rebirth fixed one category collision but cannot restore KO
    orientation, ownership color, material animation, or timing.
  - Proposed fix: review the preserved particle WIP, then graduate only the
    reachable KO subset as a strong source-derived path that preserves
    player/type, side orientation, colors, and DObj/material timing. A full
    generic effect interpreter is unnecessary if the generated DS-native
    specialization is mechanically equivalent. Remove the ring only after the
    replacement passes natural proof.
  - Required proof: strong-symbol and asset checks plus natural down/left/right
    Mario and Fox KOs, source-referenced visual A/B and owner approval, and
    pacing/memory cleanup guards. Status: PARTLY FIXED; source-faithful KO burst
    remains OPEN.

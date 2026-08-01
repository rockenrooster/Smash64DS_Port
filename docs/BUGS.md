AI Agent should mark fixed items with FIXED prefix.
These bugs should be fixed for P1 delivery.
-respawn floating platform isn't visible when respawning.
-Stray VFX are getting played across the stage when attacks are landed.
-the rolling dodge sound (escape?) sounds off, maybe too loud???
-PARTLY FIXED (2026-08-01) the KO burst freezes the game.
  **Two root causes fixed, one still open, and the burst ships OFF until it is.**
  1. FIXED -- EFDesc file offsets were symbol ADDRESSES. On N64
     `llEFCommonEffects2DeadExplodeDefaultDObjDesc` and ~180 siblings are
     absolute linker symbols, so `&llFoo` IS the offset and
     efManagerMakeEffect's raw `addr + o_dobjsetup` is correct. This port
     supplies them as `static uintptr_t llFoo = 0x4F08u`, so `&llFoo` is a RAM
     address; all 182 references are &-prefixed, so nothing read the value and
     nothing caught it. `0x023xxxxx + 0x021xxxxx` lands outside the DS's 4 MB
     and gcSetupCustomDObjs walked it as a DObjDesc tree, allocating a 136-byte
     DObj per bogus node until syMallocSet gave up. Captured: `MALLOCOVF=1
     req=136 head=112`, `dobjdesc=0x446da28`, under
     `efManagerMakeEffect(dEFManagerDeadExplodeEffectDesc)` from
     `ftCommonDeadLeftSetStatus`. NOTE: this also retires the "a source effect
     costs ~5 DObjs" figure -- that was a runaway walk, not a cost.
  2. FIXED -- `lbRelocGetFileSize` reports `sizeof(Sprite)` for an ALREADY
     LOADED file, because ndsRelocExternTreeAllocSize returns 0 once an asset
     has a status node. Reading its 68 as "the effect assets are missing" was
     wrong: EFCommonEffects1/2/3 are packed and load at 52,736 / 28,352 /
     13,616 bytes. Use `ndsRelocGetLoadedFileSize` for a live file.
  3. OPEN -- the burst still freezes, and it is NOT allocation. A/B on the
     published configuration, all with `MALLOCOVF=0`, the GObj cap unfired and
     32,196 bytes free in the frozen case:
       burst + DObj tree  FROZEN at presented frame 609  att=1 ok=1 drop=000
       burst, no tree     FROZEN at presented frame 609  att=1 ok=1 drop=400
       burst off          NO-FREEZE, two KOs, match completes, Results reached
     So neither the tree nor the heap. What is left is the particle call, whose
     one peculiarity is its generator link:
     `lbParticleMakeScriptID(bank | LBPARTICLE_MASK_GENLINK(1), ...)` -- every
     healthy effect in this port uses GENLINK(0). START THERE.
     The fault is a data abort taken in System mode (`spsr_abt` low bits 0x1f,
     and it VARIES between runs so it is live, not stale boot state); calico
     then schedules the idle thread, which is why every capture reads as a bare
     `armWaitForIrq` with zeroed registers and no guest frame.
  `NDS_R2_KO_BURST_PARTICLE=1` re-enables the particle, `NDS_R2_KO_BURST_TREE=1`
  the tree; both currently freeze. `efManagerSparkleWhiteDeadMakeEffect` is
  particle-only on link 0, works, and still plays, so a KO is not silent.
-Still get intermittent freezes when attacking (maybe collision/animation/heap
  related?).
  **A LIKELY OWNER was removed on 2026-08-01, and the owner's play test is the
  test.** "Heap related" was the right instinct. Until that date
  `gSYTaskmanGeneralHeap` sat at **14,796 bytes free** for the whole match --
  under the 25,600 at which `ifCommonSetMaxNumGObj` caps the GObj pool -- so
  `gcMakeGObj` returned NULL for the rest of every match once the pool reached
  its latched size. That was measured refusing four of Mario's eleven fireballs,
  and `wpManagerMakeWeapon` happens to CHECK for NULL. Callers that do not check
  abort instead: the recorded signature for this exact latch is
  `ifCommonTrafficMakeSObj+68` storing through a NULL GObj with a healthy
  allocator (`MALLOCOVF 0`, PORTING.md 2026-08-01). Attacks are what spawn
  transient GObjs -- effects, weapons, hit sparks -- which is why the symptom
  would present as "freezes when attacking" rather than at random.
  `NDS_R2_WEAPON_POOL = 12` returns 14,080 bytes and the latch stops firing;
  `gNdsTaskmanGeneralHeapFreeMin` is now sampled every presented frame so a
  regression is a number rather than a repro.
  Not claimed FIXED: no run in the campaign ever reproduced the freeze, so
  nothing here can be shown to have been the cause. Seven soaks since, including
  a five-minute both-CPU run through Sudden Death, are all NO-FREEZE.
  Owner evidence:
  `artifacts/visibility/2026-07-31_attack-freeze-owner-302caae.png` -- build
  `302caae`, TIME 00:37, both fighters 12%/1 stock, Mario inside a shield
  bubble. The HUD is the useful part: `ALL 1119872` current against `1680320`
  max, `WORK 1034496 / 1497856` over n:128, VBlank histogram `2:690 3:163 4:33
  5+:4` with `max:19`. So the stall is four 5+-VBlank frames and one 19-VBlank
  frame in a 128-frame window -- an event, not a slow body.
-Results screen. VFX and SFX/BGM/FGM.  [ALL FOUR FGM cues PACKED 2026-08-01;
  VFX remains]
  **534 WinnerIs, 499 Mario and 486 Fox** were packed 2026-07-31 -- derived with
  `render-audio-fgm-phase-pack.py --derive 534,499,486` and proven by their
  disappearance from the natural-match miss ring (see the TIME UP row).
  **621 PublicWin is now packed too** (2026-08-01), and the machinery it was
  said to need turned out to be one line. It is the second cue on 626's wave
  (both articulation 460 / sound 320 / wave 2966600+15876, both looping
  1..28215), differing only in note (9 vs 12), duration (950 vs 1200 ticks) and
  UCD volume (190 vs 223) -- so it takes 626's AOT loop-then-quadratic-ramp
  render unchanged. FGM 285's hardware-repeat strategy would NOT have worked for
  either: articulation 460 ramps volume across the loop, and a hardware repeat
  reproduces every cycle bit-identically by construction, which is exactly why
  it cannot ramp. The only 626-shaped thing in the renderer was a pinned sample
  count, now derived from the note's own duration (`looped_fanfare_sample_count`,
  which reproduces 626's 104,204 exactly and gives 621 69,369) and still pinned.
  Pack 535,280 -> 570,000 B, 63 -> 64 entries, cap 786,432.
  One real defect fell out: the `missing_preroll` negative control was unclamped
  while the render clamps, so 621's lower hardware gain overflowed int16 and
  crashed the generator. Clamped now -- it was also a WEAKER control unclamped,
  differing from the render in two ways instead of one. **Needs an ear check.**
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
  **BOTH HALVES IMPLEMENTED 2026-08-01; needs an ear check.**
  *Trigger side:* `ft/ftpublic.c` is compiled in place
  (`src/import/battleship_ftpublic.c`, `NDS_IMPORT_BATTLESHIP_FT_PUBLIC`), so
  the thresholds, cooldowns, repeat limits and defeated-voice queue are the
  source's rather than a translation. **It COMPILES AND LINKS as of 2026-08-01;
  before that the flag had never been built, and the claim below that "its whole
  external surface already existed" was half wrong.** The functions exist, but
  not under the declarations the decomp TU expects, and one dependency did not
  exist at all:
  `func_800269C0_275C0` is declared `void *` by `sys/audio.h:71` and
  `alSoundEffect *` by `ftpublic.c:4` (a hard conflict, resolved by renaming --
  which moves the declaration and every call site together, the one case where
  the `#define` seam's usual limitation is what is wanted); `U16_MAX`,
  `DObjGetStruct` and `ftParamGetPlayerNumGObj` had no port declaration; and
  `ftPublicCommonCheck` lost its prototype the moment its port definition
  compiled out, which broke its own caller. The real dependency was
  **`dFTCommonDataPublicFighterCallFGMs`** -- `ft/ftcommondata.c` is not
  compiled here and `nm` finds no such symbol -- now transcribed entry for entry
  beside the import, with the ten missing `nSYAudioVoicePublic*` constants added
  to `gmsound.h` at the decomp's values.
  **It RUNS: a seven-minute both-CPU soak with the flag on reported
  `gNdsFtPublicActorMakeCount 2` and `gNdsFtPublicCommonCheckCount 36`,
  NO-FREEZE, 560,419 textured quads with zero atlas misses and an empty FGM miss
  ring.** (Five of its seven counters cannot fire at all -- the `#define` seam
  renames intra-TU references -- so those two are the whole observation surface.)
  **Still default 0, and now for a MEASURED reason rather than an estimated
  one.** `gNdsTaskmanGeneralHeapFreeMin` -- the battle-time low-water of
  `gSYTaskmanGeneralHeap`, sampled every presented frame -- read **23,544** on
  that run and **26,876** on the same tree one build apart with the flag off.
  So the actor costs **3,332 bytes** and lands **2,056 under the 25,600** at
  which `ifCommonSetMaxNumGObj` caps the GObj pool for the rest of the match,
  while the shipping configuration clears it by 1,276 and never latches. That is
  the latch that was deleting four of Mario's eleven fireballs; with the actor on
  it happened to latch late enough to refuse nothing, which is luck, not
  headroom.
  `PROJECT_GOAL.md` ranks audio fidelity first in the sacrifice order and
  gameplay fidelity above it, so when the two compete for heap the crowd yields.
  Turn it on when the low-water clears 25,600 with margin.
  *Cue side:* the eleven a P1 Mario-vs-Fox match reaches are packed -- chants
  605 Fox / 609 Mario, reactions 615/616/617 Gasp L/M/S, 618 Cheer, 619 Amazed,
  620 GaspClap, 622/623/625 Damage L/M/S. 624 NoContest is unreachable in a
  two-fighter timed match. Pack 535,280 -> 672,528 B, 63 -> 75 entries.
  **The earlier "eight of the twelve are LOOPED or FORKED" was half wrong**:
  `--derive` reports `loop: False` for all eleven, so no loop machinery was
  needed at all. Five do carry fork voices (650, 627, 676, 684, 625) and those
  are omitted with the debt recorded, exactly as the shipping punch/kick cues
  do. 96 `GroundGrind2` is still out -- it has no `pitch` op, so
  `articulation_pitch_cents` derives as `None` -- and it is not a crowd cue;
  it belongs with 85 and 153 `AltitudeWarn` on the miss ring, below.
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

-FIXED (2026-08-01, needs an ear check) Three cues a natural match still asked
  for and did not get (2026-08-01 miss ring, control soak): 96 `GroundGrind2`
  x6, 85 x2, 153 `AltitudeWarn` x2. Not a row of their own before now because
  nothing had read the ring on a clean run. All three are packed.
  - **96** had no `pitch` op, so `articulation_pitch_cents` derived as `None`
    and `validate_articulation` rejected it. No pitch op means zero cents;
    the validator says so now. It is NOT a looped cue in practice -- its
    schedule (55 ticks, 0.316 s) runs out before its 13,040-sample wave
    (0.457 s at 28,509 Hz), so a one-shot plays the whole audible cue.
  - **153** did need the loop -- 300 ticks is 1.725 s against a wave that
    plays out in 0.757 s -- and it is the pack's SECOND DS hardware repeat.
    Whispy's `WHISPY_WIND_*` module constants are a per-cue `hardware_loop`
    spec now (five numbers; four are the same for every cue on the path), so
    the "machinery that needs generalising" this row kept describing was a
    dict. PNT 1 word, LEN 396, SNR 32.9 dB, pinned in the checker like 285.
  - **85 `UnkGrind4` is FIXED too (2026-08-01); needs an ear check.** The row
    used to say it "needs a decision about how the DS should represent a rate
    above the register's range". There was no such decision to make: the
    register is fine. 32000 * 2^(1800/1200) = 90,510 Hz is past the `u16
    frequency` field of the PACK ENTRY (`nds_audio_fgm.c:46`), and the DS
    channel timer reaches roughly a megahertz. `source_rate_above_u16` is a
    statement about one field of ours.
    189, 190 and 219 carry the same blocker and were already answered: render
    the whole source program AOT at `FGM_OUTPUT_RATE` and bake the note
    schedule into the samples, so the entry stores 32,000. 85 has exactly that
    shape -- three notes, no forks -- so it joined `FULL_PROGRAM_AOT_IDS` and
    needed no new machinery. 2,576 samples, 1,292 IMA bytes, SNR 25.07 dB.
    One real gap did have to be closed: its articulation spawns modulator 24,
    `shape 7` (`ramp_down_oneshot`) on `target 28`. The renderer raised on
    both. Per the decomp's own field notes
    (`decomp/tools/extract_fgm.py`), target 24+ is **cross-mod another
    voice** -- and 85 has no fork voices, so it has no destination at all.
    Cross-voice targets are now skipped *before* evaluation, which is why the
    shape never has to be interpreted, and only for a cue with no forks; with
    forks it stays a hard error, because there the modulation is real.

-FIXED (2026-08-01, needs an ear check. the dodge sounds off, maybe too loud???) Five more cues, and these are the ones
  a player would notice first: **11 `Escape`** (the dodge) x3, **13 `GuardOn`**
  x2 and **14 `GuardOff`** (the shield going up and down), **278 `GamePause`**,
  and **369 `FoxOttotto`** (the noise Fox makes teetering on a ledge). All five
  were silent.
  They were invisible until now because **every previous miss-ring read was on a
  SINGLE-CPU soak**, where Mario stands still: nobody dodges, nobody shields,
  nobody teeters. The first both-CPU stress soak (2026-08-01, crowd-actor ROM,
  full match to Results) reported them immediately -- 198 FGM play calls, 190
  supported, 8 unsupported, and those 8 are exactly these five ids.
  All five are bounded multi-note schedules with no fork voices, so all five
  take the same full-program AOT render 85 does. Pack 682,036 -> 700,892 B,
  78 -> 83 entries.
  **And two more after those five (2026-08-01, same day, next soak): 271
  `Magnify` x4 and 368 `FoxWin` x1.** The ring only names what the run reached,
  so each fix uncovers the next layer; these two are the layer under the five.
  Magnify is the zoom pulse -- five 16-pitch blips separated by rests, a bounded
  fork-free schedule, so it takes the same AOT render. FoxWin is one 90-tick
  note whose wave plays out well inside the schedule, so it takes the ordinary
  announcer path 472/471 use and retains all 3,648 source samples.
  **And three more after the weapon-pool fix**, because the first match in which
  every fireball spawned was also the first to reach Sudden Death: **18
  `LightSwingLw1`**, **514 `AnnounceSuddenDeath`** and **365 `FoxSelected`**.
  The ring only ever names what the run reached, so each fix uncovers the layer
  under it -- that is the shape of this row, not a sign it was done badly.
  Pack 700,892 -> 725,900 B, 83 -> 88 entries.
  One piece of machinery was genuinely missing and is now source-transcribed:
  **modulator shapes 6 and 7** (`ramp_up_oneshot` / `ramp_down_oneshot`), which
  FGM 11's articulation spawns. They are shapes 2 and 3 with the phase CLAMPED
  at the period instead of wrapped -- `n_env.c:4158` and `:4172` both assign
  `phase = period` past the end rather than subtracting it, and the value
  expressions are identical to the periodic pair's. Shapes 4, 5 and 8 remain
  unsupported on purpose: they call `randFloat1`/`randFloat2` and are not
  reproducible offline.

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
  VFX IMPLEMENTED 2026-08-01; needs the owner's eye. The stub this row used to
  name is gone (`NDS_R2_PARTICLE_RUNTIME`/`NDS_R2_PARTICLE_DRAW` both default 1,
  so the real `lbParticleMakeScriptID` is live and textured quads are emitted),
  and the gap that remained after that -- Dream Land's own bank registering
  EMPTY -- is closed too.
  It was measured before it was fixed: the both-CPU soak's reject ring caught
  **script 0 and script 1, bank 0, reason 2, twice each**, which is
  `grPupupuWhispyLeavesMakeEffect` and `grPupupuWhispyDustMakeEffect` failing
  closed before the atlas was ever consulted. `ndsParticleLoadEFCommonBank`
  covered only the common bank and every other bank took
  `ndsParticleRegisterEmptyBank`.
  `grpupupu_particle_scb/txb` are now packed alongside the common bank:
  **416 bytes of bytecode over five scripts, three textures**, carried far more
  cheaply than the common bank because it needs no NitroFS texel payload at all
  -- the draw path reads the atlas. Scripts 0 and 1 both draw texture 2 (16x16,
  four frames, 1,024 texels), and the sheet had 1,408 free, so Whispy's leaves
  and dust landed **without touching the 8,192-byte hard bound**.
  Three things had to be got right, and the first two attempts got two of them
  wrong in ways only a soak could show.
  Quad rows are keyed at `NDS_PARTICLE_QUAD_PUPUPU_STRIDE + id` because texture
  2 means a different image in each bank. **The key is the slot's registered
  script pointer, NOT a latched bank id.** The first version compared
  `pc->bank_id & 7` against `gNdsParticleBankPupupuID`, and the soak reported
  BOTH `gNdsParticleBankEFCommonID` and `gNdsParticleBankPupupuID` as **0** --
  `efParticleInitAll` resets `sEFParticleBanksNum`, so two bank loads either
  side of one reset are handed the same slot. That strided **128,278 of 128,298
  common particles** into Dream Land's key space: 126,621 misses against 1,677
  quads, and those 1,677 were efcommon texture 2 drawing Whispy's leaves.
  `sEFParticleScriptBanks[slot]` holds the pointer the slot was registered with,
  so comparing it is an identity test that cannot collide.
  Only the textures scripts 0/1 reference are marked measured-live: the first
  attempt made all three live, and the two that scripts 3 and 4 draw took the
  space before the four-frame sheet the wind actually uses -- the exact failure
  the `QUAD_MEASURED_LIVE` comment warns about, reproduced in one commit.
  And `QUAD_MEASURED_LIVE` itself was **wrong and had to be regraded**. It read
  `(22, 27)` because that was a SINGLE-CPU mask, and a single-CPU match is Mario
  standing still. The both-CPU mask is `0x08400007` -- bits 0, 1, 2, 22, 27 --
  so admitting Dream Land's sheet had evicted texture 0, which carries most of
  what a moving match draws. Texture 0 only fits at all because atlas cells are
  now capped at 16x16 (`QUAD_CELL_MAX`, box-averaged): at its source 32x32 the
  shelf packer gives it a row of its own and wastes half of it. Texture 1 is in
  the mask but has no image in the pack (width 0), so it fails closed forever.
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

-Correct VFX isn't played for various things (running foot dust VFX, fireball hit VFX, fox down B, shield, hard landing vfx, etc)
  **The root cause below is CLOSED as of 2026-08-01, and COVERAGE is closed with
  it -- what remains is the owner's eye.** The original particle scripts run and
  they are textured: `NDS_R2_PARTICLE_RUNTIME` and `NDS_R2_PARTICLE_DRAW` both
  default 1, the imported `lb/lbparticle.c` owns `lbParticleMakeScriptID`, and
  both the common EF bank and Dream Land's are resident.
  Coverage was the open question and it is now answered by measurement rather
  than by counting admitted textures. A five-minute both-CPU soak that ran a
  full match, GAME SET, **Sudden Death** and Results emitted **347,100 textured
  quads with `gNdsParticleQuadMissCount` 0**. A particle whose texture is absent
  draws nothing and raises that counter, so zero over 347,100 means every effect
  the milestone actually reaches found its texture -- including the KO burst and
  the Results sequence, which is what the two rows below were waiting on.
  The admitted set is `{0, 2, 22, 27, 64, 65, 66}` in 6,400 of the 8,192 bytes
  that are a measured hard VRAM bound. It is small because it is the MEASURED
  live set, not because coverage was traded away.
  The original text of this row follows, and its first two sentences are now
  historical:
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
  **SFX qualified 2026-08-01**: the star-KO path asks for nSYAudioFGMDeadUpStar
  (12) and the per-fighter deadup_sfx (Fox 360 / Mario 433), all three packed
  and allowlisted, and a five-minute both-CPU soak through a full match, GAME
  SET, Sudden Death and Results reported **282 FGM play calls with 0
  unsupported and an EMPTY miss ring**. Nothing the milestone asks for is
  missing from the pack any more; what is left is the owner's ear.
  VFX half: same root cause as the VFX row above, and it measured clean too --
  **347,100 textured quads, zero atlas misses** on that same run, so the star-KO
  script's texture is resident. What is left is the owner's eye.
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
  (2026-08-01, SETTLED and now an eye check: the "particle script is not
  resident" half of this row depended on atlas COVERAGE, and coverage measured
  clean. A five-minute both-CPU soak through a full match, GAME SET, Sudden
  Death and Results drew **347,100 textured quads with zero atlas misses**, so
  the KO burst's own texture is in the sheet -- a missing one would have been
  counted by name. What is left is whether the result LOOKS right.)
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

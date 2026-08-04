**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

This board carries verdicts and the numbers that check them. The forensics for each
fix live next to the code that owns it -- the particle generator and its checker,
`battleship_lbparticle.c`, `battleship_efmanager.c`, `render-audio-fgm-phase-pack.py`
-- so a row here should not need to be an essay.

=============================================================================
FOUR SOURCE-MODEL EFFECT ROWS -- CURRENT STATE (2026-08-03)
=============================================================================
The respawn platform, shield, Fox reflector and hard-landing wave are not
sprites: each is an EFDesc drawn as an animated DObj model tree. The port drew
flat atlas quads instead, which is why three cycles of atlas work never
satisfied any of them. All four sit behind NDS_R2_SOURCE_EFFECTS_FULL
(Makefile:1411, default 0).

FIXED. A source desc tree's id==0 root is transform-only, and three places
demanded geometry at the root or refused the shape: the admission gate, the
submit guard, and the accepted-kind list (all reloc_backend_movement.c). With
those corrected the models draw -- gate 1 green, submit 0->213, tris 0->10,551,
texready 0->5,070 on a 781-frame flag-on run -- and the long-standing flag-on
hang went with them (1,801 frames, 98.9% submit rate). See 8508fc8d6, fc905460d.

OPEN.
  * BLOCKS GATE 5: the flag-0 tickhud arm HANGS between frame 175 and 200, so
    there is no control arm to measure against. Not slowness -- both arms are
    the same speed up to 175 (5f/21s .. 177f/27s) and the flag-0 one then stops
    dead. Regression against a pre-campaign RingDump that completed
    (artifacts/performance/2026-08-03_dobjtree_control_ringdump.json).
    MECHANISM: placement, not logic. Three arms at one commit --
      4d1015b75 anchor            200f/28s
      ae7c3e735 with parsers      hangs
      ae7c3e735 parsers reverted  200f/28s
    The freeze-fix parser patches add 312 bytes to .main (0xca050 -> 0xca188),
    shifting .main.rw from 0x020cd728 to 0x020cd860. They are the TRIGGER; the
    defect is whatever those bytes displace, is older than this campaign, and
    must be fixed at its own seam -- never by re-padding .main, and the parsers
    stay bounded (the freeze row depends on them).
    Refuted, do not re-run: runaway/panic counters 0 (21); desc validation
    disabled=0 (22); resolver misalign=0, offsetcalls=0 (25); 28-byte BSS pad
    does not reproduce (26); parser revert at HEAD does not fix, which
    exonerates nothing since later commits remain (27); shift is non-uniform
    with no symbol crossing a region edge, and stale absolute addresses are
    noise at 10 hits vs ~8 expected by chance (29).
    NOT A HANG (32/33). The scheduler walk found no deadlock (mutex 0x22a1e70
    owner=0, nothing in WaitingOnMutex, the only irqWaitList entry is main on
    a VBlank that plainly arrives) and the ARM9 alive throughout.
    FIRST FROZEN LINK (33): gNdsBattlePlayablePacingDrawCalls, taskman_seam.c
    :4881 -- the EARLIEST counter in the present bracket, frozen at 195 with
    PresentedFrames (:4913), LogicFrames (390 = 2x195) and VBlanks (455).
    So the battle present function stopped being ENTERED; it is not stalled
    inside. Meanwhile gNdsFrameCounter advances ~690/s from its OTHER site,
    main.c:86 -- adjacent unconditional statements cannot diverge, so a
    different, non-VBlank-paced loop (11x 60Hz) is now running. PacingResult
    never latched PASS despite presented 195 >= the 180 threshold at :4574,
    confirming ndsBattlePlayablePacingUpdate stopped running too.
    THERE IS NO EXIT (34). Breakpoint at the last present gives the chain:
    ndsBattlePlayablePresentFrame <- ndsBattlePlayablePresentRealtimeFrame
    <- syTaskmanRunTask (:8061) <- syTaskmanLoadScene(scVSBattleStartBattle)
    <- ... <- scManagerRunLoop <- syMainThread5 <- portCoroutineTrampolineC.
    The battle scene is a COROUTINE, not the main thread. Its loop
    (syTaskmanRunTask) has exactly two breaks, :8043 terminal_update and
    :8069 stop_after_iteration, and BOTH fall through to :8074-8078
    ndsBattlePlayableRecordLifecycleTaskmanExit + ndsBattlePlayablePacingFinish
    -- and PacingFinish sets PacingResult = PASS unconditionally at :4588.
    Measured PacingResult is NOT PASS, so the loop never exited by either
    path. The coroutine is SUSPENDED inside the loop and never resumed:
    that is why DrawCalls (:4881) froze, why no exit ran, and why the machine
    stayed alive on other threads. Coroutines resume from ndsOsRunThreads, and
    a coroutine blocking on osRecvMesg(OS_MESG_BLOCK) yields (libultra_os.c
    :237).
    PARKED IN YIELD, STILL ELIGIBLE (35). At two frozen stops the battle
    coroutine (arg=gSYMainThread5, so identity is confirmed) has
    context.lr = portCoroutineYield+25 and finished=0 -- parked in a yield,
    not finished. gSYMainThread5.state = 8 = OS_STATE_WAITING (PR/os.h:96).
    REFUTED, do not re-run: the "thread latched in OS_STATE_RUNNING so
    ndsOsRunThreads skips it" theory. :294-295 accepts WAITING, and the
    thread is sThreads[1], so it IS resumed every pass -- ~690/s.
    So the coroutine is resumed, returns from its yield, re-checks, and
    yields again: it spins waiting on a condition that never becomes true.
    THE WAIT IS NAMED (36). Unwinding the parked context.sp and mapping the
    return addresses by nm gives:
      portCoroutineYield+24 <- osRecvMesg+90 <- gcRunGObjProcess+98
      <- ndsBaseGcRunAll+96 <- gcRunAll+10
      <- ndsIFCommonBattleUpdateInterfaceAllOriginal+98
      <- ifCommonBattleUpdateInterfaceAll+6 <- scVSBattleFuncUpdate+6
      <- syTaskmanRunTask+2076 <- syTaskmanLoadScene <- syTaskmanStartTask
      <- scManagerFuncUpdate   (the last three match cycle 34's live bt)
    The queue is gGCMesgQueue (.main.bss), held in the parked r6; r10 is
    gSCManagerBattleState. So a GObj process inside the INTERFACE update
    blocks in osRecvMesg on gGCMesgQueue and the message never arrives.
    PROTOCOL, from source (37). Exactly three sites touch this queue, all in
    BattleShip: objman.c:2549 creates it with OSMesg sGCMesgs[1] -- DEPTH 1;
    objman.c:2286 is our waiter, gcRunGObjProcess case nGCProcessKindThread
    doing osStartThread(gobjthread) then osRecvMesg(BLOCK); objhelper.c:147
    is the ONLY poster, gcSleepCurrentGObjThread doing
    osSendMesg(..., OS_MESG_NOBLOCK) then osStopThread(NULL), per tic.
    So it is a one-slot handshake: run the GObj thread, block until it sleeps.
    The port's wait matches the measured spin exactly -- ndsOsWaitForQueue
    (libultra_os.c:231-246) sets OS_STATE_WAITING, yields, sets RUNNING on
    resume, re-checks MQ_IS_EMPTY -- which is why state read 8 at the stop.
    NOBLOCK onto a depth-1 queue is a SILENT DROP in the port's osSendMesg
    (:248-259 returns -1 via ndsOsWaitForQueue), so a drop path exists by
    design and needs testing, not assuming.
    DEADLOCK CLOSED (38). At the frozen stop: gGCMesgQueue validCount=0,
    first=0, msgCount=1 -- empty, so NOT the drop path and NOT a broken
    re-check. sThreads[6] (id 10000001) is the GObj thread, state=1 =
    OS_STATE_STOPPED, finished=0, parked with this stack:
      portCoroutineYield+25 <- osStopThread+64 <- gcSleepCurrentGObjThread+62
      <- ifCommonCountdownThread+256 <- ndsOsThreadEntry
    So the poster is the battle COUNTDOWN element, and it is STOPPED.
    ndsOsRunThreads (libultra_os.c:294-295) resumes only WAITING or RUNNABLE,
    so a STOPPED thread is never resumed and can never reach its
    osSendMesg. gcRunGObjProcess blocks forever on a post that cannot happen.
    Both halves parked; that is the whole freeze.
    The handshake worked 307 times first (gNdsTaskmanGObjThreadSleeps=307),
    so this is the LAST countdown sleep, not a broken-from-boot path -- which
    fits the interface-update location and the ~6.5 s timing.
    LEADING MECHANISM (39), source-read only, needs one confirming probe.
    osStopThread(NULL) (libultra_os.c:152-164) sets STOPPED then yields --
    exactly where thread 6 is parked -- and ONLY osStartThread lifts it, at
    :141. But osStartThread has TWO SILENT RETURNS before that line:
      :128 thread->port_entry == NULL
      :136 portCoroutineCreate() == NULL   <- allocation failure
    Either returns with no error, no counter, and the thread never runs, so
    it never reaches its osSendMesg and osRecvMesg(BLOCK) blocks forever.
    sThreads[5] is exactly that signature: id 10000003, state STOPPED,
    port_coroutine == NULL -- registered but never started. And
    gGCCurrentProcess (0x023c6bb0) sits 0x30 below sThreads[5] (0x023c6be0),
    consistent with the blocked process owning THAT thread rather than 6.
    This is the first candidate that can explain all three sensitivities at
    once, via memory pressure on a GOBJ-stack allocation: the 312-byte
    displacement shrinks what is left, tickhud costs more, and the flag-0
    stand-ins allocate differently. Thread 6 parked mid-sleep is then normal.
    CONFIRM BEFORE FIXING: which thread the blocked gcRunGObjProcess owns,
    and whether portCoroutineCreate returned NULL -- both silent paths need a
    counter. Do not fix the wrong return.
    artifacts/verification/2026-08-03_flag0-queue.txt.
  * One unpaired flag-on reading suggests the cost is high (FPS 20.0, ALL
    1.68M/2.24M against the 1.12M gate). Warning, not a verdict -- gate 6 must
    not be proposed until gate 5 prices it. See 4c29b9615a.
  * Reflector desc is disabled at startup because gFTDataFoxSpecial2 loads
    after efManagerInitEffects. Deferred-retry fix written but UNVERIFIED --
    proof is gNdsEFDescDeferRecoverCount>0, needs a Fox down-B. See 48fe59693c.
  * Gate 2 captures not closed: link-15 arms the probe correctly now
    (artifacts/visibility/2026-08-03_shield-vfx.png) but link 15 carries the
    shield AND the reflector, so the frame does not prove which drew.
  * Impact wave draws but its triangles do not reach the effect counter.
    Segment E is excluded -- its G_DL is call-and-return and the geometry is
    inline after it. See 4c29b9615a.

Forensics live in the commit messages and artifacts/verification/2026-08-03_*;
do not restate them here.
=============================================================================

-Respawn floating platform isn't visible when respawning after KO.
    Owner: is don't see the floating revival platform at all. the Halo is not the correct asset to use
    CAUSE: dEFManagerRebirthHaloEffectDesc is a joint-animated MODEL, drawn as one flat quad.
    STAGE: draws flag-on (tree fix, 8508fc8d6). Awaiting a respawn capture. Flag still 0.

-Fox down B VFX is not correct or using correct asset.
    Owner: you are still not using the correct asset for Fox's down B reflector.
    CAUSE: dEFManagerFoxReflectorEffectDesc is an animated model from gFTDataFoxSpecial2, not a sprite.
    STAGE: desc disabled at startup -- its file loads after effect init. Deferred-retry fix written but
    UNVERIFIED (needs a down-B; proof is gNdsEFDescDeferRecoverCount>0). See 48fe59693c.

-Shield VFX not correct
    Owner: texture looks cut in half: `artifacts/visibility/2026-08-03_owner_shield-cut-in-half.png`
    CAUSE: dEFManagerShieldEffectDesc is a model; 'cut in half' is a 1:2 source cell on a square quad.
    STAGE: builds and draws flag-on (8508fc8d6). The capture arms on link 15, but link 15 also carries
    the reflector, so 2026-08-03_shield-vfx.png does not yet prove which drew. Flag still 0.

-Hard landing vfx not not using correct asset.
    Owner: incorrect asset for the impact wave is being used
    CAUSE: dEFManagerImpactWaveEffectDesc is a material-animated model, so no single atlas texture can
    be its 'correct asset'. The row also has a particle half (dust, script 0x58, efmanager.c:2982).
    STAGE: built correctly as a single DObj on link 10 -- its EFDesc omits flag 0x4, so a raw display
    list is right. Executes fully, but its triangles do not reach the effect counter. See 4c29b9615a.

-KO VFX not drawing correctly.
    Owner: Not fixed yet, the "blast pillar" VFX isn't drawing, and doesn't seen to draw on the same z axis as the fighters
    TRACED: efManagerSparkleWhiteDeadMakeEffect (efmanager.c:3725) runs particle script 0x5C, driven from
    ftCommonDeadUpStarProcUpdate. Script 0x5C asks for texture 24, which IS admitted (32x32, source size),
    so the sheet is not the blocker here -- unlike the four rows above, this one IS a particle effect.

-Some "hard hit" (side A attacks that hit) VFX look too big, please apply correct scaling to VFX.
    MEASURED: scale is source-exact (efmanager.c:2175/2197). Last cycle's clamp was in UNREACHABLE code.
    
-Shield Freeze is back
    **FIXED** (2026-08-03) all three captures spun in gcParseDObjAnimJoint's event loop, whose `default:` case is
    the one branch that consumes nothing -- an invalid opcode re-reads one word forever with interrupts still on,
    which is exactly a frozen picture that keeps presenting. Measured opcode 100 (the captures' r6=0x64) on a
    script pointer 2 mod 4, which cannot be an AObjEvent32*. All four animation parsers now record the fault and
    end that one animation. 7-min both-CPU soak NO-FREEZE with gNdsObjAnimRunawayCount=10 on mask bit 0.
    Open and no longer fatal: what hands the parser a misaligned script. Not the shield table (49 installs probed
    clean) and not ndsRelocResolvePointerFromFileBase (its offset fallback measured 0 calls).

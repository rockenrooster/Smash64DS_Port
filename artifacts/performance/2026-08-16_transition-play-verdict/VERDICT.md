# CLOSED — REJECTED. The owner played the transition-play arm and killed it: the suppressed play is not a duplicate, it is the only thing that puts the new status's first pose on the fighter, and holding it reads as animations not playing

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `e22bc197b45`**
**1 lab build, 1 live capture pair, 1 gdb engagement probe, 0 defaults flipped, 0 ROMs published,
both root ROMs byte-unchanged.**

```text
VERDICT      BLOCKED(decision: transition-frame animation play) is CLOSED - REJECTED.
             The owner played builds/build-c224-animhold and returned:

                 "kill animhold, lots of issues with animations not visibly playing"

             That is the acceptance gate this item needed and it has now been run.
             DO NOT RE-PROPOSE IT.  The attach chain (+23,801, capped 10,496 /
             uncapped 11,968) is the same question about the same transition and is
             closed by the same verdict.

FORGONE      13,376 at rank-80 capped, 37,027 uncapped, against a requirement of
             +48,081.  ATTACH_LANE.md section 4.  It was the largest item in the
             attach lane and the only one clearing the >=14,080 cross-build floor.
             It is no longer available at any price.

WHY IT FAILED, FROM THE SOURCE.  ftMainSetStatus does not play a clip that the
             frame update is about to play again.  It RESETS every common joint to
             the model's bind transform first (ftmain.c:4655-4668), zeroes
             TransN/XRotN/YRotN (4669-4703), attaches the new figatree without
             posing it (4704; gcAddDObjAnimJoint sets every AObj to
             nGCAnimKindNone, objanim.c:137-149), and only then plays (4787-4795).
             That play is the SOLE writer of the new status's first pose, and the
             reset exists BECAUSE it runs.  Section 2.

CORRECTION   ATTACH_LANE.md section 4 option 1, "defer the play to the ordinary
             frame update (drop it entirely)", does not leave the fighter one frame
             behind.  It leaves it in the BIND POSE for one frame.  This cycle
             therefore built the pose-carry-over variant instead, which is the
             cleanest expression of "one frame of hitbox lag" that exists -- and
             the owner rejected even that.  Section 3.

ALSO WRONG   ftMainRunUpdateColAnim does NOT place hitboxes or hurtboxes.  It is
             the COLOUR animation (GMColAnim, nGMColEvent*, ftmain.c:1203-1211).
             The hitbox consumer is ftMainProcSearchHitAll, a separate GObj proc.
             Section 2.3.
```

---

## 1. What was built, and what the owner played

`builds/build-c224-animhold/smash64ds-battle-playable-proof-hwtri.nds`
(12,242,944 B, 2026-08-16 15:16), lab target only, built with
`NDS_R2_FT_TRANSITION_PLAY_TOGGLE=1`. **Left on disk deliberately.** Neither root
ROM was rebuilt.

One binary, both arms, SELECT (RIGHT SHIFT in the repo melonDS) flips between
them mid-match, arm named on battle text-HUD row 3 with its own repaint gate.
Boots on the shipping arm. Same shape as the camera toggle at `35fd951aeb2`,
which is why it took one build.

The toggle's production-source half is **reverted** in the same cycle that
shipped it: it is a switch for a rejected gameplay change, and leaving a
one-flag path to it in the tree is an invitation to re-propose it. It is
committed at **`07802aa6963`** and reverted in the commit that carries this
document, so the ROM on disk stays reconstructible —
`git checkout 07802aa6963 -- Makefile include/nds/nds_startup.h
src/port/reloc_backend_compat_shims.c src/import/battleship_ftmain.c
src/nds/nds_platform.c`, then
`make TARGET=smash64ds-battle-playable-proof-hwtri BUILD=builds/build-c224-animhold
NDS_R2_FT_TRANSITION_PLAY_TOGGLE=1`. **It should not be needed. The decision is
closed.**

The one piece kept is `scripts/capture-melonds.ps1 -SelectPresses`, which sends
VK_RSHIFT with its own scancode between the two captures. Every lab toggle ROM
binds SELECT and every one of them needs exactly this from the harness; the
camera toggle cycle did it by hand, which is the second occurrence, so it is now
in the shared helper. `SendKeys` cannot express it — the repo config binds
SELECT to `0x81000020` = `Qt::Key_Shift` | melonDS's right-modifier bit, i.e.
right shift specifically, and `SendKeys` has no way to say which shift it means.

---

## 2. Why the play cannot be suppressed — the durable finding

`ftMainSetStatus`, whenever `fp->figatree != NULL` (that is, on essentially every
status change that has an animation), runs this sequence:

| ftmain.c | what it does to the joints |
|---|---|
| 4655-4668 | writes **every** common joint's `translate` / `rotate` / `scale` from `attr->commonparts_container->commonparts[detail].dobjdesc` — the model's **bind** transform — and clears `joint->flags` |
| 4669-4703 | zeroes `TransN`, `XRotN`, `YRotN` |
| 4704 | `lbCommonAddFighterPartsFigatree` attaches the new figatree. `gcAddDObjAnimJoint` sets every AObj in the chain to `nGCAnimKindNone` (`objanim.c:137-149`, and `ATTACH_LANE.md` §3.1 states the same independently) — **the attach poses nothing** |
| 4787-4795 | `ftMainPlayAnimEventsForward` (`frame_begin != 0`) or `ftMainPlayAnimEventsAll` + `ftMainRunUpdateColAnim` |

Both play arms reach `ftMainPlayAnim` (`ftmain.c:932`) → `ftParamUpdateAnimKeys`.
**That call is the only writer of the new status's first pose in the entire
sequence, and the reset three steps above it is only safe because it runs.**

The measured "~1.6 whole plays per transition" is therefore not one play plus a
duplicate. It is the **old** status's play, which the ordinary frame update
already did, plus the **new** status's establishing play. They evaluate
different clips. Deleting the second one deletes the new pose; deleting the
first one requires knowing a transition is coming, which nothing does.

### 2.1 The order inside a transition frame

`ftmanager.c:858-863` registers six per-fighter procs at descending priority.
Within `ftMainProcUpdateInterrupt` (priority 5):

```text
ftmain.c:1399   ftMainPlayAnimEventsAll      <- plays the OLD clip
ftmain.c:1401   ftMainRunUpdateColAnim
ftmain.c:1501   this_fp->proc_update(...)    <- raises ftMainSetStatus
ftmain.c:1505   this_fp->proc_interrupt(...)      "
```

then, still in the same frame, priorities 4 → 0: `ftMainProcPhysicsMapDefault`,
`ftMainProcPhysicsMapCapture`, `ftMainProcSearchCatch`, **`ftMainProcSearchHitAll`**,
`ftMainProcParams`.

So the transition play is the **last** pose written before physics, catch
search, hit detection and the draw all read it.

### 2.2 What reads that pose before the next frame's ordinary update

- `ftMainUpdateMotionEventsAll`, in the same call — `ftParamMakeEffect` spawns
  effects at joint **world** positions.
- `ftMainRunUpdateColAnim` on the next line.
- `ftMainProcSearchHitAll` (priority 1) — attack collisions hang off
  `attack_coll->joint`, so every hitbox and hurtbox position for that frame
  comes from these transforms.
- the renderer, for the frame that is presented.

### 2.3 `ftMainRunUpdateColAnim` is the colour animation

`ATTACH_LANE.md` §4 and this cycle's brief both describe it as placing hitboxes
and hurtboxes. It does not. `ftMainRunUpdateColAnim` (`ftmain.c:1203-1211`)
loops `ftMainUpdateColAnim` over `fp->colanim`, a `GMColAnim` of `nGMColEvent*`
opcodes — the damage flash and the colour scripts. Hit detection is a different
proc entirely (§2.1). Corrected here so the next reader does not inherit it.

---

## 3. What was actually built, and why it was not option 1 as written

Option 1 as written — drop the play — produces a **bind-pose frame** on every
status transition, by §2. That is a flash, and `AGENTS.md` does not allow a
flash to be handed to the owner as a feel question, so it was not built.

The variant built instead snapshots the joint locals at the top of
`ftMainSetStatus` and re-asserts them in place of the play, inside
`ftParamUpdateAnimKeys` — before `ftParamsUpdateFighterPartsTransform` on the
next line and before the motion events, so nothing downstream ever sees the bind
pose. Motion-event execution, attack-collision creation, SFX, effects and the
colour animation were all left untouched; only the joint evaluation was
deferred. `TransN`/`XRotN`/`YRotN` were deliberately not restored, because
`ftMainSetStatus` zeroes them as the new clip's root-motion origin and
re-asserting the previous status's root offset would inject a translation rather
than delay one.

**This is the cleanest possible isolation of "one frame of pose and hitbox lag,
on transition frames only", and it still failed the owner's eye.** That is a
stronger result than a rejection of the crude version would have been: it closes
the whole family, not one implementation of it.

Transition frames are **288 of 1,600** sampled frames (18.0%) on the basis run,
carrying 1.22 `ftMainSetStatus` calls each (`ATTACH_LANE.md` §§1, 3).

---

## 4. Engagement, and a control that could fail

`artifacts/verification/2026-08-16_transition-play-hold.txt`, one gdb session on
the lab ROM, breaking on `ndsBattlePlayableFrameCompleteMarker`, with the route
poked rather than keyed so the route itself is what is under test:

```text
AHOLD entered route=0 holds=0
AHOLD n=50  route=0 holds=0
AHOLD n=100 route=0 holds=0
AHOLD n=150 route=0 holds=0
AHOLD FLIP  n=200 route=1 holds=0
AHOLD n=250 route=1 holds=7
AHOLD n=300 route=1 holds=18
AHOLD n=350 route=1 holds=23
AHOLD n=400 route=1 holds=32
```

**The control can fail and did not**: `holds` is exactly 0 for 200 presented
frames of live fighting on the same binary, in the same match, and becomes
non-zero within 50 frames of the route moving. 32 holds in 200 presented frames
is 0.16/frame against ~0.22 `ftMainSetStatus`/frame expected from §3's figures —
the shortfall is the calls that take `ftMainSetStatus`'s no-figatree early exit
and therefore never reach a play.

The keypad path is proven from the linked ELF, not argued
(`ndsPlatformReadInput`, `.main`):

```text
2006d1c  bl   keysDown
2006d20  lsls r0, r0, #29        ; KEY_SELECT is bit 2
2006d22  bpl.n 2006d2e
2006d24  ldr  r2, [pc, #108]     ; literal 0x020e6dc0
2006d26  ldr  r3, [r2, #0]
2006d28  negs r1, r3
2006d2a  adcs r3, r1             ; logical not
2006d2c  str  r3, [r2, #0]
...
2006d94  .word 0x020e6dc0        ; = gNdsR2FtTransitionPlayRoute  (nm: D)
```

and the wrapper is where it claims to be (`ftMainSetStatus`, `.main`):

```text
20b2e70  bl 2054944 <ndsR2FtTransitionPlayBegin>
20b2e82  bl 20b2554 <battleship_ftMainSetStatus>
20b2e88  bl 2052350 <ndsR2FtTransitionPlayEnd>
```

Live, both arms, one binary, fight continuing across the flip —
`artifacts/visibility/2026-08-16_transition-play-toggle/`:
`toggle-before-play.png` reads `ANIM PLAY  shipping[SELECT]` at `FPS 27.9`,
`toggle-after-hold.png` reads `ANIM HOLD  1-frame [SELECT]` at `FPS 29.8` three
seconds and one SELECT press later. Both are cropped 30 px in from the window
rect's left edge, which `CopyFromScreen` fills with desktop rather than
emulator; that clips the first two characters of each HUD row and nothing else.

**The toggle was never a measurement instrument and must not be used as one.**
It changes gameplay, so the two arms stop playing the same fight the moment it is
flipped; and the hold arm pays a joint snapshot and restore that no real
implementation would.

---

## 5. What this cycle did NOT do

- **No default was flipped, no ROM published, nothing banked.** The requirement
  is unmoved. Both root ROMs were not rebuilt and both hashes are unchanged.
- **`NDS_R2_FTR_DRAW_MEMO` was NOT touched and stays `?= 1`.** It was briefly
  suspected of the owner's symptom; the owner was playing this cycle's own ROM,
  so the symptom is §2's mechanism and the memo is not implicated. Its
  display-list-swap hole was **not** re-verified this cycle.
- **No measurement run.** No gate run, no profile, no tick figure. This cycle
  produced a thing to play, not a number.
- **`build-c205-camtoggle` was not rebuilt**, and `build-c224-animhold` was not
  deleted.
- **`decomp/` untouched.**

---

## 6. Verification and hashes

```text
root ROMs, unchanged and not rebuilt this cycle:
  smash64ds.nds                        54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
  smash64ds-battle-playable-hwtri.nds  6c939434c53c9b3a76ff016540b810a84f207b1a4e24540b8653b15717367c99
```

Boundary green, exit 0, `0 Exception:`. It ran against the tree that still
carried the toggle source; every block of that source is gated on
`NDS_R2_FT_TRANSITION_PLAY_TOGGLE`, which is `?= 0` and which no target block
sets, so the verified object code and the object code of the tree after the
revert are the same.

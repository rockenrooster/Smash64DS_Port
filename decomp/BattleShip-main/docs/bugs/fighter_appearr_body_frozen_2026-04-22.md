# Fighter AppearR body frozen — walker raw-scan 1-step-End false positive (2026-04-22)

## Symptom

Fighter Appear intros (Mario 0xDD / Link 0xE1) rendered bodies frozen in
Wait stance. Entry effects (pipe, shiny beam, capsule, pokeball, egg,
barrel, Arwing, car) animated correctly but the fighters themselves stood
still for the full Appear duration, then snapped to the final on-stage
position. See handoff notes at
`docs/fighter_appear_body_static_handoff_2026-04-21.md`.

## Root cause

Regression introduced by commit 3161c13 ("Walker: reject ambiguous streams
where raw form also parses cleanly"), which hardened the AObjEvent32
halfswap walker (`port/port_aobj_fixup.cpp`) against over-accepting
already-native streams by rejecting any stream where BOTH the
un-halfswapped and raw interpretations scan to a clean terminator.

The new raw-scan check validated on a **one-step End** — a stream where
the very first command's bits [31:25] happen to equal opcode 0 and the
scan returns immediately. That case is overwhelmingly a halfswap-
coincidence rather than a genuinely native stream:

- The file-level halfswap in `portRelocFixupFighterFigatree` moves the
  original opcode bits from [31:25] to [15:9] and places the original
  low payload bits into [31:16].
- Low payload bits are frequently zero (short counts, small enum
  values, zeroed padding in the authored data).
- So bits [31:25] of the halfswapped u32 read as opcode 0 (End)
  extremely often, and the raw scan terminates in a single step.

Example from Mario AppearR (`reloc_animations/FTMario`):

    dobj=0x1017dcef8 ev32=0x1017b3c54 aw=CHANGED
      raw=[0x00000810 0x00000000 0x00001628 0x00000000]

Raw interpretation of u32[0]=0x00000810: opcode = (0x810 >> 25) & 0x7F =
0, so raw scan hits End at step 1 and returns true. Un-halfswap of
0x00000810 = 0x08100000, opcode 4 (SetVal), flags 0x20 (1 data slot)
— the stream's real first command.

Pre-3161c13 walker: un-halfswap scan validated, stream was un-halfswapped,
parser read real animation commands. Post-3161c13: both scans validated
(raw via one-step End, un-halfswap via full walk), walker rejected the
whole stream, parser read the raw bytes, hit End on command 0, terminated
the animation immediately. `anim_wait` advanced from AOBJ_ANIM_CHANGED to
AOBJ_ANIM_END to AOBJ_ANIM_NULL in a single parse call, and every
subsequent parse early-returned. Fighter body joints stayed at their
Wait-stance default pose for the entire intro.

18 of Mario's ~22 body joints exhibited this pattern; the remaining ~4
had raw u32[0] bits [31:25] ≠ 0 (the original payload happened to set a
high bit) and took the correct un-halfswap path, which is why the
characters "moved their arms and legs" visibly but were in the wrong
pose / orientation — a mix of animated and frozen joints.

## Fix

Replace the binary "both scans valid → reject" rule with a **length
comparison**. Phase 1 runs the un-halfswap scan on the main stream
(recursion disabled, just to get a step count). Phase 2 runs the raw
scan on the main stream. If raw validates **and** its step count is
≥ 2, the stream is treated as genuinely native and rejected. Otherwise
the un-halfswap interpretation wins and a phase-3 full scan (with
recursion enabled) collects the sub-stream pending list.

The `>= 2` threshold is the key heuristic:

- A real native animation carries SetVal / Wait / etc. before reaching
  End — raw will always walk ≥ 2 commands.
- A halfswap-coincidence raw-End almost always terminates at step 1.
- 2-command raw false positives (SomeValidOpcode → End) are
  statistically rare: both commands' opcode bits, flag popcounts, and
  advance positions must all land in the valid ranges simultaneously.

Verified: Mario's pipe rise + camera-facing pose and Link's sky descent
both play correctly post-fix. The previously-confirmed Explain-tutorial
gestures and mvOpeningRoom animations still work.

## Class-of-bug lesson

A dual-interpretation validator (does the stream parse under form A?
under form B?) needs a tiebreaker for ambiguous cases. Binary
validity ("both valid → reject") is fragile when one form's terminator
is a single-byte-wide opcode (like End's 7 bits in the top 7 of the
u32): low-entropy data hits the terminator condition frequently enough
that the "validity" signal is near-useless.

A cheap defense is length comparison: the correct interpretation walks
further than the accidental one. For this codebase, the threshold lives
in `port/port_aobj_fixup.cpp:walk`. If future changes add new opcodes or
adjust field layouts, re-verify the 1-command-raw false-positive rate
stays the dominant failure mode and the threshold still catches it.

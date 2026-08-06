# CSS Win-Pose Script Pointer Cleared by Defensive NULL Guard — 2026-04-29

**Status:** RESOLVED (issue [#8](https://github.com/JRickey/BattleShip/issues/8))

## Symptom

Locking in a character on the VS / 1P / Bonus / Training Character Select Screen plays the body animation but no per-character VFX (Link sword spark) or SFX (Yoshi "Yoshi!", DK growl, Samus sword unsheathe, Link swoosh) fire. Same on every fighter.

## Root cause

In `ftMainSetStatus` (`src/ft/ftmain.c` ~line 4949–4992), the win-pose path resolves the motion-event script via:

```c
event_file_head = *fp->data->p_file_submotion;            // or p_file_mainmotion
event_script_ptr = motion_desc->offset + event_file_head; // BE original
```

`motion_desc->offset` for opening / win-pose / results entries is **already a fully-resolved script pointer** — the submotion-desc tables in `src/sc/scsubsys/scsubsysdata*.c` cast `D_ovl1_*` symbol addresses (e.g. `(intptr_t)D_ovl1_80391C08` for Yoshi Win2's `ftMotionPlayFGM(nSYAudioVoiceYoshiAppeal)` blob) into the offset slot. On N64 the addend `event_file_head` came from dereferencing a NULL `p_file_submotion`, which on the N64 RDRAM map reads `0`, so `pointer + 0 = pointer` worked.

Commit `f255803` (2026-04-06, "Fix NULL pointer crashes in fighter animation and motion script setup") added a defensive guard for the (real) NULL deref crash on PC:

```c
event_file_head  = (p_file_submotion != NULL) ? *p_file_submotion : NULL;  // good — guard the deref
event_script_ptr = (event_file_head != NULL) ? offset + event_file_head : NULL;  // bad — kills offset-as-pointer
```

The deref guard is correct. The addend guard is wrong: with `event_file_head=0`, `offset + 0 = offset`, which is the script pointer the table author stored. The guard turned that into NULL, the per-frame parser saw `p_script == NULL`, and no opcodes ever parsed.

The `pkind`/`is_muted` gates are unrelated red herrings — `is_muted` is only set TRUE by `mncharacters.c` (the standalone "Characters" demo menu, not the VS/1P CSS), and the SFX opcodes never gate on `pkind`.

## Fix

`src/ft/ftmain.c`: drop the addend NULL guard, keep the deref guard. `motion_desc->offset + event_file_head` always produces the right value — when `event_file_head == 0` (table stores absolute pointer) it returns the pointer; when nonzero (legitimate file-relative offset) it returns the resolved address.

```c
event_file_head = (fp->data->p_file_submotion != NULL) ? (intptr_t)*fp->data->p_file_submotion : 0;
event_script_ptr = (void*) ((intptr_t)motion_desc->offset + (intptr_t)event_file_head);
```

Three call sites in the same function — both branches of the `is_use_submotion_script` switch in the `status_struct != NULL` path, plus the `status_struct == NULL` (opening / win-pose) path. Comment explains the offset-as-pointer convention so the next defensive-guard pass doesn't re-introduce it.

## Verification

- Yoshi Win2 lock-in: `ftMotionPlayFGM(nSYAudioVoiceYoshiAppeal)` (id `0x247`) reaches `func_800269C0_275C0`, queues an FGM slot. Voice audible.
- Link, DK, Samus also fire their CSS-selected voices.
- Link Win 1115 (`D_ovl1_8039191C`) lock-in: `nFTMotionEventEffect` opcode (38) following `BladeSwing3` SFX dispatches `nEFKindSparkleWhiteScale` on joint 11; white sword spark renders. Confirmed visually 2026-04-29 in VS-mode CSS. Same parser fix unblocks both SFX and VFX opcodes — issue #8 fully resolved.

## Class of bug

Defensive NULL guards over arithmetic that reads a NULL pointer's value (rather than dereferencing through it). On N64 the value is `0` and arithmetic is harmless; on PC the deref crashes but `0 + offset` is still the right answer once the deref is guarded. Audit any `f255803`-era guards that combine a NULL check with an arithmetic combine — guard the **load**, not the **add**.

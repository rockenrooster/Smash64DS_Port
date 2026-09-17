# The cleanup audit, verified item by item: one landed, one refuted at its premise

An owner-supplied dead-code audit proposed **−7,500 lines** across five items.
This is what each one measured out to. The headline: **the largest item cannot
be done at all**, and the whole campaign returns a rounding error against the
constraint that is actually blocking P2.

## The measurement that reframes the campaign

Every item was sized against the shipping shell ELF before any edit, because
`-ffunction-sections` plus `--gc-sections` means unreferenced code is *already*
absent from the ROM and deleting its source returns nothing.

| item | source lines | ROM bytes returned |
|---|---:|---:|
| 4 `port_probe` | 64 | **216** (measured, `.text` 1,536,756 → 1,536,540) |
| 1 proof fleet | 5,147 | **360** (17 diagnostic globals; every function inlined away) |

The proof fleet is **80× the source churn of `port_probe` for 1.7× the bytes**.

For scale, the thing that actually blocks P2 right now: Ness needs **20,480
bytes** (5 arena pages) to become selectable, and Kirby **86,016**. The entire
audit does not approach either. The dormant audio boot thread in the owner's own
OTHR review is **16,384 bytes from a handful of lines** — roughly 45× this
campaign's return per line changed.

## Item 4 — `port_probe`: DONE

Correct as described. Renderer empty, state unread outside the file, its own
comment says asset previews replaced it, and it still ran `portProbeUpdate`
every main-loop iteration.

**One caveat in the audit was unnecessary, and checking mattered.** It said to
keep `syUtilsSetRandomSeedPtr(NULL)` "if required". This is the *only* caller of
that function in the port, so if it had been required, deleting it would have
left the RNG dereferencing a null seed pointer. It is not required —
`decomp/.../sys/utils.c:15` already has

```c
s32 *sSYUtilsRandomSeedPtr = &sSYUtilsRandomSeed;
```

which is exactly what the `NULL` argument assigns. Nothing was preserved.

## Item 1 — the proof fleet: REFUTED AT ITS PREMISE

**`src/port/reloc_backend_ftmain_*_proofs.c` is not a deletable fleet. Twenty of
its forty-three functions are live gameplay helpers.**

They are called from outside the fleet by `reloc_backend.c`,
`reloc_backend_cliff_ledge.c` and `reloc_backend_ftmain_status_compat.c`:

```c
/* reloc_backend_cliff_ledge.c:6830 -- assigning a fighter's motion */
fp->motion_id = ndsFTCommonDamageMotionForStatus((s32)fp->status_id);
fp->hitlag_tics = hitlag;
fp->is_knockback_paused = TRUE;
```

The twenty include `ndsFTCommonDamageIsStatus`,
`ndsFTCommonDamageMotionForStatus`, `ndsFTCommonDamageSelectStatus`,
`ndsFighterDashRunProbeHurtboxDamageConsume`,
`ndsFighterDashRunProbeCatchSearch`,
`ndsGMCollisionCheckFighterAttackShieldCollideSelected` and twelve more —
damage-status selection, hurtbox consumption, catch search, shield collide.
Deleting the files breaks the build; deleting their call sites too would remove
real behaviour.

### Why they look dead when they are not, and how that nearly fooled me

The functions are `static` in translation units textually `#include`d into
`reloc_backend.c`, so the compiler inlines them and **no standalone symbol
reaches the ELF**. My own first measurement — *"43 functions defined, 0 linked,
0 bytes"* — is exactly what an inlined static looks like, and I read it as
confirmation the fleet was dead. It only ever meant the symbols were inlined.

**The filename is the trap.** These are named `*_proofs.c` and roughly half
their contents are ordinary gameplay. A directory listing cannot tell you which.

## Item 1's survivors, and a scope correction

The **158 unreachable harness constants are real**. Verified independently: the
Makefile admits exactly five harness names — `normal`=0, three battle variants
=163, `results_playable`=164 — and every other name hits
`$(error Unknown NDS_DEV_SCENE_HARNESS)`. So IDs 1..162 cannot be selected by
any build, and 158 of the 163 constants are dead.

But **three files carrying those references are not in the audit's list**, and
one of them carries more than any file that is:

| file | refs to dead constants | named by the audit |
|---|---:|---|
| `src/port/taskman_seam_harness.c` | **455** | **no** |
| `src/port/reloc_backend_fighter_model.c` | 270 | yes |
| `include/nds/nds_scene_harness.h` | 158 | yes |
| `src/port/scene_harness.c` | 158 | yes |
| `src/port/reloc_backend_compat_shims.c` | **26** | **no** |
| `src/port/reloc_backend_movement.c` | **6** | **no** |

Two of the unnamed files are large shared TUs (14,031 and 14,181 lines), so this
is excision threaded through live code, not whole-file removal. The risk profile
is inverted from the line count: deleting files is safe, unpicking 481
references from three live 14k-line TUs is where a silently-changed predicate
hides.

## Items 2, 3 and 5 — not attempted

Deliberately. Item 1's central claim did not survive contact with the linker and
the call graph, so the remaining items get the same two checks first — what is
actually linked, and what actually calls it — rather than being executed on the
audit's description.

## What this says about audits generally

The audit's three self-corrections were all sound, and its reasoning is careful.
It still got its biggest item wrong, because **"what is in this file" and "what
is reachable" are different questions, and neither is answerable from a name or
a line count.** The two cheap checks that settle it are `nm` against the linked
ELF and a call-graph sweep that excludes self-references — both used here, both
under a minute.

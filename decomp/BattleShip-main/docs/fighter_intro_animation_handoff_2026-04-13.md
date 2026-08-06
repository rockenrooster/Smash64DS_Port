# Fighter Intro Animation — Handoff Notes (2026-04-13)

Continues from `docs/fighter_intro_animation_investigation.md`. That prior doc
defined three leads for the next session; all three have been dispatched. One
found the root cause, two ruled their hypotheses out. The root-cause fix is
committed (`006128e`) but exposes a downstream **progression segfault** in
Samus's intro scene that this doc hands off.

## TL;DR

- **Committed fix (`006128e`):** `FTMotionFlags` positional-initializer bug —
  every `FTStatusDesc` literal on LE was writing `motion_id = 0` for every
  action, so fighters approximately played the right body motion but never
  fired SetFlag/Effect/SetTexturePartID events. Fox laser, Yoshi egg, DK
  Hand Slap, mouth opens, eye blinks, rumble, etc. were all gated on this.
  Runtime evidence: post-fix Fox `status=0xbe motion=165`, DK
  `status=0xe8/0xe9 motion=207/208`, 15+ other status→motion mappings
  verified reading real values where they were all 0 before.
- **Open progression segfault:** post-fix, ~1 in 3 runs crashes around frame
  1180 during `mvOpeningSamusStartScene` with `EXC_BAD_ACCESS` inside
  `gcParseDObjAnimJoint` called from `ftCommonGuardUpdateJoints`. Full
  backtrace below. Pre-fix was stable — this is a code path that was
  previously never reached because motion_id was always 0.
- **Leads 1 & 3 ruled out:** matanim event32 parsing is byte-exact vs ROM;
  `FTMotionDesc` LP64 stride is fine (compile-time arrays, no file-backed
  indexing, runtime `sizeof=24` confirmed and strides match source).
- **Known secondary issues below**, including six partial-bit
  `FTMotionEvent*` structs whose `<32`-bit totals make the LE mirror bits
  land at wrong offsets, and an incorrect attribution in
  `ftcommonguard1.c`'s `#ifdef PORT` bailout comments.

## The committed fix — `FTMotionFlags` positional initializer

`src/ft/fttypes.h:271-290` (before `006128e`) declared:

```c
struct FTMotionFlags {
#if IS_BIG_ENDIAN
    s16 motion_id : 10;
    u16 attack_id : 6;
#else
    u16 attack_id : 6;   // LE mirror — fields in reverse source order
    s16 motion_id : 10;  //   so runtime bit positions match BE u32 layout
#endif
};
```

The runtime bit layout is correct on both ABIs. **But C positional
initializers fill fields in source declaration order, not bit order.** Every
`FTStatusDesc[]` literal like:

```c
{
    nFTCommonMotionWait,       // meant for motion_id
    nFTMotionAttackIDNone,     // meant for attack_id
    ...
}
```

on LE wrote `nFTCommonMotionWait` into `attack_id` (the first-declared field)
and left `motion_id = 0` for every action. `ftMainSetStatus` then looked up
`dFT*MotionDescs[0]` — typically the Wait stance — for every action, so the
per-status motion script never ran and no `SetFlag0`/`nFTMotionEventEffect`/
`SetTexturePartID` events fired.

The fix replaces the bitfield with plain fields under `#ifdef PORT`:

```c
#ifdef PORT
    s16 motion_id;
    u16 attack_id;
#elif IS_BIG_ENDIAN
    ...
```

Struct grows 2→4 bytes. `FTStatusDesc` is never loaded from file data so the
size bump has no data-layout fallout. Verified in a 2000-frame intro run:
Fox, DK, Yoshi, Samus, and the other fighters all produce diverse non-zero
`motion_id` values for action states in `ssb64.log`.

## The open progression segfault

Post-fix, a 2000-frame intro run segfaults ~1 in 3 runs around frame 1180.
Pre-fix was stable at the same frame count. Two recent crash reports are in
`~/Library/Logs/DiagnosticReports/ssb64-2026-04-13-*.ips`.

### Backtrace (from `ssb64-2026-04-13-004119.ips`)

```
EXC_BAD_ACCESS (SIGSEGV) at 0x0a03800000000000 → 0x0000000000000000
(possible pointer authentication failure)

#00 ssb64  gcParseDObjAnimJoint+568
#01 ssb64  ftCommonGuardUpdateJoints+296
#02 ssb64  ftCommonGuardOnSetStatus+244
#03 ssb64  ftCommonGuardOnCheckInterruptSuccess+88
#04 ssb64  ftCommonGuardOnCheckInterruptCommon+28
#05 ssb64  ftCommonWaitProcInterrupt+200
#06 ssb64  ftMainProcUpdateInterrupt+2804
#07 ssb64  gcRunGObjProcess+188
#08 ssb64  gcRunAll+280
#09 ssb64  syTaskmanCommonTaskUpdate+44
#10 ssb64  syTaskmanRunTask+948
#11 ssb64  syTaskmanLoadScene+956
#12 ssb64  syTaskmanStartTask+752
#13 ssb64  mvOpeningSamusStartScene+88
#14 ssb64  scManagerRunLoop+1932
```

### Why it's a progression, not a regression

Pre-fix: `motion_id = 0` for every action meant the "Wait stance" figatree
was always loaded. `ftCommonGuardUpdateJoints` then either hit the
`yrotn_joint == NULL` bailout (the `#ifdef PORT` guard at
`src/ft/ftcommon/ftcommonguard1.c:224-245`) or ran with valid joints because
the Wait stance's hidden-parts mechanism produced them. Either way, the
buggy code path was unreachable.

Post-fix: real action motions load real figatrees, `anim_desc.word`
correctly requests the YRotN joint (via `is_use_yrotn_joint` bit),
`ftMainUpdateHiddenPartID` creates it, and then
`ftCommonGuardUpdateJoints` progresses past the null guard and into
`gcAddDObjAnimJoint` + `gcParseDObjAnimJoint`. The crash happens inside
`gcParseDObjAnimJoint` at +568.

### Investigation starting points for the next session

1. **Symbolize `gcParseDObjAnimJoint+568`.** Build with debug symbols (it
   already is — the .ips resolved the function name). Use
   `lldb build/ssb64 -c /path/to/core` or disasm:
   ```bash
   dsymutil build/ssb64 -o build/ssb64.dSYM 2>/dev/null || true
   lldb build/ssb64 -o 'dis -n gcParseDObjAnimJoint' -o quit 2>&1 | head -120
   ```
   At offset +568, the crash is inside an inner loop — `gcParseDObjAnimJoint`
   lives in `src/sys/objanim.c`. Cross-reference the disasm against that
   source to identify the exact line.
2. **The event16 parser** reads `AObjEvent16` streams from figatree files
   (bswap32 + ROT16 pipeline). Figatree byte-swap is verified byte-perfect
   (see the prior investigation doc's "2026-04-12 evening update"). So the
   crash is likely **not** raw byte corruption — more likely one of:
   - An event16 opcode the parser doesn't handle (wrong default path,
     dereferences garbage).
   - An AObj/joint wiring mismatch where `fp->joints[YRotN]` is now
     non-NULL but some sibling pointer (`fp->attr->shield_anim_joints[]`,
     `fp->attr->translate_scales`, `fp->attr->dobj_lookup`) is still NULL or
     still stale from when the bailout was taken.
   - A `PORT_RESOLVE` call on an unresolved token in the shield-anim path.
3. **Reproduce deterministically.** The ~1/3 failure rate hints at ASLR-
   dependent uninitialized memory. Try `SSB64_MAX_FRAMES=2000 ./build/ssb64`
   in a loop until a crash is captured:
   ```bash
   for i in {1..10}; do
     rm -f ssb64.log
     SSB64_MAX_FRAMES=2000 ./build/ssb64 >/dev/null 2>&1
     [ $? -ne 0 ] && { echo "crash on run $i"; break; }
   done
   ```
4. **Check Samus's action motion list.** The crash only surfaces in
   `mvOpeningSamusStartScene` in my tests. Samus's intro triggers an early
   shield/guard interrupt path (`ftCommonGuardOnCheckInterruptCommon`) —
   look at her status descs and verify whether the newly-reached motion
   numbers are legal indexes into `dFTSamus*MotionDescs[]`.
5. **Revisit the bailout comments.** The `#ifdef PORT` guard at
   `src/ft/ftcommon/ftcommonguard1.c:224-245` attributes the null-joint
   crash class to the FTMotionDesc LP64 stride. **Lead 3 agent's runtime
   evidence disproved that attribution** (see `runtime log Mario motion 0
   = llFTMarioAnimWaitFileID, motion=8 matches row 8, stride=24, etc.`).
   The real cause of the null joints is almost certainly the Lead 2
   FTMotionFlags bug we just fixed — but removing the bailout (now that
   Lead 2 is fixed) may be what makes Samus's scene reach the new crash.
   Similar comments exist in:
   - `src/ft/ftcommon/ftcommonguard1.c:451-471` (`ftCommonGuardInitJoints`)
   - `src/lb/lbcommon.c:1598-1617` (another bailout referenced by the Lead 3
     agent's report — verify location before touching)

   Before attempting to remove any bailout, read the code that runs *after*
   the bailout and confirm it's actually safe now. If the same bailout is
   what's preventing the crash, removing it won't fix anything until the
   post-bailout path works.

## Other findings kept on ice

### 1. Six partial-bit `FTMotionEvent*` structs (bit layout bug)

Six structs in `src/ft/fttypes.h` total fewer than 32 bits per u32 word, so
the `IS_BIG_ENDIAN` / `!IS_BIG_ENDIAN` mirror places fields at different
offsets from the bottom of the word rather than at 32-complement positions.
The runtime reads wrong bit ranges on LE.

| Struct | Line | BE bit total | Fields |
|---|---|---|---|
| `FTMotionEventMakeAttack5` | 383 | 25 | `shield_damage:8, fgm_level:3, fgm_kind:4, knockback_base:10` |
| `FTMotionEventSetAttackOffset1` | 407 | 25 | `opcode:6, attack_id:3, off_x:16` |
| `FTMotionEventSetAttackCollDamage` | 437 | 17 | `opcode:6, attack_id:3, damage:8` |
| `FTMotionEventSetAttackCollSize` | 450 | 25 | `opcode:6, attack_id:3, size:16` |
| `FTMotionEventSetAttackCollSound` | 463 | 12 | `opcode:6, attack_id:3, fgm_level:3` |
| `FTMotionEventSetDamageCollPartID1` | 565 | 13 | `opcode:6, joint_id:7` |

**Fix pattern:** add an explicit `u32 pad:(32 - sum)` field at the *start*
of the LE branch (and the *end* of the BE branch) so the high bits of the
u32 that the BE layout leaves empty line up to the same physical positions
on LE. Example for `FTMotionEventSetAttackCollSize`:

```c
#if IS_BIG_ENDIAN
    u32 opcode : 6;
    u32 attack_id : 3;
    u32 size : 16;
    u32 pad : 7;   // low 7 bits unused on N64
#else
    u32 pad : 7;
    u32 size : 16;
    u32 attack_id : 3;
    u32 opcode : 6;
#endif
```

These affect attack/damage-collider runtime behavior but **not the intro
visual symptom** the main investigation chased. Worth fixing as a separate
focused commit once the progression segfault is resolved, because they
will silently bite as soon as any fighter actually lands a hitbox on
another fighter.

### 2. `FTData::p_file_submotion == NULL` (latent, non-critical)

`FTData::p_file_submotion` is NULL for every fighter in the port
(`src/ft/ftdata.c:1118,1128,...`). The N64 code path computed
`event_script_ptr = *NULL + motion_desc->offset` which is UB, but the
`offset` field in submotion descs actually holds direct pointers to
`D_ovl1_*` data (see `src/sc/scsubsys/scsubsysdatafox.c:42-68`), so the
N64 path effectively did `0 + pointer = pointer` and silently worked.

The port's safe NULL check in `ftMainSetStatus` (around line 4883-4885)
makes `event_script_ptr = NULL` for submotion-attached scripts, so they
silently skip. Visually affects opening-scene mouth blinks, voice lines,
one-shot texture flips — **not** projectile spawning (which goes through
`mainmotion`, which is populated correctly).

**Fix direction:** add a PORT-only path in `ftMainSetStatus` that, when
`p_file_submotion == NULL`, treats `motion_desc->offset` as a raw pointer
(it already is on LP64 via the `intptr_t offset` field) and uses it
directly.

### 3. `GMStatFlags` — same class of bug as FTMotionFlags

`src/gm/gmtypes.h:65-87` has an `IS_BIG_ENDIAN`-switched anonymous struct
inside a union with a `u16 halfword` alternate view:

```c
union GMStatFlags {
#if IS_BIG_ENDIAN
    struct { u16 unused:3; ub16 is_smash_attack:1; ub16 ga:1;
             ub16 is_projectile:1; u16 attack_id:10; };
#else
    struct { u16 attack_id:10; ub16 is_projectile:1; ub16 ga:1;
             ub16 is_smash_attack:1; u16 unused:3; };
#endif
    u16 halfword;
};
```

Positional initializers in every `FTStatusDesc[]` literal pass values in BE
source order (`{unused, is_smash, ga, is_projectile, attack_id}`), which on
LE get written to the reversed fields. **Unlike FTMotionFlags, this one is
accidentally mostly harmless on LE** because the only fields callers read
(`ga`, `is_projectile`, `attack_id`) all default to 0 for most entries, and
`ga == nMPKineticsGround == 0` happens to be the common case.

The `u16 halfword` union member is read by `ftParamSetStatUpdate` at
`ftmain.c:4700`. A naive "plain fields" fix (like the FTMotionFlags fix)
would change the struct's bit layout and break the halfword accessor.

**Fix direction (requires design):** either
- Keep the bitfield but rewrite every `FTStatusDesc[]` initializer to use
  **designated initializers** (`.attack_id = X, .is_projectile = Y, ...`)
  so source order no longer matters. Tedious but cleanly correct.
- Or replace the struct with plain fields AND update every `.halfword`
  reader to use per-field access instead. Less tedious but touches more
  call sites.

Not blocking any current visible bug — leave until we're sure we want to
touch every status desc.

## Leads 1 & 3 — ruled out (detailed evidence)

### Lead 1 — matanim event32 parsing

Ruled out by agent `lead1-matanim`. Key findings:

- `gcParseMObjMatAnimJoint` (`src/sys/objanim.c:968`) and `gcPlayMObjMatAnim`
  (`src/sys/objanim.c:1382`) decode every matanim event byte-exact vs the
  raw ROM-extracted bytes. Verified for MVCommon (file 52), MarioModel
  (296), FoxModel (317), SamusModel (320), LinkModel (324), YoshiModel
  (338), PikachuModel (341).
- **Structural insight worth keeping:** `gcParseMObjMatAnimJoint` is
  one-shot — it runs once per MObj at setup, then `lbCommonAddMObjForFighterPartsDObj`
  (`src/lb/lbcommon.c:1036-1046`) calls `gcRemoveAObjFromMObj` which sets
  `mobj->anim_wait = AOBJ_ANIM_NULL`, so every subsequent call returns
  early at the entry guard. **Live fighter texture swaps go through
  motion-script `nFTMotionEventSetTexturePartID` → `ftParamSetTexturePartID`
  at `src/ft/ftparam.c:1176`, not matanim**. This is why the fix for the
  matanim parser wouldn't have moved the needle — the motion script path
  is what was broken (Lead 2).
- Evidence: `debug_traces/port_file_338.bin` (YoshiModel raw) + Python
  decode confirmed every event byte-exact. Lead 1 also left 135 lines of
  `SSB64_TRACE_MATANIM=1`-gated diagnostic infrastructure in
  `src/sys/objanim.c` — currently stashed.

### Lead 3 — `FTMotionDesc` LP64 stride landmine

Ruled out by agent `lead3-ftmotiondesc`. Evidence:

- **Static:** No field-level writes to `FTData::mainmotion` or
  `FTData::submotion` anywhere. All 42 `dFT*MotionDescs[]` / `dFT*SubMotionDescs[]`
  arrays are compile-time externs defined in `src/ft/ftdata.c` and
  `src/sc/scsubsys/scsubsysdata*.c`, populated only via static `FTData`
  struct initializers. The C compiler lays these out at the current ABI's
  `sizeof(FTMotionDesc)` stride, so producer and reader agree by
  construction on both N64 (stride 12) and LP64 (stride 24).
- **No file-backed `FTMotionDesc` array anywhere.** The only
  `lbRelocGet*` call related to motion data is
  `lbRelocGetStatusBufferFile(data->file_mainmotion_id)` at
  `src/ft/ftmanager.c:314`, which returns a `void*` raw event-script file
  blob stored in `*data->p_file_mainmotion`. **This is NOT an
  `FTMotionDescArray*`** — it's the raw event script bytes, indexed
  separately by `motion_desc->offset` (a byte offset inside the blob).
- **Runtime:** logged `sizeof(FTMotionDesc) = 24` on LP64 and strides
  match `motion_id * 24` perfectly for Mario submotion rows 0, 8, 9, 15.
  Field values match source row-for-row (`motion=8 file_id=0x16a
  offset=0x80000000 anim=0x80000000` matches
  `{ll_362_FileID, 0x80000000, 0x80000000}` in
  `src/sc/scsubsys/scsubsysdatamario.c`).

Lead 3 impossible by construction. The pre-existing comments in
`src/ft/ftcommon/ftcommonguard1.c` and `src/lb/lbcommon.c` that blame the
"FTMotionDesc LP64 stride" for null-joint crashes are **incorrect** — the
real cause was the FTMotionFlags positional-initializer bug (Lead 2),
which those comments predate.

## Diagnostic infrastructure — stashed

The Lead 1 and Lead 2 agents left diagnostic traces in the working tree
which were stashed before committing the fix:

```
git stash list
# stash@{0}: On main: agent diagnostic infrastructure from leads 1/2
```

Contents (218 lines across 4 files):

- `src/sys/objanim.c` — 137 lines of `SSB64_TRACE_MATANIM=1`-gated
  instrumentation for the matanim event32 parser (Lead 1). Turnkey for
  future matanim investigations.
- `src/ft/ftmain.c` — 57 lines of `SSB64_TRACE_MOTION=1`-gated
  instrumentation for the motion event parser, plus unconditional logs in
  each `SetFlag0..3` case (Lead 2).
- `port/bridge/lbreloc_bridge.cpp` — 11 lines adding
  `SSB64_DUMP_MAIN_FILES=1` env var to dump every
  `reloc_fighters_main/*` file pre-byteswap to
  `debug_traces/port_file_<id>.bin` (Lead 2). Useful to pair with
  `SSB64_DUMP_ALL_FIGATREE=1`.
- `src/ft/ftchar/ftfox/ftfoxspecialn.c` — 15 lines of
  `ftFoxSpecialNProcUpdate` diagnostic (Lead 2).

The `SSB64_TRACE_MATANIM` and `SSB64_DUMP_MAIN_FILES` hooks are the most
likely to be worth reviving and committing as permanent diagnostic
infrastructure — they're clean env-var-gated, have no runtime cost when
disabled, and both were designed to be self-contained. The other two
touch hot paths or add unconditional spam and should probably be
discarded.

## Files touched during this session

- **Committed (`006128e`):** `src/ft/fttypes.h` — FTMotionFlags plain fields
- **Stashed:** `src/sys/objanim.c`, `src/ft/ftmain.c`, `port/bridge/lbreloc_bridge.cpp`, `src/ft/ftchar/ftfox/ftfoxspecialn.c`
- **Untracked in `debug_traces/`:** `port_file_338.bin`, `port_file_208.bin`, `matanim_evidence/port_matanim_trace.txt`, plus pre-existing figatree dumps
- **Crash reports:** `~/Library/Logs/DiagnosticReports/ssb64-2026-04-13-003406.ips` and `ssb64-2026-04-13-004119.ips`

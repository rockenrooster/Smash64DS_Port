# CMake warning audit — handoff (updated 2026-04-20)

## Status

**Category A (LP64 landmines) is complete.** All 6 flags identified in
`docs/cmake_warning_audit_2026-04-20.md` are now `-Werror=` in
`CMakeLists.txt:198-217`:

| Flag | State |
|------|-------|
| `implicit-function-declaration` | `-Werror=` (commit `b53ffc2`) |
| `incompatible-library-redeclaration` | `-Werror=` (commit `c59cbde`) |
| `return-type` | `-Werror=` (commit `a61dc7e`) |
| `return-mismatch` | `-Werror=` (commit `a4d2cc9`) |
| `int-conversion` | `-Werror=` (commit `94d1fc9`, preceded by `055aacc`/`82982ec`) |
| `incompatible-pointer-types` | `-Werror=` (commit `21466b0`, this session) |

Flag 6 smoke-test passed; build is clean on `main`.

## Flag 6 summary (commit `21466b0`)

99 errors across 6 files, all resolved without regressing the N64 build path.

| File | Count | Class of fix |
|------|------:|--------------|
| `src/ft/ftdata.c` | 55 | 54× cast `&dFT*MotionDescs` → `(FTMotionDescArray *)&…` at FTData initializers; 1× `(void **)` cast on NPikachu particle-bank placed in `p_file_special2` slot (decomp layout quirk). |
| `src/sys/audio.c` | 16 | Retyped `gSYAudioCSPlayers` from `ALCSPlayer *[]` → `N_ALCSPlayer *[]` (all 15 call sites use `n_alCSP*` API); `#ifdef PORT` widened `syAudioDma` to `uintptr_t(uintptr_t,s32,void*)` to match PORT-conditional `ALDMAproc` typedef (1 decl + 1 defn). |
| `src/sys/scheduler.c` | 11 | Cast `sSYSchedulerPausedQueueHead/Tail` (typed `SYTaskGfx *` but used as `SYTaskInfo *` in paused-queue linkage) at 8 sites; broke up chained `sSYScheduler* = … = NULL;` init at 1219-1224 so each pointer global gets its own `NULL` (can't chain across differently-typed pointers on strict C). |
| `src/sys/objdisplay.c` | 9 | Fixed decomp typo `GCTranslate *scale;` → `GCScale *scale;` in `gcPrepDObjMatrix`; changed 8× `mtx_hub.f` → `mtx_hub.gbi` for `func_8001{0AE8,0748,0C2C,0918}` calls (union-aliased matrix hub — those funcs take `Mtx *` / fixed-point, not `Mtx44f *` / float). |
| `src/libultra/n_audio/n_env.c` | 5 | Cast `N_ALVoice *` punning at 4508; cast `ALWhatever8009EE0C *` → `_2 *` at 4579; cast `siz34 *` → `siz24 *` at 4597/4609; cast `client->handler = (ALVoiceHandler)func_…` at 5463. All are in the FGM synth path which is already stubbed `return NULL` on PORT (n_env.c:5000), so behavior is unchanged; casts just satisfy the type checker. |
| `src/lb/lbparticle.c` | 3 | Removed stray `&` at 3 call sites (`&projection_f` → `projection_f`) where callee `syMatrixPersp{F,FastF}(Mtx44f mf, …)` decays the parameter to `f32(*)[4]`; decomp had one-level-too-many of indirection. `syMatrixOrthoF(Mtx44f *mf, …)` call at 1596 kept its `&`. |

## Open follow-ups (structural, not blocking)

These accumulated across the Category A passes. None block new warning flags;
all are latent risks if/when the surrounding subsystems get enabled.

1. **`libultra/n_audio/n_env.c` — `s32` fields storing pointers.** `unk20`,
   `unk24`, `unk40`, `unknown0`, `unknown1` all silently truncate pointers
   on LP64. Dead today because the FGM synth path is short-circuited at
   `n_env.c:5000` (`#ifdef PORT return NULL;`). Flag-6 casts at 4508 / 4579 /
   4597 / 4609 / 5463 land in the same dead paths — they satisfy the type
   checker but the underlying struct-layout problems remain. **DEFERRED
   to Phase 5/6 audio work.** Widening the structs now changes layouts
   in code that's already dead on PORT, and the `ALWhatever8009EE0C` /
   `ALWhatever8009EE0C_2` pair is punned through casts that only overlay
   correctly while pointer-holding fields stay the same width in both
   structs — any partial widening risks breaking the punning. Do this
   in lockstep with re-enabling FGM playback.

2. **`sys/audio.c` — `syAudioDma` body casts.** ~~Prototype is now
   PORT-widened to `uintptr_t`, but the body still uses `(u32) dBuff->addr`
   and `(s32) osVirtualToPhysical(...)`.~~ **Done in commit `63cb50f`:**
   locals retyped to `uintptr_t`, return cast widened. On N64 `uintptr_t
   == u32` so behavior is unchanged; on PORT the body is dead today
   (portAudioDma replaces it via `syn_config.dmaproc`), so this is pure
   type-safety cleanup that matches the widened signature.

3. **`sys/scheduler.c` — paused-queue type.** ~~`sSYSchedulerPausedQueueHead/Tail`
   typed `SYTaskGfx *` but queue is polymorphic.~~ **Done in commit
   `3846ee3`:** head/tail retyped to `SYTaskInfo *`; the 8 flag-6 casts at
   link-manipulation sites dropped; Gfx-specific field accesses at
   `case FALSE:` (line 1030) and the `type == nSYTaskTypeGfx` branch
   (line 1177) downcast locally via `SYTaskGfx *head_gfx =
   (SYTaskGfx *)sSYSchedulerPausedQueueHead;`.

4. **`mp/mpcommon.c` — `func_ovl2_800EBC0C(s32 arg0, ...)`.** ~~Declares
   `s32 arg0` but callers pass `FTStruct *`.~~ **Done in commit `634f8b1`:**
   prototype widened to `FTStruct *arg0`; the `(s32)(intptr_t)fp` casts at
   mpcommon.c:379/391 dropped. Body still ignores arg0 (matches decomp),
   but the type is now honest.

5. **`sys/objscript.c` gc* callback family.** Flag 5 widened `u32 param` →
   `uintptr_t param` across `gcFuncGObjByLink`, `gcFuncGObjAll`,
   `gcFuncGObjByLinkEx`, `gcFuncGObjAllEx`, `gcGetGObjByID`,
   `gcAddGObjScript`, plus 2 `ifCommon*GObj` and 3 `sc1PGameBoss*`
   callback signatures. `gcGetGObjByID`'s `u32 id` compare is still low-32
   only — intentional, IDs are `u32` values and zero-extend cleanly. No
   action needed, documenting for future readers.

## Next session — Category B flags

4 flags still `-Wno-*` in `CMakeLists.txt:198-217`. Original plan labels
these "real-bug hints, unlikely to silently corrupt LP64 data." Audit order
is a judgment call; my guess at effort-to-value:

1. **`-Wno-implicit-int`** — variable/function declared without a type
   (usually a typo or `K&R`-style int-return). Fewer sites than Category A
   but each is potentially a real bug. Promote and triage.
2. **`-Wno-constant-conversion`** — narrowing int literal. Mostly decomp
   artifacts (e.g. assigning `0xFFFF` to `s16`); expect many sites, all
   benign. If the pass gets noisy, leave it off.
3. **`-Wno-shift-negative-value`** — UB but deterministic on our targets.
   Lowest priority.
4. **`-Wno-tautological-constant-out-of-range-compare`** — decomp macro
   artifacts; almost certainly not hiding real bugs.

Same procedure as Category A: flip one flag, `cmake --build build --target
ssb64 2>&1 | grep -c error:`, fix, rebuild, commit, repeat.

## Category C — cosmetic, leave off

Per original plan:
- `-Wno-parentheses-equality`
- `-Wno-pointer-sign`
- `-Wno-unused-value`, `-Wno-unused-variable`, `-Wno-unused-but-set-variable`, `-Wno-unused-function`

Decomp idioms; not hiding real bugs.

## Reference — how to audit a new flag

1. Flip `-Wno-X` → `-Werror=X` in `CMakeLists.txt` (and temporarily add
   `-ferror-limit=0` next to it so clang doesn't cap per-TU output at 20).
2. `cmake --build build --target ssb64_game -j 1 -- -k > /tmp/errs.log 2>&1`
   — single-threaded, keep-going. Parallel builds interleave stderr and
   mangle the log.
3. `grep "error:" /tmp/errs.log | awk -F: '{print $1}' | sort | uniq -c | sort -rn`
   for a per-file count.
4. Triage, fix, rebuild. Remove `-ferror-limit=0` before committing.

## Related

- `docs/cmake_warning_audit_2026-04-20.md` — original plan (Category
  A/B/C rationale).
- `docs/fighter_intro_animation_handoff_2026-04-13.md` — `p_file_submotion
  == NULL` context.
- `docs/bugs/item_arrow_gobj_implicit_int_2026-04-20.md` — the motivating
  LP64 incident.
- `MEMORY.md` → *Implicit-int LP64 trunc trap* — crash-class fingerprint.

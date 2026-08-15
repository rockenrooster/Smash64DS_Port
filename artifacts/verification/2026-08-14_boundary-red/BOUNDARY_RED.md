# Boundary is green: the red was a corrupt DLDI SD image, not a commit

**Date:** 2026-08-14 · **Branch:** `codex/r2-runtime2` · **HEAD `ccbd80a783b`.**
**Builds spent: 1** (one rebuild of the existing Boundary proof target, which
made no difference — the red reproduced identically with and without it).
Every artifact cited below is in this directory.

---

## 0. Outcome first

1. **Boundary passes.** `boundary-after-dldi-reset.log`:
   `Boundary verification profile passed.`, exit 0, **zero `Exception:` lines**,
   and `GDB marker capture: 27.8s elapsed of 120s ceiling (23% used)`. The
   120 s constant was never marginal — the guest was dead.
2. **The cause is `emulators/melonds/dldi.bin`, the local DLDI SD image.** It is
   gitignored (`.gitignore:63 /emulators/*`), untracked, 536,870,912 B, and
   with it in place the ROM loads **no assets at all** and data-aborts in battle
   setup. Moving it aside so melonDS creates a fresh one (16,957,440 B) turns
   the same ROM green. **No source change was needed and none was made.**
3. **The inherited five-commit bisect window is REFUTED by measurement.** Three
   independently built ROMs — `build-c-collfixed` (08-13 19:25, *before* the
   window), `build-c156-vfxsymmetry` (08-14 10:44, before the window) and the
   Boundary proof build rebuilt on HEAD — crash with byte-identical signatures.
   No commit in `8fc8b47c9ce..HEAD` can be responsible for a fault that three
   different builds spanning that range all share.
4. **The previous cycle's "REGRESSION, not a ceiling" was half right.** It was
   not a ceiling — that stands, and 1,800 s proved it. But it was not a
   regression either: it is environment.
5. **Task D was not reached.** §6 records what it inherits, including the
   orchestrator's widened counter set.

---

## 1. Task A — does the red survive the `gdb-markers.ps1` fix?

**Yes, and it never could have been cured by it.** Two independent reads:

- **Static.** The fix (`ccbd80a783b`, `[string[]]` → `[object[]]` + flatten)
  only changes behaviour for a *jagged* command list. The Boundary path cannot
  produce one: `verify-battle-mariofox-gcrunall-loop-harness.ps1` builds
  `$gdbCommands` with newline-separated `@( … )` array subexpressions (`:1966`,
  `:2036`, `:2183`) and `+=`, both of which flatten in PowerShell. The jagged
  form came from a hand-written probe, not from any verifier.
- **Empirical.** Boundary run on HEAD with the fix in place, `-NoBuild`
  (`boundary-taskA-head-ccbd80a.log`) and again with a full rebuild
  (`boundary-taskA-head-withbuild.log`): both RED, both
  `GDB marker capture timed out after 120 seconds`.

**Ordering, since the brief asked:** the retracted probe ran at 20:25
(`artifacts/emulator-logs/slot2/melonds.r0-progress.stdout.log` mtime), and the
fix landed at 21:17 (`scripts/lib/gdb-markers.ps1` mtime), commit 21:19. The
1,800 s harness run therefore **pre-dates** the fix, so Task A was a real open
question rather than one already answered.

---

## 2. The failure is a crash, and the existence chain that found it

Rung by rung, no build spent on any of it.

| read | result |
|---|---|
| presented-frame marker after bp1 (`r0-battle-setup.txt`) | `scVSBattleStartBattle` hits 0.06 s after attach; `ndsBattlePlayableFrameCompleteMarker` — one call per presented frame — **never fires in 240 s**. Not slow: never presents. |
| attach as a PC sample (`r0-pc-samples.txt`) | `cpsr=0x400000b7` → mode `0b10111` = **ABORT**, Thumb, IRQs masked; `pc=0x02000ff2`, an address no ELF section owns. |
| full register file (`r0-dump.txt`, `r0-userstack.txt`) | `spsr_abt == cpsr` (a *nested* abort); `r12 = 0x205`; `pc` slides 0x02000c22 / 0x02000dfa / 0x02000f6e / 0x02000ff2 through memory that reads all zero (`movs r0,r0`). |
| `__excpt_entry` disassembly | Calico's handler `bic ip,ip,#1; mcr p15,0,ip,c1,c0,0` — it **disables the protection unit** — then `ldr ip,[sp&~3,#16]`, `blxne ip`. That slot (`0x02fffd9c`) holds `0x00000205`, so it branches to junk. `r12 = 0x205` is that value, still in the register file. This is why a breakpoint on `__excpt_entry` was useless after the fact and why the abort's own state is destroyed. |

**The faulting call, from the surviving USER-mode banked registers** (abort mode
does not bank `lr_usr`, and the zero-slide writes only `r0`):

```
lr_usr = 0x0205ed3f = lbCommonMakeSpriteGObj+26
                    = the return address after `bl lbCommonMakeSObjForGObj`
```

The user stack (`r0-userstack.txt`, resolved against `nm`) reconstructs the
whole chain without trusting a frame walk:

```
portCoroutineResume -> ndsOsThreadEntry -> syMainThread5 -> scManagerRunLoop
  -> ndsBaseSCManagerRunLoop -> scVSBattleStartScene -> ndsBaseSCVSBattleStartScene
  -> syTaskmanStartTask -> syTaskmanLoadScene -> scVSBattleStartBattle
  -> ndsBaseSCVSBattleStartBattle+0x5a      <- the return from `bl grWallpaperMakeDecideKind`
  -> grWallpaperMakeCommon -> lbCommonMakeSpriteGObj -> lbCommonMakeSObjForGObj
```

`grWallpaperMakeCommon` (`decomp/…/src/gr/grwallpaper.c:132`) passes
`gMPCollisionGroundData->wallpaper`. `lbCommonMakeSObjForGObj`
(`src/port/sprite_preview_backend.c:128`) dereferences it immediately —
`ldrb r3,[r1,#49]` = `sprite->bmsiz`.

**Logged directly** (`r0-sobjlog.txt`, breakpoint on the entry):

```
R0 SOBJ n=1 gobj=0x22be080 sprite=0x3eb lr=0x205ed3f
```

`0x3eb` is a raw asset token, not a pointer: `0x3eb + 49 = 0x41c` lands in the
MPU's low no-access page and aborts. **Control:** every recorded healthy capture
in the repo prints this same field as a real arena pointer —
`BPLAY_WALLPAPER=1,0x234a188 … 0x2359bb8`, nine distinct values across
`builds/final-boundary.err.log` and neighbours. `0x3eb` appears in none of them.

**RETRACTED, mine, same cycle.** The post-mortem register file reads `r1 = 0`
and I first called that a NULL sprite. Its own breakpoint —
`break lbCommonMakeSObjForGObj if $r1 == 0` — **never fired**
(`r0-nullsprite.txt`), so `r1` was zeroed by the post-abort runaway, not by the
caller. The `sprite=0x3eb` reading above replaced it and is a direct entry-time
log, not an inference.

---

## 3. The cause, and the control that separates it from the source

Assets are simply not loading: at the crash, `sNdsRelocLoadedFileCount = 0` and
`gNdsRelocAssetPayloadReadCount = 0`. So `gMPCollisionGroundData->wallpaper` is
never relocated and keeps its file token.

Three arms, one variable — the emulator's DLDI SD image:

| arm | DLDI | image | guest at t≈25 s | artifact |
|---|---|---|---|---|
| A | on | `dldi.bin` 536,870,912 B | `cpsr=0x400000b7` **ABORT**, `r12=0x205`, `lr_usr=lbCommonMakeSpriteGObj+26` | `r0-dump.txt` |
| B | **off** | same file, unused | `cpsr=0x2000003f` **SYSTEM**, ARM, IRQs on, `pc=0x20426ee` | `r0-nodldi.txt` |
| C | on | fresh melonDS-created `dldi.bin` 16,957,440 B | `cpsr=0x6000001f` **SYSTEM**, `pc` inside `ndsRendererEndParticleQuads` | `r0-freshdldi.txt` |

Arm B is diagnosis only — DLDI-on is the retail-parity configuration and is
worth ≈29,696 P95 ticks (`scripts/lib/melonds.ps1:352-362`); it must never be
turned off to make a verifier pass. **Arm C is the fix**: DLDI stays on, the
image is replaced.

The old image is preserved, not deleted, at
`emulators/melonds/dldi.bin.broken-2026-08-14` (and
`dldi.bin.idx.bak-2026-08-14`); both are gitignored. The owner may delete them.

**Not fully explained, stated as such:** both images have a zeroed FAT BPB
(`bytes/sector=0`, `numFATs=0`, only the `0x55AA` signature), so *validity* is
not the difference — **size** is, 512 MB versus 16.2 MB. Why melonDS's DLDI
serves reads from one and not the other was not measured, and no claim is made
about it. What is measured is the three-arm table above.

---

## 4. Bisect — collapsed by file inspection, then refuted by measurement

The brief's window `8fc8b47c9ce..HEAD` is eleven commits. **Eight cannot change
a shipped byte**, established from the diffs rather than inherited:

| commit | verdict |
|---|---|
| `697303ed77c` | **can ship bytes** — `battleship_ftanim.c`, `reloc_backend_compat_shims.c` |
| `33d7cc5d3b7` | **can ship bytes** — `nds_renderer.c`, generated particle-bank header, `generate_nds_particle_banks.py` (a Makefile-invoked generator) |
| `9b6c9e72a25` | **can ship bytes** — four headers, four sources, `scripts/stages/generate_nds_native_stage.py` |
| `54d7d7862e4` | comment-only. Every added/removed line in `battleship_sys_objanim.c` is inside a `/* */` block; filtering comment lines from the diff leaves one blank line. |
| `813207773c1` | `scripts/check-gbi-decode-fixtures.ps1` only. **Verified rather than inherited:** the Makefile's complete script list is 16 entries and does not include it. |
| `e8cb0e9de11` `869eaea1c30` `574ffb93bea` `9e7cfab4f50` `a159069af0d` | census/analysis Python, not referenced by the Makefile |
| `ccbd80a783b` | docs + `scripts/lib/gdb-markers.ps1` |

**No build was spent on the survivors, because the window itself is refuted:**
`build-c-collfixed` (2026-08-13 19:25, two hours *before* the last green
Boundary log) and `build-c156-vfxsymmetry` (2026-08-14 10:44) both show
`cpsr=0x400000b7`, `r12=0x205`, `lr_usr=lbCommonMakeSpriteGObj+26` —
`r0-abort-build-c-collfixed.txt`, `r0-abort-build-c156-vfxsymmetry.txt`. A fault
shared by builds on both sides of a window is not caused inside it.

---

## 5. The seam that gets fixed, so this cannot cost a fourth cycle

The SD image is environment and cannot be committed. What *is* committable is
the reason three cycles misread it: a capture that times out looks exactly like
a capture that is slow, so the response was to raise the ceiling
(120 → 600 → 1,800 s), then to bisect eleven commits.

`scripts/lib/gdb-markers.ps1` now classifies the timeout from evidence it
already holds. gdb prints the stop location the instant `target remote` lands;
when the guest is healthy that names a function
(`0x0203642c in ndsRelocAssetIDForToken (…)`), and when it has crashed it is
`0x02000dfa in ?? ()`, because no symbol owns zeroed RAM. The throw now carries
that verdict.

Proven both directions on the two **real** captures from this cycle:

- crashed (`boundary-taskA-head-withbuild.log`, `0x02000dfa in ?? ()`) →
  `GUEST STATE AT ATTACH: … THE GUEST HAD ALREADY CRASHED BEFORE THIS CAPTURE
  ATTACHED …`
- healthy (`boundary-taskA-head-ccbd80a.log`,
  `0x0203642c in ndsRelocAssetIDForToken`) → empty string, no claim.

**Retracted within the cycle:** the first implementation re-attached gdb after
the timeout to read `$cpsr` directly. It returns nothing — **melonDS's GDB stub
refuses every reconnection after the first session ends** (measured twice:
`r0-pc-samples.txt`, where the t=45/90/150 s re-attaches all time out, and
`r0-timeout-classify-run.log`). That approach was removed rather than shipped
dark, and the limitation is recorded in the helper's comment so nobody rebuilds
it.

---

## 6. What Task D inherits

Not started — the cycle's context went to Tasks A–C. The orchestrator's widened
brief, recorded verbatim so the next cycle does not re-derive it: land per
presented frame on the `NDS_R2_BOTH_CPU=1` gate arm, intersected with the 80
frames that set P95 —

asset IDs requested per frame · distinct animation assets over the match · cache
hit/miss · `get_fat` / `f_lseek` / `f_read` call counts · bytes read from ROM ·
bytes copied from cache into the status heap · `ndsRelocNormalizeFighterAObj16File`
executions · `ndsRelocFinalizeLoadedFile` executions.

Counters get declared in a header, defined in `src/port/diagnostics.c`, marked
`__attribute__((used))` and `nm`-verified against `--gc-sections`. Then the
static enumeration of the complete reachable Mario/Fox animation asset set
against the 85-entry warm list.

**One free observation this cycle turned up for that lane:** at
`scVSBattleStartBattle` on a *healthy* run the asset layer is already at
`gNdsRelocAssetOpenFailCount` ≈ 1,385–1,402. That counter is not a file-open
failure — it is `ndsRelocAddStatusNode` (`reloc_backend_assets.c:2536`)
declining because the scene's `LBFileNode` status buffer is full. It was never
established whether that number is normal, and nothing in this cycle depends on
it, but a BattlePack design that removes token→file discovery should know it
exists.

---

## 7. State

- Root ROMs **unchanged** across the cycle:
  `smash64ds.nds` `54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a`,
  `smash64ds-battle-playable-hwtri.nds` `2015fbd1f68b81c03626d8c6d473c8bcbcf527a3a26dfe86ff19bd74ecbb1360`.
- The `build-c158-gate` bank — P95 **1,184,064 raw / 1,159,117 net**, P50
  939,136 — now sits on a tree whose widest verifier is green. **It was measured
  before Boundary went green, on the same HEAD and the same sources**, and no
  source byte changed to make Boundary pass, so the bank stands as recorded.
- The temporary probe used for §2–§3 was removed. Its method is §2's table.

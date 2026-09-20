# Crash/stability candidate — 2026-09-19

Status: IMPLEMENTED_NOT_ACCEPTED. All three owner reports remain open.

The additional Fox/Mario startup exception is repaired and retested below.
This remains a candidate, not a qualified release: low heap margin, the wider
verifier's CPU-level fixture mismatch and owner acceptance remain unresolved.

## Identity and scope

- Owner ROM before this task: `52110122320F59D6EA6235DC9BFE9C526263B0055208A399335CC357C8B00657`.
- Frozen diagnostic ROM: `F4AC5B10E56B83FB36BFB15D2B89B986E31F8CD3961DE09CAFB1B7518D7334E6`.
- Matching ELF: `7204FDF4A1725E3578CB65E2089EDE907DC7761E7F88746CA3797130D9318A1E`.
- Diagnostic config: `35A93A6067B025F6E006D2160C00C2F8A596E2BF9CC9F5FF56EF1CCC4ABB46A4`.
- Diagnostic path: `builds/build-crash-diagnostic/smash64ds-p2-shell-hwtri.nds`.
- Build: `make TARGET=smash64ds-p2-shell-hwtri BUILD=build-crash-diagnostic NDS_P2_1P_GAME=1 NDS_P2_COMPACT_BATTLE_FIGHTERS=1`.
- Full roster/campaign, hardware renderer, FPC2, fast logic 0, menu walk 1;
  `NATIVE_ONLY_PASS` examined 315 actual link inputs.
- Final normal `smash64ds.nds`: `00E8777758F926EEA79A282B15C86B4AFC67DFBC4E2F4A7A6194C2FF153BFBCE`.
- Normal ELF: `E98167690CEA85EF359BAFF56201F81F0D38F833C78E59934096ACEA680D6B01`.
- Normal config: `28CF28AB7F7BC4DE1A9B8097E678B7A97DDDCF888C917C7999682D682BE57DC9`.
- `make TARGET=smash64ds` exited 0; native-only enforcement examined 316 actual
  link inputs. Its config has full campaign/compact fighters enabled, hardware
  rendering, menu walk 0 and fast logic 0. Latest profile limitations are below.
  Diagnostic evidence is not attributed to this different natural-input binary.

## First divergences and owning fixes

1. **1P CSS residency:** the imported startup loop retained all twelve preview
   closures in the general scene heap. It now skips that preload and uses one
   existing VS-sized resident block. Only files/native owner-image backing live
   in that block; reusable GObj/DObj/pose allocations remain in the scene heap.
   Destroy, FTData clearing, native image release, preview ownership release,
   relocation range release and arena reset happen before another preview.
   On scene exit, source taskman's CommonTaskUpdate/Draw ejects all GObjs on
   its break-loop condition before the scene wrapper retires the resident;
   the wrapper must not eject the stale preview handle a second time.
   Kirby reuses the existing VS compact
   copy-table preparation instead of dereferencing a NULL MainMotion base.
2. **Ness/Sector Z:** the first diagnostic OOM requested 4,896 bytes for Fox's
   figatree heap with only 1,408 free, arena 916,992. This was shared startup
   residency, not a Ness move or malformed Ness FPC. High-detail VS fighters
   no longer retain an unused low-detail owner image; AutoDemo and low-detail
   battles still retain both where source transitions need them.
3. **Stage-dependent startup:** the Dream Land Ness/Fox case hit the battle
   extern halt after raw FoxSpecial3 pulled in an additional 47,120-byte bank
   109, leaving too little for Fox ShieldPose. The three exact bank references
   are SETTIMG texture slots already represented by the native Arwing bake.
   The raw texture pointers are cleared and that dependency edge is omitted;
   Sector Z's own geometry dependency still loads bank 109 normally.
   Fox/Ness now use the existing source-derived compact ShieldPose format;
   their FPC2 extern metadata is regenerated from the same manifest.
   The GO announcement's 60-update sleep uses a function process instead of
   retaining a 4,216-byte stack and 464-byte thread. Its source lifetime is
   checked independently; no countdown, selection or gameplay shortcut.
4. **Additional Fox/player-1 startup abort:** `efManagerFoxEntryArwingMakeEffect`
   dereferenced a NULL DObj while pack/OOM/stage counters were zero. Its file
   was live (12,160 bytes), but the effect callback was NULL and absent from
   the deferred table. A breakpoint at the disable branch traced the loss to
   CSS/battle initialization: backend file residency preceded publication of
   the FTData slot. That branch cleared the callback without preserving it
   for retry. One shared `ndsEFManagerDeferDesc(desc)` call now preserves it,
   including incomplete preview mappings. No Fox-only NULL workaround was
   added. Fox/Mario now reaches GO on both stages; all eight Arwing roots draw
   60 times each, including Dream Land where the raw donor bank is omitted.

No taskman arena increase, Ness gameplay workaround, geometry omission,
compatibility renderer or stale-format fallback was added. No stale FTData
generation witness was found, so the shared preview-loader fast path was not
speculatively changed. External patch failures remain fail-closed.

## Frozen diagnostic observations

Repo-local melonDS interpreter, JIT disabled from boot, isolated slot 8.
Each match used the source CSS → SSS → battle route, 240 presented frames,
GO asserted and source `time_passed=88`. These are startup/correctness probes,
not whole-match performance measurements.

| Human / CPU | Stage | Arena bytes | General heap low-water | Libc top-chunk minimum | Extern patches / loads |
|---|---|---:|---:|---:|---:|
| Ness / Fox | Dream Land (6) | 921,088 | 22,732 | 14,144 | 9 / 8 |
| Ness / Fox | Sector Z (1) | 921,088 | 2,756 | 16,008 | 9 / 7 |
| Mario / Fox | Dream Land (6) | 921,088 | 41,316 | 14,904 | 18 / 9 |
| Mario / Fox | Sector Z (1) | 921,088 | 24,724 | 16,344 | 18 / 8 |
| Fox / Mario | Dream Land (6) | 921,088 | 41,316 | 14,904 | 18 / 9 |
| Fox / Mario | Sector Z (1) | 921,088 | 22,140 | 16,344 | 18 / 8 |

All six: preview failure 0, battle extern failure 0, relocation fixup failure
0, taskman overflow 0, pose-track overflow 0. Stage blob read/hash failures,
prepare/validate failure steps and unresolved count all 0. Unresolved kind 255
is the unused sentinel, not a failure. Native failure, fighter-slot rejects,
stage-owner rejects, renderer fallback buckets and entry-effect fallback are
all 0. Both fighter identities are asserted and both emit native triangles.

**Headroom gate is not passed:** Ness/Sector Z has only 2,756 bytes free;
Ness/Dream Land and Mario/Sector Z also fall below the 25,600-byte standing
floor. Fox/Sector Z is also below the floor. No pooling and no full-match
stability claim. Holding shield through source tick 30 reaches Guard (153)
for both new compact-pose fighters with positive native body output
(Ness 318 / Fox 306 triangles), no rejects, no pose overflow and no pack/OOM
failure. Ness/Sector Z drops to **2,020 bytes** free while shielding; Fox/Mario
remains at 22,140. This is the lowest measured headroom in the batch.

The 1P walk completes two 12-kind sweeps with native-draw masks `fff/fff`,
28 preview loads (447,028 cumulative bytes, not simultaneous residency),
general heap minimum 307,992 bytes, arena 921,088, pack/overflow/native/reject
counts 0. Backing out while Ness is live leaves resident kind 28 (none),
Ness Main NULL and Model NULL. The normal scene's general-heap minimum
counter was unengaged (`UINT_MAX`), so the hover probe measures free bytes
directly at frame boundaries instead of reporting that sentinel as headroom.

Captures: `artifacts/visibility/2026-09-19_1p-hover-defer-fixed-css.png`,
`artifacts/visibility/2026-09-19_crash-startup/{ness,mario,fox}-{dreamland,sector}.png`
and `artifacts/visibility/2026-09-19_crash-startup/shield-kind{1,11}.png`.
The eight match/shield images were hash-verified copies into this dated
directory; the original probe output images were retained.
Raw diagnostic transcripts/JSON remain local under
`artifacts/verification/2026-09-19_match-start-final/` and
`artifacts/verification/2026-09-19_1p-full-roster-hover-defer-fixed.txt`.
Their completed invocations exited 0. No timing/cadence claim is derived from
screenshots or these short probes; WORK-H P50/P95 and whole-match FPS are owed.

## Focused checks and probe corrections

- Requested FPC tests plus the new source-executing residency/GO/Arwing checks:
  **32 passed, 10 subtests passed** on the final sources (13.70 seconds).
- Requested owner-size checker: **PASS**, 767 packs across 64 directories;
  no new drift, 21 already-recorded old-build Yoshi size differences (two older
  directories were refreshed by the wider verifier).
- Normal-ROM boot/title: **PASS** on the final normal binary, 123 title frames, controllers 2,
  all seven asset/scene failure fields 0;
  `builds/2026-09-19_crash-final-normal-runtime.log`.
  This is title coverage, not a natural-input full-match claim.
- Initial Latest stopped at a false-positive `item_egg` name collision with
  `yoshi_egg`. All three real guard arms already existed. The checker now uses
  its explicit alias table for this owner and fails if that exact guard is
  absent. Its added regression passes (four focused tests pass in 1.50 s),
  and all 44 native-owner wiring checks pass. Latest was resumed because this
  concrete checker blocker changed, not because an earlier run was forgotten.
- The resumed Latest run passed its static gates, normal startup and one
  complete shell loop: 10 scene entries, rematch 1, zero faults, 118,796-byte
  loop free floor. It then **failed** the realtime descriptor check: the CSS
  tour was required to commit CPU level 2, but the observed descriptor was
  level 3 (`CPU_CONFIG=0,1,3,1,1,1,0,0,1`). The assertion was not weakened and
  gameplay/CPU configuration was not changed. Four-CPU stress and whole-match
  WORK-H P50/P95, FPS and VBlank gates were **not reached**. These wider lab
  arms have different configurations from the full-content crash diagnostic.
  Log: `builds/2026-09-19_crash-latest-binding-fixed.log` (exit 1).
  This run preceded the final EF deferral repair; final focused probes and the
  normal build/boot were rerun after that repair. **No final Latest GREEN.**
- Shield assets regeneration check: **PASS**, 9 fighters, 31,995 blob bytes,
  modeled old 107,512 bytes, recovery 75,517 bytes; max base 32/scratch 58.
- Architecture: **PASS**, 249 imports, 6 registry entries, two existing warnings.
- melonDS policy: **FAIL**, four existing owner probe files launch visibly
  (`probe-kirby-copylink-owner.ps1:341`, `probe-ness-upb-real.ps1:469`,
  `probe-ness-upb-shell-real.ps1:333`, `probe-purin-owner.ps1:224`). These are
  outside the crash implementation; two are staged deletions with untracked
  replacements. Their ambiguous owner edits have not been changed. The actual
  crash probes launch hidden, isolate storage and disable JIT.
- Shield-pose generator reuses the existing quantized format and independently
  checks source trajectories/history and all motion descriptors. Fox/Ness blobs
  are 3,011/3,753 bytes; modeled residency recovery 7,637/8,499 bytes. Maximum
  pose error is 0.01303228/0.00909830 source units. These are modeled per-pack
  differences, not measured whole-batch performance gains.
- First-failure probing now preserves breakpoints across the temporary battle
  entry stop. Earlier timeouts that deleted those breakpoints are invalid
  instrumentation, not evidence of a different runner's behavior.
- Removed the unsuccessful asynchronous GDB interrupt experiment and the
  temporary broad startup trace. Kept validated failure/optional allocation
  diagnostics and the full-roster regression on the existing probes.
- Action probes now write the existing DTCM input pad instead of inferior
  function calls. The latter stopped in GDB's call trampoline with exception
  stops armed. Writing the unused second pad also disconnected this emulator's
  debugger; only pad 0 is connected, so those redundant writes were removed.
  Both final shield probes then complete. These failed harness attempts are
  not counted as ROM failures or positive coverage.
- The unlocked roster uses a fresh isolated diagnostic-save default before
  backup initialization, then ordinary controller playback. No production save
  or live selected-fighter/scene state is overwritten. Stale/locked fixtures
  are rejected. Earlier cached menu pokes/SIGILL runs are not valid coverage.

## Changed paths in this batch

- `include/ft/fighter.h`, `include/nds/nds_preview_pack.h` — shared declarations/policy.
- `src/import/battleship_mnplayers1pgame.c`, `battleship_mnplayersvs.c` — preview lifetime.
- `src/import/battleship_ftmanager.c`, `battleship_ifcommon.c`, `battleship_efmanager.c` — owner-image admission/Kirby/GO and recoverable effect descriptors.
- `src/port/reloc_backend_assets.c` — native Arwing dependency elision.
- `src/nds/nds_shield_pose.c`, `scripts/fighters/generate_nds_shield_pose_pack.py`, `Makefile` — Fox/Ness source-derived shields and packaging.
- Producer outputs: shield-pose `01.bin`/`11.bin`, generated shield header/manifest,
  regenerated battle-pack manifest and build-staged FPCs (no hand-edited packs).
- `scripts/menus/test_crash_residency.py`, `probe-p2-campaign.ps1`,
  `scripts/diagnostics/probe-native-render-scene.ps1` — focused regression checks.
- `scripts/check-native-owner-wiring.py` — disambiguate item/Yoshi egg guards;
  no renderer production change.
- This receipt, `docs/p2/BUG_NOTES.md`, `docs/BUGS.md`, `docs/PERF_LEDGER.md`
  and the execution cursor.

Pre-existing staged/unstaged owner edits are preserved. The whole dirty tree is
not attributed to this batch. Owner acceptance, wider roster/stage/lifecycle
coverage, required performance and sufficient resource margin remain open.

Earlier ROMs `4F12955B…185B23E` / `1514440E…636A29F` are superseded by the
identities at the top; their discovery logs remain intact. The owner's index
contains staged FPC1 reversions while the requested working tree uses FPC2.
The owner approved reconciling related FPC2 changes into a combined candidate
checkpoint. The stale empty lock was moved, with approval, to
`builds/stale-index-20260919-000525.lock`; no index content was discarded.

Checkpoint scope also includes the pre-existing FPC2 effect-offset mapping,
native root identity API, per-slot validation/material bounds, 1P native 2D
admission, two-player AObj pool, all-content libc reserve and entry-focus
function-process implementation consumed by this candidate. These are combined
integration inputs, not separately measured gains authored by this crash fix.
The pack-version/generator/test reversions are reconciled to FPC2. Unrelated
Kirby copy programs, new weapon/VFX owners and probe deletions stay outside the
checkpoint. The captured ROMs include preserved working-tree changes beyond
the checkpoint, so the commit alone is not claimed to reproduce their exact
binary layout or establish a clean-checkout release qualification.

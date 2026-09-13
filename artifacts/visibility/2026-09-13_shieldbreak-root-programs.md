# Samus status-166 native-owner repair — 2026-09-13

Scope: Boundary four-fighter stress (`Donkey` human + `Samus`/`Link`/`Kirby`
level-3 CPUs on Dream Land), native-only renderer.

## Source contract

The reported numeric status was initially described as ShieldBreakStandD, but
BattleShip's status enum makes the distinction material: ShieldBreakFly starts
at 158, ShieldBreakStandD is 162, and status 166 is Catch
(`decomp/BattleShip-main/decomp/src/ft/ftdef.h:680-688`; mirrored by
`include/ft/fighter.h:1076-1084`).

Samus's shield-break family does not introduce a model-part program. The
ShieldBreak motion at `decomp/BattleShip-main/decomp/src/relocData/216_SamusMainMotion.c:908-913`
contains effects/voice/hit-status only. Its following Damaged, StunLand and
StunStart scripts at `:915-927` likewise contain no model-part writes, and the
Samus motion descriptors use `FTANIM_FLAG_NONE` for ShieldBreak/Damaged/
StunLand/StunStart at `decomp/BattleShip-main/decomp/src/ft/ftdata.c:2351-2356`.
ShieldBreakFly enters without `FTSTATUS_PRESERVE_MODELPART`
(`ftcommonshieldbreakfly.c:29`), while Down and Stand deliberately preserve the
already-established model parts (`ftcommonshieldbreakdown.c:8,34` and
`ftcommonshieldbreakstand.c:8,28`). Therefore this family must select native
root program 0.

Status 166 is different. Catch enables hidden parts through its motion flags
(`ftdata.c:2359`) and calls `dSamusMainMotion_0x0D10`; that helper writes
`(joint, modelpart)` `(24,1)`, `(25,-1)`, and `(17..22,0)` at
`216_SamusMainMotion.c:954-962`, with Catch invoking it at `:971-974`. Those
events are the source for Samus native root program 1.

## Observed reject sequence

The pre-fix stress diagnostic `builds/p2-fourcpu-pal256-512.txt` recorded:

`FTRREJECT=0,6,0,0;0,166,0,0;0,2,0,0`

`NATIVEFAIL=6,1,22,262143,166,0,0,2`

The bounded GDB proof at frame 373 (`builds/codex-shieldbreak-preflightbranch.txt`)
stopped at the same Samus reject with `status=0xA6`, selected program 1, and 21
live roots. The draw-plan root count was 21 and the generated Catch owner root
count was also 21. Exact ARM branch markers reported
`MS-SAMUS-PREFLIGHT-PASS` and `SAMUSPREFLIGHTVERDICT=1`, proving selection,
root count, root resolution, matrices/preambles/tables and material-cardinality
preflight all succeeded before the owner later rejected.

## Corrected slot attribution and derivation

The earlier `0xff` attribution was incomplete. `INVALID_U8` was certainly an
illegal production palette value, but replacing it wholesale with 31 did not
identify which roots actually require a stored matrix. The generator now derives
program bindings from the **source joint** first (the same setup-parts walk used
for canonical roots), then computes the union of bindings consumed by shipped
`CROSS_MATRIX` runs. Only roots whose resident binding participates in that union
receive the canonical physical GX slot; a same-matrix root remains
`PACKED_GX_SLOT_CURRENT` (31). Generation fails if any shipped CROSS run's current
root resolves above slot 30, or if a required CROSS binding has no live storing
root.

For Samus Catch, the source-joint derivation is identical in High and Low:

`(joint:binding:slot)` =
`5:0:31, 6:1:31, 8:2:31, 9:3:31, 10:4:31, 13:5:31, 15:6:31,
16:7:31, 17:8:31, 18:9:31, 19:9:31, 20:9:31, 21:9:31, 22:9:31,
24:14:31, 27:8:31, 28:9:31, 30:10:31, 32:11:31, 33:12:31,
35:13:31`.

This is not a blanket sentinel substitution: every resident Samus Catch root
currently executes only `RAW_CURRENT` runs in the generated High/Low table, so
the derived answer is 31 for all 21 roots. MorphUnfold and MorphBall likewise
derive joint 6 -> resident binding 1 -> slot 31. Link's appended programs exercise
the physical-slot branch and therefore guard the derivation: High keeps bindings
2/3/6/7/11/12 at slots 16..21; Low keeps 11/12 at 16/17. Catch's hidden joint 18
shares binding 11 and now receives the same physical slot as that source binding
(High 20, Low 16). Regeneration changed exactly those two Link Catch slot-table
entries and no canonical geometry/table rows.

## Verification ledger

- `python scripts/fighters/generate_nds_native_owners.py`: PASS.
- `python scripts/fighters/generate_nds_native_owners.py --check`: PASS.
- `python scripts/fighters/check_nds_native_owner_packet.py`: PASS.
- `python scripts/fighters/check_native_owner_geometry_closure.py`: PASS
  (`NATIVE_OWNER_GEOMETRY_CLOSURE_OK`).
- `python scripts/fighters/test_native_samus_morph.py`: PASS.
- `python scripts/check-native-owner-wiring.py`: PASS (`owners=39 gaps=0`).
- `python scripts/check-untracked-dependencies.py`: PASS
  (`committed_refs_clean=YES`).
- `pwsh -NoProfile -File scripts/check-architecture.ps1`: PASS with two
  pre-existing warnings (large-file split plan; local generated outputs).
- `pwsh -NoProfile -File scripts/check-docs.ps1`: PASS (`docs=18`,
  `registryEntries=6`, `AGENTS.md=112 lines`).
- Regenerated include A/B against the pre-edit file: exactly 2 added / 2 removed
  lines, both `sNdsNativeLinkCatchCrossPaletteSlots{,Low}` values described above.
- Shared-lane stress build: PASS. Lane witness `ACQUIRED codex-catchslots`;
  exact target build printed `NATIVE_ONLY_PASS` with 245 actual link inputs,
  then the lane was immediately released. Final ELF timestamp
  `2026-09-13T14:39:00.5854206Z`; ROM timestamp
  `2026-09-13T14:40:24.3222586Z`.
- Slot-9 default four-fighter stress: pending.
- ELF SHA-256:
  `07BBA44632158AAE86333C526D1B0EFA0C77421C875FC6C7F721997FF27A2ED5`.
- ROM SHA-256:
  `EBBA79F3A87CAB037695C91DC5F89DB8853140344F1C7D5941F17CE80C7F509F`.

## Orchestrator takeover (2026-09-13 08:35)

The Codex session's two builds compiled the changed objects but never relinked
(its shell died on a bridge interruption holding the lane), and the stress arm's
static preflight then failed on the shared particle bake (see BUG_NOTES). The
orchestrator released the lock, made the particle pin follow the flags stamp, and
is running `verify-all -Only p2_fourcpu_stress` (build + proof) on runner slot 3.

## Orchestrator result (2026-09-13 08:40): the slot-31 change did not clear the reject

`verify-all -Only p2_fourcpu_stress` rebuilt the target (ELF 08:32, packs 07:36) and
failed with the identical tuple (`count=71 ... status=166 ... reason=2`). The
production submitter rejects any `NDS_NATIVE_RUN_CROSS_MATRIX` run whose palette
slot exceeds `NDS_NATIVE_GX_MATRIX_SLOT_MAX` (30); `PACKED_GX_SLOT_CURRENT` (31)
is legal only for same-matrix runs. Canonical Samus has no cross-matrix runs, so
its 31s are never checked; Catch's reparented parts make cross-matrix runs, whose
slots must be the bound joint's real GX palette slot. A follow-up package derives
them the way canonical roots are derived and adds a generator-time assertion.

## Orchestrator diagnosis (2026-09-13 10:20): the reject is an unlit run, not a slot

The slot hypothesis was wrong twice over: Samus Catch owns no cross-matrix run
(all 33 of its runs are submit class 0, so the all-31 slot table is correct),
and the failure record's identity 262143 (Samus, no display list) names the
whole-owner executor failure branch, not a per-run reject. A new sticky
companion record, `gNdsRendererNativeDirectReject` (count, return address of the
first rejecting call, othermode_l, combine words, env colour, geometry word,
command count), flushed like the failure record, was read at the first Samus
reject by `probe-p2-fourcpu-sparse.ps1 -FirstSamusReject -Frame 416`:

`SAMUSDIRECTREJECT=count:1,site:0x1ffb348,othermode_l:0xc4113079,combine:0xfc127e05/0xff17f3ff,env:0xffffffff,geometry:0x5,command_count:462`

The site resolves to `ndsRendererNativePrepareProductionRunCore` (its seven
reject sites share one merged tail; the function is in the full four-CPU ITCM,
which is why the record carries a return address rather than a per-site line:
a per-site constant overflowed ITCM by 312 B). The geometry word 0x5 is
ZBUFFER|SHADE with G_LIGHTING clear, and command count 462 places the reject in
program root 8, source root 0x8d90 (the first grapple-chain part). Every
production direct-policy family requires lighting, so the run is rejected at the
policy validation. Static decode of the Samus tables agrees: epochs 32/34/36
(roots 0x8d90, 0x9140, 0x8a70) end their spans with delta 61
(`0xd9ddfbff/0` = clear LIGHTING|CULL_BACK|SHADING_SMOOTH) and the root tails
restore it with delta 62; the canonical program never draws those roots.

## Fix (generator, data only): bake unlit uniform-colour roots as lit

Every chain run's vertices carry one colour (0xffffffff on 0x8d90/0x9140,
0xfeffffff on 0x8a70, both detail levels). The DS light equation is
diffuse * max(0, N.L) + ambient per vertex, so a lit run with diffuse 0 and
ambient = that colour reproduces SHADE exactly. `_bake_unlit_uniform_program_roots`
in `scripts/fighters/generate_nds_native_owners.py` walks each appended program
in draw order with the carried lighting state, and for a non-canonical root
whose runs are unlit and single-coloured rewrites its geometry deltas to keep
G_LIGHTING (new delta `0xd9dffbff/0`) and points the root at a `(0, colour)`
light preamble; any other unlit root fails generation. The generated include
records the bake above each program table. Samus Catch bakes 0x8d90, 0x9140 and
0x8a70; Link Catch's appendix roots 0x7db0 (0xfffffd00), 0x7ea8 and 0x7f98 were
unlit too and bake the same way, so Link's Catch would have been the next
first cause. Host checks after regeneration: generator --check, owner packet,
geometry closure, owner wiring, image ABI, untracked dependencies, Samus morph
(4 passed) and Kirby trio (21 passed) all green.

The first cut of the bake ran inside `build_owner_root_programs`, and a rebuilt
ROM rejected identically: Samus and Link ship their state and sequence tables in
NitroFS owner images that `generate_nds_native_owner_images.py` builds from
`build_p2_owner_runtime_context`, which never ran the program builder, so the
binary's Catch roots pointed at the new preambles while the image still carried
the lighting clear. The bake now runs in the shared context builder on every
root beyond the canonical draw (variant and appendix bakes), the program
builders only verify that no root runs unlit in draw order, and the Link
appendix pins record the baked light indices (8/9/9). Both generators' `--check`
modes and the owner host checks are green on the moved bake; the Samus/Link
`.image.c` sources carry the kept-lighting delta `0xd9dffbff`.

## Probe verdict (2026-09-13 10:45) and stress rerun

On the rebuilt ROM (10:44, `NATIVE_ONLY_PASS`) the same probe reports
`SAMUSREJECT_NONE_THROUGH=416` with `SAMUSFINAL=grapple:6,entryDraw:308,fallback:0,texReject:0x0`:
no Samus native reject through frame 416, where the Catch reject fired at frame
373 on every earlier ROM. The full four-fighter stress verifier is running on
runner slot 9 against this ROM; its verdict is recorded below when it lands.

## Stress arm 10:45 (ROM `EBB430F9`): native fence GREEN, heap floor RED

`verify-p2-four-fighter-stress.ps1 -NoBuild -RunnerSlot 9` on the fixed ROM
passed the native-render fence for the whole 1,972-frame match
(`nativeFailureCount 0`, `nativeDirectRejectCount 0`, slot 0/1 hardware
triangles 344,773 / 329,789) and then failed the next assertion, which every
earlier run had masked: "breached the general-heap safety floor: 24552 B <
25600 B" (`arenaChosenBytes 1,400,832`, page fail count 79, DObj high-water
190). A new `-HeapFloorStaircase` mode of `probe-p2-fourcpu-sparse.ps1` (a
breakpoint on the battle seam's low-water store) placed every new minimum under
the floor: frame 1556 25,560 B (DObj max 187), 1724/1725 25,380/25,056, 1858
24,696 (188), 1860 24,552 (189), end 1952 with DObj max 190. The dips track the
source objman growing its DObj pool from the general heap late in the match, not
a leak; the earlier 38,832 B free at frame 512 was measured before that growth.

Lever taken: the boot arena chooser (`diagnostics_taskman_heap.c`) stepped down
in 4 KiB pages and kept the first page that fit, leaving up to 4,095 B of the
libc top chunk unclaimed; it now probes upward from that page in 256 B steps
(`gNdsTaskmanArenaRefineBytes`, read by the stress arm) and keeps the largest
block, then applies the same 8 KiB libc reserve. Static RAM for reference
(`arm-none-eabi-size`, four-CPU ELF): `.main.bss` 885,552 B, of which the FGM
cache is 237,568, framebuffer sets 147,840, the Task36 replay owner and the
texture scratch 32,768 each, and the tick-HUD ring 10,240 (instrument only).

## Generator cross-slot hardening (2026-09-13)

Before: Link Catch High was `[31,31,16,17,31,31,18,19,31,31,31,20,31,31,20,21,31,31,31,31,31,31]`.
After: Link Catch High is `[31,31,16,17,31,31,18,19,31,31,31,31,31,31,20,21,31,31,31,31,31,31]`.
Before: Link Catch Low was `[31,31,31,31,31,31,31,31,31,31,31,16,31,31,16,17,31,31,31,31,31,31]`.
After: Link Catch Low is `[31,31,31,31,31,31,31,31,31,31,31,31,31,31,16,17,31,31,31,31,31,31]`.
Kirby Stone High/Low changed `[255]` to `[31]`; CopyLink High/Low changed `[255,17,16,255,19,18,31,31]` to `[31,17,16,31,19,18,31,31]`.

The generator now leaves non-cross appendix roots on `PACKED_GX_SLOT_CURRENT`, rejects duplicate or illegal physical program slots, and rechecks derived program store counts against `DETAIL_GX_PLAN_COUNTS`.
The saved-include A/B diff contained only the two Link Catch index-11 corrections and the Kirby Stone/CopyLink sentinel corrections above.
After that A/B, a concurrent LinkBoomerang lighting package regenerated the shared include; its additional root/light rows are outside this package and were preserved.

Host verdicts were all PASS: owner packet, native-owner geometry closure, native-owner wiring, untracked dependencies, generator `--check`, owner-image regeneration, and owner-image `--check`.
The required Samus-morph/Kirby-trio pytest run passed 25 tests with 1 skipped; the focused Link regression run passed 7 tests.
No ROM build or emulator run was performed in this package; the stress-arm proof remains with the orchestrator.

## Stress arm 11:10 (ROM `AA4F4BED`, arena refinement): +3,840 B, then Link SpecialN

The refined chooser recovered `gNdsTaskmanArenaRefineBytes 3,840` (arena
1,404,672) and the low-water rose to 25,480 B, still 120 B under the floor:
this match peaked at 203 DObjs against 190 in the 10:45 run, so about 2,900 B
of the gain went into pool growth. Matches are not identical run to run, so
the floor needs margin, not a 120 B fix. The run also failed the native fence
first: `count=16 identity=393215 status=229 reason=2` = Link (fkind 5) in
SpecialN with `geometry=0x5` and `command_count 302`, which lands in root 5 of
`sNdsNativeLinkSpecialNRoots`, the LinkBoomerangModel donor root 0xf8: the
boomerang draws unlit in one colour exactly like the grapple chain. The donor
context builder now runs the same bake and the program verifier walks each
root against the table set the runtime selects for it. The 10:45 run's Link
never threw the boomerang, which is why it passed.

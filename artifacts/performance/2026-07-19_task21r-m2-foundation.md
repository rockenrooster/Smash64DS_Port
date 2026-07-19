# Task 21R M2 structural foundation — 2026-07-19

## Scope and current owner

- Source base: `b8c70f9fb9ae5ee1d44b9942c6f8b25e4fc9a467` on `master` in the
  primary worktree.
- Canonical window: Boundary `battle_playable_realtime`, mode 163, profile 1,
  fast mode 8, frames 600–607, static texture AOT, incremental wallpaper,
  hybrid OAM, Task 9 phase 2, and Task 16 `1/1/1`.
- Current control ROM / ELF:
  `C777D2D1CE323769706A51E913CE4A3014A05E6F23CF1F9080CEA9C64A1BA0A4`
  / `25953C4345722EC1CC853C00A982AAA4455CD3517087D6715F9BBB7614E6AAF0`.
- BattleShip `ftdisplaymain.c` remains the source owner for selected display
  lists, material progression, hierarchy matrices, and draw order;
  `ftdisplaylights.c` remains the live light-direction source. No gameplay or
  renderer production behavior is retained by this task.

## Phase 0 field, LUT, and cost census

- `ndsRendererExecuteNativeFighterOwnerHierarchy` is 5,812 ARM bytes in main
  RAM with a 308-byte local frame plus nine saved registers.
  `ndsRendererNativePrepareProductionRun` is 2,188 ITCM bytes.
- `sNdsNativeFighterOwnerExecution` remains 8,800 BSS bytes. The prepared-run
  array remains 49 × 56 = 2,744 bytes, or 86 aggregate 32-byte line
  equivalents: 40 hot bytes, 12 cold validation bytes, and one 4-byte
  unconsumed `vertex_flags` field per record.
- The generated representation remains exactly 32 roots, 49 epochs, 67 runs,
  626 triangles, 1,878 corners, 52 `u16` joint schedule entries, and 32 `u8`
  binding-joint entries. The old adapter scan performs 199 Mario plus 264 Fox
  pointer comparisons per frame; the tested direct consumer performed 14 plus
  18 checks.
- The existing shade LUT and prepared live light direction already cover the
  retained shade and light work. Task 21R adds no second cache.
- The generated consumed-field manifest closes seven source closures during
  the experiment and six after the rejected runtime consumer was removed. It
  classifies immutable generation fields, camera-dependent fields, other live
  fields, callback-visible outputs, invalidation, and Task 27 ownership.

## Cut 21A — retained census, no cache

The prior complete-key census remains authoritative: 16/392 resident hits
(4.08%; Mario 11, Fox 5), zero collisions. This is below the 20% gate, so no
shade/color/UV cache is present.

## Cut 21B — hot/cold and clear deletion REVERT

A temporary `0xA5` poison fill proved that every runtime-consumed prepared-run
field is fully assigned: the poison ROM
`3B84C65FB65EF17045AA2021E8F32F4F33E52F9245F1FD81C966CDF3827E915E`
retained exact 320/306 fighter ownership, 70 runs, 686 triangles, static
residency, and 0/49,152 changed synchronized pixels versus control.

The independent candidate then deleted the 49 × 56-byte zero clear and the
dead `vertex_flags` store without changing the 56-byte record or 8,800-byte
workspace. P50/P95/maximum are identical in the last two positions because an
eight-sample P95 is the maximum.

| Metric | Control | Candidate | Delta P50/P95 |
|---|---:|---:|---:|
| Matrix | 158,464/158,528/158,528 | 157,120/157,568/157,568 | -1,344/-960 |
| Mario | 169,792/171,392/171,392 | 170,336/171,968/171,968 | +544/+576 |
| Fox | 209,408/209,792/209,792 | 210,016/210,368/210,368 | +608/+576 |
| Draw | 1,002,496/1,005,824/1,005,824 | 1,003,136/1,006,400/1,006,400 | +640/+576 |
| Active | 1,006,560/1,060,672/1,060,672 | 1,007,136/1,061,120/1,061,120 | +576/+448 |
| Loop | 1,680,448/1,680,448/1,680,448 | 1,680,448/1,680,448/1,680,448 | 0/0 |

The candidate saved 12 ITCM bytes but failed the complete fighter/draw/P95
gate. A permitted A2 was attempted because the phase counters were internally
inconsistent, but two default-runner and one isolated-runner attempts failed
before sampling when melonDS did not open its GDB listener. Those attempts are
not evidence. The original clear, assignment, layout, code, and BSS are
restored.

## Cut 21C — compact foundation KEEP, runtime consumer REVERT

The tested runtime consumer exposed the existing compact schedule to the
adapter and replaced the 463 pointer comparisons with 32 checked direct
indices. The same slot-5 melonDS instance, configuration, flags, ROM window,
and synchronized frames produced this valid A/B:

| Metric | Control | Direct-index candidate | Delta P50/P95 |
|---|---:|---:|---:|
| Present | 1,471,456/1,475,264/1,475,264 | 1,471,104/1,475,008/1,475,008 | -352/-256 |
| Matrix | 158,464/158,528/158,528 | 157,024/157,632/157,632 | -1,440/-896 |
| Material | 15,712/15,936/15,936 | 16,224/16,448/16,448 | +512/+512 |
| DL | 204,224/207,360/207,360 | 205,120/208,256/208,256 | +896/+896 |
| Mario | 169,792/171,392/171,392 | 170,432/171,968/171,968 | +640/+576 |
| Fox | 209,408/209,792/209,792 | 210,176/210,496/210,496 | +768/+704 |
| Draw | 1,002,496/1,005,824/1,005,824 | 1,003,872/1,007,168/1,007,168 | +1,376/+1,344 |
| Active | 1,006,560/1,060,672/1,060,672 | 1,007,904/1,070,016/1,070,016 | +1,344/+9,344 |
| Loop | 1,680,448/1,680,448/1,680,448 | 1,680,448/1,680,448/1,680,448 | 0/0 |

Both ROMs preserve exact 70/686 runs/triangles, 320/306 ownership, zero
fallback/conservation/fence faults, and 0/49,152 changed synchronized native
pixels. The candidate added 120 main-text/rodata bytes while leaving ITCM,
BSS, the 8,800-byte owner workspace, and the 308-byte owner frame unchanged.
It therefore fails the complete-owner timing gate and is fully reverted. No
retail claim is made and no retail repeat is requested.

The already-valid generated `u16` joint schedule, `u8` binding indices,
16-byte root/epoch records, 8-byte run records, source order, provenance, and
new consumed-field/invalidation manifest are retained as Task 27 inputs. This
is only the Task 27 foundation, not the generated fighter architecture.

## Retained files and verification

- `docs/optimization/NDS_NATIVE_FIGHTER_CONSUMED_FIELDS.generated.json`
- `scripts/generate_nds_native_owners.py`
- `scripts/check_nds_native_owner_hierarchy.py`
- Control/candidate JSON:
  `artifacts/performance/2026-07-19_task21r-21c-control.json` and
  `artifacts/performance/2026-07-19_task21r-21c-candidate.json`.
- Control/candidate screenshots:
  `artifacts/visibility/2026-07-19_task21r-21c-control.png` and
  `artifacts/visibility/2026-07-19_task21r-21c-candidate.png`.

Focused generation, hierarchy, packet, GBI, parity-corpus, registry, Task-9
float-ITCM, and renderer-ITCM checks pass. The restored production build is
byte-identical to the already-green 00:25 Boundary checkpoint:
`757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4`
for the battle ROM and
`D06323485C866D74BA5D82F87B58182C82A3D7FBE5E9AAC08B83807583171A9E`
for the public ROM.

Two closeout `-NoBuild` runs, on independent runner slots 5 and 6, each pass
the complete canonical lifecycle smoke and two-ROM publication contract. Both
then stop only at the duplicate host capture step because unattended melonDS
stays responsive but exposes no top-level window handle. A direct launch
reproduces that host UI state. This is excluded as screenshot-transport
evidence: the synchronized 21C A/B screenshots above already prove
0/49,152 changed pixels, and the retained production ROM is the exact prior
full-Boundary ROM. No further emulator or retail repeat is requested.

**Decision: KEEP CENSUS AND COMPACT MANIFEST FOUNDATION / REVERT 21B AND THE
21C RUNTIME CONSUMER / TASK 27 NEXT.**

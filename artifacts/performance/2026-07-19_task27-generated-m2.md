# Task 27 generated M2 Mario root/epoch program — 2026-07-19

## Scope and decision

- Source checkpoint: `c6b0d6695e22a6d11f6d6acf56b3da4d541cde42` on `master` in the
  primary worktree.
- Boundary window: mode 163, profile 1, fast mode 8, frames 600–607,
  source updates 1206–1220, static texture AOT, incremental wallpaper,
  hybrid OAM, Task 9 phase 2, and Task 16 `1/1/1`.
- Phase A is retained: the generated source-order certificate, manifest, and
  falsifiers cover Mario's exact program without changing production code.
- The independently selected Mario runtime candidate is **REVERT**. It missed
  the 8,000-tick continuation gate, regressed Mario and complete draw, and was
  removed before any Fox expansion. No retail repeat is requested.

## Phase A certificate and bound

The generator derives the program from the hashed Mario O2R resource and the
already-retained Task-21R compact tables. It emits a straight-line macro with
106 structural events and records every immutable state effect in the
consumed-field manifest.

| Contract | Exact value |
|---|---:|
| Joint / root / epoch / run events | 25 / 14 / 18 / 30 |
| Raw / cross runs | 21 / 9 |
| Triangles / corners | 320 / 960 |
| Root-prefix / intra-root light commands | 48 / 4 |
| Immutable state effects | 62: 39 before / 22 after / 1 tail |
| Source checksum | `0x791c164e` |
| Table checksum | `0xd78db920` |
| Event checksum | `0xbf8dccfd` |

The checker rebuilds the program twice, requires roots `0..13`, epochs
`0..17`, and runs `0..29` in exact source order, rejects scanner/packet forms,
and proves that a required-mask mutation changes only the table checksum while
a run-class mutation is rejected. The current renderer consumed-field closure
remains fail-closed, so a new live read cannot silently enter the program.

Task 25R sets Mario's observed P95 ceiling at 171,520 ticks. The Task-27
continuation threshold is therefore an 8,000-tick combined-fighter P50 saving
(4.66% of that Mario ceiling), or a credible 35,000-tick two-fighter
projection. A temporary detailed-ledger control
(`A21795739FB4C4673791EEAECF29A08F9E7BE33515F01B572761B6E38FA4D354`)
localized Mario's current work as follows; these rows are diagnostic and are
not mixed into the standard A/B vote.

| Mario detailed row | P50 / P95 / max ticks |
|---|---:|
| Production | 122,272 / 124,352 / 124,352 |
| Preflight state | 42,560 / 43,776 / 43,776 |
| Lighting | 34,496 / 36,224 / 36,224 |
| Root GX | 7,648 / 7,744 / 7,744 |
| Run preparation | 13,792 / 13,888 / 13,888 |
| Emit/account | 23,520 / 23,616 / 23,616 |
| Residual | 12,960 / 13,120 / 13,120 |

## Mario-only runtime experiment — REVERT

The candidate kept the complete current preflight before GX, expanded the
generated joint/root/epoch/run program only for Mario, used the current live
materials, CPU lighting, prepared texture state, and raw/cross emitters, and
left Fox on the complete control executor. A one-time 106-event validation ran
before the first GX mutation; there was no post-GX fallback. The candidate ROM
and ELF were
`207588F9640B4312FCD0ED08223F8AF7C3327E268668B4E704564741349F82AC` /
`A6A8CCC927C442297A4199C50108068373C6817F7D940B9B6AD8016210468DF2`.
ITCM remained 28,808/32,768 bytes with 3,960 bytes free.

The control is the retained Task-21R same-slot frame-600 artifact; Task 21R
restored production behavior before its compact manifest was committed. P95
is the maximum for these eight samples.

| Metric | Control P50/P95/max | Generated Mario P50/P95/max | Delta P50/P95 |
|---|---:|---:|---:|
| Present active | 1,471,456/1,475,264/1,475,264 | 1,471,328/1,475,200/1,475,200 | -128/-64 |
| Matrix | 158,464/158,528/158,528 | 155,328/155,520/155,520 | -3,136/-3,008 |
| Display-list work | 204,224/207,360/207,360 | 204,928/208,064/208,064 | +704/+704 |
| Mario | 169,792/171,392/171,392 | 169,920/171,520/171,520 | +128/+128 |
| Fox control | 209,408/209,792/209,792 | 209,632/209,984/209,984 | +224/+192 |
| Complete draw | 1,002,496/1,005,824/1,005,824 | 1,005,120/1,008,384/1,008,384 | +2,624/+2,560 |
| Whole loop | 1,680,448/1,680,448/1,680,448 | 1,680,448/1,680,512/1,680,512 | 0/+64 |

Both sides report the same `70/686` run/triangle total,
`60/320/306` stage/Mario/Fox ownership, `29/0/0` current fallback-state /
vertex / command census, and zero conservation error. The candidate's matrix
reduction is real but is not an independent owner win: the generated calls and
code layout erase it inside Mario and regress complete draw. It is 8,128 ticks
short of the continuation threshold even before accounting for the draw
regression. Because the timing gate is decisive, no screenshot/pixel promotion
run was spent on the candidate.

All selector, renderer, validation, and runtime-executor code is removed.
Fox was not attempted. The retained generated macro is inert unless a future
task explicitly consumes it, and the checked-in JSON remains the provenance,
source-order, state-effect, invalidation, and falsifier record.

## Closeout verification

Generation/check mode, the eight-mutation hierarchy checker, native-owner
packet census, GBI fixtures, Task-9/16 float placement, renderer ITCM, the
canonical lifecycle smoke, and the two-ROM publication contract pass. The
production rebuild is byte-identical to the already-green Task-21R Boundary
checkpoint:

- battle ROM: `757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4`
- public ROM: `D06323485C866D74BA5D82F87B58182C82A3D7FBE5E9AAC08B83807583171A9E`

That byte identity proves the retained certificate has zero production or
pixel effect. The full Boundary wrapper stopped only after those gates, when
unattended melonDS again exposed no capturable top-level window and produced
no new screenshot. This is the same excluded host UI-transport condition as
the immediately preceding Task-21R checkpoint; no duplicate capture or retail
repeat is requested.

**Decision: KEEP PHASE-A CERTIFICATE/MANIFEST / REVERT MARIO EXECUTOR / STOP
TASK-27 EXPANSION / DO NOT ATTEMPT FOX.**

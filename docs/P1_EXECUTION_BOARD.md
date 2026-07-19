# P1 Execution Board

Updated: 2026-07-19 06:45 Central

Deadline: 2026-07-19 23:59 Central

Boundary: `battle_playable_realtime`, mode 163

This is the only dynamic P1 queue. `HANDOFF.md` owns the restart surface,
`optimization/NATIVE_RENDERER_PLAN.md` owns the renderer contract,
`PERF_LEDGER.md` owns measurements, and `PORTING.md` is append-only history.

## Artifact Identity

Integrated user-facing candidate:

```text
smash64ds-battle-playable-hwtri.nds
14,688,256 bytes
SHA-256 C344CA8B89903A35678A6DC4A849F217B690CBCE16841FAC47974626899EB9DF
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Hardware reality (2026-07-18)

A manual retail-DS observation currently puts real-hardware throughput near
`0.75x` the local melonDS run, but that is a starting observation rather than
an admitted uniform multiplier. Task 10 separates CPU/ITCM, CPU/main-RAM, and
GX work so each owner can be projected from its matching on-device bench.

The first same-workload retail baseline is now authoritative: the mode-163 ARM
control photographed `UPD 374,464`, `DRW 1,743,296`, `ACT 1,745,984`,
`LOOP 2,240,384`, `FPS 13.5`, and `SLIP 0`. That is roughly four VBlanks per
update and 13.5-15 FPS in the observed heavy-combat phase. Against its matching
melonDS packet, draw is about x1.51 and update about x1.73 on device. A credible
20 FPS draw budget is below 1,680,570 device ticks, so this sample still needs
roughly another 100K repeatable device draw ticks of margin. The next admitted
levers are the now-proved update locality cut and a measured DTCM audit after
the demo; emulator-only cache projections and GX FIFO DMA stay out of the queue.

The ARM-base Task 19 pair now confirms the update-only placement at the same
`00:58` phase: UPD 366,016→363,456 (-2,560/-0.70%) and DRW
1,699,328→1,696,640 (-2,688/-0.16%) cross LOOP 2,800,832→2,240,448
(five to four VBlanks) and FPS 13.9→14.3, with SLIP 0. ACT rises 15,872
(+0.93%). Keep the threshold win, but do not project the earlier Thumb-base
44,160-tick UPD result onto ARM or call every owner row improved.

| Resource / bench | Fixed work | melonDS 1.1 ticks | Retail DS ticks | HW / melonDS multiplier |
|---|---|---:|---:|---:|
| CPU-ITCM / ALU-ITCM | 1,000,000 dependent chains, 32 adds each | 17,501,568 | 18,002,048 | 1.03 |
| CPU-mainRAM / MEM-THMB | 8,388,608 loads over 256 KiB | 33,557,632 | 50,170,432 | 1.50 |
| CPU-mainRAM / MEM-ARM | identical loads and buffer, ARM state | 37,752,448 | 45,362,752 | 1.20 |
| CPU-cache control / CACHE4K | same load count over 4 KiB | 33,565,696 | 29,676,224 | 0.88 |
| GX / GX-BRST | 10,000 immediate triangles, flush every 2,048 | 2,729,728 | 2,378,432 | 0.87 |
| CARD (optional) | no safe media-independent run admitted | not run | not run | — |

Retail column source: Tyler's device photo
`artifacts/visibility/2026-07-17_233353-8578811_task10-hardware-calibration_real_nds.jpg`
(claude-read 2026-07-18). Two photo digits were disambiguated by physics, not
guessed: ALU-ITCM must exceed the 16,777,216-bus-tick single-issue floor for
33,554,432 dependent adds, fixing the second digit to 8 (18,002,048); MEM-THMB
matches the CACHE4K hardware floor plus ~20 bus ticks per 32-byte line fill
(1,048,576 fills), fixing 50,170,432 over 58,170,432. MEM-ARM mid digits carry
about +-2% photo uncertainty; its ratio is 1.15-1.22 under any plausible read.

Calibrated findings (2026-07-18):

1. melonDS 1.1 does not model the ARM9 dcache: it charges the 4 KiB and
   256 KiB load loops identically (0.02% apart) while hardware runs the
   cache-resident loop 41% faster. Cache-locality wins are invisible in
   melonDS and must be measured on hardware (TASK 12 hardware-primary rule).
2. The hardware tax is streaming/miss-heavy main-RAM traffic: x1.50. This is
   the bucket most of our draw-prep and update CPU work lives in.
3. The ARM/Thumb ranking inverts between emulator and hardware: melonDS
   penalizes ARM code in main RAM continuously (no icache model); hardware
   runs the icache-resident ARM loop faster than the Thumb one. Hot,
   cache-resident functions prefer ARM; only the cold bulk favors Thumb.
   Neither bench measures large-footprint code streaming, so TASK 12's
   on-workload hardware A/B remains the decisive test.
4. GX transport is cheaper on hardware (x0.87): moving work onto the GX
   (TASK 8 CUT A style) is worth more on-device than melonDS credits, and
   GX FIFO DMA feeding is deprioritized (no hidden stall tax observed).

No emulator-to-device formula or universal multiplier is admissible. The rows
above describe only their exact microbench workloads; they may rank a matching
diagnostic, but they cannot synthesize a whole-frame device result or promote a
candidate. Emulator evidence remains authoritative for deterministic state,
semantic/GX traces, arithmetic, and pixels. Retail A/B is authoritative for
DTCM, ARM/Thumb, code/data/cache layout, generated-program footprint, direct
VRAM stores, DMA, GX FIFO behavior, and final pacing. The former `840,000`
melonDS planning figure is retired; stable 30 is decided by the device's actual
two-VBlank interval distribution and zero-slip lifecycle.

Hardware operator packet:

Lab ROM: `builds/build-task10-hardware-calibration/smash64ds-task10-hardware-calibration.nds`
(`04F32CA76045B821A5404D55F740808DF13B3A0BA2261CC8D7AD5DB3459D263C`).
Profile-1 HUD ROM: `builds/task10-hardware-packet/smash64ds-task10-phase-hud-profile1.nds`
(`37AD2C8A4C388F3969A52E048D4921B524F8F579516A6973944CE5A7D9E81B60`).

1. Copy `builds/build-task10-hardware-calibration/smash64ds-task10-hardware-calibration.nds`
   to the flashcart, boot it on retail hardware, wait for `COMPLETE`, and take
   one legible photo containing the five tick rows and the displayed Git hash.
2. Report the DS model, flashcart, loader/settings, and photographed values.
3. Run the profile-1 HUD ROM above in the same combat phase and photograph the
   half-second HUD sample (`UPD`, `DRW`, `ACT`, `LOOP`, `SLIP`, `GIT`).

`PRE`/`PRP`/`CMT` rows are populated only by the existing detailed renderer
phase profiler; the low-observer profile-1 ROM intentionally leaves them off.

## Tasks 20R-25R atomic reconciliation checkpoint

Live-unit capture began on branch `master`, worktree
`D:\Stuff\DevFolder\Smash64DS_Port`, HEAD
`458191bef147f1c6963b2f533c601f9f68fc7730`. The tree was intentionally dirty:
the user's 344-line
`docs/optimization/ClaudeFable5_JumpABC_Tasks_20260715_2326.md` change is
preserved, unedited, and outside this checkpoint; the user-supplied
`docs/optimization/tasks.md` is the new authoritative queue.

| Unit | Class / atomic sub-cut | Owned files or hunks | Checkpoint disposition |
|---|---|---|---|
| Task 20R | **Complete audit / no placement candidate** | retained default-off census; `scripts/check-task20-dtcm-layout.ps1`; canonical verifier hook; report | Profile 0 has exactly 0/152 DTCM/DTCM-BSS bytes owned only by Calico's 128-byte IRQ table and 24-byte scheduler state. The data/user-stack gap is 15,848 bytes. The intact 16,384-byte gameplay stack plus a 64-byte guard is `NO_FIT` by 600 even assuming zero user-stack use; the provisional 13,044/3,700 replay is `NO_FIT` by 960, or 3,008 with margins. The plausible 2,048-byte modelview scratch has no DMA/IPC/ARM7 escape but cannot be promoted without the declined retail A/B. Keep the fail-closed owner/boundary/DMA checker and census; no profile-0 placement or symbol exists. |
| Task 21R | **Complete** — 21A no-cache; 21B runtime REVERT; 21C compact foundation KEEP / runtime consumer REVERT | `scripts/generate_nds_native_owners.py`; `scripts/check_nds_native_owner_hierarchy.py`; `docs/optimization/NDS_NATIVE_FIGHTER_CONSUMED_FIELDS.generated.json`; report | Preserve 21A's 16/392 exact hits (4.08%). A poison fill proves full prepared-run assignment, but deleting the clear/dead store regresses draw P50/P95 +640/+576. Replacing 463 pointer comparisons with 32 direct checks saves matrix -1,440/-896 but regresses complete draw +1,376/+1,344 and active P95 +9,344 in a same-slot A/B; both runtime cuts are exact-pixel REVERTs. Retain the 32/49/67 compact `u16`/`u8` tables, provenance, widths, source order, and six-closure consumed-field manifest for Task 27 only. |
| Task 27 | **Complete** — Phase-A generated certificate KEEP / Mario runtime REVERT / Fox not attempted | `scripts/generate_nds_native_owners.py`; `scripts/check_nds_native_owner_hierarchy.py`; generated manifest/include; report | Deterministically certify Mario's 25/14/18/30 joint/root/epoch/run program, 320 triangles, 960 corners, 48/4 light commands, 62 immutable effects, and source/table/event checksums. The temporary Mario executor saves matrix -3,136/-3,008 but regresses Mario +128/+128, Fox +224/+192, and draw +2,624/+2,560 P50/P95; it misses the 8K gate by 8,128 ticks. All runtime/selector code is removed. Retain only the inert Phase-A certificate and stop before Fox. |
| Task 22R | **Complete** — production key/census KEEP; threshold-4 writer REVERT; DMA not admissible | retained production complete key; neutral span/store census; verifier oracle; closeout report | Production already has complete 16-field exact final-key reuse and double-buffered X/Y maps. The candidate improves moving-camera windows but regresses natural-KO wallpaper P95 366,784 -> 370,944 (+4,160), over the +2,000 gate; profile 2 records 136,192 map and 14,942,208 pixel checks with zero mismatch. The writer/selector remain absent. Phase D requires a retained CPU writer and retail proof, so it is skipped. Profile 0 has zero Task-22/profile/oracle symbols. |
| Task 24R | **Complete** — prior worktree batch plus quiet-slot output diet | migration manifest; retained evidence checker; closed lab-build/telemetry deletion report | The prior batch migrated 225 unique evidence files and removed 17 proven worktrees. The quiet-slot batch removes 7,929 files / 3,746,285,595 bytes from 14 closed/reverted lab builds and three telemetry roots. Protected performance/visibility digests are unchanged across deletion; 1,745 immutable migration destinations rehash with zero failures. All 15 held worktrees, all 24 branches, current generated A/Bs, hardware pairs, logs, and user dirt remain. Post-delete Boundary passes. |
| Old Task 25 | Not live — completed immediately before reconciliation | historical phase-matrix evidence/tooling | Preserve as historical input only. It is not Task 25R and cannot set current priority. |
| Task 25R | **E -> complete report-only baseline** | bounded profile-1 source-update/interval timers; profile-0 external pacing trace; phase-matrix/identity checker; evidence only | One profile-1 ROM supplies all seven detailed windows and one profile-0 sibling supplies the 4,084/2,042 lifecycle. Pixels are 0/49,152 and reserve is 166,672, but stable-30, fixed-window exactness, KO-audio cleanliness, and the post-GO texture fence fail. The same-ROM P95 ranking selects M3-first. No production executor or profile-0 symbol/layout changed. |
| Task 23R Phase 1 | Complete census / cache **REVERT** | compile-gated residual counters and marker/checker exports only | Five passing eight-frame windows find 40/40 complete-key hits and an 832–896-tick key against an 85.4–86.3K-tick avoided upper bound. One same-ROM narrow cache saves about 13.1K residual-prepare and 12.0K active/draw ticks with exact pixels, but changes the DS working set, leaves the VBlank histogram unchanged, and lacks the required profile-2 sampled recompute and retail falsification. All cache code/data/selectors were removed; retained production footprint is zero. |
| Task 29 | **Complete Phase-A census / no Phase-B or Phase-C promotion** | default-off GX command/owner census; exact verifier exports; runner-port isolation; report | Three synchronized windows conserve every command, word, owner, boundary, and actual triangle with zero census faults. Control and alpha are the only untried state-sized opportunities, but retail proof is mandatory and the user declined repeats. Existing texture/poly shadows already remove their safe repeats; matrix mode has none; prior color/texcoord suppression regressed. Task-26 segment 0 is not an immutable Task-29 run and misses the 5K actual-run gate. Keep diagnostic infrastructure only; profile 0 gains no behavior or footprint. |

## Attack / hit audio-visual asset audit (2026-07-19)

The pre-Task-30 audit is complete. Preserve all seven `five-effects-*`
branches and the patch-unique attack/Fox/hit worktrees; none was reset, merged,
or deleted. Their source requirements are reconciled with the current DS paths
in `artifacts/performance/2026-07-19_attack-hit-av-asset-audit.md`.

The exact resident FGM pack now has 27 entries / 128,196 bytes, 20 unique
samples / 127,044 bytes, and 2,876 bytes remaining under its 128 KiB cap.
Mario down-bounce ID 303 is an exact 32-byte alias of Fox ID 300's byte-identical
source program and AOT sample. Fox Smash2/Smash3 IDs 373/374 add their exact
source samples and schedules with zero fidelity debt, so all three random Fox
smash variants are present. The focused deterministic window improves from
22 supported + 19 unsupported requests to 26 + 15; focused runtime keeps
183,056 bytes reserve. Pitch-scheduled fighter voices 375/429/431/435/440,
nonrepresentable activation cues, and composite hit cues remain explicitly
fail-closed rather than playing a wrong substitute.

The new Boundary-owned visual checker regenerates exactly 178 Mario/Fox motion
effect calls across 17 source kinds and proves a bounded live DS presentation
route for every one. It separately guards Fox reflector, Fox blaster glow, and
all three Mario fireball effect kinds. Focused fireball and natural hit paths
pass at 3 created / 13 submitted / 96 triangles and source contact ID 32 with
audio `1/0/1`, visual `3/3/24`, respectively. This is P1 source-derived routing,
not a claim that the original common particle texture/script banks are resident.

The rebuilt canonical Boundary passes with the current ROM identity above. Its
fixed-frame capture reports 786 / 49,152 meaningfully changed motion pixels
(1.599%) and all independent visibility/detail gates green; the smoke is
25.9 presentations/s / 51.6 updates/s. A PowerShell-only checker integration
bug that could exit before the build was removed; thrown checker failures now
propagate through the existing `ErrorActionPreference = Stop` contract.

During the only renderer edit, `/root/task21_23_renderer` was the sole writer of
`src/nds/nds_renderer.c`; that lane is stopped. Focused PowerShell parsing, GBI
fixtures, the Task 20 startup-sample recheck, Task 21 census, Task 22 oracle,
post-revert census, and the final profile-0 Boundary gate are green. `DevFast`
is retired and is not a registry profile; no obsolete gate was revived. This
closes the Task 24 safety gate for the evidence-cleared batch.

## Stable-30 retail contract

The final profile-0 device gate requires exactly two unchanged source updates
per presentation, with no debt, catch-up, skipped logic, interpolation, or
speculative state. Across the complete canonical lifecycle it permits zero
presentation intervals of three or more VBlanks and zero pacing-slip events.
State, audio, lifecycle, geometry, material, texture, depth, and ownership must
remain exact; synchronized native top-screen delta must be `0/49,152`; net
reserve must be at least 128 KiB. Each phase publishes P50, P95, maximum, N,
the interval histogram, presentations/s, and source updates/s.

## Task 25R authoritative matrix (2026-07-18)

Task 25R is complete as a report-only baseline. All detailed rows come from
source HEAD `f088db98de272e9788405c2181029ad4a4c353ba`, profile-1 ROM/ELF
`6E90D4140E6332E8F37BB05CB8B35ED192AAB448B26E110916992F2C15701921` /
`55CC8EF067E68310441F7025978FCF7569E95E51A3B6B937E7811A7D25B44C06`.
The production sibling ROM/ELF is
`E685C034D301D3C6881D398B14820D0D60A112FA259657A6B05408C90683C5CE` /
`E9428051194F9E6917427DCD54BE44838C049E856ECBAFD6A702D605A34A1A96`.
Both use mode 163, mode 9, static AOT 1, incremental wallpaper 1, live Fox,
Task 9 1/1, and Task 16 compare/i2f/add-sub 1/1/1. Profile 0 contains none of
the three new profile-1 timing symbols.

| Phase | Frame / source window | Loop P50/P95/max | Stage P95 | Mario+Fox P95 | 2/3/4/5+ | Exactness note |
|---|---|---:|---:|---:|---:|---|
| Countdown / GO | 438–445 / 484–498 | 1,680,448/1,680,512/1,680,512 | 464,320 | 380,928 | 0/8/0/0 | Pass |
| Early combat | 600–607 / 808–822 | 1,680,448/1,680,448/1,680,448 | 464,448 | 207,040 | 0/8/0/0 | Mario is source-owned nonrendering: 91/508 = 202+0+306 |
| Whispy | 1398–1405 / 2404–2418 | 1,680,448/2,240,640/2,240,640 | 464,576 | 381,312 | 0/6/2/0 | Post-GO texture fence already nonzero |
| Late | 1846–1853 / 3300–3314 | 1,120,256/1,120,320/1,120,320 | 464,384 | 380,416 | 8/0/0/0 | Fence nonzero |
| Natural KO | 566–573 / 740–754 | 1,120,256/1,680,512/1,680,512 | 468,480 | 205,440 | 6/2/0/0 | Mario nonrendering; historical 708 gate is stale |
| Rebirth | 589–596 / 786–800 | 1,680,448/2,240,640/2,240,640 | 464,192 | 207,616 | 0/7/1/0 | Mario nonrendering; historical 730 gate is stale |
| Time Up | 1988–1995 / 3584–3598 | 1,120,256/1,680,448/1,680,448 | 464,320 | 381,632 | 7/1/0/0 | Fence nonzero; Results/teardown proven by profile 0 |

Profile 0 retains exactly 4,084 updates / 2,042 presentations and exactly one
teardown. It runs at 18.6 presentations/s and 37.3 source updates/s with
interval histogram `61/1547/396/38` for 2/3/4/5+ VBlanks: 1,981 intervals are
three or more VBlanks and the excess is 2,457 VBlanks. Net reserve is now
166,672 bytes, so the 128 KiB floor passes. Profile-1/profile-0 frame 607 is
exactly `0/49,152` changed native top-screen pixels.

Two current correctness debts are explicit rather than hidden by the
report-only path. The post-GO texture fence first trips at class+1/frame
`10/1111` with counts `1,1,1,0,1,1,0,0,0,1`. The natural Mario KO emits the
correct `439/292/154` source triplet and mask `0x13`, but playback/generation
failure counts are 1/1, so the audio gate is false. Normal verification remains
strict for both. `-RequireStable30` correctly fails on pacing, phase exactness,
audio, and fence while reserve passes.

The same-ROM leaf-owner P95 result selects **M3-first**: the largest stage P95
is 468,480 ticks (natural KO), versus 380,544 for the largest combined fighter
pair (Countdown). Task 23R Phase 0 is complete; resume with Task 26, then only
the residual Task 23R work Task 26 leaves. The complete P50/P95/max/N table, ranked
owners, task bounds, interval rows, identity packet, disassembly, map, and
screenshots live under
`artifacts/performance/2026-07-18_task25r-*` and
`artifacts/visibility/2026-07-18_task25r-*`.

## Task 23R Phase 0 certificate (2026-07-18)

The generated manifest binds 588 pointer-field accesses across 36 M3 production
closures: 140 immutable-generation, 43 live-camera-dependent, 260 live-camera-
independent, and 145 callback-visible mutation/output. Every pointer base is
observed, so a newly named live cache or output base cannot evade the
unclassified-read gate. Generation/stamp admission, per-frame
matrices and near-plane work, four live material snapshots, callback order and
restoration, texture/config state, and pre-GX fail-closed behavior are explicit.
The checker regenerates the manifest byte-for-byte and rejects every new or
removed tracked access until classified. No runtime reuse, cache, code, data,
stack, reserve, or DS working-set change was made.

Eight same-ROM eight-frame windows cover Countdown, early, the exact natural
Whispy Wait→Open/material-animation edge at frames 675→676, Whispy steady,
natural KO, rebirth, late, and Time Up/Results. Every one of the eight prepared
segment lanes and four material lanes records `7/0` adjacent hits/changes in
every window. The transition capture independently exports the source-state
edge and exact G2 tuple change; first-sample and cross-window boundaries remain
explicitly uncounted. The ROM/ELF remains `88EF4931...` / `FEC9EB30...`.
Early frame 607 is exactly `0/49,152` changed pixels against Task 25R. Boundary
passes in 77.8 seconds with the new checker in the normal GBI fixture path.
Evidence is `artifacts/performance/2026-07-18_task23r-phase0.md`, the eight
adjacent JSON captures, and
`docs/optimization/NDS_NATIVE_STAGE_CONSUMED_FIELDS.generated.json`.

Task 26 segment 0 is retained and broad expansion stops at that atomic unit.
The generated fixed program covers 21 DObjs, bindings 0–19, runs 0–25, 54
triangles, 22 epochs, 108 dense vertices, 123 state effects, and 90
synchronization effects, while matrices, clipping, materials, texture/color/
alpha/UV selection, validation, commit, and GX emission remain live. Exact
current/generated CPU traces match all 2,775 words and 26 rows across eight
frames; the live-mutation proof is inject/mismatch/revalidate `1/1/1` before
GX. Five Task-25R-control phase pairs save 3,424–3,616 stage P50 ticks with no
relevant P95 loss and synchronized pixels remain `0/49,152`.

The current hardware-HUD melonDS pair saves 10,240/10,368 stage P50/P95 ticks
and 11,232/11,200 draw ticks. The user's single retail observation saves
21,568 DRW ticks (-1.25%), changes ACT +4,288 (+0.25%), and leaves LOOP in the
same VBlank bucket (-384); its refreshing control UPD spike is excluded.
This is device-informed KEEP evidence, not a repeatability or general hardware-
speed claim. The user declines further retail repeats, so Task 26 does not
widen the generated working set. Task 23R Phase 1 now owns residual
remeasurement; add no cache unless exact hits are at least 20% and key cost is
less than half the avoided residual.

Task 23R Phase 1 is now closed. Five passing synchronized windows cover
Countdown/GO, early combat, the material transition, natural KO, and natural
rebirth. The complete 484-byte residual key hits 40/40 opportunities and costs
832–896 ticks, while the measured avoided upper bound is 85,376–86,272 ticks.
A same-ROM, default-off candidate reused only the 204 residual dense color/UV
values; all state spans, texture resolution, material progression, matrices,
and 118 near transforms stayed live. Early-combat P50/P95 moved residual
prepare 108,480/108,672 -> 95,360/95,616, draw
1,406,912/1,431,040 -> 1,394,944/1,419,072, and active
1,413,312/1,437,376 -> 1,401,312/1,425,408. Pixels are exactly
`0/49,152`, but loop and the `3/4/4` VBlank interval row are unchanged. The
working-set-sensitive mechanism cannot be promoted without retail
falsification, and its measured window did not exercise the required profile-2
sampled recompute. The cache, selector, globals, BSS, and helper code are
removed. Keep only the compile-gated census; production footprint is zero.

## Task 29 exact GX census (2026-07-19)

The profile-1, real-GX-only census observes every control, texture, matrix,
geometry, and flush command after the normal emission path has been selected.
It records exact command words, equal consecutive values, owner partitions,
dual stream hashes, boundary hashes, and flush lifetimes without suppressing
or reordering anything. Early, Whispy, and natural-KO windows conserve the
synchronized 202/320/306 fast-owner contract plus 16-28 real dynamic
stage/effect triangles; all windows have zero census, fallback, and fence
faults. Instrumented versus compiled-out frame 445 is exactly `0/49,152`
changed native pixels, and the independent profile-2 semantic oracle passes.

Early combat records 7,227 commands / 14,415 words and 1,976-1,991 equal-value
observations. The only untried state-sized opportunities are control (330 of
412 equal) and alpha test (28 of 36). They are not promoted because Task 29
requires a repeatable retail result for GX/MMIO behavior and the user declined
further device repeats. Texture-parameter and polygon-format safe repeats are
already removed by the exact Task-8 shadow; matrix mode has zero repeats;
color/texcoord suppression previously regressed and remains reverted.
Side-effectful binds, matrix stack/load operations, begin/end, vertices, and
flush stay fail-closed in the never-suppress mask.

No immutable-run template is admitted. Task-26 segment 0 is an independently
retained generated representation, not one Task-29 immutable GX span. Its
same-control phase savings are 3,424-3,616 stage P50 ticks, below the 5,000
actual-run entry gate, and its one non-repeatable retail observation improves
DRW by 21,568 while ACT rises 4,288. Keep the compile-gated census, exact
owner/conservation verifier, runner-slot GDB-port isolation fix, and forensic
timeout forwarding; add no profile-0 GX code, data, shadow, or template.
Evidence: `artifacts/performance/2026-07-19_task29-gx-census.md` and its four
JSON captures plus paired exact-pixel screenshots. Final profile-0 Boundary
ROM/ELF are `21D789F3...` / `89C83C40...`; the ELF has zero Task-29 symbols.

## Task 24R quiet-slot cleanup (2026-07-19)

The evidence-first batch removes exactly 7,929 files / 3,746,285,595 bytes:
6,581 files / 699,406,817 bytes from 14 closed/reverted Task-13, Task-20R,
Task-21R, Task-22R, Task-28, and Task-29 lab directories, plus 1,348 files /
3,046,878,778 bytes from `verifier-cost`, `verifier-temp`, and
`emulator-logs`. The pre-delete Lean snapshot at
`C:\Users\Tyler\Desktop\Snapshots\Smash64DS_Port_Lean_20260719_053053.zip`
is the recovery surface. No worktree, branch, ref, Git object, log entry,
permanent evidence file, canonical build, active hardware pair, current
generated A/B, or user-owned file was removed.

`artifacts/performance` stays exactly 329 files / 91,169,794 bytes with digest
`25B1BD9A...`, and `artifacts/visibility` stays exactly 1,577 files /
209,195,688 bytes with digest `586ADF5B...` across the deletion itself. The
post-delete Boundary gate then adds its normal two dated captures. The new
fail-closed manifest checker verifies 1,745 immutable destinations and skips
only the two intentional rolling aliases. All 15 held worktrees and all 24
branches remain because their dirty/unique/detached state is not safely
discardable. Focused parsers, GBI fixtures, melonDS policy, the migration
manifest, and final profile-0 Boundary pass. Full inventory:
`artifacts/performance/2026-07-19_task24r-quiet-slot-cleanup.md`.

## Phase Evidence (historical pre-Task-25R)

Every timed row below came from one profile-1 ROM with live Fox, production
incremental wallpaper, and eight samples. It repaired the older mixed-ROM
comparison, but it is not Task 25R: it lacks the matching profile-0 sibling,
maximums, complete owner rows, interval histogram, stable-30 deficit, and
artifact/code-layout pack. Values are retained as historical P50/P95 ARM9
ticks; `UPD` is the source-update owner, and residuals are draw/present/loop.

Same artifact: profile-1 ROM
`FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144`,
ELF `A1811E43CFF97DC57EC1DB19DCE89CE4E0E36C50900E83857A371B0340F2CF68`.

| Match phase | ROM SHA-256 | Profile frames / source ticks | N | UPD | Draw | Stage | Mario | Fox | Wallpaper | Active | Loop | Residual D/P/L | State |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| Countdown / GO | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 438–445 / 484–498 | 8 | 199,776/200,896 | 1,148,416/1,216,576 | 465,984/466,112 | 169,440/171,200 | 208,960/209,344 | 286,112/351,296 | 1,153,248/1,221,376 | 1,680,448/2,240,640 | 19,616/19,968; 1,408/1,536; 1,600/1,728 | Exact natural countdown window |
| Early combat | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 600–607 / 808–822 | 8 | 144,480/145,920 | 1,063,232/1,085,824 | 466,112/466,240 | 9,920/9,984 | 196,800/196,928 | 340,544/362,880 | 1,068,032/1,090,624 | 1,680,448/1,680,512 | 50,048/50,304; 1,376/1,536; 1,600/1,728 | Mario naturally off-screen; fast owners remain exact |
| Mid combat / Whispy | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 1398–1405 / 2404–2418 | 8 | 214,944/814,080 | 1,183,328/1,221,312 | 466,016/466,048 | 169,504/171,264 | 209,184/209,728 | 320,416/358,592 | 1,188,160/1,226,112 | 1,680,448/2,240,640 | 19,520/19,776; 1,376/1,536; 1,536/1,664 | One live texture conversion/decode/alloc/create/upload/fallback reopens the lifecycle fence |
| Natural KO | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 566–573 / 740–754 | 8 | 194,336/222,912 | 799,872/1,053,248 | 470,016/470,144 | 7,584/7,616 | 197,568/197,632 | 73,888/327,360 | 804,672/1,058,048 | 1,120,256/1,680,512 | 50,720/50,880; 1,408/1,408; 1,568/1,664 | Exact source-event gate |
| Natural rebirth | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 589–596 / 786–800 | 8 | 149,120/824,640 | 1,042,880/1,046,592 | 465,504/466,112 | 9,920/9,984 | 197,504/197,632 | 319,904/323,968 | 1,048,864/1,303,360 | 1,680,448/2,240,640 | 49,920/50,048; 1,408/1,472; 1,600/1,728 | Exact source-event gate; update P95 spike retained |
| Late combat | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 1846–1853 / 3300–3314 | 8 | 197,696/209,728 | 864,864/867,328 | 466,048/466,176 | 169,568/171,328 | 208,384/208,768 | 2,400/2,432 | 869,696/872,192 | 1,120,256/1,680,448 | 19,520/20,736; 1,408/1,536; 1,600/1,600 | Source-tick gate; no stale presentation-frame assumption |
| Time-Up boundary | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 1988–1995 / 3584–3598 | 8 | 197,568/617,920 | 865,728/896,064 | 465,984/466,112 | 169,504/171,200 | 208,096/210,112 | 2,400/2,432 | 870,528/900,864 | 1,120,256/1,680,448 | 19,584/49,600; 1,408/1,472; 1,600/1,600 | Final battle owner window; Results exposes no equivalent owner timers |
| Expiry → Results | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | full one-minute lifecycle | 1 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | Transition/teardown and 4,084/2,042 pacing pass; reserve gate fails at 117,520 net bytes versus 131,072 |

Profile-1 hard-checkpoint windows are O2-equivalent feasibility evidence, not
profile-0 release baselines. M2 detail and profile-2 forensic samples stay in
`PERF_LEDGER.md`.

## Reconciled stable-30 queue

The old Task 25 matrix above remains historical. Task 25R now supplies the
authoritative pair and selects the implementation order:

1. **M3 lane complete at its current expansion gate:** Task 23R Phase 0,
   Task 26 segment 0, and Task 23R residual Phase 1 are closed. The exact Task
   26 slice remains banked; the residual cache is reverted.
2. **M2 lane complete at its expansion gate:** Task 21R and Task 27 are
   closed. Task 27 retains its exact Phase-A certificate only; the Mario
   executor regressed complete draw and stopped before Fox.
3. **Disjoint lane complete:** Tasks 20R and 22R are closed without retained
   DTCM/span-writer placement.
4. **Exact-kernel lane closed at its evidence gate:** Task 28 tested one
   bounded matrix leaf, proved its host arithmetic and desired ARMv5TE shape,
   then reverted it because the runner never produced the required natural,
   ARM9-golden, state/pixel, or retail evidence.
5. **GX lane closed at its evidence gate:** Task 29 retains its exact
   no-behavior census only. No suppression or immutable stream clears the
   retail/repeatability and first-run gates.
6. **Quiet-slot cleanup complete:** Task 24R reclaims 3,746,285,595 bytes
   without changing permanent evidence, held Git surfaces, or current A/Bs.
7. **Selected next lane:** reconcile the attack/hit A/V asset audit already in
   the board before Task 30.
8. Task 30 is the final stable-30 gate.

`src/nds/nds_renderer.c` is a mandatory one-writer surface throughout.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | Task 24R closed / A/V audit next | live tree | Retained Task-26 segment 0, Task-27 Phase-A fighter certificate, rejected performance candidates, integrated battle ROM, permanent evidence, and current terminal lifecycle | no runner active; attack/hit asset ownership is the next existing red release row |
| Renderer implementation | ARM restored; Task 12 closed REVERT; Task 14 KEEP | shared integration tree | Retail hardware rejects blanket Thumb by +594,816 DRW ticks (+34.1%); its renderer hot group is device noise and reverts with that base. The renderer TU is ARM again with Task 14 intact | no runner active |
| Gameplay + QA | Playtest review fixed / manual candidate retest pending | shared live tree / disjoint files | Down+A is source-fixed at the shared ClearAll seam. Human-P2 Fox completes all nine Down-Air callbacks and exits naturally; Mario completes the same focused route with eight live imported CPU updates. Latest then passes canonical mode 163, two-ROM publication, runtime, registry, renderer/ITCM, and visual gates. Countdown also remains fixed | no runner active |
| Performance research | Measured cuts accumulate | shared live tree / read-only | Milestone targets no longer discard smaller correct gains; measured regressions and invalid visual packets remain rejected | no runner active |

Runner/window/port policy lives in `VERIFYING.md`; scripted launches normalize
the selected TOML and never touch the user's manual instance.
Workers edit only explicitly assigned disjoint surfaces; the integration lane
owns current-truth docs, shared-file arbitration, commits, and publication.


## Acceptance Matrix

| Acceptance condition | State | Lane | Blocker / evidence | Integration decision |
|---|---|---|---|---|
| Mario human versus original level-3 Fox CPU, Dream Land, one-minute Time match, items off | CPU-on public default / visible fast-iteration override | Gameplay + QA | The published ROM and source runtime gates retain flag `1`; the separate `-FastIteration` screenshot launch selects flag `0` before battle, skipping countdown/Fox and freezing `1:00` | Keep lifecycle gates on `1`; use the focused flag-`0` capture during routine visual iteration |
| Original Wait → countdown → GO control/timer gate | Pass | Integration | Automated synchronized source-state proof passes | Keep |
| Locked-30 scheduler | Pass / slowdown published | Integration | Task 9 natural match: 4,084 updates / 2,042 presents, exact fixed-two pacing; phase rates 39.9/37.4/39.3/n.a./58.2 updates/s and slips 196/1088/946/0/3. Latest profile-0 smoke is 19.7 FPS. Focused natural KO/rebirth active P95 is 1,524,864/1,112,256 ticks | Keep exact 2:1 and conditional 59.0–61.0 gates; full-speed locked 30 remains unmet |
| Mario can damage Fox | Pass / continuous gate | Gameplay | User confirmed the repair manually. The focused current-ROM route now damages Fox 0→59, lets imported level-3 Recover return naturally to line 3, then damages Fox again 59→72 within 78 frames while all 11 damage colliders and global/special/star hit statuses remain normal | Keep the post-Recover assertion in `verify-battle-playable-fox-recovery.ps1` |
| Fireball spawn/render/damage/destruction | Pass | Gameplay | Source spawn/damage/lifetime and 40 moving visible textured hardware draws pass with zero rejects | Keep dedicated natural-input gate |
| Common visual effects | Natural common-hit pass / broader attack-hit audit open | Renderer + Gameplay | A natural mode-163 Mario contact reached BattleShip kick ID 32 at frame 125. Its following source hitlog selected DS `HitNormal` kind 1, created the normal/orb/spark trio with zero drops, and submitted 3/3 effects / 24 triangles with zero texture or draw rejects. Dated Dream Land capture `2026-07-17_220144-9409465_fox-recovery.png` passes nonblank green/detail/single-color gates. Fireball's separate focused route remains green | Keep the exact-event counter and image-content gate. Continue tracing the remaining reported attack visuals; common particle banks remain unresolved and this row is not globally closed |
| Fireball trajectory, floor rebound, and terminal | Pass | Gameplay | Current custom `0x47` submits 40/40 and first rebound is 55→46.75 at lifetime 122. The exact ELF destroy callsite then records the same weapon crossing Pupupu bottom −3500 at x/y/z −4650.5/−3504.8/−66.9 with lifetime 10; it is absent next frame. The deterministic source-MVP ROI changes 50→0 orange pixels | Keep the focused countdown/Fox-off gate and both `20260716_fireball-source-mvp-long-travel*.png` captures; the earlier far-left theory was false |
| Damage/throw map collision | P1 Dream Land boundary pass | Gameplay | Five source up-smashes proved DamageFall recovery. A separate external-input route then used nine short source Walk/Dash/Run steps for one Mario catch/forward throw: Fox took 0→12%, released status 169→186, swept/clamped to line 3, DownBounced once, cleared every catch link, and retained 202,256 bytes. Source Dream Land contains one static collision group: 4 floors, 1 ceiling, 1 right wall, and 1 left wall | Keep both sparse gates; throw evidence is `2026-07-16_063512-7696185_throw-release-recovery-p21764.png`. Moving-stage/project providers are P2 and must not delay this static-stage P1 |
| One-way platform semantics | Pass / hardened natural gate | Gameplay | Current-ROM mode 163 completed 715 natural frames: six ordered continued-ascent/strict-descent/downward-crossing flights (`0x3f`), all three platform masks `0x7`, two side cycles, three exact ignore-line Pass crossings, nine landings, and 214,544-byte reserve | Keep the focused gate; screenshot `2026-07-16_052652-7356809_platform-semantics-p984.png` |
| Edge behavior and specials | Retest | Gameplay | Up-B was manually accepted; other edge behavior needs current-ROM qualification | No unrelated behavior change without reproduction |
| Down+A CPU-stall report | FIXED / source-exact shared clear + three runtime gates | Gameplay + QA | BattleShip ClearAll preserves reusable collider payload; the port shim incorrectly erased it before Fox's scripted one-tick-later Refresh. The shim now clears only `attack_state` plus `is_attack_active`, and the existing damage-common probe requires seeded damage 7 to survive. Human-P2 Fox completes 9 callbacks and exits to status 31 with logic/cpu/reads 8/0/12, 116,992 update ticks, and 205,744-byte reserve. Mario exits to status 26 with 8/8/12, 134,784 ticks, and 203,536-byte reserve. Both routes load their exact `0x303`/`0x272` source assets. CPU-on Current passes in 661.5 seconds with the two-ROM contract and mode-163 CPU setup/proc/target 1/33/33 | Keep `verify-battle-playable-down-air-stall.ps1` for both actors and the corrected payload-preservation assertion. Do not restore the compatibility shim's non-source field clearing |
| Normal-play stage painter/depth order | FIXED / pixel + profile-2 pass | Renderer + QA | BattleShip layer modes classify 66 source-Z and 126 no-Z triangles; one full v16 step per no-Z triangle removes the grass/bush overlap, preserves 202 triangles, and reserves disjoint endpoint bands | Keep as correctness fix; final frame 438/501 captures and zero-collision trace are authoritative |
| Pause-orbit geometry containment | FIXED / user confirmed | Renderer + QA | Clip-space near-plane containment removes screen-blocking triangles at the breaking orbit angles without changing normal-play profile-2 output | Keep focused angle gate; paused −33.6° is also the strongest Mario underside view |
| Mario pant/underside visual | FIXED / user confirmed | Renderer + QA | Source root light preambles were missing; replay restores blue right pant and closed underside with unchanged 320-triangle census | Tyler accepted `20260716-034036_slot3_p10612_mode163_camera_pause_minus33p6.png` on 2026-07-16 |
| Natural Fox recovery | Pass | Gameplay | Current-ROM mode 163 used only external Mario input: Fox took 0→59 damage, selected BattleShip Recover for 40 frames at offstage x=2379.905, grounded on line 3 at x=1336.084, and took a later hit to 72 without KO/rebirth in 897 frames | Keep focused gate; reserve 202,256 and screenshot `20260716_fox-recovery-post-hit.png` |
| Cut G M1 affine BG2, 5–35K ticks | Pass | Renderer | 1,856/1,856 ticks; exact frames 438/439 pass and publish | Keep canonical |
| M2 Mario/Fox AOT, 170–250K ticks | Incremental compute KEEP at 385.3K | Renderer | Current ledger-off Mode 8 is 385,312/388,480 after exact light-state restoration. The raw texture-class split moves the synchronized current-build combined fighter 386,624/389,824 → 385,312/388,480, draw 1,011,648/1,014,976 → 1,009,824/1,013,120, and active 1,015,680/1,018,880 → 1,014,048/1,017,088. The corpus proves 43 untextured and 11 textured raw calls, so the untextured callee removes 172 main-RAM stack word transfers per frame. Exact 70/686, 60/320/306/29/0/0, all sampled non-timing fields, and 0/49,152 changed pixels remain | Keep the scalar reset, raw-corner representation, and two raw texture-class callees. ITCM is 28,052/32,768; full inlining, tail dispatch, and shared camera hoist remain rejected. The 170–250K milestone is still red and directional |
| Mode-8 generic/fast forensic parity | PASS / source-exact light state | Renderer | F3DEX2 `G_MOVEWORD` decodes/emits index bits 16..23 and offset bits 0..15. The exact O2R census is 148 fighter `G_MW_LIGHTCOL` commands: 120 compact root preambles plus 28 intra-root changes carried by epoch state spans. Current profile-2 ROM `75A731E1...` frames 180–187 has 0 semantic, owner, or geometry mismatches and retains 686 triangles in both arms | Keep the exact 120/28 representation and validator gate. Evidence: `2026-07-17_m2-raw-texture-split-{generic,fast}-profile2.json` |
| M3 complete stage AOT, 150–250K ticks | Task 26 segment 0 KEEP / broad expansion STOP | Renderer | The fixed generated layer0 program covers 21 DObjs, 20 bindings, 26 runs, 54 triangles, and 22 epochs. Current/generated traces match 2,775 words over eight frames, the live-fault proof is `1/1/1`, and pixels are 0/49,152. Five Task-25R-control phases save 3,424–3,616 stage P50 ticks; the current hardware-style pair saves 10,240/10,368 stage P50/P95 and 11,232/11,200 draw. One retail sample saves DRW 21,568 while ACT rises 4,288 and LOOP stays in the same VBlank bucket | Bank this exact segment beside the retained Task-6 cuts. Do not claim retail repeatability or widen the generated working set without device falsification. Task 23R Phase 1 may measure only residual work and must satisfy its 20% hit/cost gate |
| Task 8 CUT A constant-depth GX painter | PASS / already canonical | Renderer | The requested mechanism had already landed at the pause/depth root-cause fix: each no-Z band loads a matrix whose Z row is the exact band constant times W, so the GX divider produces constant post-divide depth without the retired CPU `div64` projector. The subsequent exact AOT-shift and zero-shift specializations saved another 22,016/22,208 and 14,304/14,080 stage ticks | Reconcile as completed; do not add a second constant-depth path. Retain the depth trace, 121/828 census, zero fallback/fence/conservation, and pixel gates |
| Task 9 bit-exact soft-float | PASS / Phase 1 16.54%, Phase 2 8.86% owner gains | Renderer + Gameplay | Phase 1 keeps six byte-identical GCC 15.2.0 objects (1,952 ITCM bytes). Phase 2 replaces only the 556.25-calls/update `__aeabi_fcmpeq` with a 36-byte, nine-instruction ARM leaf: source-update 236,640/238,016 → 215,680/217,024 P50/P95. Host proof covers 16,777,216 directed plus 100,000,000 deterministic random pairs; the standalone ARM9 ROM covers 2,916 directed pairs; 3,892 full-state rows match exactly | Keep both phases. ITCM is 28,088/32,768 with 4,680 bytes free. The renamed stock helper remains the linked golden only in Phase 2; Phase 1 proves it absent. Main-RAM coroutine stack finding remains report-only |
| Task 12 renderer code placement | CLOSED / Phase A REVERT; Phase B REVERT WITH BASE | Renderer | Device ARM→Thumb moves DRW 1,743,296→2,338,112 (+594,816/+34.1%), LOOP 2,240,384→2,800,640, and FPS 13.5→10.6. The Thumb hot group then changes DRW only -5,440 (-0.23%) while ACT/LOOP worsen slightly: noise | Keep mode-163 renderer ARM. Retain the safe placement mechanism, not either rejected Task 12 candidate; melonDS cannot referee cache placement |
| Task 13 fighter decimation pack | CLOSED / REVERT | Renderer + Assets | The deterministic 17/32-part pack reduced natural Mario/Fox fighter triangles 626→402. Even mask 0 paid +5,120 draw ticks; the best mask `0x89` removed 64 triangles but regressed draw +3,168 and active +3,328, and the final cold-placement/Fox-skip salvage regressed every paired frame by 3,136–3,264 ticks. Runtime, tools, and derived assets were removed | Do not revive this hash lookup/ITCM design. The performance gate was decisive before visual sign-off or any default-on discussion |
| Task 14 dense-preparation reuse | KEEP / exact | Renderer | A 312-entry generation-gated first-visit plan removes the per-frame 606-corner mask walk while leaving matrices, materials, texture selection, alpha, color, UV, and near-plane work live. Stage is 904,928/905,088 → 895,872/896,000 (-9,056/-9,088); draw is -9,280/-8,704 and active is -9,248/-8,704. All eight paired stage frames improve, native pixels are 0/49,152, production is full1/hit437→444 with zero mismatch/inject/revalidate, and the fault lab is full2/hit436→443/mismatch1/inject1/revalidate1 | Bank the 736-byte BSS / zero-ITCM cut. Keep the exact 55-offset/312-permutation/uniform-alpha host contract and fail-closed publication checker; all live prepared attributes remain per-frame |
| Task 16 extended bit-exact soft-float | PASS / compare, i2f, add/sub shipped; fmul REVERT | Gameplay + Toolchain | The three exact candidates coexist in one link and move source-update 215,104/216,512 → 200,000/201,088 P50/P95 (-15,104/-15,424; -7.02% median). Combined ITCM is 28,820/32,768 with 3,948 bytes free and zero fill; all 3,892/3,892 six-field state rows match with zero overflow. The exact-zero fmul wrapper passed 100M host and 1,050,880 literal ARM9 pairs but regressed its representative microbench 5.34% and natural source-update 1.64%, so all fmul code/tools/link changes were removed. Canonical Boundary passed with published Task16 1/1/1 and ROM `DADB7C96...` | Keep global selectors default OFF for exact lab controls and the published/freeze-equivalent overrides at 1/1/1. Keep stock 408-byte `__aeabi_fmul`; the authoritative combined verifier always rebuilds both sides and binds build, ROM, ELF, and all six row fields |
| Task 17 update hot-text round 2 | KEEP CONFIRMED / update-only on ARM | Gameplay + Toolchain | The original Thumb-base device pair moves UPD 386,240→342,080 (-44,160/-11.4%). The decisive ARM-base pair narrows that claim: UPD 366,016→363,456 (-2,560/-0.70%) and DRW -2,688 cross LOOP 2,800,832→2,240,448 (five→four VBlanks) and FPS 13.9→14.3, while ACT rises 15,872. Exact retained surface remains 11 functions / 5,016 bytes under an 8 KiB ASSERT; Task 14 and Task 16 1/1/1 remain intact | Keep the threshold win and both device photos. Do not claim -44K on ARM or infer cache placement from melonDS; continue phase-wide performance qualification normally |
| Task 18 KO wallpaper spike | CLOSED / bad baseline | Renderer + Platform | Production incremental mode measures KO wallpaper at 302,880/357,824 versus steady 292,224/360,000: +10,656/-2,176, not the cited 547,584/547,648 full-raster control. No runtime change is justified | Keep incremental mode and the affine lab retired. The Task 18 ledger row is the durable correction |
| Pre-25R same-artifact phase matrix | HISTORICAL / replaced | Integration | Profile-1 ROM `FB0704BA...` remains valid historical evidence but lacks the Task 25R sibling, maximums, full rows, interval histogram, and identity packet | Preserve as history; current estimates and priority come only from Task 25R ROM `6E90D414...` |
| M4 zero gameplay conversion/preparation | PASS / exact complete lifecycle | Renderer | The generated corpus now includes naturally observed Whispy mouth asset 152 and late Fox material asset 313: 24 keys, 23 outputs, 132,096 payload bytes, and 136,192 prepared bytes in VRAM A+B. The checker reports hits/outputs 24/23, classified/explicit misses 1,344/3, and six invalid-key falsifiers. The refreshed one-minute lifecycle has all ten post-GO conversion/upload/fallback/fence counts zero and one teardown | Keep the exact corpus and fail-closed key checks. No primary-image approximation remains; reserve is requalified after the 5,120-byte payload increase |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | FIXED / full runtime gate pass | Renderer + QA | The source-backed point sample changes 49 atlas pixels only inside the 12x9 `ShadowGo`; all five GO frames retain its 70-pixel count plus 10/10/10 draws/queued/emitted, 57,344 texture bytes, 608 palette bytes, and zero hot conversion/upload. The large-GO mismatch was a stale crop lock after the source-light repair: only 125/26,400 pixels changed, all inside Mario's 22x14 area, while the GO RGB555 payload remained `05330f47...`. The rebuilt full verifier passes with crop `d968b0cc...`, GO `3 OBJ + 10 quads`, and 31,168 OBJ bytes | Keep both accepted crop locks and the source-derived DS assets; no GO source change was warranted |
| Dream Land BGM | Pass | Audio | Tyler reports the stage theme sounds normal. The exact source-derived initial 65,536-byte DS ring has peak 9,928 / RMS 2,283.623; the natural public-ROM recovery route observes the live BGM channel bit in Calico's ARM7-shared mask with clean 44.1 KB/s streaming and zero I/O/unsafe/overrun faults | Keep; repeat only in final lifecycle qualification |
| Required FGM, attack/hit sounds, and Mario/Fox voices | Common contacts and KO lifecycle PASS / residual cue debt | Audio + Gameplay | The 121,720-byte pack retains the exact common punch/kick paths and natural `439/292/154` Mario-KO sequence. Task 26 fixed a Calico/software-owner race: an exactly completed hardware channel now retires its stale owner before reuse, while inconsistent ownership remains fail-closed. The focused automated qualification passes 21 supported starts, 17 explicit unsupported cues, phase mask `0x1f`, channel mask `0xe`, max three live channels, 59 envelope steps, zero playback/generation failure, and 187,152-byte headroom | Keep the exact retirement seam and automated channel/acoustic gate. This is not an ear test. Residual unsupported special/projectile/movement cues remain explicit fidelity debt for the existing A/V audit before Task 30; do not require another user response tonight |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 172,024 | Keep gate |
| Stable reserve / no corruption | PASS / current Task 26 artifact | QA | The refreshed profile-0 lifecycle reaches Results once with 232,208 arena headroom and 65,536 resident audio bytes: 166,672 net versus the 131,072-byte floor after the exact 5,120-byte static-texture payload increase. Exact 4,084/2,042 pacing and teardown pass; stale/safety/eviction counters stay zero | Preserve the floor and recheck after every representation or resident-asset change |
| Focused/checkpoint verification | Task 24R complete / A/V audit queued | QA | The quiet-slot batch reclaims 3.49 GiB while protected evidence digests and 1,745 immutable migration destinations remain exact. All held worktrees/branches and current A/Bs survive; focused checks and post-delete Boundary pass | Commit and Lean-snapshot Task 24R, then reconcile the existing attack/hit audio-visual cue audit before Task 30 |
| Cut G capture / final dated capture / manual retest | Automated exactness pass / manual current-ROM retest pending | QA + user | Latest capture is `2026-07-18_canonical_fast_101715-1383371-p57268.png`; its paired frame has 747/49,152 meaningful changes, 100% overlap, and all named-region/detail gates pass. Task 6 C/D, Task 8 G2, reserve repair, Task 9/16 state identity, source-light parity, both emitter splits, raw-corner cut, Down+A, and the common-contact checkpoint remain retained | Manually retest exact ROM `DADB7C96...`; automated common-contact A/V closure is focused, not global |

## Dated Gates

### July 16 — Hard Feasibility Checkpoint

- Publish phase-specific P95 active ticks for countdown, early/late combat,
  KO/rebirth, and Results.
- A credible 60 FPS path must converge near one VBlank (~560K ticks, preferably
  ≤520K with margin) in every material phase, including interface work.
- If evidence does not support that path, present an explicit decision: keep
  pursuing 60 FPS without claiming P1 complete; request approval for a lower
  stable presentation target; or deliver the best verifier-covered incomplete
  candidate. Do not silently redefine P1.

Checkpoint decision: presentation targets locked 30; 60 FPS is not claimed.
The scheduler uses exactly two source updates per present and never catches up
after a slip. This matches Smash 64's uniform slowdown under load, preserves
real-time hardware-paced audio, avoids 2/3-tick motion judder, and self-recovers
after one slow frame. Old one-update-per-present and debt/cap-4 samples are not
comparable; all phase baselines require fixed-two resampling. The natural
one-minute run closes the Results lifecycle gap. Its older pacing classifier
watched proof-route counters and therefore reported zero KO/rebirth presents
despite a real KO FGM triplet. The focused profile-1 sampler now gates on
BattleShip's natural KO trace and `ftCommonRebirthDownSetStatus`: KO frames
708–715 are the worse 1,261,344/1,524,864-tick active window, while rebirth
frames 730–737 measure 1,110,528/1,112,256. This closes the missing phase
baseline, not the full-speed requirement.

### July 17

- Architecture and integration freeze. Merge only measured candidates and
  confirmed blocker fixes. Run Latest instead of Boundary when shared runtime
  or normal launch changes.

### July 18

- Release qualification: one-minute soak, focused checks, one Boundary or
  Latest run, clean canonical rebuild/parity, exact-ROM manual retest, and
  dated capture set.
- Release-candidate fixes and verification only. No new architecture and no P2
  work. Report every remaining red acceptance row honestly.

### July 19
- Get ready for public Github release
- Public repo must be able to build current nds rom with legal user supplied n64 ROM with one .ps1 script.


## Integration Rule

Performance iteration is eight synchronized A frames and eight B frames with
ticks, FPS, screenshots/image analysis, and cheap correctness counters. Add A2
only when inconclusive. Compilation alone never closes a row. P2 begins only
after every required P1 row is green, documented, and snapshotted.

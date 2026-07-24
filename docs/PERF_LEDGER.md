# Smash64DS Performance Ledger

Do not delete reverted or inconclusive rows. Measurements use synchronized
canonical mode 163 unless a row explicitly says otherwise. Nested renderer
timers are diagnostic subdivisions and are never added to the whole-loop sum.

**Emulator change, 2026-07-22.** Every row dated before 2026-07-22 was measured
on a stock melonDS that does not model ARMv5 icache/dcache. The repo emulator is
now the owner's cache-modelling fork, which reports roughly 40% more ticks for
the same work. Old and new tick counts are NOT comparable in absolute terms;
within-row A/B deltas measured on one emulator remain valid. See
`TICK-HUD BASELINE / FORK EMULATOR` at the end of this file.

**Consequence: `verify-dev-fast.ps1` is red on the fork.** The `battle_playable`
locked-30 contract (exact 2:1 update/present, 30 Hz present cap) fails under the
fork and passed under stock melonDS. Confirmed to be the emulator and not a
source change by stashing all local edits and reproducing on clean `093690b`.
The contract is not being met because it was never met on hardware either - the
retail and fork VBI histograms both show 3/4/5+ VBlank intervals, not a locked
30 Hz - so stock melonDS was flattering the pacing and the contract encoded that
artefact. Decide whether to re-baseline the contract against the fork or gate it
by emulator; do not "fix" it by reverting the emulator.

## Benchmark identities

Both accepted runs use the canonical idle `battle_playable_realtime` mode-163
scene on worktree `f37fd14be` plus the uncommitted profiler sprint.

```text
T0 target/profile:        smash64ds-battle-playable-coarse-hwtri / profile 1
T0 final flags:           common/scene -O2 -mthumb; renderer -O2 -marm
T0 ROM:                   1AF1F28BC24488534661B5060465597B8C20F901A5607837B546C2EC25D56F4F
T0 ROM bytes:             11,672,576
T0 ELF:                   262ABE9656E8209708205BB6FB73C55FEFB64FD1114DE150D68646C70C8B980B
T0 ELF bytes:             8,186,872
T0 sampling:              warm-up 186; frames 187..314; logic 194..321; n=128

T0C target/profile:       smash64ds-battle-playable-canonical-hwtri / profile 0
T0C ROM / ELF:            AD267DD8C2260CC86CC210680BF4FB6A640F7D4A5552397B2E44B4284DDBD25F /
                          27F02F2E9F26E1ADE087EAE21985C20F9A9C9FEB2D0F8C5062DEB9CB3A37978B
T0C sampling:             warm-up 298; frames 299..330; n=32; delay 12 s
T0C sections (bytes):     ITCM 20,916; DTCM/DTCM-BSS 0/152;
                          main/main-RW/main-BSS 623,536/124,044/1,857,584

T0 sections (bytes):      ITCM 20,916; DTCM/DTCM-BSS 0/152;
                          main/main-RW/main-BSS 625,168/124,044/1,858,096

T1A target/profile:       smash64ds-battle-playable-forensic-hwtri / profile 2
T1A final flags:          common/scene -Os -mthumb; renderer -Os -marm
T1A ROM:                  142EFC9F7D61FE32C2332EAE6B07435FCC653AF30C676665F81E18D21720DC82
T1A ROM bytes:            11,614,208
T1A ELF:                  27B32B4D950382F20CE51D8B1D066B71E058EFCE9F23B2D5CA0C09741D0026AD
T1A ELF bytes:            7,771,668
T1A sampling:             warm-up 44; frames 45..172; logic 52..179; n=128
T1A sections (bytes):     ITCM 19,872; DTCM/DTCM-BSS 0/152;
                          main/main-RW/main-BSS 567,760/123,908/1,949,616

melonDS:                  1.1; JIT false; LimitFPS false
Input/state:              canonical idle realtime BattleShip scene
Reference capture:        artifacts/visibility/2026-07-12_canonical_fast_121423-0241726-p3736.png
```

## 2026-07-20 - Task 30 BGM refill slicing — REVERT

```text
IDEA ID: TASK30-BGM-REFILL-SLICING-20260720
DECISION: REVERT; refill spike dropped but retail pacing distribution regressed
CONTROL/CANDIDATE ROM:
  00BC098C91722A7A63B239C96B1A5FFC70F5165DF7149DC3ADECC12479231319
  393B63A69C3B653C86AF93D74AD3AE33BC19F7AA3D2A5C0CA00CA462A88AC157
CORRECTED CLEAN-HUD DEVICE ROM:
  FFC2FEA839566D0ACB93B8BF700A69B4F9079ED4B50965DE9550947FB93E6B51
  EB01127F9AA5EE997AD2143C370002C6B113D7A1FE312A12564595A7D60053F5
WINDOW: profile 1, affine 0, mode 9, Fox active, frames 600..607, n=8
REFILL LAST/MAX: 76,160 / 76,224 ticks
SLICES/HALVES/SAFETY: 160 / 40 / unsafe-overrun-readfail 0-0-0
AUDIO SHELL P50/P95: 2,368/268,672 -> 3,040/156,160
COMPLETE UPDATE P50/P95: 156,800/423,104 -> 157,536/311,424
SOURCE UPDATE P50/P95: 153,952/155,584 -> 154,048/155,712
DRAW P50/P95: 1,068,544/1,091,200 -> 1,071,392/1,094,144
LOOP P50/P95: 1,680,448/1,680,512 -> identical
PIXELS: synchronized frame 607 = 0/49,152, mean 0.00
LIFECYCLE: DevFast PASS; Boundary PASS; one-minute PASS; Results audio PASS
RESULTS AUDIO: Fox 16 -> Results 22, 95 refills, 44,040/44,100 B/s,
  errors/unsafe/overrun/cleanup 0, reserve after ring 462,160 bytes
RETAIL CONTROL: VBI 2/3/4/5+=0/95/321/88, N=504, 4+=81.15%, max=17
RETAIL SLICED: VBI 2/3/4/5+=0/74/336/90, N=500, 4+=85.20%, max=16,
  BGM slices=180
RETAIL DELTA: normalized 4+ +4.05 pp; 5+ +0.54 pp; REVERT
```

The candidate read and flushed one 8 KiB slice per audio update; only the
two-frame deadline guard finished the remaining slices together. Preload and
rare resync remained whole. Full evidence and final retail decision:
`artifacts/performance/2026-07-20_task30-bgm-refill-slicing.md`.

the owner reported on 2026-07-20 that the sliced profile-1 ROM audio sounds good.
Task-32 retail photos first proved sliced-path engagement with `BGM slices`
counts 180 and 184. The corrected dedicated pair then failed the pacing gate:
4+ and 5+ both shifted worse after normalization. The original copied device
pair exposed the generic debug wall and was removed; the clean pair and final
retail photos are permanent evidence. Whole-half refill is restored.

## 2026-07-20 - Task 31 coroutine-stack census

```text
IDEA ID: TASK31-DTCM-GAMEPLAY-STACK-20260720
DECISION: STOP AT MANDATORY CENSUS; NO DTCM PLACEMENT
WINDOW: profile 1, affine 0, mode 9, frames 600..607
CENSUS: 7 rows, 5 live, peak 6, overflow 0
LARGE 16-KIB STACKS: 5 live, peak 6
GAMEPLAY WATERMARK OWNER: thread ID 5
DTCM LAYOUT: .dtcm/.dtcm.bss 0/152; shared gap 15,848 bytes
ROM / ELF:
  874E9CB9C1CAEFB89A88D405514BAD3C9FED6F260F55566433FF5EB229C4666F
  0834451F31AD98181C9A35AED489A5664442F881C5077E492EC8971C928C0040
```

Task 31 required exactly one large gameplay-class coroutine before sizing or
placement. Ordinary combat instead retains IDs 1, 3, 4, 5, and 6 as concurrent
16 KiB allocations, with six large stacks at peak. The profile-only lifetime
census is retained; no stack was resized or moved and no device A/B is needed
for an unimplemented feature. Full evidence:
`artifacts/performance/2026-07-20_task31-coroutine-census.md`.

## 2026-07-20 - Task 32 draw-path hot-text retail KEEP

```text
IDEA ID: TASK32-DRAW-HOT-TEXT-20260720
DECISION: KEEP; PUBLISHED TARGET ENABLED AFTER RETAIL HISTOGRAM WIN
PC CENSUS: 2 x 450 samples, frames 600..1498, 30 s source time each
SELECTED: 13 functions; 382/900 combined samples
SECTION: profile1 8,168 bytes; profile0 8,060 bytes; limit 8,192
TASK17: unchanged 5,016 bytes; __main_start unchanged
MELONDS M3 P50/P95: 461,088/461,376 -> 465,344/465,728
MELONDS DRW P50/P95: 857,312/860,736 -> 861,760/865,024
LOOP P50/P95: 1,120,256/1,120,256 -> 1,120,256/1,120,320
PIXELS: synchronized frame 607 = 0/49,152, mean 0.00
SEMANTIC / GX TRACE: exact
DEVICE ROMS:
  control   28CCE18784D8AA413C2E58A9811547258A905C06DCFFCF5C39455BDCCF6D17EC
  candidate 69B0050E6CECBBBA78FDFC43AF0945A0549049380349C69824623D976C914016
RETAIL CONTROL DHT0: DRW 1,677,760; VBI 2/3/4/5+ 0/186/347/67; max 7
RETAIL CANDIDATE DHT1: DRW 1,658,560; VBI 2/3/4/5+ 6/187/331/60; max 7
NORMALIZED 4+: 69.00% -> 66.95%; 5+: 11.17% -> 10.27%
PROMOTED ROM: B73D9BDBF36C780C44F4898213A069FFF250716F2B77C6773C22DA28B8BB98D2
VERIFY: focused placement PASS; DevFast PASS; Boundary PASS
```

The linker rejected the first 8,292-byte profile-1 set. Removing only the
124-byte, once-per-frame `gcDrawAll` entry leaves every sampled stage leaf and
fits both profiles. Retail hardware shifts the normalized histogram toward
shorter intervals without increasing the maximum, satisfying the task's KEEP
rule. The generic default stays off while published/release-equivalent targets
force the feature on. Full evidence:
`artifacts/performance/2026-07-20_task32-draw-hot-text.md`.

## 2026-07-20 - Task 34 E1 native-stage stream certificate

```text
IDEA ID: TASK34-IMMUTABLE-STAGE-STREAM-E1-20260720
DECISION: STOP_BELOW_60_PERCENT; E2/E3 NOT AUTHORIZED
WINDOWS: 438..445, 600..607, 1398..1405; 24 synchronized frames
STREAM/FRAME: 2,557 commands; 6,894 parameter words; 30,136 serialized bytes
DOBJ PARTITION: bit-identical 0; varying 42; no-stream 15
IDENTICAL SHARE: 0 / 6,894 words = 0.000% (gate approximately 60%)
FAULTS/OVERFLOW: 0 / 0 in every frame
ROM / ELF:
  F64EA4E6EC15334DD730FF75A92D8CAB4E7BE0E90FA42E615328ABF8A8F130A2
  E698CE3A708E2A278F3CD45E90A659A66128207499725E7C5CFB37D9E7BAECD4
```

Every display-bearing DObj emits live matrix operands. This matches Task 23R's
classification of `projection` and all 42 `binding_composed` matrices as
camera-dependent, and falsifies the proposed immutable stream before any
buffer, replay, DMA, overlap, arena, or retail work. The retained default-off
census copies only during the selected eight-frame gate and changes no shipping
behavior. Full certificate:
`artifacts/performance/2026-07-20_task34-e1-stage-stream.md`.

## 2026-07-20 - Task 36 Phase A2 hardware matrix compose

```text
IDEA ID: TASK36-PHASEA2-HARDWARE-COMPOSE-20260720
STATUS: PHASE A KEEP; OWNER VISUAL APPROVED; PHASE B USER-OVERRIDE KEEP
MECHANISM: 26 rigid projected no-Z bindings compose camera x local in GX;
  binding 29 raw source-Z/range remains exact CPU-composed
ENGAGEMENT: 26 DObjs; 3 camera loads; 31 world multiplies; pre-GO fallback 1;
  post-arm failures 0
STAGE P50/P95 DELTA:
  frames 438..445   -40,128 / -40,000
  frames 600..607   -40,128 / -39,872
  frames 1398..1405 -40,000 / -39,936
FIDELITY: 355/49,152 changed pixels (0.722%), mean 0.28, 100% overlap in all
  three synchronized pairs; main floor and all three platforms visible
CONTROL ROM:   8E85E951EED6805F350BBDE0DEFB29706A7A5D49433119D82E016BA38D9CD71D
CANDIDATE ROM: FD29F0656BCDC1B83160DB3D3D481C2820AFAE224A0A52966D7BF611642CCBE8
VERIFY: focused fixtures PASS; DevFast PASS; Boundary PASS; three complete
  one-minute Results lifecycles PASS; strict lab start-state assertion not claimed
```

The missing cards were bindings 39/40 (DObjs 54/55, runs 50/51). Binding-39
hardware clip readback differs from the exact CPU reference by at most two
20.12 units with no translation wrap. The discarded band-view experiment was
not the fix: excluding binding 29 had left raw fallback matrices uninitialized.
Initializing that exact fallback and delaying GX admission until the first rigid
run restores the platform and slightly exceeds the original WIP saving. the owner
approved the result and Phase A is committed at `c08e8ee`.

Phase B's admitted standalone ROM/ELF are `E5E6F66...` / `13713DDE...`, with
Task29/34/36/affine `0/1/1/0` and full arena `0x150000/0`. Across countdown and
early combat, 3,896 of 6,664 whole-stage words are conserved (58.463%). The
entire 3,929-word rigid partition is itself only 58.959% of the stage stream,
making 60% impossible even before Whispy. The 33 varying rigid words are the 11
live camera `MATRIX_LOAD4X4` lanes at DObjs 0, 40, and 54. Whispy timed out at
the fixed 30-second limit and was not rerun. the owner explicitly accepted 58.463%
on 2026-07-20, superseding the task-local 60% threshold. Full evidence:
`artifacts/performance/2026-07-20_task36-phaseb-conservation.md`.

Phase B captures and replays complete rigid segments 0/5/7: 33 runs, 26 rigid
bindings, and 3,916 FIFO words in a fixed 4,608-word BSS buffer. The final lab
row proves READY/one-bake/full-arena/zero-fallback:
`445,2,1,1,0,443,3,33,3916,0,0,0,3916,1376256,0,3916,161,0,0`.

```text
CONTROL ROM: 08B8D0D78F4CFF56F58E13C291A99376D1FCD337BAA89DA633F706A122592BDF
REPLAY ROM:  048F32EDD864D638228ACCEB5C61ADA0D8FAD1D8DFB8D7BCF0906F41C74762A2
STAGE P50/P95: 430,368/430,528 -> 284,320/284,544 (-146,048/-145,984)
DRAW P50/P95:  832,736/835,776 -> 704,672/707,712 (-128,064/-128,064)
FIDELITY: 0/49,152 changed pixels, mean 0.00, 100% overlap
WINDOWS: 438..445, 600..607, and 1398..1405 all engaged with zero fallback
```

The published profile-0 target forces Task 36 mode 2 alongside generated segment
0, affine BG-0, static textures, and Task-32 hot text. Mode 2 substitutes the
3,916-word replay emitter for the now-oversized generic commit owner in the
Task-32 set; final draw-hot size is 6,760/8,192 bytes. The profile-0 one-minute
Time Up/Results soak and Boundary pass. Final ROM/ELF are
`C1B3DDE3044BFF2C5F9B66F9D5CFFE7E4600A0467F43CB1CF032D3E086460761` /
`6E21517AA50D6C479A967AD38F65C7DF28750D3E1AC364BA045EBD400CBA74A5`.
These are melonDS correctness/performance results; retail engagement remains the
hardware referee.

## 2026-07-18 - Tasks 20R-25R atomic queue reconciliation

```text
IDEA ID: TASKS20R-25R-ATOMIC-RECONCILIATION-20260718
STARTING ATOMIC IDENTITY:
  Branch/worktree: master / .
  HEAD: 458191bef147f1c6963b2f533c601f9f68fc7730
  The user's 344-line ClaudeFable5_JumpABC_Tasks_20260715_2326.md change was
  present, preserved, and excluded from this checkpoint. The new untracked
  docs/optimization/tasks.md became the authoritative queue.

AUTHORITY / QUEUE POLICY:
  No universal melonDS-to-device multiplier is valid. Emulator is authoritative
  for deterministic state, semantic/GX traces, arithmetic, and pixels. Retail
  DS A/B is authoritative for DTCM, ARM/Thumb, code/data/cache layout,
  generated-program footprint, direct VRAM stores, DMA, GX FIFO behavior, and
  final pacing. The old Task 25 matrix below remains historical evidence and is
  not Task 25R. After these atomic units, Task 25R is the sole next task.

TASK 20R — CLASS B / MEASUREMENT ONLY:
  Earlier diagnostic ROM / ELF:
    A3268E7B61115012BF378B0418200400288CFC0502E45732AB3DF03ECDFC1E4B
    ECC9BD6566B3E0BC592A18471050D1F74449B60B31B9809A7A3C0F3D16DDF46C
  Earlier console output from Countdown, early, and Whispy agreed:
    gameplay stack base/capacity/HWM = 0x02296140 / 16,384 / 13,044
    post-init main DTCM/user-stack HWM = 3,700
    __dtcm_bss_end -> __sp_usr gap = 15,848
    raw guarded need = 13,044 + 64 + 3,700 = 16,808: NO_FIT by 960
    two 1,024-byte margins + 64 guard = 18,856: NO_FIT by 3,008
  The three legacy JSON exports retained renderer timing and artifact identity
  but omitted the Task-20 compile flag, raw TASK20_STACK rows, and derived fit.
  They therefore do not independently substantiate those console values and
  remain local, uncommitted diagnostics. Treat the deeper NO_FIT observation
  as provisional until Task 20R runs a complete exported lifecycle census
  after Task 25R establishes the new baseline.
  No scratch block or external-stack candidate was added. IRQ/SVC high-water,
  full lifecycle/repeat execution, and pointer-escape proof remain incomplete,
  so this is not Phase A completion. Retained support is compile-gated,
  garbage-collectable from profile 0, refuses phase-matrix co-measurement, and
  scans once at the first suspension-safe startup return or on coroutine
  completion.
  Self-contained evidence:
    2026-07-18_task20-reconciled-stack-countdown438.json
      6173F456D742484A7237A8B7FD38CB632BCADA41E278CAABACA3BEB0D7D2E36C
      ROM CA743BF94FC16A13D9EB8D10EEB2051557035A548994E67A057577416010101F
      ELF 9DE93ADA8275DB11BD31F1F6BDBCF258AAEF7ECE4D9913700F0777FE78304396
      Focused frames 438..445 pass exact 16 KiB capacity, alignment, 64-byte
      guards, and fit arithmetic. The export carries identity flag 1, eight raw
      records, scope `startup-only`, and sampleCount 1. Its 252/2,832-byte HWM
      and FIT 3,148/5,196-byte raw/margin needs prove exporter and
      instrumentation safety only; they do not decide a gameplay-phase stack
      move. The rebuilt profile-0 ELF contains zero Task-20/profile-helper
      symbols.
  VERDICT: PRESERVE CENSUS SUPPORT / NO CANDIDATE / TASK 25R INPUT.

TASK 21R — CLASS B / CUT 21A CENSUS ONLY:
  Census ROM / ELF:
    E43A24875DCB2328E9ADF14CFDC3036A30AED19B68AD26F7BC4EBA32C3322316
    131AA6C7FD84BD7CF83FC49F04543E97C9DE55B0017D49B5706C20B0C9DADF02
  Frames 438..445 contain 392 production epochs: Mario 144, Fox 248.
  Exact-key and producer-resident hits are 16 total (Mario 11, Fox 5),
  4.08%, with zero hash collisions. Each frame visits 541 dense outputs:
  519 computed + 22 aliases; all 519 computed shades use the LUT and 297
  outputs use material packing. This is below the 20% cache gate.
  The owner-slot/generation-aware census is compile-gated and attached only to
  the actual mode-9 production loop. No cache, 21B, 21C, or Task 23R work was
  implemented. Task 21C remains only the compact-table foundation for Task 27.
  ITCM is 29,688/32,768, 3,080 free. Task16 modes were 0/0/0, so timing from
  this census build is not a performance comparison.
  Evidence:
    2026-07-18_task21a-shade-census-countdown438-445.json
      2CD54EF21AD90C6D9B4C307ADB6C63A0725C1679BF32D5E310B1167BCC7706D2
  VERDICT: PRESERVE CENSUS / CUT 21A CACHE KILLED BELOW 20%.

TASK 22R — CLASS A / THRESHOLD-4 PACKED DIRTY-SPAN WRITER:
  Same profile-1 candidate ROM, runtime selector A=0/B=1:
    285867B3D8182E0991D2A5C17234DB2E934722670B2597D9A4A38ABB6185BAD6
  Phase                 wallpaper P50/P95/max A -> B       writer P50/P95/max A -> B
  Countdown 438..445    342656/425856/425856 -> 336128/411264/411264
                       294176/376192/376192 -> 283904/356992/356992
  Early 600..607        403904/428032/428032 -> 377920/400896/400896
                       355040/379008/379008 -> 326560/348608/348608
  Whispy 1398..1405     383520/433984/433984 -> 374592/419456/419456
                       334496/384000/384000 -> 321280/364864/364864
  Natural KO            109440/366784/366784 -> 109504/370944/370944
                        62496/316160/316160 -> 61952/315968/315968
  KO wallpaper regresses +64/+4,160/+4,160 while its physical writer saves
  only -544/-192/-192. The +4,160 P95 exceeds the +2,000 gate and independently
  forces REVERT, despite positive moving-camera windows. Retail A/B was not run.
  Profile-2 ROM B5E03AD6A11D6DB87C0A6BDE5B8A955DF986CE80A97718F3BFD084DF8322850E
  records 136,192 map checks and 14,942,208 pixel checks with zero mismatches
  and no first failure. The writer, scratch plan, threshold, runtime selector,
  and selector plumbing were removed. Neutral default-off maximal-run/store
  census remains. Post-revert ROM
  3556D08FB1C31C2420F0505E5FC29B0E33E101500EA033BC963DC9E8031F87E7
  passes 192-row conservation: 10,570 scalar + 10,496 DMA = 21,066 pixels.
  Evidence SHA-256:
    threshold4-A-countdown  D8BAEE8790E4FF33972807BE493BDA4C35B9CABEFA9ABE6687C1B4474F63208E
    threshold4-B-countdown  6C5C88969B82F5ED60A454E70217B7F0B0761DFA86BEBF5A620172882B4C19C8
    threshold4-A-early      16A65D188247FB88E09CE3FEF1E70F8F6456DD7BDF1B112B167039A74C6B5B37
    threshold4-B-early      542DD8B1220017D86411CC3AE8E7D5566D57FA50AD3B129E8C01EEC0BBDF405B
    threshold4-A-whispy     410F809FC9B410BD0C69D887FDB236C5EC335A00C73B8672C838ECD587172AE3
    threshold4-B-whispy     D2054415AC3AD0C355569BB1E61DF18C3752347F7E1B85743D022CAF59D3FF0A
    threshold4-A-KO         909348B5EE8AE8018242623B8C7C907786D9F300FDF8732BD942202A32A0883E
    threshold4-B-KO         57A4572039E3BDFA94AE00DE5E04CA7019024D074D265CF44AA8C995AADA79A6
    threshold4-profile2-B   B80C5AA3A1EFD027F3D262A780E310E87DB35CBAFABDE5B405B2300338D462DD
    post-revert census      888F8D4F3B747B217BCCD66BC7BBCF8743F5FAADACE09C1FB74DA2B7B2C38748
  VERDICT: REVERT / CENSUS RETAINED / NO NEXT TASK-22 CUT.

TASK 24 — CLASS C / EVIDENCE-CLEARED WORKTREE BATCH ONLY:
  Evidence migration commit: 458191bef14 chore: preserve closed-worktree evidence
  Manifest:
    artifacts/performance/2026-07-18_task24-worktree-evidence-migration-manifest-v3.json
    62585F0AE38C8B2ED4E0DC855DE56102B046585C69A208A6624EC0C16A8AF1BB
  Audit: 33 worktrees; 1,522 canonical files / 253,169,779 bytes and
  292 external files / 24,666,164 bytes hashed. Migration copied 225 unique
  files / 20,501,076 bytes; 50 were already canonical and 17 duplicated a
  migrated file. All 292 destinations were rehashed; zero failures.
  The manifest itself is a pre-deletion assessment. After its evidence commit,
  the 17 cleared worktrees below were rechecked clean and removed:
    .tura/goal-task11
    .tura/goal-task10
    .tura/goal-task9-phase2-proof
    ../Smash64DS_Port-worktrees/{audio-union,countdown-refine,integration,mario,
      shadowgo-point-qual,visual,visual-control}
    ../Smash64DS_Port-wt-m4-aot
    ../Smash64DS_Port-wt-root-countdown-review
    ../Smash64DS_Port_baseline_audit
    ../Smash64DS_Port_task12_{arm_hud,thumb_hud}
    ../Smash64DS_Port_task16_fmul
    ../Smash64DS_Port_task18_probe
  Exactly 15 ambiguous/dirty worktrees remain held:
    .tura/control-task8-cut-e
    %TEMP%/smash64ds-task16-{bisect-51fc,statehash-b175cffb,statehash-c9a}
    ../Smash64DS_Port-worktrees/{attack,fox,hit}
    ../Smash64DS_Port-wt-{audio,gameplay,soak}
    ../Smash64DS_Port_task14_verifier
    ../Smash64DS_Port_task16_{fadd_fsub,i2f,i2f_c9}
    ../Smash64DS_Port_task17_census
  No branches, builds, logs, root snapshot, telemetry, or git objects were
  deleted. No honest byte-reclaim total was captured before removal, so none is
  fabricated. Further Task 24 work is deferred to a quiet slot.
  SAFETY GATE: focused hash/parser/checker checks and the final profile-0
  Boundary profile pass. DevFast is retired and is not a valid registry profile;
  it was not revived.

QUEUE AFTER THIS ATOMIC CHECKPOINT:
  Task 25R -> priority selected by its current artifact:
    M3: 23R Phase 0 -> 26 -> residual 23R Phase 1
    M2: 21R -> 27
    disjoint: 20R -> 22R
    then 28 -> 29; Task 24 quiet-slot only; Task 30 final.
  src/nds/nds_renderer.c remains a mandatory one-writer surface.
```

## 2026-07-18 - Task 25 same-artifact all-phase matrix

Historical predecessor only: this row remains valid evidence under its old
contract, but it is superseded by Task 25R and does not set current priority.

```text
IDEA ID: TASK25-SAME-ARTIFACT-PHASE-MATRIX-20260718
SCOPE / IDENTITY:
  Measurement and verifier/tooling only; no runtime behavior changed. One
  profile-1 mode-163 ROM supplies every timed phase: fast mode 9, static AOT 1,
  Task 16 compare/i2f/addsub 1/1/1, update-hot retained, renderer ARM, live Fox,
  production incremental wallpaper, eight samples, melonDS 1.1 interpreter.
  ROM / ELF SHA-256:
    FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144
    A1811E43CFF97DC57EC1DB19DCE89CE4E0E36C50900E83857A371B0340F2CF68
  ROM bytes 14,676,992; ITCM 28,820/32,768, 3,948 free.

ARCHIVED PRE-TASK-25 MIXED-ROM BOARD:
  Countdown 2868DEC6..., frames 438..445, active 1,559,616/1,560,448.
  Early     2868DEC6..., frames 600..607, active 1,250,432/1,251,264.
  Whispy    426B821A..., frames 1398..1405, active 1,616,000/1,617,920.
  Late      426B821A..., frames 3300..3307, active 1,240,832/1,414,912.
  KO        32C957AD..., frames 708..715, active 1,261,344/1,524,864.
  Rebirth   32C957AD..., frames 730..737, active 1,110,528/1,112,256.
  Results   9C35F4B3..., one profile-0 lifecycle, owner timers unavailable.
  These rows remain historical evidence only. Cross-build presentation-frame
  coordinates 708/730/3300 are not valid gates for the new ROM.

SAME-ROM MATRIX, P50/P95 ARM9 TICKS:
  Phase       frame/source       source UPD        draw              stage
  Countdown   438..445/484..498  199,776/200,896   1,148,416/1,216,576 465,984/466,112
  Early       600..607/808..822  144,480/145,920   1,063,232/1,085,824 466,112/466,240
  Whispy    1398..1405/2404..2418 214,944/814,080  1,183,328/1,221,312 466,016/466,048
  KO          566..573/740..754  194,336/222,912     799,872/1,053,248 470,016/470,144
  Rebirth     589..596/786..800  149,120/824,640   1,042,880/1,046,592 465,504/466,112
  Late      1846..1853/3300..3314 197,696/209,728    864,864/867,328   466,048/466,176
  Time Up   1988..1995/3584..3598 197,568/617,920    865,728/896,064   465,984/466,112

  Phase       Mario              Fox                wallpaper         active             loop
  Countdown   169,440/171,200    208,960/209,344    286,112/351,296  1,153,248/1,221,376 1,680,448/2,240,640
  Early         9,920/9,984      196,800/196,928    340,544/362,880  1,068,032/1,090,624 1,680,448/1,680,512
  Whispy      169,504/171,264    209,184/209,728    320,416/358,592  1,188,160/1,226,112 1,680,448/2,240,640
  KO            7,584/7,616      197,568/197,632     73,888/327,360    804,672/1,058,048 1,120,256/1,680,512
  Rebirth       9,920/9,984      197,504/197,632    319,904/323,968  1,048,864/1,303,360 1,680,448/2,240,640
  Late        169,568/171,328    208,384/208,768      2,400/2,432      869,696/872,192   1,120,256/1,680,448
  Time Up     169,504/171,200    208,096/210,112      2,400/2,432      870,528/900,864   1,120,256/1,680,448

  Phase       draw residual      present residual    loop residual
  Countdown    19,616/19,968       1,408/1,536        1,600/1,728
  Early        50,048/50,304       1,376/1,536        1,600/1,728
  Whispy       19,520/19,776       1,376/1,536        1,536/1,664
  KO           50,720/50,880       1,408/1,408        1,568/1,664
  Rebirth      49,920/50,048       1,408/1,472        1,600/1,728
  Late         19,520/20,736       1,408/1,536        1,600/1,600
  Time Up      19,584/49,600       1,408/1,472        1,600/1,600

LIFECYCLE RESULT:
  The same ROM, reused with -NoBuild, reaches natural expiry and Results with
  SCENE 24->22->6, VSB_END one teardown, VS_RESULTS initialized/displayed,
  4,084 logic updates / 2,042 presents, fixed-two pacing, CPU process 3,601,
  and clean KO trace 439/292/154. Results does not publish battle-owner timers,
  so the final timed row is the Time-Up battle boundary; no Results P50/P95 is
  invented. The verifier is RED only at the current reserve contract:
  arena headroom 183,056 minus 65,536 resident audio = 117,520 bytes, 13,552
  below the 131,072-byte floor. This is a real Task 20 input, not a relaxed gate.

ADDITIONAL FINDING:
  The Whispy window records one post-GO conversion, decode, allocation, GL
  create, upload, and fallback from a live weapon texture. Strict zero-fence
  mode correctly rejects it. The profile matrix remains valid with that fence
  disabled for observation, but the M4 lifecycle claim is reopened.

TOOLING / METHOD:
  PhaseMatrixMode is an explicit verifier contract, not a relaxation of the
  generic exact-828 path. It admits only mode 163/profile 1/fast 9/live Fox/
  incremental wallpaper/eight samples and the exact fixed or natural phase
  gates. Natural Late keys on source time_passed >=3300; Time Up keys on the
  final battle rows with time_remain <=16. A concurrent Task 9 placement check
  exposed shared-object objcopy races; the checker now copies objects into a
  per-process temp directory before inspection, and two concurrent checks pass.

EVIDENCE (JSON SHA-256):
  artifacts/performance/2026-07-18_task25-same-rom-countdown438-445.json
    D782016D71C13461430EB13F586EA802E7C500D384320D75D7E50767A81E836A
  artifacts/performance/2026-07-18_task25-same-rom-early600-607.json
    E8951D48B06F648946D04FAF4C19DDCC61EBB66E2CA7ED1C59B587D021AD1C9C
  artifacts/performance/2026-07-18_task25-same-rom-whispy1398-1405.json
    BC672E8FCAFF5D44A646B74A0EF28F9CBC1EB477F95E04FA837BB055859A7AB0
  artifacts/performance/2026-07-18_task25-same-rom-natural-ko.json
    44A6B49B663A27CF00A0F64029B7E11D9B09EA08F4004D96F98CF7FA94D52FD0
  artifacts/performance/2026-07-18_task25-same-rom-natural-rebirth.json
    F51C29B41E24BAD0501B7BFD71D8830A432C8D7A9BC3D970930DE396AA0FCF20
  artifacts/performance/2026-07-18_task25-same-rom-late-source3300.json
    8B48C757E8E36F024D21B8161CF20FE024B626602159C896FB1EC04EB63A2F73
  artifacts/performance/2026-07-18_task25-same-rom-timeup-boundary.json
    B62FDBAB318F36E3B76BF7BB83D02C16CAD5C90B9788CEF3D5F26F1686F88A14
KEEP / REWORK / REVERT: TASK 25 COMPLETE / RESERVE AND M4 LIFECYCLE REOPENED
```

## 2026-07-18 - Task 12 renderer Thumb conversion and hot-text grouping

```text
IDEA ID: TASK12-RENDERER-THUMB-HOT-TEXT-20260718
SCOPE / CALIBRATION:
  Pure ARM9 codegen and placement; no renderer packets, gameplay state, DTCM,
  decomp source, or visual semantics changed. melonDS 1.1 has no useful
  icache/dcache model for this decision: it overcharges blanket main-RAM ARM
  fetches and cannot value cache locality. Retail hardware is the sole keep
  referee; the photographed device rows below close both original phases.

PHASE A - MAIN-RAM THUMB, ITCM ARM:
  Commit 51fc1fe3e0 removes the mode-163 renderer -marm override. Every
  .itcm* function remains ARM. Two measured local optimizer constraints remove
  an interworking tail veneer and retain the exact compact ARM forms, keeping
  renderer object ITCM at 24,364 bytes and final ITCM at 28,088/32,768.
  Main renderer text is 79,068 -> 60,694 bytes: -18,374 (-23.24%).
  Synchronized frames 438..445, P50/P95 ticks (ARM -> Thumb):
    loop    1,680,448/1,680,512 -> 2,240,640/2,800,832
    update    218,976/220,288   ->   217,152/499,776
    active  1,155,776/1,227,520 -> 1,770,016/1,861,504
    draw    1,150,944/1,222,720 -> 1,765,152/1,856,640
    stage     474,688/474,752   ->   807,040/807,232
    Mario     172,736/174,656   ->   302,560/314,240
    Fox       213,120/213,440   ->   374,752/375,040
  This severe emulator regression is reported, not treated as a hardware
  verdict. Raw native top-screen delta is 0/49,152; fast raw remains exactly
  121 runs / 828 triangles partitioned 202/320/306 with zero fallbacks.

PHASE B - 7,040-BYTE SORTED WORKING SET:
  NDS_HOT_TEXT(00/10) selects the 1,772-byte raw-run kernel (121 runs and 828
  triangles per frame) and 5,268-byte fighter-root kernel (32 roots per frame).
  The Calico 1.2.0-1 linker script is provenance-pinned and differs only by a
  sorted renderer .text.hot output plus the matching main-load start. Final
  order is raw-run then fighter-root, 7,040/8,192 bytes, immediately before
  .main; unrelated wallpaper hot text remains in .main.
  Phase-A Thumb -> Phase-B hot-text P50/P95:
    loop    2,240,640/2,800,832 -> 2,240,640/2,800,832
    update    217,152/499,776   ->   217,152/499,776
    active  1,770,016/1,861,504 -> 1,767,904/1,859,328 (-2,112/-2,176)
    draw    1,765,152/1,856,640 -> 1,763,040/1,854,464 (-2,112/-2,176)
    stage     807,040/807,232   ->   806,016/806,080 (-1,024/-1,152)
    Mario     302,560/314,240   ->   302,112/313,728 (-448/-512)
    Fox       374,752/375,040   ->   374,240/374,528 (-512/-512)
  Raw frame delta remains 0/49,152; GBI fixtures, parity corpus, exact codegen,
  ITCM, ordering, load-range, and runtime marker gates pass. The first linker
  attempt correctly failed at runtime because Calico's load table still began
  at .main and omitted the leading .text.hot bytes. __main_start now begins at
  .text.hot (0x020013c0), and the focused checker permanently rejects that
  omission.

RETAIL-DS VERDICT (2026-07-18):
  Phase A ARM control:
    FPS 13.5; UPD 374,464; DRW 1,743,296; ACT 1,745,984;
    LOOP 2,240,384; SLIP 0.
  Phase A Thumb candidate:
    FPS 10.6; UPD 387,008; DRW 2,338,112; ACT 2,367,040;
    LOOP 2,800,640.
  Blanket Thumb adds 594,816 DRW ticks (+34.1%), moves the loop from four to
  five VBlanks, and drops the observed rate from 13.5 to 10.6 FPS. Phase A is
  REVERT: restore the mode-163 renderer to ARM.

  Phase B hot-text candidate on the Thumb base:
    FPS 10.6; UPD 385,664; DRW 2,332,672; ACT 2,367,808;
    LOOP 2,800,960.
  Against its Thumb control, DRW moves only -5,440 (-0.23%), ACT moves +768,
  and LOOP moves +320. That is device noise, not a workload win. Phase B is
  REVERT WITH BASE. The linker placement mechanism remains safe and available;
  Task 17 reuses it only for the independently measured update working set on
  the restored ARM renderer.

  Authoritative photos and SHA-256:
    artifacts/visibility/smash64ds-task12-phase-a-arm-control.jpg
    AE7449E06C71EBAB4202472368134D5EFBBA6C9AC4A9274CA3634B04E9B134EA
    artifacts/visibility/smash64ds-task12-phase-a-thumb-final.jpg
    DF5227E5AE12C104E38126FEB59C769703165882CC7AF61A7C2082F3E7D1BEA4
    artifacts/visibility/smash64ds-task12-phase-b-hot-text.jpg
    7511F848FACBEE465CB0DBE80F1F46D269B3905EFCEA0F3D2011AE391E2856B0

HISTORICAL MELONDS A/B EVIDENCE (NOT THE DEVICE PACKET):
  Phase-A ARM control ROM:
    builds/task12-phase-a-arm-control/smash64ds-task12-phase-a-arm-control.nds
    B11B5D29978C442627758D8B44D9D09D876242EBE9EF889846FE21EDE0933F70
  Phase-A Thumb ROM:
    builds/task12-phase-a-thumb-final/smash64ds-task12-phase-a-thumb-final.nds
    4793D93ECB06BE22A505533275E5E6342D3B93B960A2134183DE53147298BD0C
  Phase-B hot-text ROM / ELF:
    builds/task12-phase-b-hot-text/smash64ds-task12-phase-b-hot-text.nds
    66BDD5FF9E58C55B9C9B157F111D84DA14D9EF03452387E9E388918F9B95F171
    75339DD7B188D15309CD9D3D8821FF52FB425B5C195B3AEB71EBAA73C5621F4A
  artifacts/performance/2026-07-18_task12-phase-a-arm-control.json
  artifacts/performance/2026-07-18_task12-phase-a-thumb-final.json
  artifacts/performance/2026-07-18_task12-phase-b-hot-text.json
  artifacts/visibility/2026-07-18_task12-phase-a-arm-control-frame445.png
  artifacts/visibility/2026-07-18_task12-phase-a-thumb-final-frame445.png
  artifacts/visibility/2026-07-18_task12-phase-b-hot-text-frame445.png

CORRECTED HARDWARE HUD PACKET:
  builds/task12-hardware-hud-pair/
  Phase-A ARM control ROM / ELF (GIT bdc9d3d):
    0747E5A0C07298D8ED00A0C8053E6DD4E195D316E65FDF25571A44D1D250B0B3
    5E4548978FF7FE86040063567591F60EAF885DEBB97D9193F91932044B15D29B
  Phase-A Thumb and Phase-B control ROM / ELF (GIT 51fc1fe):
    EDBED875FE2A70A27D544FA9D6BD7C3A2ADBA3B008926C9FE4070532A1F408E5
    C9C7F105A2B741BA835D5607267829BC3EC1BD0F92F084F39623747CA97975E0
  Phase-B hot-text candidate ROM / ELF (GIT 19fdafa):
    7DF7F9473AD8700F8E2F26415121524E6A1E2971B5D58BB061314FBBE015A2FA
    F56A2EEF7C1B1E429BCC52C72983D0062115E608835AAAA778B7FB374A883516
  All three are mode 163, profile 1, fast mode 9, static-residency 1, and
  NDS_DEBUG_HUD=0. The focused checker proves ARM/Thumb state, unchanged
  28,088-byte ITCM, the 7,040-byte sorted hot region, exact build flags, and
  all six embedded HUD rows. Frozen-Fox melonDS launch checks passed through
  frame 445 with exact 121/828/202/320/306 fast-raw rows, zero fallbacks, and
  visible UPD/DRW/ACT/LOOP/SLIP/GIT output. These checks prove packet function,
  not a hardware verdict.
  artifacts/performance/2026-07-18_task12-hardware-arm-control.json
  artifacts/performance/2026-07-18_task12-hardware-thumb-control.json
  artifacts/performance/2026-07-18_task12-hardware-hot-text.json
  artifacts/visibility/2026-07-18_task12-hardware-arm-control-frame445.png
  artifacts/visibility/2026-07-18_task12-hardware-thumb-control-frame445.png
  artifacts/visibility/2026-07-18_task12-hardware-hot-text-frame445.png
DECISION: PHASE A REVERT; PHASE B REVERT WITH BASE. RETAIN ONLY THE PROVED
PLACEMENT MECHANISM FOR A NEW MEASURED WORKING SET.
```

## 2026-07-18 - Task 15 constant-depth GX painter reconciliation

```text
IDEA ID: TASK15-CONSTANT-DEPTH-GX-RECONCILE-20260718
SOURCE / MECHANISM:
  BattleShip grdisplay.c preserves the no-Z / source-Z / no-Z / no-Z draw
  order, and grpupupu.c creates the four Dream Land owners in that order. The
  canonical renderer already implements TASK 15: each no-Z run replaces the
  GX matrix Z row with painter_band * W, so the divider produces the exact
  constant post-divide depth while GX performs the XY transform. Whole-owner
  preflight fails closed before GX mutation if a run cannot express that form.

CURRENT POST-TASK-12 PROOF:
  No second implementation or candidate was created. The focused native-stage
  checker passes 8 callbacks, 57 DObjs, 42 bindings, 54 runs, 49 epochs, 202
  stage triangles, the exact 5/10/15 cross-matrix census, 12 fail-closed
  perturbations, and slab SHA-256
  053444F8474B4E7A80E0AD7F8F68272AF9D7CB7089036E665574ABF3DEE95EC7.
  The current Task-12 Phase-B run retains 121/828 total runs/triangles,
  stage/Mario/Fox 202/320/306, zero fallback/fence/conservation failures, and
  a raw 0/49,152 gameplay-crop delta. Its widest Boundary run passed.
  Existing depth-trace gates retain background 4095..4024, real source-Z
  3605..3728, and foreground -3969..-4022 with no synthetic/source collision.

GX RESOURCE CONTRACT (before == after; reconciliation made no code change):
  Conservative submitted maximum is 828 polygons and 2,484 vertices against
  2,048/6,144 hardware limits: 1,220 polygon and 3,660 vertex headroom. The
  source class census remains 648 raw / 44 cross-matrix / 126 constant-depth /
  10 source-Z triangles; all 126 constant-depth triangles use the GX form.

PERFORMANCE:
  The original retained mechanism and its later exact specializations are
  already measured below: AOT shift saved 22,016/22,208 stage ticks and the
  zero-shift builder saved another 14,304/14,080. Blanket renderer Thumb makes
  melonDS timing non-authoritative for the current hardware decision, so no
  emulator-only number is relabeled as a new TASK 15 win.

DECISION: COMPLETE / ALREADY CANONICAL.
  A duplicate constant-depth path would add risk without a new treatment.
  Retain the existing source-order, depth, resource, pixel, and fail-closed
  gates.
```

## 2026-07-18 - Task 16 extended bit-exact soft-float closure

```text
IDEA ID: TASK16-EXTENDED-BIT-EXACT-SOFT-FLOAT-20260718
BOUND / RETAINED SET:
  TASK 16 extends the Task-9 Phase-2 exact-ARM method without touching the
  renderer TU, decomp source, gameplay semantics, or the stock low-frequency
  helpers. Three independently proved candidates remain integrated behind
  default-OFF lab selectors and intrinsic published-target overrides: compare
  236 bytes / SHA-256
  F822244564E6EFF11F3812B241A2E71ED7E013D34905687C4D9E2E8242C1185D,
  i2f 92 bytes / EE6FE8233D98D26A602988362BF014D08515AA477EF493DE55F31B26ED8D0573,
  and add/sub 404 bytes /
  9A74410744210A544CC57EA1323C3C9A896D430E2295718870DA4B827E4139FE.
  i2f covers all 2^32 host inputs plus 393,256 literal ARM9 vectors; add/sub
  covers 233,554,432 host and 200,005,832 literal ARM9 operations. Compare,
  i2f, and add/sub all report zero mismatches against their selected GCC
  15.2.0 golden objects.

FMUL AUDIT / DECISIVE REVERT:
  The selected Thumb-multilib archive is
  C755ADC33ECA252260360327904591B8462CCE5C25E48B0E881AC0B295953F48.
  Its _arm_muldivsf3.o text is 760 bytes / SHA-256
  C313BBE04B3484CDBE9E6B14BCB14B0828F308B076616D080AA89204FDFD940A:
  stock ARM-ITCM fmul is 408 bytes, fdiv is 352 bytes, and fdiv branches into
  four fmul tails, so fmul cannot independently reclaim its stock body.
  The natural census recorded 1,467,051 calls: 79.07% both-normal, 21.50%
  either power-of-two, 20.93% either zero, and 6.64% either +1. Stock already
  specializes zero and power-of-two inputs.

  The only justified candidate was a 48-byte, 12-instruction, stackless,
  call-free finite-zero wrapper (machine SHA-256
  AC1351F61DC2F02B91F738A2E3D26B7598A9EBA88D9E0D9E55B8105D499F0B8B)
  that tail-branches every nonzero or exponent-255 input to a renamed literal
  stock golden. It passed 2,304 directed plus 100,000,000 deterministic host
  model pairs, then 1,050,880 candidate-versus-literal-stock ARM9 pairs with
  zero mismatches. ARM9 lab ROM/ELF SHA-256 values are
  9565D561B8AEF7A7B1E5B6DF91B0990445E64FFA43FA5DC498DBE7890B9C7E22 /
  0B19EDFF4671DFDFAAD9F151A64FC9E9D1DD3AE98BCA6EA083E2078893796F89.

  Correctness did not rescue the cost. A 65,536-call representative ARM9
  distribution measured candidate/stock 1,459,328/1,385,344 ticks (+5.34%).
  Synchronized frames 438..445 then measured:
    source update  215,104/216,512 -> 218,624/219,968 (+3,520/+3,456; +1.64%)
    total update   217,152/499,776 -> 220,704/503,232 (+3,552/+3,456; +1.64%)
  Fast-raw and M3 semantic rows remained exact. Fmul state JSON SHA-256 values
  DE2387B5E261746E0FBBE0F1E0E90E88C3887CBFB6E80C5031866A3392354764 /
  F07CA0C224E6596ABE3C6AD7A65ADB64B37EADD0BD63F42B488FF3799FF4F040
  cover 3,892/3,892 identical rows with zero overflow. Natural benchmark JSON
  hashes are 80B8BD1F2667736C67A785F96541AF3627213AD8DE409C960ECCDE291832E0A5 /
  33D053E0098936571015E03C5B3531FF36C96DE98788343221C6935D16E7DC85.
  The candidate, Makefile filter, proof-only scripts, and branch-local linker
  edit were removed. After that clean revert, the lane fast-forwarded to
  ebd0f8a238; that accepted integration already contains the same known
  `.main : ALIGN_WITH_INPUT` linker correction, so this fmul closure neither
  adds nor attributes that correction to fmul.

COMBINED SUPREME / PLACEMENT:
  A new fail-closed verifier builds mode 163 at Task16 compare/i2f/addsub
  0/0/0 and 1/1/1, re-runs the exact libgcc/object/codegen/ITCM checker, and
  requires stock 408-byte fmul in both ELFs. Both one-minute CPU-on lifecycles
  produced exactly 3,892 six-field rows, zero overflow, and no divergence.
  Control/candidate state ROM SHA-256 values are
  86FB8764E500D79C9FE6DA48E07735CC9D474A38EDA5DA98BB5FC1E09A67EB33 /
  EA37B0B9BD24C3890FEACA333B457252232D512CFDD7C95689652660EBF00D4D;
  ELF hashes are
  0384BF9FB60D13076D8165152692EC6964BC33CB3F069FF6C1B52739A91189B5 /
  C635451B6EE894AD5E10DC89261A4BDE78484B7A5CA9E35BC9CC6DDCA24AD1D7;
  JSON hashes are
  63BA3649F81C24726E6A72ACFA6E535933C94368BADD5B8F1A454AEA60D52CBF /
  2116151492EE972920D4AFE114A445A31AEB1A30F8B5E98C9177CD4A921C3BE1.
  Profile-1 ITCM moves 28,088 -> 28,820 / 32,768 bytes: exactly 732 raw and
  final candidate bytes, zero fill, 3,948 bytes free, and no renderer eviction.

COMBINED SYNCHRONIZED A/B (frames 438..445, live Fox):
  owner             control P50/P95       all-three P50/P95      delta
  source update      215,104/216,512        200,000/201,088   -15,104/-15,424
  total update       217,152/499,776        202,080/484,608   -15,072/-15,168
  stage              806,016/806,080        805,344/805,440      -672/   -640
  Mario              302,112/313,728        299,776/311,552    -2,336/ -2,176
  Fox                374,240/374,528        371,648/371,968    -2,592/ -2,560
  active           1,767,904/1,859,328    1,762,016/1,853,440 -5,888/ -5,888
  present          1,763,040/1,854,464    1,757,152/1,848,512 -5,888/ -5,952
  Source-update median falls 7.02%. Fast-raw remains 121/828 and the exact
  202/320/306 stage/Mario/Fox partition; all M3 semantic rows match. Control /
  candidate ROM hashes are
  66BDD5FF9E58C55B9C9B157F111D84DA14D9EF03452387E9E388918F9B95F171 /
  E91B59CD010C95824299EAFB899D9DD5F09F461332388F6199ABBF8C16694CE9;
  ELF hashes are
  D653BD4D7863A3E3A2894C2902C0BAC7D797A79A6CC9D7A8CF16D9F98738585F /
  FE6FCD789B1220FEF609C34AC7A0C14D157BDAC9D97ADC58D49256F95ECC8146;
  benchmark JSON hashes are
  34B2DDFCC3954AFEB3CAB6941782282B0DD15552275B38DED7BBCABA99803B59 /
  763805DFDF8955A3E1A199305D83BA32711F3B0AE63FA050358EF0BA442C3636.

EVIDENCE:
  scripts/verify-task16-combined-state-hash-ab.ps1
  artifacts/performance/2026-07-18_task16-combined-state-{control,candidate}.json
  artifacts/performance/2026-07-18_task16-combined-perf-{control,candidate}.json
  artifacts/performance/2026-07-18_task16-fmul-state-{control,candidate}.json
  artifacts/performance/2026-07-18_task16-fmul-bench-{control,candidate}.json
  builds/build-task16-fmul-arm9-lab/
PUBLICATION GATE:
  Commit 8a4c7185dda intrinsically enables compare/i2f/addsub for the published
  and release-equivalent freeze targets while retaining global zero defaults
  for exact lab controls. Canonical Boundary passed in mode 163 with
  task16Compare/I2f/AddSub=1/1/1, stock fmul=408 bytes, ITCM 28,820/32,768,
  zero fill, and 3,948 bytes free. The published 14,651,392-byte ROM is
  SHA-256 41E457A68CBAC94EF389BE6B9677BE60CFB61E81B97E5C3F49F2DBB4296469BF.
  The owner verifier derives effective published modes at its make, placement,
  identity, and state-export seams. The combined SUPREME gate has no stale
  NoBuild/CompareOnly pass: it rebuilds both sides and binds exact build, ROM,
  ELF, and all six per-update row fields.
DECISION: KEEP AND SHIP COMPARE/I2F/ADDSUB; KEEP GLOBAL LAB DEFAULTS OFF;
REVERT FMUL.
```

## 2026-07-18 - Task 17 update hot-text hardware verdict and rework

```text
IDEA ID: TASK17-UPDATE-HOT-TEXT-20260718
ORIGINAL DEVICE EXPERIMENT:
  Isolated commit 910662d28c placed a 12,056-byte renderer-plus-update working
  set on the Task 12 Thumb base. The retail-DS photographs close its actual
  update-locality question:
    control:   FPS 10.6; UPD 386,240; DRW 2,335,552; ACT 2,367,232;
               LOOP 2,800,704
    candidate: FPS 10.9; UPD 342,080; DRW 2,312,192; ACT 2,331,264;
               LOOP 2,800,640
  The candidate saves 44,160 UPD ticks (-11.4%), 23,360 DRW ticks (-1.0%),
  and 35,968 ACT ticks (-1.5%). This is a decisive hardware locality win even
  though melonDS reported a +192-tick source-update wash. Photos:
    artifacts/visibility/smash64ds-task17-control.jpg
    6FE51745D7787A1741A7980977899CC6D39A53B98121DF4A2415108DAE819DF0
    artifacts/visibility/smash64ds-task17-candidate.jpg
    969646FF5020B079B9C2AB46E4B29A7B92B675FD908B73EE506EAF2449D20824

REWORKED RETAINED IMPLEMENTATION:
  Task 12's blanket Thumb renderer and 7,040-byte renderer grouping are gone.
  Mode 163 is ARM again. The sole .text.hot output contains exactly these 11
  measured update functions, in this order:
    gcPlayDObjAnimJoint 500; gcRunGObjProcess 160; gcRunAll 120;
    ndsBaseFTComputerProcessAll 80; battleship_ftMainProcSearchHitAll 76;
    battleship_ftMainProcSearchCatch 72; ftMainProcPhysicsMapDefault 72;
    ftMainProcPhysicsMapCapture 72; ftMainProcUpdateInterrupt 2,176;
    ftComputerProcessAll 604; ftMainProcPhysicsMap 1,084.
  Total is exactly 5,016/8,192 bytes. The linker ASSERT and focused checker
  enforce exact membership, order, size ceiling, and main-load range. Decomp
  sources and generated assets are untouched. Renderer object/final ITCM is
  24,364/28,820 bytes; Task 14 remains live; Task 16 compare/i2f/addsub remains
  1/1/1; stock 408-byte __aeabi_fmul remains linked.

ARM-BASE SAME-HUD CONFIRMATION PAIR:
  builds/task19-hardware-hud-pair/
  control ROM / ELF:
    609E1B57022ECCA6A821124DED4A464C26BCB660D9F84102E705C1F52FD5CCC0
    C7F407E90D5A45C2FE7D049005E8CFCEA4364402ECA1850D55915F7B1870485B
  ARM + update-hot ROM / ELF:
    381914BD34E34114E06A59E3642CC0896A88736EE9A66947AA7C80B5D4AE30E7
    EBF8EEC6F37946A9943C86B7E9CE887CF375269C578192ED6B5D4E3448AD4408
  Both ROMs are 14,673,920 bytes and embed the same T19PAIR phase HUD.

RETAIL-DS ARM-BASE CONFIRMATION (2026-07-18, TIMER 00:58):
  control:   FPS 13.9; UPD 366,016; DRW 1,699,328; ACT 1,699,072;
             LOOP 2,800,832; SLIP 0
  update-hot: FPS 14.3; UPD 363,456; DRW 1,696,640; ACT 1,714,944;
              LOOP 2,240,448; SLIP 0
  Delta is UPD -2,560 (-0.70%), DRW -2,688 (-0.16%), ACT +15,872
  (+0.93%), LOOP -560,384 (-20.01%, five to four VBlanks), and FPS +0.4.
  The earlier -44,160 UPD result on the Thumb base does not carry to this ARM
  sample. The retained 5,016-byte set is still a device KEEP because its small
  compute win crosses the presentation threshold in the matched phase; that is
  the admitted claim, not a broad 11.4% ARM-base update reduction. the owner also
  reports that the update-hot ROM runs better.
  Photos and SHA-256:
    artifacts/visibility/2026-07-18_task19-arm-control-retail-ds.jpg
    95E2897047E38BD2C2289DCC8980783C8E47D291323C0A05AF6E8E33F62616DF
    artifacts/visibility/2026-07-18_task19-arm-update-hot-retail-ds.jpg
    9AE9777BB31F5B003A5BD67E04F164546D031015F7E6B10F20D6ED5FF5045ECD

MELONDS ARM-BASE REPORT-ONLY CHECK (FRAMES 438..445):
  source update 196,352/197,312 -> 196,160/197,056 (-192/-256)
  draw          857,856/860,928 -> 861,664/864,704 (+3,808/+3,776)
  Raw top-screen delta is 0/49,152 with 100% meaningful overlap. Both sides
  retain exact 121/828, 202/320/306 fast-raw rows and zero fallbacks. The
  exhaustive Task 16 A/B also passes all 3,892 six-field gameplay-state rows.
  These emulator results prove exactness and packet function, not device speed.
  Evidence:
    artifacts/performance/2026-07-18_task19-arm-control.json
    artifacts/performance/2026-07-18_task19-arm-update-hot.json
    artifacts/visibility/2026-07-18_task19-arm-control-frame445.png
    artifacts/visibility/2026-07-18_task19-arm-update-hot-frame445.png
DECISION: KEEP CONFIRMED, REWORKED UPDATE-ONLY ON ARM. THE MEASURED ARM-BASE
GAIN IS THE FIVE-TO-FOUR-VBLANK CROSSING; DO NOT CLAIM THE EARLIER -44K UPD.
```

## 2026-07-17 - Task 11 screen-space census and stage economy

```text
IDEA ID: TASK11-SCREEN-CENSUS-STAGE-ECONOMY-20260717
SOURCE / SCOPE:
  BattleShip grdisplay.c:52-141 supplies strict stage callback order;
  grpupupu.c:637-690 maps owners 4/5/6/7 to Whispy eyes, Whispy mouth,
  back flowers, and front flowers. Gameplay and decomp sources are unchanged.
  The profile-1-only census uses integer Q4 screen coordinates and Q8 doubled
  area; profile 0 and default NDS_RENDER_ECONOMY=0 compile the work out.
IDENTITY / WINDOWS:
  Census ROM/ELF SHA-256:
    E23D8ACAB0692A0BD87174BEC1F1CE7FFEE231B57E6B6E09F1F381A28235E0E7
    7FF5C449F804D115A64E81BC7EEF86F1D5C290F991E6AE38BA0B01C977AFE791
  Exact idle frames 600..607 use Fox frozen for the canonical 828-triangle
  partition. Exact natural frames 438..1037 use the imported level-3 Fox and
  span 600 live presentations. Stage coverage is 121,200 = 202*600; Mario is
  165,120 = 516 whole 320-triangle packets; Fox is 183,600 = 600 whole
  306-triangle packets; invalid projections and census overflow are zero.
  Extending the window through frame 1111 encounters the already-owned Whispy
  post-GO texture debt in KNOWN_ISSUES, so that unrelated gate was not weakened.
RANKED NATURAL CENSUS (ticks/frame * subpixel fraction):
  owner/source                 tri/f   <1 px2   <4 px2  ticks/f  score1/score4
  5 Whispy mouth                   8    43.62%    43.62%    7,342   3,203/3,203
  6 back flowers                   6     5.97%    27.25%    8,565     511/2,334
  1 stage layer 1                 76     2.52%     9.28%   14,808     373/1,374
  3 stage layer 3                 28     0.60%     1.41%   19,315     116/271
  2 stage layer 2                 17     0.23%     1.21%   11,623      27/140
  0 stage layer 0                 54     0.01%     0.11%   54,474       8/62
  4 Whispy eyes                    4     0.00%     0.00%    2,955       0/0
  7 front flowers                  9     0.00%     0.00%   12,746       0/0
  Idle 600..607 independently ranks owner 6/5/1 by the <4-pixel score
  (2,867/1,828/578). Fighter planning only: idle Mario/Fox <4 fractions are
  54.57%/53.55%; natural fractions are 76.99%/77.63%. No fighter cut is made.
SAME-ROM A/B/A (frames 600..607; ROM/ELF SHA-256):
    44A9F913525802A6A5157131B066572C589951B72A6245CF89CFBD4123EFF972
    58942D83340FCEFFC30078CC272BE7376A40399124A136B297A9D56F84E91294
  Runtime mask 0 A and repeated A are identical: stage/draw/active P50
  478,368/882,176/887,040 ticks and a pixel-exact native top-screen crop.
PER-OWNER RATCHETS (stage/draw/active P50; channel delta >=25):
  owner 5: 471,296/875,008/879,936; -7,072/-7,168/-7,104 ticks;
           8 triangles and 4 runs skipped; 117/49,152 meaningful pixels;
           protected top-left 0/7,200 changed; all visibility, named-region,
           horizontal-detail, and required-region gates green. KEEP.
  owner 6: 470,048/873,760/878,688; -8,320/-8,416/-8,352 ticks;
           6 triangles skipped; 1,019/49,152 pixels. REVERT (>500).
  owner 1: 463,680/867,456/872,384; -14,688/-14,720/-14,656 ticks;
           76 triangles skipped; 6,323/49,152 pixels and 540 protected-region
           changes. REVERT (>500 and protected region changed).
DEFAULT / PAUSE / ORDER CONTRACT:
  Retained mask is owner 5 (0x20), but NDS_RENDER_ECONOMY defaults OFF. A full
  default-off rebuild is byte-identical to the frozen control ROM SHA-256
  1D55C7F38B1E987488882CFC51FA36B5DE7E9F457F24627D231BC6DBD8F11018
  and keeps 828 triangles on every canonical frame. The task manager samples
  the real BattleShip game_status once per presented frame; only GO activates
  the mask, so Pause/Unpause/Wait/End render the full owner set. Skipped runs
  advance the same no-Z/foreground painter cursor and source counters; all
  remaining owner order is unchanged.
EVIDENCE:
  artifacts/performance/2026-07-17_task11-census-idle600.json
  artifacts/performance/2026-07-17_task11-census-natural438-1037.json
  artifacts/performance/2026-07-17_task11-economy-lab-control-repeat-idle600.json
  artifacts/performance/2026-07-17_task11-economy-owner5-idle600.json
  artifacts/performance/2026-07-17_task11-economy-owner6-idle600.json
  artifacts/performance/2026-07-17_task11-economy-owner1-idle600.json
  artifacts/performance/2026-07-17_task11-default-off-rebuild-idle600.json
  artifacts/visibility/2026-07-17_task11-economy-lab-control-repeat-idle607.png
  artifacts/visibility/2026-07-17_task11-economy-owner5-idle607.png
  artifacts/visibility/2026-07-17_task11-census-natural1037.png
KEEP / REWORK / REVERT: KEEP owner 5 only; owner 6 and owner 1 reverted alone.
```

## 2026-07-17 - natural KO and rebirth phase baseline

```text
IDEA ID: NATURAL-KO-REBIRTH-PHASE-20260717
PURPOSE:
  Close the missing material-phase timing row without scripted combat, a new
  harness mode, or gameplay mutation. Extend the existing renderer sampler with
  exact BattleShip event gates and dynamic transient-geometry conservation.
IDENTITY / SOURCE GATES:
  Mode 163, profile 1, fast mode 9, static AOT 1, imported level-3 Fox enabled,
  fixed-two presentation, no JIT. ROM/ELF SHA-256:
    32C957ADBB0D61F031DC9FF743BE4983175CCBBAE84924788C48FA67C8ECB154
    638778926805CBFD38140C8CB4C897C17C3BADAAD4B68E472A26AB823F53C511
  KO starts on the populated exact KO FGM trace; rebirth starts on
  ftCommonRebirthDownSetStatus. Both then sample eight contiguous presents.
KO FRAMES 708..715, LOGIC 1024..1038, P50/P95 TICKS:
  active 1,261,344/1,524,864; draw 1,256,544/1,258,112; update
  170,304/447,168; wallpaper 547,584/547,648; stage 477,504/478,784.
REBIRTH FRAMES 730..737, LOGIC 1068..1082, P50/P95 TICKS:
  active 1,110,528/1,112,256; draw 1,105,760/1,107,456; update
  179,040/835,968; wallpaper 359,616/361,472; stage 481,600/481,792.
CORRECTNESS:
  Every frame keeps the exact 202-triangle stage owner, one source-visible
  320-triangle fighter, the exact additive 16-triangle death/rebirth effect,
  zero owner fallback/rejects, M4 22/131072, zero hot conversion/upload, and
  zero post-GO fence work.
  Screenshots pass top-screen content analysis.
DECISION: KEEP (EVIDENCE / TOOLING), NO RUNTIME CUT.
  KO is the worse active window. Its dominant wallpaper and stage buckets are
  already measured owners whose exact affine/correction and remaining stage
  alternatives are rejected or stopped. Do not improvise another cosmetic or
  packet shortcut from this phase; return to the measured M2 fighter emit path.
EVIDENCE:
  artifacts/performance/20260717-ko-natural-profile1.json
  artifacts/performance/20260717-rebirth-natural-profile1.json
  artifacts/visibility/20260717-ko-natural-profile1.png
  artifacts/visibility/20260717-rebirth-natural-profile1.png
```

## 2026-07-16 - Task 8 Phase 0.5 complete draw conservation

```text
IDEA ID: TASK8-PHASE05-DRAW-CONSERVATION-20260716
PURPOSE:
  Name every draw/present/loop shell outside the stage and fighter owners before
  selecting another cut. Extend the existing compile-gated M3 Phase-0 lab; do
  not change profile-0 code or add a telemetry mode.
IDENTITY / WINDOW:
  Mode 163, profile 1, fast mode 9, static AOT 1, live Fox, bitmap IFCommon,
  frames 600..607, no JIT. Instrumented ROM SHA-256 is
  B6C3EF22EF18781B4D9E379F6850A36915FEE6D1196DE436F358D5CDB7AA0714.
  The retained production wallpaper arm uses selector 1; the full-raster oracle
  arm uses selector 0 on the same ROM and window.
PRODUCTION-PATH P50/P95 TICKS:
  Draw+flush 1,289,856/1,327,936; stage 586,272/586,496; Mario
  174,528/174,592; Fox 209,344/209,408; wallpaper 298,176/336,000;
  true draw residual 21,568/21,696. The profiler overhead makes these unsuitable
  as an uninstrumented speed baseline; they are conservation evidence.
WALLPAPER SPLIT P50/P95:
  Setup 2,816/2,816; X map 10,848/11,200; Y map 29,632/29,952; physical
  write/raster 254,144/291,584; commit 256/256; residual 512/576. Subtracting
  the measured 64-tick read calibration gives a 241,856/279,296 write bound and
  a 272,928/310,720 complete named-wallpaper subtotal. All eight frames visit
  192 rows and write 20,724..49,152 exact destination pixels.
OTHER SHELLS P50/P95:
  gcDrawAll shell 16,768/16,896; fighter duplicate guard 384/384; present shell
  4,128/4,160; outer draw 320/384; reset 640/704; tail 64/64; flush prep
  320/320; present outer 1,920/1,984; bookkeeping 224/256; publication 320/320;
  loop outer 1,280/1,344. Every nested equation and conservation error is zero.
CORRECTNESS:
  Exact owner 121/828 and 202/320/306 partition; stage
  8/255/57/42/54/202/49/4; cross 5/10/15; M4 22/131072; zero owner fallback,
  post-GO fence work, and conservation error. The verifier initially rejected
  incremental frame 600 because it incorrectly demanded 49,152 physical writes;
  its durable contract now requires exactly 49,152 for selector 0 and a positive
  bounded subset for selector 1. The rejected 27,904-write frame then passes.
BOUND / NEXT CUT:
  Wallpaper is the largest non-owner bucket, but production already retains the
  exact dirty-map/DMA path and prior exact affine/correction alternatives are
  decisively rejected. The next fresh owner lever is Cut E. Refreshed M3 Phase 0
  measures preflight 284,960/284,992, prepare-runs 213,408/213,504, and the
  preflight-minus-prepare shell 71,552/71,872; immutable validation may be
  removed only behind scene-generation stamps while all live preparation stays.
EVIDENCE:
  artifacts/performance/2026-07-16_task8-phase05-fighter600.json
  artifacts/performance/2026-07-16_task8-phase05-incremental-fighter600.json
KEEP / REWORK / REVERT: KEEP (DIAGNOSTIC)
  Retain the compile-gated conservation net. It adds no production behavior or
  claimed speed gain and nominates generation-gated preflight without reopening
  forbidden packet, order, polygon-ID, translucency, or gameplay-float work.
```

## 2026-07-17 - Task 8 Cut E generation-gated preflight

```text
IDEA ID: TASK8-CUT-E-GENERATION-GATED-PREFLIGHT-20260717
PURPOSE:
  Stop rewalking immutable native-stage topology every presented frame. Cache
  only topology identities and a fully validated generated-table summary; keep
  matrices, materials, colors, texture selection, alpha, near-plane work, and
  run preparation live. Any generation/stamp mismatch fails closed through the
  original full validators before GX mutation.
PRODUCTION A/B IDENTITY / WINDOW:
  Mode 163, profile 1, fast mode 9, static AOT 1, live Fox, retained wallpaper,
  frames 600..607, no JIT, common/scene -O2 -mthumb, renderer -O2 -marm.
  Frozen control ROM/ELF SHA-256:
    07FBFCB21586AA3964432ADD9055A98DB29E0D317895B02E1B1FE6DFEBD67765
    B13F18DF0F034EE9AFF3BBB96061ADAE54ED175022AC842A0FB1494C30AEF60A
  Candidate ROM/ELF SHA-256:
    6496D8907BFD67A46647A246A77EEEC1326D195B7ED8DEF13D2A65A9538518CB
    801F872115F37BEEFA4199C7D77F6C059172A01D3E533EF10EE75D1C5FA0AF5B
P50/P95 TICKS (CONTROL -> CANDIDATE; DELTA):
  Stage:  489,600/489,664 -> 470,784/470,912; -18,816/-18,752.
  Draw: 1,168,960/1,204,992 -> 1,149,248/1,185,152; -19,712/-19,840.
  Active: 1,173,824/1,209,856 -> 1,154,016/1,189,888; -19,808/-19,968.
  Update: 312,736/314,112 -> 309,920/311,296; -2,816/-2,816 noise-side
  movement, not attributed to this draw cut.
FAIL-CLOSED LAB:
  Final-source profile-1 Phase-0 lab ROM/ELF SHA-256:
    9246A45B17FFBAF0D79BE8067AAC2697F408B2EDA420979606EB1229D58C9DC7
    2F02287E2176EF4FAF038379571DC5DB40ABDEB9FED249E20E780D2BEA117AC2
  The one-shot lab mutation flips the cached stamp. The terminal census is
  full=2, hit=605, mismatch=1, inject=1, revalidate=1; frames 600..607 add one
  hit each and no additional validation. Production is full=1, hit=606 and
  mismatch/inject/revalidate=0/0/0. Lab-only fault symbols are absent from the
  production ELF.
CORRECTNESS / RESOURCE GATES:
  Native frame 607 is exact: raw and meaningful delta 0/49,152, mean 0.00.
  Owner 121/828 with 202/320/306 partition; stage
  8/255/57/42/54/202/49/4; cross 5/10/15; M4 22/131072; zero owner fallback,
  post-GO fence work, and conservation error. A separate current-source M2
  detailed-ledger build (ROM SHA-256
  5E7E56D401665F0EA259F04C812EF057A5BFDABE5AD7AC698E7C223C702027B9)
  proves Mario 14/18/30 and Fox 18/31/37 roots/epochs/runs, totaling the
  required 32/49/67 over 626 fighter triangles. The battle arena >=128 KiB
  gate passes. ITCM is 26,104/32,768 bytes, leaving 6,664 bytes.
PACING:
  The explicit -RequireLocked30Pacing smoke preserves fixed-two cadence and all
  gates but warns at 19.0 presents/s. This cut is a measured gain; it does not
  by itself meet locked 30.
EVIDENCE:
  artifacts/performance/2026-07-17_task8-cut-e-control-fighter600.json
  artifacts/performance/2026-07-17_task8-cut-e-candidate-fighter600.json
  artifacts/performance/2026-07-17_task8-cut-e-lab-fighter600.json
  artifacts/performance/2026-07-17_task8-cut-e-m2-fighter600.json
  artifacts/visibility/2026-07-17_task8-cut-e-control-frame607.png
  artifacts/visibility/2026-07-17_task8-cut-e-candidate-frame607.png
KEEP / REWORK / REVERT: KEEP
  Bank the generation-gated topology preflight. It changes neither FIFO packet
  ownership, interpreter count, run order, polygon/translucency semantics,
  gameplay floats, nor decomp sources.
```

## 2026-07-16 - Locked-30 fixed-two scheduler decision

```text
DECISION:
  Mode 163 presents on a two-vblank boundary and runs exactly two unchanged
  source updates per presented frame. Vblank-owed debt, catch-up updates, the
  cap-4 path, and dropped-update accounting are removed.
RATIONALE / BUDGET:
  Two updates plus draw are about 1,081K ticks against the 1,120,380-tick
  two-vblank budget. A catch-up third update makes a slipped slot about 1,360K,
  which cannot return below the two-vblank boundary and can turn one transient
  spike into a permanent ~20 FPS cadence. Fixed-two returns to the 30 FPS slot
  on the next frame and avoids visible 2/3-update judder.
SOURCE BEHAVIOR:
  Smash 64 slows uniformly under load and does not run later logic catch-up.
  Hardware-paced BGM can therefore advance ahead of gameplay during a slowdown
  stretch; that is source-faithful behavior, not a scheduler defect. Crowd/fade
  timing must be ear-checked against this final scheduler before more audio work.
HARD GATES:
  Updates equal exactly two times presents over every sampled window. Presents
  never occur inside two vblanks. Any phase with zero slips must report
  59.0..61.0 updates/s and 1 source-second of timer progress per wall second.
  Other phases publish updates/s as their direct slowdown percentage.
PHASE METRICS:
  Countdown, early combat, late combat, KO/rebirth, and Results each publish
  present count, vblank-slip count, and derived source updates/s.
SEMANTICS RETAINED:
  The LoadScene terminal update remains undrawn. Wait->GO still arms static
  textures before the first GO draw. Each of the two updates retains its own
  input read and one original scheduler tick.
BASELINE COMPATIBILITY:
  All one-update-per-present and debt/cap-4 A/B baselines are non-comparable.
  Future baselines must identify fixed-two pacing and be resampled. Jump A
  CUT 3-NEW remains the margin target where phase update rates show slowdown.
VERIFIER AUDIT:
  BPLAY_PACE drops all debt/cap/drop fields; the owner hard-checks the 2:1
  ratio, draw/present equality, two-vblank minimum, zero cadence violations,
  phase accounting, held-30 phase rates, terminal teardown, and zero M4 fence.
  The synchronized MATCH_START debugger pause requests a clean pacing epoch at
  the next iteration boundary so instrumentation is not labeled game slowdown.
  The lifecycle branch runs these phase gates before returning. MATCH_SAFETY
  keeps its first 16 corruption/safety fields hard-zero; its final field is the
  source camera visibility predicate. Natural-match verification separately
  requires selected parts = submitted parts, at least one target-bounds
  decision, and MATCH_SAFETY[16] = bounds-fail; part submissions and target
  checks intentionally have different granularities. The dedicated camera-
  containment verifier retains the zero-failure envelope gate.
STATUS:
  Implemented and naturally qualified on canonical ROM SHA-256
  8B949194C5EF02CCA2A59479F67F99E4A6D73A41E7972DBD95CD3CF78BCF1DAA.
  The 3,600-tick match reached Results with 4,084 committed updates / 2,042
  presents, 2..5-vblank intervals, zero early-present violations, exactly one
  teardown, and zero post-GO M4 fence work. Phase presents were
  194/900/900/0/48, slips 196/1050/931/0/3, and update rates
  39.9/37.9/39.5/n.a./58.2 per second. Countdown and combat remain below the
  two-vblank budget; Jump A CUT 3-NEW still owns that performance gap.
```

Profile 1 is the O2-equivalent low-frequency coarse observer. Profile 2 is the
size-optimized forensic `-Os` observer; its timings are diagnostic and must not
be compared with shipping or profile-1 timings.

## Reference and accepted measurement summary

| Metric | Median | P95 | Notes |
|---|---:|---:|---|
| Shipping O2 draw reference | 2,044,640 | 2,046,080 | Accepted 12.3-FPS snapshot |
| Phase-matched profile-0 control | 2,050,112 | 2,053,824 | +0.27% / +0.38%; under 1% gate |
| Clean profile-1 loop | 2,240,704 | 2,800,896 | O2-equivalent, 128 frames |
| Clean profile-1 present | 2,115,584 | 2,665,728 | Includes VBlank wait |
| Clean profile-1 active | 2,053,888 | 2,419,648 | VBlank removed |
| Clean profile-1 draw | 2,050,592 | 2,416,384 | Low-frequency observer only |
| Clean profile-1 stage layer 0 | 292,288 | 292,352 | First prepared-owner candidate |
| Profile-2 semantic events | 828 | 828 | Zero overflow; timing noncomparable |

Exact correctness identity:

```text
triangles        828 (stage 202; Mario 320; Fox 306)
classes          648 raw-current / 44 cross / 126 no-Z / 10 range
batches          121 / 707 / 121
prepare          98 / 730
all profiles    every sampled pair is canonical: 0/0, one 4,096- or
                32,768-byte upload, or 2/36,864; stable T0C was 2/36,864
T0C upload SHA  EC2EA0B6A9AB3F69B6BDA3978B912D7D765BAB6F4D0E11671E875D559D0BDEBA
GX RAM           715 / 2,167 at the accepted final logic ticks
profile-2 oracle 2,484 / 0 / 0
```

## Experiment index

| ID | Date | Hypothesis | Exclusive phase | A0 med/P95 | B0 med/P95 | Whole-draw delta | Trace/oracle | Verdict |
|---|---|---|---|---:|---:|---:|---|---|
| BASE-12.3 | 2026-07-12 | Accepted shipping reference | draw | 2,044,640 / 2,046,080 | — | — | exact | KEEP |
| T0-O2-COARSE | 2026-07-12 | Low-frequency O2 timing identifies the largest owner | draw / owners | 2,044,640 / 2,046,080 | 2,050,592 / 2,416,384 | phase-matched profile-0 +0.27% / +0.38% | exact counters | KEEP |
| T1A-TRACE | 2026-07-12 | Profile-2 trace guards prepared activation | semantic emission | n/a | 828 / 0 overflow | timing noncomparable | 2,484 / 0 / 0 | KEEP |
| T1B-STAGE0 | 2026-07-12 | Compile immutable Dream Land layer 0 topology | stage layer 0 | 308,864 / 308,928 | 321,472 / 321,536 | -12,640 ticks | exact 128-frame A/B | REVERT |
| REJ-T0-MIXED | 2026-07-12 | Combined coarse timing and detailed census | whole loop / owners | — | loop 3,361,024 / 3,361,280 | observer-contaminated | counters exact | REJECT |
| REJ-GXLIST | prior | 121 small GX lists | vertex submit | — | ~0.96M -> ~1.28M | regression | output retained | REVERT |
| REJ-PACKET | prior | 1,406 generic packets | scan/setup | — | scan flat/setup regressed | regression | output retained | REVERT |
| REJ-WATER | 2026-07-12 | indexed water | texture/upload | — | draw ~2.88M | regression | GX drift | REVERT |
| M4-STAGE-WORLD | 2026-07-13 | Reuse exact stable Dream Land world matrices | matrix / stage | 2,323,008 / 2,355,712 | 2,263,616 / 2,280,512 | -59,392 / -75,200 | 42 / 0 shadows | KEEP |
| M2-GX-SKELETON | 2026-07-14 | Compose the whole fighter hierarchy in GX while retaining exact CPU lighting modelviews | Mario + Fox | 493,696 / 520,896 detailed | 445,568 / 472,768 detailed | -48,128 paired median | exact 626-triangle contract | REVERT |

## T0-O2-COARSE — clean exclusive loop and owner baseline

```text
HYPOTHESIS:
  An O2-equivalent, non-overlapping low-frequency split identifies the largest
  safe owner and separates active work from VBlank wait.
TARGETED EXCLUSIVE COUNTER:
  input, update, begin, wallpaper, stage, Mario, Fox, foreground, HUD,
  glFlush CPU, VBlank wait, post-wait, threads, and residual.
MEASURED UPPER BOUND:
  Stage layer 0 is 292,288 / 292,352 ticks. Its 25% gate is 73,072 ticks;
  the independent whole-draw gate remains 50,000 ticks.
LIVE-TREE RECONCILIATION:
  Worktree f37fd14be plus the uncommitted profiler sprint; user-local roadmap,
  review, log, prompt, and attachment files were preserved; no decomp edits.
FILES/FUNCTIONS CHANGED:
  Profiler/trace plumbing only; no prepared renderer operation was activated.
BUILD TARGET/FLAGS:
  smash64ds-battle-playable-coarse-hwtri; profile 1; final common/scene
  -O2 -mthumb and renderer -O2 -marm.
BASELINE ROM SHA-256:
  5E502A39FB46000176E2EA3BA54511EEB39AE9068DC666104991BF9AEFE51872
EXPERIMENT ROM / ELF SHA-256:
  1AF1F28BC24488534661B5060465597B8C20F901A5607837B546C2EC25D56F4F /
  262ABE9656E8209708205BB6FB73C55FEFB64FD1114DE150D68646C70C8B980B
FRAME/LOGIC-TICK WINDOW:
  Warm-up 186; 128 synchronized frames 187..314; logic ticks 194..321.
A0 MEDIAN/P95:
  Accepted shipping draw reference 2,044,640 / 2,046,080.
B0 MEDIAN/P95:
  Loop 2,240,704 / 2,800,896; present 2,115,584 / 2,665,728;
  active 2,053,888 / 2,419,648; draw 2,050,592 / 2,416,384.
A1 OR DISABLED-CONTROL MEDIAN/P95:
  Phase-matched profile-0 control, 32 frames after a 12-second warm-up:
  2,050,112 / 2,053,824 draw ticks. This is +0.27% / +0.38% against the
  accepted reference and passes the 1% no-op ceiling. The profile-1 observer
  contains no per-command, per-triangle, owner-census, or semantic hashing.
ACTIVE/WAIT SPLIT:
  Active 2,053,888 / 2,419,648; VBlank wait 167,744 / 506,624.
OWNER SPLIT:
  Stage 1,008,896 / 1,027,136; Mario 488,832 / 488,896; Fox
  521,024 / 521,088. Stage layer 0 is 292,288 / 292,352.
OTHER EXCLUSIVE PHASES:
  input 768/768; update 136,256/418,816; source 135,840/154,880;
  audio 192/282,752; begin 64/128; wallpaper 2,368/383,424;
  foreground 640/768; HUD 64/64; flush 64/64; post 192/256;
  threads 1,600/1,600 ticks.
CONSERVATION/FIFO:
  Conservation error 0/0. Draw/active/loop residuals are 13,952/14,080,
  1,344/1,408, and 1,088/1,152 ticks; median/P95 ratios are 0.68%/0.79%,
  0.07%/0.08%, and 0.05%/0.05%. GX status/control had 0 adjacent changes and
  one distinct value; final GX RAM was 715/2,167.
OP/PROGRAM BYTES/FALLBACKS:
  n/a; this experiment adds observation only.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  ITCM 20,916 bytes; DTCM/DTCM-BSS 0/152; main-BSS 1,858,096.
  Profile-0 control main-BSS is 1,857,584, so coarse observer adds 512 bytes.
  ROM is 11,672,576 bytes and ELF is 8,186,872 bytes.
SEMANTIC TRACE RESULT:
  Unavailable by design in profile 1; see T1A.
ORACLE/COUNTER/GX RESULT:
  828 triangles / 2,484 vertices; 648/44/126/10 classes;
  121/707/121 batches; 98/730 prepares; 2 uploads / 36,864 bytes;
  stage 42 lists/202 triangles; fighters 2 owners/626 triangles.
CAPTURE RESULT:
  No new visual capture; the fixed canonical reference remains listed above.
VERIFIER COMMANDS AND RESULTS:
  The build-backed 128-frame realtime profile-1 run and phase-matched
  32-frame profile-0 control passed every exact gate. The latter published
  upload-sequence SHA-256 EC2EA0B...D0BDEBA.
DECISION:
  KEEP the low-frequency coarse plumbing. The earlier mixed observer is
  rejected and retained only as REJ-T0-MIXED history.
NEXT MEASURED BOTTLENECK:
  Dream Land layer 0, then remaining stage owners if the first cut clears its
  73,072-tick owner-relative and 50,000-tick whole-draw gates.
```

## T1A-TRACE — profile-2 forensic semantic and owner-state oracle

```text
HYPOTHESIS:
  One isolated semantic event per source triangle can guard prepared execution
  without sending a second stream to GX.
TARGETED EXCLUSIVE COUNTER:
  Semantic output and owner entry/exit contracts; profile-2 timing is diagnostic.
MEASURED UPPER BOUND:
  n/a; forensic -Os timing cannot rank shipping work.
LIVE-TREE RECONCILIATION:
  Same reconciled f37fd14be worktree and scene as T0.
FILES/FUNCTIONS CHANGED:
  Profile-2 trace/census plumbing only; the generic path remains the sole GX writer.
BUILD TARGET/FLAGS:
  smash64ds-battle-playable-forensic-hwtri; profile 2; final common/scene
  -Os -mthumb and renderer -Os -marm.
BASELINE ROM SHA-256:
  1AF1F28BC24488534661B5060465597B8C20F901A5607837B546C2EC25D56F4F
EXPERIMENT ROM / ELF SHA-256:
  142EFC9F7D61FE32C2332EAE6B07435FCC653AF30C676665F81E18D21720DC82 /
  27B32B4D950382F20CE51D8B1D066B71E058EFCE9F23B2D5CA0C09741D0026AD
FRAME/LOGIC-TICK WINDOW:
  Warm-up 44; 128 synchronized frames 45..172; logic ticks 52..179.
A0 MEDIAN/P95:
  n/a; profile 1 deliberately does not execute the semantic observer.
B0 MEDIAN/P95:
  Diagnostic only: draw 23,546,368/23,583,680; stage subdivision
  6,622,336/6,661,568; stage/Mario/Fox owner walls 7,649,824/7,689,152,
  7,820,672/7,822,336, and 7,609,664/7,610,176.
A1 OR DISABLED-CONTROL MEDIAN/P95:
  Profile 1 is the compile-time-disabled control; compare semantics, not timing.
ACTIVE/WAIT SPLIT:
  Not decision data in forensic -Os.
OWNER SPLIT:
  Stage layer 0 diagnostic wall 1,929,600/1,929,984 ticks.
RENDERER NESTED/EXCLUSIVE SPLIT:
  material 15,168/15,552; matrix 425,344/425,728; DL
  20,461,760/20,499,712; texture 1,367,392/1,407,168; submit
  19,353,312/19,391,424; vertex 1,172,736/1,175,360; setup
  18,180,288/18,219,840; scan 1,108,480/1,109,056. Diagnostic only.
OP/PROGRAM BYTES/FALLBACKS:
  n/a; no prepared program is active.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  ITCM 19,872 bytes; DTCM/DTCM-BSS 0/152; main-BSS 1,949,616.
  No prepared arena exists yet. ROM is 11,614,208 bytes and ELF is
  7,771,668 bytes.
SEMANTIC TRACE RESULT:
  Exactly 828 events/frame and zero overflow. Stage 202/0 across 8 occurrences;
  Mario 320/0 across 1; Fox 306/0 across 1. Both frame hashes and both hashes
  for every owner changed 127 times with 128 distinct values.
ORACLE/COUNTER/GX RESULT:
  Oracle 2,484/0/0; 828 triangles; exact 648/44/126/10 classes;
  121/707/121 batches; 98/730 prepares; final GX RAM 715/2,167. The independent
  forensic cache retained its independent bytewise decoder. Sampled uploads
  followed the same exact canonical phase pairs as profiles 0/1.
CAPTURE RESULT:
  No new capture; trace equality and exact GX/counter gates passed.
VERIFIER COMMANDS AND RESULTS:
  The build-backed 128-frame forensic run passed semantic completeness, owner
  conservation, nonzero boundary hashes, oracle, batch, class, and GX gates.
DECISION:
  KEEP. The profile-2 observer is exact, -Os, and noncomparable for performance.
NEXT MEASURED BOTTLENECK:
  Activate only the first whole-owner prepared stage program under this oracle.
```

### T1A profile-2 owner census and churn

| Owner | Lists | Commands | VTX / vertices | TRI / triangles | Submit classes | Material / matrix / texture / runs |
|---|---:|---:|---:|---:|---|---|
| Stage | 42 | 886 | 59 / 302 | 113 / 202 | raw 66, no-Z 126, range 10 | 4 / 42 / 49 / 54 |
| Mario | 14 | 419 | 18 / 237 | 163 / 320 | raw 284, cross 36 | 14 / 14 / 18 / 30 |
| Fox | 18 | 553 | 36 / 282 | 157 / 306 | raw 298, cross 8 | 16 / 18 / 31 / 37 |

```text
Frames 45..172:
  semantic frame and every owner dual hash: 127 changes / 128 distinct
  selected-event, material, light, texture: every owner 0 / 1
  camera: every owner 127 / 128
  DObj matrices: stage 0/1; Mario 127/50; Fox 127/70
  topology: stage 95/96 from recursive live-branch normalization;
            Mario/Fox 0/1
  owner entry/exit state/cache/resolver/global hashes: nonzero gates passed;
            prepared-path comparison begins in T1B
```

## Prepared-owner candidate census

```text
Owner:                    gGRCommonLayerGObjs[0] / gr_desc[0]
Primary asset/list:       asset 104 / dStagePupupuFile2_data_0x1008
Texture dependency:       immutable asset 103
DObjs / non-null lists:   21 / 20
Executed commands:        297
VTX loads / vertices:     26 / 108
TRI commands / triangles: 36 / 54
Adjacent triangle runs:   26
Immutable branch:         one internal call, 0x06F8 -> 0x0708
Dynamic dependencies:     no MODIFYVTX, MObj, AnimJoint, MatAnim, or dynamic branch
Activation boundary:      whole owner, before first GX write; next-frame only
Storage ceiling:          16 KiB fixed arena
```

## T1B-STAGE0 — exact prepared owner missed the keep gate

```text
IDEA ID:
  T1B-STAGE0
HYPOTHESIS:
  Replacing the 297-command immutable Dream Land layer-0 source scan with a
  coarse prepared program will save at least 50,000 whole-draw ticks or 25%
  of the owner's exclusive cost without changing semantic output.
TARGETED EXCLUSIVE COUNTER:
  Whole draw and gGRCommonLayerGObjs[0] owner wall.
MEASURED UPPER BOUND:
  Generic layer 0 was 308,864 / 308,928 ticks in the phase-synchronized
  profile-1 A/B/A window; the 25% threshold was 77,216 ticks.
LIVE-TREE RECONCILIATION:
  Built on c5f815562 after preserving all user-local roadmap, review, log,
  prompt, and attachment files. No decomp file was edited.
FILES/FUNCTIONS CHANGED:
  The experiment added a bounded prepared-list ABI/executor, exact shared VTX
  decode and TRI submission, a 20-list layer-0 compiler, whole-owner preflight,
  live DObj/camera binding, persistent cache/state replay, execution telemetry,
  synchronized benchmark start frames, and strict profile-2/profile-1 A/B tools.
  Experiment commit 9e466fd15 was reverted by e446ddab4 after the keep gate.
BUILD TARGET/FLAGS:
  Profile 1: smash64ds-battle-playable-coarse-hwtri, common/scene -O2
  -mthumb and renderer -O2 -marm. Profile 2: forensic -Os, renderer -marm.
BASELINE ROM SHA-256:
  Profile-1 generic and prepared used the same runtime-selected ROM:
  3FF0CB4AD3782C8DF2510A718F740E7A9089275C0912646CF0F4BFE84EAB2FCC.
EXPERIMENT ROM SHA-256:
  Profile-2 generic and prepared used the same ROM:
  534EAC077BB9DFE537466B1C24B077D0EA71B0759A7259D953431CB0A4BEA8C9.
FRAME/LOGIC-TICK WINDOW:
  Profile 1 A/B/A: 128 synchronized frames 240..367, logic 247..374.
  Profile 2 A/B: 128 synchronized frames 45..172, logic 52..179.
A0 MEDIAN/P95:
  Generic A and B were identical: draw 2,083,264 / 2,102,784; layer 0
  308,864 / 308,928.
B0 MEDIAN/P95:
  Prepared draw 2,095,904 / 2,115,456; layer 0 321,472 / 321,536.
A1 OR DISABLED-CONTROL MEDIAN/P95:
  Generic B exactly repeated generic A at both draw and owner medians/P95s.
ACTIVE/WAIT SPLIT:
  Generic active 2,086,560 / 2,106,112 and wait 45,856 / 547,648.
  Prepared active 2,099,232 / 2,118,784 and wait 274,976 / 558,208.
OWNER SPLIT:
  Stage generic 1,031,680 / 1,049,536 versus prepared
  1,044,192 / 1,062,144. Mario remained 500,736 / 500,800 and Fox
  534,272 / 534,336.
OP/PROGRAM BYTES/FALLBACKS:
  91 logical operations: owner begin/end, 20 live list/matrix binds, and 69
  executable ops (17 APPLY / 26 VTX / 26 DRAW). The compiler retained 123
  state actions, 108 decoded vertices, and 36 TRI commands / 54 triangles in
  5,712 bytes inside a 6,144-byte arena. Compiler, preflight, binding, and
  renderer fallbacks were zero. Profile 1 completed 177 prepared owners and
  3,540 lists; profile 2 completed 146 / 2,920.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  Profile-1 experiment: ITCM 15,804; DTCM-BSS 152; main-BSS 1,864,624;
  text/data/BSS 672,016/126,112/1,864,776. The prepared proof added 6,528
  main-BSS bytes over T0. Profile-2 main-BSS was 1,956,208. Arena 6,144.
SEMANTIC TRACE RESULT:
  The 128-frame profile-2 A/B comparer passed every one of 828 events/frame,
  owner/list/command provenance, submit class, final XYZ/ST/RGB15, material/
  texture/source-state hashes, owner state/cache/resolver/global hashes, GX
  boundary, and upload sequence. Upload SHA-256 was
  63114295EA9C027387E25D7C3C56DA83C5DE52AB36C7A9AF82CD5830A118878B.
ORACLE/COUNTER/GX RESULT:
  Oracle 2,484/0/0; 828 triangles; 648/44/126/10 classes; 121/707/121
  batches; 98/730 prepares; terminal GX RAM 715/2,167. Profile-1 upload
  sequence SHA-256 matched across A/B/A:
  BE547A01609587869FD64C3C943B21AB6D6D5810AD1C57B627597E44EE1C4CAD.
CAPTURE RESULT:
  No new visual capture was needed; isolated semantic/GX state was exact.
VERIFIER COMMANDS AND RESULTS:
  GBI/static fixtures passed. An 8-frame prepared forensic smoke passed.
  The 128-frame profile-2 A/B comparer passed. The synchronized profile-1
  A/B/A comparer failed exactly as intended: whole draw regressed 12,640
  ticks and owner improvement was -408 bp; owner P95 also regressed 4.08%.
DECISION: REVERT
  The candidate missed both the 50,000-tick and 25% gates and exceeded the
  owner P95 ceiling. The source/trace plumbing commit c5f815562 remains; the
  prepared activation was not retained in the live tree.
NEXT MEASURED BOTTLENECK:
  Do not retry this 54-triangle owner shape. The next compiler experiment must
  amortize binding/preflight across a materially larger stage or frame slice,
  or fuse setup/transport work in addition to source dispatch.
```

## T2A-TRIANGLE-NOOP — source traversal + state/VTX cost floor

```text
IDEA ID:
  T2A-TRIANGLE-NOOP
HYPOTHESIS:
  Preserving source traversal, branches, state/matrix mutation, VTX decode,
  persistent cache mutation, and owner boundaries while replacing the first
  triangle-submission boundary with one compact count will isolate the live
  source/state/VTX floor.
TARGETED EXCLUSIVE COUNTER:
  Whole draw and stage/Mario/Fox owner walls over one synchronized 128-frame
  profile-1 window.
MEASURED UPPER BOUND:
  Relative to T1B's generic medians, work above this floor is at most 1,126,528
  whole-draw ticks: stage 630,464, Mario 259,136, and Fox 242,048. The floor
  itself is 45.9% of generic draw, 38.9% of stage, 48.2% of Mario, and 54.7%
  of Fox.
LIVE-TREE RECONCILIATION:
  Started from 2bf4df137 after the exact T1B revert and later texture-upload
  acceptance. Preserved all untracked user roadmaps, reviews, prompts, logs,
  and docs/optimization files; decomp remained read-only. Placement guard
  commit a1844bcd3 preceded the experiment.
FILES/FUNCTIONS CHANGED:
  Added a compile-time benchmark-mode identity, dedicated O2/profile-1 target,
  one 24-byte no-op submit boundary, synchronized benchmark-only verifier,
  source timer-start tick-reset recognition, and post-link ITCM assertion.
  Normal targets compile with benchmark mode 0 and contain no runtime branch.
BUILD TARGET/FLAGS:
  smash64ds-battle-playable-coarse-triangle-noop-hwtri; common/scene
  -O2 -mthumb and renderer -O2 -marm; benchmark mode 1.
BASELINE ROM SHA-256:
  T1B same-scene generic profile-1:
  3FF0CB4AD3782C8DF2510A718F740E7A9089275C0912646CF0F4BFE84EAB2FCC.
EXPERIMENT ROM / ELF SHA-256:
  E03841FD3D8A673E98AF7F2B24E03CF04E42D16EF8C4E8D39A9140E7E3A06249 /
  5A32E24CD3B379BABD631865CA9311B6DACB6F6E390AA4135ED411CDDC29F109.
FRAME/LOGIC-TICK WINDOW:
  Warm-up 375; frames 376..503; logic 383..112 with the single source-backed
  ifcommon.c timer-start reset. Frames and all other logic ticks were exact.
A0 MEDIAN/P95:
  Generic T1B draw 2,083,264/2,102,784; stage 1,031,680/1,049,536;
  Mario 500,736/500,800; Fox 534,272/534,336.
B0 MEDIAN/P95:
  TRIANGLE_NOOP draw 956,736/957,056; stage 401,216/401,408;
  Mario 241,600/241,664; Fox 292,224/292,288.
A1 OR DISABLED-CONTROL MEDIAN/P95:
  Compile-time nonvisual floor; normal mode 0 is the disabled control. A
  same-ROM runtime branch was deliberately not added to the shipping loop.
ACTIVE/WAIT SPLIT:
  Loop 1,120,256/1,120,576; present active 959,840/960,192;
  VBlank wait 33,536/40,128; update 125,280/150,528;
  conservation error 0/0 and all residual ratios below 2%.
OWNER SPLIT:
  Stage layer 0 159,552/159,552. Whole-owner values are listed above.
RENDERER NESTED/EXCLUSIVE SPLIT:
  Detailed DL/submit/vertex timers are compile-time absent in profile 1.
  Texture conversion/upload and GX flush were 0/0 by construction.
OP/PROGRAM BYTES/FALLBACKS:
  One 24-byte noinline ARM/O3/ITCM submit counter. Exactly 828 source
  triangles were counted on every frame; no fallback or allocation exists.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  ITCM 11,472 bytes, including 9,712-byte scanner and 24-byte no-op submit;
  text/data/BSS 633,052/126,096/1,797,512; DTCM 0. No arena or per-frame
  allocation. ROM 11,641,856 bytes; ELF 7,968,156 bytes.
SEMANTIC TRACE RESULT:
  Not applicable: this is an explicitly nonvisual feasibility ablation that
  stops before classification and semantic output.
ORACLE/COUNTER/GX RESULT:
  Exact 828 source triangle count per frame. Oracle, submit classes, texture
  preparation, uploads, GX geometry, and flush work were intentionally zero.
CAPTURE RESULT:
  None; the benchmark is nonvisual by contract.
VERIFIER COMMANDS AND RESULTS:
  GBI fixtures passed. ITCM placement historically passed under Windows
  PowerShell 5.1; current repository commands require PowerShell 7 (`pwsh`).
  benchmark-renderer-triangle-noop.ps1 -NoBuild -DelaySeconds 3
  -RendererBenchmarkSamples 128 passed frame/count/conservation gates.
DECISION: KEEP
  Keep the benchmark-only ablation and measurement tooling. It is not a
  production renderer path. The source/state/VTX floor is too large to treat
  the historical scan bucket as fully removable.
NEXT MEASURED BOTTLENECK:
  Run full CPU preparation with final GX writes redirected to a calibrated
  bounded aligned ring sink, then measure the warm no-upload control.
```

## T2B-CPU-PREP-NO-GX — exact CPU policy/derive cost floor

```text
IDEA ID:
  T2B-CPU-PREP-NO-GX
HYPOTHESIS:
  Running exact triangle classification, projection, color/ST derivation,
  matrix/texture policy, and final GX value calculation while redirecting GX
  writes to a bounded aligned ring will separate CPU policy/derive cost from
  GX transport without allowing the compiler to delete prepared values.
TARGETED EXCLUSIVE COUNTER:
  Whole draw and stage/Mario/Fox owner walls over one synchronized 128-frame
  profile-1 window, plus owner sink-word totals and a warm-up-only sink-store
  calibration.
MEASURED UPPER BOUND:
  The raw no-GX draw saves only 27,296 ticks versus T1B generic. Scaling the
  measured 1,024-word/6,720-tick ring calibration to 11,090 words gives about
  72,764 ticks of sink overhead, placing the approximate CPU policy/derive
  floor near 1,983,204 ticks and the non-additive GX-transport/substitution
  upper bound near 100,060 ticks. CPU policy/derive, not GX transport, remains
  dominant.
LIVE-TREE RECONCILIATION:
  Continued from T2A commit 6e039ad82. Preserved all untracked user roadmaps,
  reviews, prompts, logs, and docs/optimization files; decomp remained
  read-only.
FILES/FUNCTIONS CHANGED:
  Added benchmark mode 2 and a dedicated O2/profile-1 target; compile-time GX,
  matrix, texture, color, ST, polygon, begin/end, upload, and refresh sinks;
  a bounded 4 KiB aligned ring; warm-up calibration; exact CPU-prep gates;
  owner sink-word publication; synchronized verifier support; and ITCM/static
  target assertions. Normal targets remain benchmark mode 0.
BUILD TARGET/FLAGS:
  smash64ds-battle-playable-coarse-cpu-prep-no-gx-hwtri; common/scene
  -O2 -mthumb and renderer -O2 -marm; benchmark mode 2.
BASELINE ROM SHA-256:
  T1B same-scene generic profile-1:
  3FF0CB4AD3782C8DF2510A718F740E7A9089275C0912646CF0F4BFE84EAB2FCC.
EXPERIMENT ROM / ELF SHA-256:
  9709D05C907859902E1EDD63F15307ABF041FD3FECEFF86DC041D3E05C7412D5 /
  9403FF50B0B79D1C5F5CA90457DE48C0867F106824B2C9F43EE06DD7BA5A686B.
FRAME/LOGIC-TICK WINDOW:
  Warm-up 194; frames 195..322; logic ticks 202..329. All frame and logic
  ticks were consecutive, with no timer-start reset in the decision window.
A0 MEDIAN/P95:
  Generic T1B draw 2,083,264/2,102,784; stage 1,031,680/1,049,536;
  Mario 500,736/500,800; Fox 534,272/534,336.
B0 MEDIAN/P95:
  CPU_PREP_NO_GX draw 2,055,968/2,423,104; stage 985,472/1,004,672;
  Mario 504,640/504,704; Fox 535,744/535,808.
A1 OR DISABLED-CONTROL MEDIAN/P95:
  Compile-time nonvisual floor; normal mode 0 is the disabled control. No
  runtime branch was added to normal renderer targets.
ACTIVE/WAIT SPLIT:
  Loop 2,240,704/2,800,896; present active 2,059,232/2,426,368;
  VBlank wait 85,664/466,624; update 137,280/435,904;
  conservation error 0/0 and all residual ratios below 2%. Long-window P95
  contains known periodic audio/wallpaper spikes and is not a transport-only
  comparison.
OWNER SPLIT:
  Stage/Mario/Fox ring words were 2,952/4,144/3,994, exactly summing to the
  11,090-word frame total. Calibration-scaled owner sink costs are about
  19,373/27,195/26,211 ticks. Subtracting them gives approximate owner CPU
  floors of 966,100/477,445/509,533 ticks and non-additive generic-to-floor
  upper bounds of 65,581/23,291/24,739 ticks.
RENDERER NESTED/EXCLUSIVE SPLIT:
  Profile-1 detailed DL/submit/vertex timers remain compile-time absent.
  Logical texture work remained exactly two uploads and 36,864 bytes; real
  VRAM/MMIO and GX geometry writes were suppressed.
OP/PROGRAM BYTES/FALLBACKS:
  One 4,096-byte aligned ring, 11,090 stores per frame with bounded wrap, and
  no allocation. Calibration is 1,024 stores/6,720 ticks and occurs once
  before the measured warm window. No fallback exists.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  ITCM 21,284 bytes: CI4 direct 2,344, vertex submit 3,348, triangle submit
  4,368, scanner 9,488, renderer emitted total 19,548. Text/data/BSS
  659,400/126,112/1,862,376; DTCM 0. ROM 11,668,480 bytes; ELF 8,132,156
  bytes. No arena or per-frame allocation.
SEMANTIC TRACE RESULT:
  Not applicable: this is a deliberately nonvisual transport ablation. The
  existing exact CPU decisions and derived values execute and are consumed by
  the ring, but no GX stream is submitted.
ORACLE/COUNTER/GX RESULT:
  Exact 2,484 vertices/828 triangles; batches 121/707/121; texture prepare
  98/730; classes 648/44/126/10; logical uploads 2/36,864 bytes; terminal GX
  polygons/vertices intentionally zero.
CAPTURE RESULT:
  None; the benchmark is nonvisual by contract.
VERIFIER COMMANDS AND RESULTS:
  The synchronized 128-frame CPU-prep benchmark passed frame, identity,
  counter, sink, and conservation gates. ITCM placement and target/static
  fixture assertions passed during iteration.
DECISION: KEEP
  Keep the benchmark-only ablation and measurement tooling. It is not a
  production renderer path. The result rejects GX transport as the primary
  explanation for the remaining 2.08M-tick generic draw cost.
NEXT MEASURED BOTTLENECK:
  Measure the independent warm no-upload control, then frozen owner-scale
  transport. Use those floors plus a stable-run census to select K1.
```

## T2D-WARM-NO-UPLOAD — animated texture critical-path control

```text
IDEA ID:
  T2D-WARM-NO-UPLOAD
HYPOTHESIS:
  After one exact resident frame, retaining normal texture lookup, live key and
  parameter mutation, binding, batches, and geometry while suppressing only
  animated conversion/upload will bound the texture critical path.
TARGETED EXCLUSIVE COUNTER:
  Whole draw and stage/Mario/Fox owner walls over one synchronized 128-frame
  profile-1 window, plus the canonical suppressed upload count/byte sequence.
MEASURED UPPER BOUND:
  Whole-draw median improved 315,584 ticks and stage improved 291,904 ticks.
  Mario/Fox improved only 11,136/12,928 ticks. Animated texture work is a
  first-order stage critical path and exceeds the 150,000-tick immediate-action
  threshold in CODEX_60FPS_FASTEST_PATH_20260712.txt.
LIVE-TREE RECONCILIATION:
  Continued from CPU-preparation floor commit 944e7e08f. Preserved all user
  roadmaps, reviews, prompts, logs, and docs/optimization files; decomp remained
  read-only.
FILES/FUNCTIONS CHANGED:
  Added benchmark mode 4 and a dedicated profile-1 target. On a compatible
  TEXEL0/TEXEL1 fraction refresh after the first resident frame,
  ndsRendererHardwareBindTexture retains live key/hash/params/binding and exact
  geometry but returns before source resolution, conversion, or VRAM upload.
  Added compact suppressed-upload publication and synchronized verifier gates.
BUILD TARGET/FLAGS:
  smash64ds-battle-playable-coarse-warm-no-upload-hwtri; common/scene
  -O2 -mthumb and renderer -O2 -marm; benchmark mode 4.
BASELINE ROM SHA-256:
  T1B same-scene generic profile-1:
  3FF0CB4AD3782C8DF2510A718F740E7A9089275C0912646CF0F4BFE84EAB2FCC.
EXPERIMENT ROM / ELF SHA-256:
  26482D03A04F4450D4020B520E5B9CA90F1C744E93B815CC89B0F88BA8DFBA3B /
  6E3D5C05D4044B6F3DC62E1AB76757957CDFEF7C2862622DC8317B292CB66DB6.
FRAME/LOGIC-TICK WINDOW:
  Warm-up 200; frames 201..328; logic ticks 208..335. All frame and logic
  ticks were consecutive, with no timer-start reset in the decision window.
A0 MEDIAN/P95:
  Generic T1B draw 2,083,264/2,102,784; stage 1,031,680/1,049,536;
  Mario 500,736/500,800; Fox 534,272/534,336.
B0 MEDIAN/P95:
  WARM_NO_UPLOAD draw 1,767,680/2,148,800; stage 739,776/739,968;
  Mario 489,600/489,664; Fox 521,344/521,472.
A1 OR DISABLED-CONTROL MEDIAN/P95:
  Compile-time benchmark control; normal mode 0 is unchanged. The source
  branch is absent from normal renderer builds.
ACTIVE/WAIT SPLIT:
  Loop 2,240,672/2,800,896; present active 1,771,072/2,152,128;
  VBlank wait 332,416/513,216; update 135,968/441,920; conservation error 0/0
  and all residual ratios below 0.8%. Draw P95's 46,016-tick regression versus
  T1B is explained by the independent 383,360-tick wallpaper P95 phase; owner
  stage P95 improves 309,568 ticks.
OWNER SPLIT:
  Stage is the affected owner. Mario/Fox remain within 13K of generic, proving
  that the control did not broadly alter fighter work.
RENDERER NESTED/EXCLUSIVE SPLIT:
  Detailed DL/submit/vertex timers remain compile-time absent in profile 1.
  Real conversion/upload counters are 0/0. The suppressed sequence preserves
  canonical resident or 1x4,096/1x32,768/2x36,864 refresh phases; median/P95
  are 2/2 uploads and 36,864/36,864 bytes.
OP/PROGRAM BYTES/FALLBACKS:
  One cold compile-time cache-refresh branch, no program arena and no per-frame
  allocation. Non-TEXEL1 cache misses and the first resident frame retain the
  normal exact path.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  ITCM 20,936 bytes, renderer-emitted 19,200: CI4 direct 2,344, vertex submit
  3,184, triangle submit 4,208, scanner 9,464. Text/data/BSS
  664,436/126,104/1,858,280; DTCM 0. ROM 11,672,576 bytes; ELF 8,189,832
  bytes. No arena or per-frame allocation.
SEMANTIC TRACE RESULT:
  Not applicable to the deliberately frozen resident pixels. Source topology,
  live texture key/params, triangle preparation, and geometry remain normal.
ORACLE/COUNTER/GX RESULT:
  Exact 2,484 vertices/828 triangles; batches 121/707/121; texture prepare
  98/730; classes 648/44/126/10; zero real uploads. A 32-frame run ended at
  canonical 715/2,167 GX RAM. The later 128-frame terminal probe observed the
  live animation/clipping boundary one triangle behind on one run, while exact
  source geometry and submitted/flush frame conservation remained intact.
CAPTURE RESULT:
  None; animated pixels are intentionally frozen by this nonshipping control.
VERIFIER COMMANDS AND RESULTS:
  GBI fixtures and PowerShell parse checks passed. ITCM placement passed. The
  32-frame exploratory and 128-frame synchronized WARM_NO_UPLOAD benchmark
  passed count, upload-suppression, GX submission, and conservation gates.
DECISION: KEEP
  Keep the benchmark-only control and tooling. It is not a production path.
  Texture conversion/upload is large enough to optimize immediately after the
  shared raw-current kernel rather than deferring until draw falls below 900K.
NEXT MEASURED BOTTLENECK:
  Adopt CODEX_60FPS_FASTEST_PATH_20260712.txt: derive the stable-run census from
  retained semantic data and build one shared K-RAW kernel with owner masks.
  The prior frozen-stream probe is optional because NO_GX already bounded
  transport below 300K.
```

## KRAW-SHARED — shared raw-current run kernel

```text
IDEA ID:
  KRAW-SHARED
HYPOTHESIS:
  Once the generic scanner has established exact live matrix, material,
  texture, alpha, and batch state for the first triangle, one shared owner-
  masked kernel can consume the rest of each immutable adjacent TRI run and
  emit exact raw-current vertices without re-entering the generic command and
  per-triangle setup path.
TARGETED EXCLUSIVE COUNTER:
  Same-ROM profile-1 whole draw and exclusive stage/Mario/Fox walls, with a
  profile-2 dual semantic trace and owner entry/exit state/cache comparison.
MEASURED UPPER BOUND:
  The 128-frame whole-draw median/P95 improvement is 208,672/180,224 ticks.
  The remaining generic raw-current work is bounded by the unaccelerated first
  triangle of each run, 47 state fallbacks, seven vertex-mask fallbacks, and
  all projected classes.
LIVE-TREE RECONCILIATION:
  Continued from T2D commit ad71a4a03 and adopted
  docs/optimization/CODEX_60FPS_FASTEST_PATH_20260712.txt. Preserved all user
  roadmaps, reviews, prompts, logs, and goal files; decomp remained read-only.
FILES/FUNCTIONS CHANGED:
  Added one owner-boundary selector, a shared raw-current run executor, narrow
  textured/untextured GX writers, aggregate accounting, exact generic
  fallback, profile-2 semantic reconstruction, synchronized fast-run export,
  and a generic-versus-fast comparison script. The profile-0 default selects
  all three owners; profile 1/2 retain runtime A/B selection.
BUILD TARGET/FLAGS:
  Same-ROM smash64ds-battle-playable-coarse-hwtri; common/scene -O2 -mthumb,
  renderer -O2 -marm, benchmark mode 0. Canonical uses the same profile-0 O2
  renderer with the all-owner selector initialized at the owner boundary.
BASELINE / EXPERIMENT ROM SHA-256:
  83B6452C94327309F218A2ACE9EB5361C63BC94A8EB699DC14ADC4237CDCDD4E
  for both profile-1 A and B; only the GDB owner selector changed. Promoted
  canonical/shipped ROM:
  19D8C30B18F5973EF7D75F26EF9033AB5FE7C453A6D5EFD88EFBE6848EF3CCFD.
FRAME/LOGIC-TICK WINDOW:
  Decision A/B/A used 32 synchronized frames: A 207..238, B 211..242,
  A 206..237. Promotion used A 212..339 and B 211..338 (128 frames each).
A0 MEDIAN/P95:
  Generic 128: draw 2,067,296/2,407,872; stage 1,017,408/1,035,328;
  Mario 498,368/498,432; Fox 530,688/530,752.
B0 MEDIAN/P95:
  K-RAW 128: draw 1,858,624/2,227,648; stage 999,840/1,017,728;
  Mario 399,872/399,936; Fox 437,440/437,504.
A1 OR DISABLED-CONTROL MEDIAN/P95:
  The 32-frame A repeat was draw 2,177,696/2,441,280 versus initial A
  2,177,664/2,441,280. B was 1,968,032/2,233,344, proving a 209,632-tick
  median win on the same ROM and stable generic control within 32 ticks.
ACTIVE/WAIT SPLIT:
  Generic/K-RAW active median is 2,070,688/1,862,080. Loop median remains
  2,240,704/2,240,640 because VBlank wait absorbs the saved active work.
  Conservation error is 0/0 and draw/present/loop residuals remain below 2%.
OWNER SPLIT:
  Stage improves 17,568 ticks median, Mario 98,496, and Fox 93,248. The shared
  kernel executes 45 runs and 540 triangles every frame: stage 60, Mario 246,
  Fox 234. Fallbacks are bounded at state/vertex/command 47/7/0.
OP/PROGRAM BYTES/FALLBACKS:
  Main-RAM ARM code is 2,392 bytes for the shared executor, 520 bytes for slot
  preparation, and a 4-byte cold fallback wrapper: 2,916 bytes total, with no
  program arena, final-GX cache, expanded water phases, or allocation.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  Canonical ITCM is 20,120/32,768 and renderer-emitted ITCM is 18,384. K-RAW
  remains in main RAM. Canonical main BSS is 1,857,648; DTCM payload is zero
  plus 152 bytes DTCM BSS. No per-frame or arena storage was added.
SEMANTIC TRACE RESULT:
  A same-ROM 32-frame profile-2 generic/fast comparison matched both full-
  frame and owner rolling semantic hashes, event counts, first provenance,
  final v16 XYZ/ST/RGB15/poly/texture/source-state inputs, and exact upload
  sequence with zero mismatches or overflow.
OWNER ENTRY/EXIT STATE/CACHE RESULT:
  Zero mismatches across all three owners for census/topology, runtime state,
  32-slot input/transformed cache and validity/snapshot ownership, resolver,
  global state, camera/DObj/material/light/texture signatures, and semantic
  output. Profile-2 oracle remains 2,484/0/0.
ORACLE/COUNTER/GX RESULT:
  Exact 828 triangles, 648/44/126/10 classes, 121/707/121 batching, 98/730
  texture prepare/reuse, and canonical 715/2,167 GX RAM. The profile-2 upload
  sequence and bytes match generic exactly.
CAPTURE RESULT:
  Canonical capture remains recognizable and detailed. Faster valid capture
  timing measured stage varied coverage 46.7% with a 68px flat run; the fast-
  iteration-only ceiling is now 72px while the full gate remains unchanged:
  artifacts/visibility/2026-07-12_canonical_fast_183402-2942634-p17080.png.
VERIFIER COMMANDS AND RESULTS:
  GBI fixtures, PowerShell parsing, ITCM placement, 32-frame profile-2 exact
  compare, 32-frame same-ROM A/B/A, 128-frame A/B promotion, canonical realtime
  smoke/parity/counter gates, and canonical capture content passed. DevFast's
  earlier 64px fast-only stage ceiling was the only failure and was corrected
  from measured exact output before the final retry.
DECISION: KEEP
  Promote the shared all-owner K-RAW kernel in profile 0. It clears the fastest-
  path 100K whole-draw threshold, improves P95, and preserves exact semantics.
NEXT MEASURED BOTTLENECK:
  The T2D warm-no-upload control already saved 315,584 ticks, so move the
  animated texture conversion/refresh off the critical path next. Coordinate
  its VRAM commit with VBlank because it is also the leading flicker cause.
```

## Per-experiment report template

```text
IDEA ID:
HYPOTHESIS:
TARGETED EXCLUSIVE COUNTER:
MEASURED UPPER BOUND:
LIVE-TREE RECONCILIATION:
FILES/FUNCTIONS CHANGED:
BUILD TARGET/FLAGS:
BASELINE ROM SHA-256:
EXPERIMENT ROM SHA-256:
FRAME/LOGIC-TICK WINDOW:
A0 MEDIAN/P95:
B0 MEDIAN/P95:
A1 OR DISABLED-CONTROL MEDIAN/P95:
ACTIVE/WAIT SPLIT:
OWNER SPLIT:
OP/PROGRAM BYTES/FALLBACKS:
ITCM/DTCM/BSS/STACK/ARENA DELTA:
SEMANTIC TRACE RESULT:
ORACLE/COUNTER/GX RESULT:
CAPTURE RESULT:
VERIFIER COMMANDS AND RESULTS:
DECISION: KEEP / REWORK / REVERT
NEXT MEASURED BOTTLENECK:
```

## 2026-07-12 - T2D pair conversion and VBlank refresh safety

```text
IDEA ID: T2D-PAIR-VBLANK
HYPOTHESIS:
  Split conversion from upload, replace the changing 8 KiB/16-plane CI4 table
  with exact palette-pair RGB/coverage state, and stage both current payloads
  so texture VRAM is never remapped during active scanout.
TARGETED EXCLUSIVE COUNTER:
  texture convert, queue/stage, VBlank commit, outside-window/fallback.
MEASURED UPPER BOUND:
  Retained WARM_NO_UPLOAD control saved 315,584 whole-draw ticks.
LIVE-TREE RECONCILIATION:
  Started from K-RAW commit 15f6b78a8; preserved all user untracked files and
  reconciled the concurrent live-input correction separately.
FILES/FUNCTIONS CHANGED:
  nds_renderer.c/.h texture pair conversion, two-entry refresh queue/oracle;
  nds_platform.c post-wait commit; benchmark/static verifier plumbing.
BUILD TARGET/FLAGS:
  battle_playable coarse/forensic/canonical; common/scene O2 Thumb, renderer O2
  ARM with the existing targeted O3/ITCM path; benchmark mode 0.
BASELINE ROM SHA-256:
  Localization ROM 7E820AAC2D4D884830910212D17EE0BC8B8ABD26DD58A6C1C49E342979CE62AD.
EXPERIMENT ROM SHA-256:
  128-frame coarse D944503003D2B44F798D529769BA3ED4BF687A64856F1E42F526099B7D55E461;
  canonical 0C564D4822011FCAB9DC5CA4F52C5F8EBB7339354BFD3FBA93A035C5F90F2663.
FRAME/LOGIC-TICK WINDOW:
  128 synchronized warm frames 208..335; logic ticks 215..342.
A0 MEDIAN/P95:
  Eight-frame localization before edits: draw 2,199,168/2,224,384; conversion
  201,568/226,752; synchronous remap/DMA 33,696/33,728.
B0 MEDIAN/P95:
  128-frame retained path: draw 2,179,072/2,199,808; conversion
  190,528/208,704; distinct-row stage/queue 18,304/21,184.
A1 OR DISABLED-CONTROL MEDIAN/P95:
  No same-ROM A1 was retained because the independent live-input source fix
  changed gameplay concurrently; no 50K optimization claim is made.
ACTIVE/WAIT SPLIT:
  loop 2,800,896/2,834,432; active 2,216,160/2,236,800; VBlank wait
  440,416/486,272; post-VBlank 33,600/33,856; conservation error 0/0.
OWNER SPLIT:
  stage/Mario/Fox 943,840/964,864, 399,552/399,616, 437,984/438,656.
OP/PROGRAM BYTES/FALLBACKS:
  Logical outputs remain 4,096 + 32,768 bytes. Physical persistent staging is
  4 KiB plus at most 64 distinct 256-byte rows (16 KiB) and a 128-byte row map.
  Queue/commit is 2/36,864; outside/fallback 0/0; no phase-frame cache/program.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  Canonical ITCM 20,088/32,768; main BSS 1,871,280 (+13,632 versus K-RAW after
  pair-table reduction/compact staging); DTCM BSS 152. Profile 2 omits staging.
  Its 4 KiB-granular taskman fallback selects 0x14c000 and leaves 145,472 bytes
  after BGM, 14,400 above the 128 KiB reserve.
SEMANTIC TRACE RESULT:
  Profile 2 checks all 18,432 changed-frame RGB15/coverage outputs against the
  old exact blend formula with zero mismatch; semantic events remain 828/0.
ORACLE/COUNTER/GX RESULT:
  Oracle 2,484/0/0; triangles/classes 828 and 648/44/126/10; batch 121/707/121;
  prepare/reuse 98/730; exact 36,864 bytes. Profile 2 uses synchronous refresh
  only as the independent oracle. Live fighter motion makes GX RAM
  position-dependent (canonical sample 695/2,119) without geometry loss.
CAPTURE RESULT:
  Canonical image is recognizable with flowers, fence, pond, tree, and fighters;
  live-input camera crops retain 46.7% bush, 24.9% stage, and 24.2% pond detail.
VERIFIER COMMANDS AND RESULTS:
  128-frame coarse pass, 8-frame forensic pair/oracle pass, GBI fixtures,
  architecture, registry, ITCM, canonical smoke/parity, and P1 memory passed. Fast capture's
  stale flat-run limits were corrected from repeated valid live-input frames.
DECISION: KEEP VBlank safety / REWORK performance
  The path removes the diagnosed active-scanout remap and remains exact, but
  does not meet the 50K performance gate or the 80K-100K texture target.
NEXT MEASURED BOTTLENECK:
  Split small/large conversion and emit two aligned RGB15 pixels per 32-bit
  store; retain only if a same-ROM 128-frame control clears the keep gate.
```

## 2026-07-12 - M2A immutable direct TRI-run records

```text
IDEA ID: M2A-DIRECT-TRI-RUNS
HYPOTHESIS:
  Predecode immutable adjacent TRI topology once per scene, prepare the union
  of required live slots once, and execute the whole eligible K-RAW remainder
  without repeated source decode/state checks.
TARGETED EXCLUSIVE COUNTER:
  Whole draw and exclusive stage/Mario/Fox owner walls.
MEASURED UPPER BOUND:
  The retained K-RAW path still paid per-command decode and slot preparation
  for 330 adjacent commands / 540 raw-current triangles.
LIVE-TREE RECONCILIATION:
  Started from texture/input commits 2c4f62a70/0b12a8737; preserved all user
  roadmaps, reviews, prompts, logs, and goal files. Decomp stayed read-only.
FILES/FUNCTIONS CHANGED:
  nds_renderer.c adds a scene-lifetime immutable topology cache, exact compact
  indices/masks, one direct remainder executor, and cold generic fallback.
BUILD TARGET/FLAGS:
  Profile-1 coarse O2 ARM renderer, O2 Thumb scene/common, benchmark mode 0;
  profile-2 forensic used the same runtime selector for isolated A/B.
BASELINE ROM SHA-256:
  Retained pre-change canonical 0C564D4822011FCAB9DC5CA4F52C5F8EBB7339354BFD3FBA93A035C5F90F2663.
EXPERIMENT ROM SHA-256:
  Same-ROM profile-1 A/B CFFC7159CCC87A6EF0EF1282E4931ACB0F74CF106B585DF5384798C4BF693866.
FRAME/LOGIC-TICK WINDOW:
  Generic frames 209..336 / logic 216..343; direct frames 214..341 / logic
  221..348; 128 synchronized warm frames each.
A0 MEDIAN/P95:
  Same-ROM generic draw 2,376,000/2,396,160; stage 957,056/976,128; Mario
  494,656/494,720; Fox 526,304/527,040.
B0 MEDIAN/P95:
  Direct K-RAW draw 2,118,688/2,138,816; stage 934,400/953,408; Mario
  372,416/372,544; Fox 413,856/414,656.
A1 OR DISABLED-CONTROL MEDIAN/P95:
  The pre-change retained K-RAW draw was 2,179,072/2,199,808. The direct table
  saves 60,384/60,992 ticks against that live baseline and clears the 50K gate.
ACTIVE/WAIT SPLIT:
  Direct loop 2,800,832/2,834,304; active 2,155,488/2,175,936; conservation
  error 0/0. VBlank wait absorbs the active gain at the current frame cadence.
OWNER SPLIT:
  Stage/Mario/Fox are 934,400/372,416/413,856 median. The existing 45/540
  owner split remains 60/246/234 with bounded fallbacks 47/7/0.
OP/PROGRAM BYTES/FALLBACKS:
  One 2,908-byte main-RAM direct executor consumes 330 compact topology
  records. Fixed topology storage is exactly 8 KiB; no arena or allocation.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  Profile-1 ITCM stays 20,088; main BSS is 1,880,016 (+8,192); DTCM BSS 152.
  A broader first-command variant reached 632 fast triangles but displaced hot
  code and made the absolute fast draw about 46K slower, so it was removed.
SEMANTIC TRACE RESULT:
  Eight-frame isolated profile-2 generic/direct traces matched all 828 events,
  provenance, owner state/cache/resolver/signatures, and uploads exactly.
ORACLE/COUNTER/GX RESULT:
  Oracle 2,484/0/0; exact 828, 648/44/126/10, 121/707/121, 98/730, and 36,864
  upload bytes. Direct path remains 45 runs / 540 triangles.
CAPTURE RESULT:
  P1Gate capture 2026-07-12_canonical_fast_220350-6133192-p35444.png passes
  independent stage/fighter/detail checks on both sampled presentations; the
  direct path submits no duplicate GX stream.
VERIFIER COMMANDS AND RESULTS:
  128-frame profile-1 A/B, 8-frame profile-2 semantic comparison, DevFast,
  P1Gate, Boundary, GBI, docs, architecture, and registry checks passed. Short
  live-input capture allows 50% / mean 45 pairwise motion while independently
  gating both frames; normal realtime stays 30% / 32.
DECISION: KEEP
  The exact direct remainder table clears the requested 50K live-baseline gate
  while reducing both median and P95.
NEXT MEASURED BOTTLENECK:
  Move to the 126-triangle stage no-Z class/direct owner kernel. Packed RGB15
  pair stores regressed changed-frame conversion to 228.8K and were removed.
```

## 2026-07-12 - rejected adjacent stage no-Z kernel

```text
IDEA ID: M2B-ADJACENT-NO-Z
HYPOTHESIS:
  Reuse the immutable TRI topology table for adjacent stage no-Z commands,
  prepare each unique transformed/projected/color/ST slot once, and emit a
  narrow projected loop with the exact synthetic painter-depth sequence.
TARGETED EXCLUSIVE COUNTER:
  Whole draw and stage owner median/P95 in one profile-1 ROM, with retained
  mode 3 as raw-only control and temporary mode 4 adding the no-Z candidate.
MEASURED UPPER BOUND:
  Dream Land has 93 no-Z run starts but only 33 adjacent remainder triangles.
  Remainder-only mode saved 13,504/9,664 draw ticks and 13,824/10,176 stage
  ticks. Extending the narrow vertex emitter to all 126 no-Z triangles still
  saved only 12,384/8,832 draw and 12,896/9,216 stage ticks.
LIVE-TREE RECONCILIATION:
  Started from collision checkpoint 12bf51e34 and retained direct raw commit
  63384b41d. User roadmaps/reviews/logs remained untouched; decomp was read
  only and sm64-nds no-Z painter behavior was checked before the experiment.
FILES/FUNCTIONS CHANGED:
  Temporary changes only in nds_renderer.c/header and benchmark runtime-mode
  ranges. They added exact stage-only eligibility, projected slot preparation,
  narrow textured/untextured emitters, profile-2 semantic construction, and a
  9.5 KiB shared topology table. All production changes were removed.
BUILD TARGET/FLAGS:
  smash64ds-battle-playable-coarse-hwtri; profile 1; common/scene O2 Thumb,
  renderer O2 ARM; benchmark mode 0.
BASELINE / EXPERIMENT ROM SHA-256:
  Remainder-only same ROM B959AED668B11EDC4D46607279DE1EF2B34D9FB766BABEB3B07D60F0DD85903A.
  All-no-Z same ROM 3E2654E38F15652F3FC6AA66E36A9DF5D7C2EB7130ED7C98CFA923D55A4E53CF.
FRAME/LOGIC-TICK WINDOW:
  Three synchronized 16-frame remainder A/B/A windows and one synchronized
  16-frame all-no-Z B/A pair after at least 205 warm frames; conservation 0/0.
A0 MEDIAN/P95:
  Remainder raw-only control draw 2,128,640/2,185,152; stage
  924,416/980,544 on repeat.
B0 MEDIAN/P95:
  Remainder candidate draw 2,115,136/2,175,488; stage 910,528/970,432.
A1 / ALL-NO-Z CONTROL MEDIAN/P95:
  All-no-Z ROM raw-only control draw 2,104,256/2,160,704; stage
  913,152/969,344. Candidate draw 2,091,872/2,151,872; stage
  900,256/960,128.
ACTIVE/WAIT SPLIT:
  All-no-Z candidate active 2,128,480/2,189,248; wait 472,544/541,568;
  all frame/logic samples consecutive and conservation error 0/0.
OWNER SPLIT:
  Candidate stage 900,256/960,128; Mario 375,648/375,680; Fox
  418,208/418,496. The no-Z change was stage-only.
OP/PROGRAM BYTES/FALLBACKS:
  Remainder coverage was 6 runs / 33 triangles. The exact first-triangle
  extension covered the other 93 triangles but retained generic run setup.
  Raw K-RAW remained 45/540; bounded fallback changed 47/7/0 -> 30/7/0.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  First-triangle specialization grew ITCM 20,088 -> 21,052 (+964); temporary
  topology BSS grew by 1,536 bytes and remained below the 16 KiB proof cap.
SEMANTIC TRACE RESULT:
  Profile 2 was intentionally skipped: the 16-frame gain missed both the
  50K/25% keep gate and the roadmap's >=100K or >=5% forensic-entry cadence.
ORACLE/COUNTER/GX RESULT:
  Profile 1 retained exact 828 triangles, 648/44/126/10 classes,
  121/707/121 batches, 98/730 texture prepare/reuse, and upload phases.
CAPTURE RESULT:
  None; the candidate failed the performance gate before visual acceptance.
VERIFIER COMMANDS AND RESULTS:
  Incremental compile, ITCM placement, 8-frame smoke, and synchronized
  profile-1 A/B/A passed. Candidate production source was then fully reverted.
DECISION: REVERT
  Adjacent no-Z topology is too fragmented, and generic first-run setup still
  dominates. Do not retry this shape or add more emitter variants.
NEXT MEASURED BOTTLENECK:
  A future stage cut must fuse the 93 short run starts through direct owner
  records/live binding, or attack the independently measured texture wall.
```

## 2026-07-12 - exact DObj index and affine world composition

```text
IDEA ID: M3-DOBJ-AFFINE-INDEX
HYPOTHESIS:
  The newly exposed 330K matrix-preparation wall is dominated by repeated
  linear searches through the 128-entry frame cache and generic 4x4 products
  for source DObj world matrices that are exactly affine.
TARGETED EXCLUSIVE COUNTER:
  Profile-1 matrix preparation plus whole draw and stage/Mario/Fox exclusive
  owner time. Profile 2 compares every affine result with the former generic
  product in the same execution.
MEASURED UPPER BOUND:
  Forensic census is camera cache 73/1/0 and DObj cache 64/107/0. All 107 live
  hierarchy products are affine. Material preparation is only 15.6K ticks,
  while initial matrix preparation was 330.3K.
LIVE-TREE RECONCILIATION:
  Started at collision checkpoint 12bf51e34 with K-RAW/direct table 63384b41d.
  User roadmaps/reviews/logs stayed untouched and BattleShip objdisplay/lbcommon
  matrix order and fighter-parts behavior were read from decomp without edits.
FILES/FUNCTIONS CHANGED:
  reloc_backend_renderer_dl.c indexes the unchanged 128-entry cache through a
  256-byte half-full open-address table and uses the exact affine helper only
  for parent-chain world composition. nds_renderer.c retains the generic
  fallback and a profile-2-only generic-result oracle. Diagnostics/verifier
  plumbing publishes and requires affine samples/mismatches/max delta.
BUILD TARGET/FLAGS:
  smash64ds-battle-playable-coarse-hwtri; profile 1; common/scene O2 Thumb,
  renderer O2 ARM; benchmark mode 0 and retained fast-run mode 3.
BASELINE / EXPERIMENT ROM SHA-256:
  Baseline 5A1C78FA2F6B96C95610E979232F598B696AFE5C189E38AF684E85C4E20C9149.
  Final 128-frame profile-1 candidate
  CBB1710EB0C360671D0A23247CB0EC429FD679723EDDA1A1E10704CE54C38D24.
FRAME/LOGIC-TICK WINDOW:
  Baseline and staged candidates use 32 synchronized frames after 206-209 warm
  frames. Final candidate uses 128 frames 209..336 / logic 216..343; timer
  resets and conservation error are 0/0.
A0 MEDIAN/P95:
  Baseline draw 2,126,752/2,169,600; matrix 330,304/330,624; stage
  934,464/976,704; Mario 376,320/376,448; Fox 419,200/419,648.
B0 MEDIAN/P95:
  Index-only draw 2,098,240/2,138,560 and matrix 298,976/299,648. Combined
  draw 2,057,376/2,098,880 and matrix 254,528/254,848, saving
  69,376/70,720 whole-draw and 75,776/75,776 matrix ticks.
A1 OR FINAL 128-FRAME MEDIAN/P95:
  Draw 2,067,712/2,088,064; matrix 256,256/256,704; stage
  914,464/933,760; Mario 359,424/359,552; Fox 396,288/396,992.
ACTIVE/WAIT SPLIT:
  Loop 2,767,392/2,804,800; active 2,104,416/2,125,120; wait
  231,808/548,928; draw/present/loop residual ratios 67/74, 7/8, 4/5 basis
  points with 0/0 conservation error.
OWNER SPLIT:
  The 32-frame combined candidate saves stage 29,024/30,272, Mario
  17,472/17,472, and Fox 23,808/23,936 ticks. The final 128-frame values are
  listed above; selected event and fast-run ownership remain exact.
OP/PROGRAM BYTES/FALLBACKS:
  No prepared program or GX stream was added. The 256-slot u8 index is at most
  half full for 128 exact DObj entries. K-RAW remains 45/540 with stage/Mario/
  Fox 60/246/234 and 47/7/0 bounded fallbacks.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  Profile-1 ITCM remains 20,088 and DTCM BSS 152. Main BSS is 1,880,304,
  +288 bytes for the index, three oracle words, and alignment. No scene arena
  or stack allocation was added.
SEMANTIC TRACE RESULT:
  Profile 2 exercised 107 affine products and compared every 16-cell result
  with the former generic multiply: 107/0/0 samples/mismatches/max delta.
  Owner state/cache/resolver accounting and the 828-event semantic stream ran
  in isolation; no second GX stream was submitted.
ORACLE/COUNTER/GX RESULT:
  Profile-2 vertex oracle 2,484/0/0; exact 828 triangles, 648/44/126/10
  classes, 121/707/121 batches, 98/730 texture prepare/reuse, 36,864 upload
  bytes, cache 73/1/0 and 64/107/0, and zero reject/overflow.
CAPTURE RESULT:
  DevFast synchronized capture
  2026-07-12_canonical_fast_233629-6422977-p4736.png passes full-screen,
  stage/fighter region, horizontal-detail, and paired-frame motion gates.
VERIFIER COMMANDS AND RESULTS:
  GBI/docs/architecture/registry/dry-run/diff checks, rebuilt forensic profile
  2, rebuilt DevFast, and Boundary modes 161/162 pass. P1Gate and Boundary mode
  163 repeatably fail the existing 9,000-tick natural-motion gate after the
  preceding source collision-callback activation; renderer exactness remains
  green. Full Regression is intentionally skipped by user request.
DECISION: KEEP
  The combined exact slice clears the 50K whole-draw gate, improves P95, and
  adds only 288 bytes of bounded state.
NEXT MEASURED BOTTLENECK:
  Matrix preparation still costs 256K. The 8-frame profile-2 churn census shows
  stage DObj signatures constant (0 changes / 1 distinct) while Mario/Fox
  change every frame; measure a live-signature persistent stage-world cache
  before a larger owner compiler. Wallpaper is independently about 383K.
```

## 2026-07-13 - exact persistent Dream Land world matrices

```text
IDEA ID: M4-STAGE-WORLD
HYPOTHESIS:
  Dream Land's source DObj transforms and parent topology are stable after
  update, so exact source-bit/parent-generation validation can reuse its world
  matrices across frames while fighter matrices remain frame-local.
TARGETED EXCLUSIVE COUNTER:
  Profile-1 matrix preparation, stage owner, and whole draw median/P95. Profile
  2 independently recomputes every reused selected world and compares all 16
  20.12 cells without submitting a second GX stream.
MEASURED UPPER BOUND:
  The live stage hierarchy has 57 reusable nodes feeding 42 selected world
  outputs. Cache-off matrix prep is 255,872/256,256 ticks and the stage owner is
  937,920/970,688, leaving a useful whole-draw cut without touching fighters.
LIVE-TREE RECONCILIATION:
  Started at 77b6f4258 after the source-collision choreography checkpoint.
  Existing user roadmaps, reviews, logs, prompts, and decomp trees were
  preserved; decomp remained read-only.
FILES/FUNCTIONS CHANGED:
  reloc_backend_renderer_dl.c adds a scene-generation-bounded stage metadata
  cache, exact float-bit/XObj/parent matching, stage-only selection, and the
  profile-2 uncached matrix shadow. The 57 worlds occupy spare upper slots in
  the existing 128-entry world cache. Diagnostics, startup declarations,
  taskman reset, verifier markers, and static fixtures publish and gate it.
  nds_renderer.c/header right-size measured P1 bounds to 48 texture entries,
  40 pending matrix tests, and 832 semantic events.
BUILD TARGET/FLAGS:
  smash64ds-battle-playable-coarse-hwtri; profile 1; common/scene O2 Thumb,
  renderer O2 ARM; benchmark mode 0. Forensic profile 2 remains Os/ARM.
BASELINE / EXPERIMENT ROM SHA-256:
  Cache-off 9CC93F4733F88468C8B9F37B26132704451C8B820BFEA778EA43EB56A3884030.
  Cache-on B742DDC129E3B12EA570C5E799B8EC3CFFF9530C71BFDCC761A0368B78F8B3F6.
  Final canonical profile-0 candidate
  C62C20F409B5A1F7A553462953EC48B6F9135E5AFC87EA24D226C2542C7E970F.
FRAME/LOGIC-TICK WINDOW:
  Cache-off uses 128 consecutive frames 193..320 / logic 200..327 after 192
  warm frames. Cache-on uses frames 216..343 / logic 223..350 after 215 warm
  frames. Both use the same O2 flags, emulator identity, neutral input, and
  zero timer reset/conservation error; windows are independent and consecutive.
A0 MEDIAN/P95:
  Cache-off draw 2,323,008/2,355,712; matrix 255,872/256,256; stage
  937,920/970,688; Mario 480,320/480,384; Fox 506,816/507,328.
B0 MEDIAN/P95:
  Cache-on draw 2,263,616/2,280,512; matrix 188,544/215,168; stage
  874,496/888,512; Mario 482,432/502,336; Fox 508,672/515,456.
WHOLE-DRAW / OWNER DELTA:
  Whole draw improves 59,392 median and 75,200 P95, clearing the 50K keep gate.
  Matrix prep improves 67,328/41,088 and stage improves 63,424/82,176. This is
  a stage-only cut; fighter variation is sampling noise, not claimed savings.
ACTIVE/WAIT SPLIT:
  Cache-off active 2,359,680/2,393,088 and wait 310,048/529,152. Cache-on active
  2,300,320/2,317,568 and wait 351,584/463,040. Loop medians remain VBlank-
  paced at 2,800,832; P95 improves 2,834,816 -> 2,834,304. Conservation is 0/0.
CHURN / SIGNATURE RESULT:
  The prior owner census measured stage DObj signature churn 0 changes / 1
  distinct value while fighters changed every frame. The exact live matcher
  now observes 57 static-node hits per forensic frame with no rejected source
  shape or capacity overflow.
OP/PROGRAM BYTES/FALLBACKS:
  No prepared program or GX stream was added. Metadata is 64 * 72 = 4,608
  scene-heap bytes; matrix storage reuses the spare upper 64 entries already
  allocated by the 128-entry frame cache. This is below the 16 KiB proof cap.
  Camera-facing kinds 33..40, direct kind 1, fighter-parts kind 0x4B, live
  vector tracks, and more than five XObjs fall back to uncached exact build.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  Canonical ITCM/DTCM-BSS is 20,088/152; main/main-RW/main-BSS is
  637,336/124,052/1,875,504. Forensic is 18,584/152 and
  580,824/123,916/1,942,800. The fast forensic memory gate retains 202,416
  bytes before and 136,880 after the resident 65,536-byte BGM ring.
SEMANTIC TRACE RESULT:
  Profile 2 reports stage-world 57/0/reject0/overflow0/oracle42/0. Its 832-event
  bound retains all 828 triangle events with zero overflow. Owner state/cache
  tracing remains isolated; the shadow recomputation never reaches GX.
ORACLE/COUNTER/GX RESULT:
  Vertex oracle 2,484/0/0; affine 158/0/0; exact 828 triangles and
  648/44/126/10 classes; 121/707/121 batches; 98/730 prepare/reuse; 36,864
  uploaded bytes; DObj dynamic cache 30/50/0; zero matrix-cache mismatch.
  Realtime and accelerated Boundary divider coverage is 1,404/0/0/0/0. The
  first accelerated frame separately gates 0/57 stage hits/misses before reuse.
CAPTURE RESULT:
  P1Gate synchronized capture
  2026-07-13_canonical_fast_012843-3972323-p5864.png passes full-screen,
  stage/fighter-region, horizontal-detail, pairwise-motion, and exact ROM
  parity at 11,683,840 bytes / C62C20F409B5A1F7A553462953EC48B6F9135E5AFC87EA24D226C2542C7E970F.
VERIFIER COMMANDS AND RESULTS:
  Focused fast profile 2, realtime profile 2, matched 128-frame profile-1 A/B,
  128-frame canonical profile 0, docs, GBI fixtures, and diff checks pass.
  Rebuilt DevFast, P1Gate (opening/canonical/fast/lifecycle/CPU), and Boundary
  modes 161/162/163 pass. Full Regression is intentionally skipped at the
  user's requested fast-iteration cadence.
DECISION: KEEP
  Exact shadow equality and the 59,392/75,200 whole-draw reduction clear the
  semantic and >=50K/P95 keep gates while preserving the memory reserve.
NEXT MEASURED BOTTLENECK:
  Compile larger direct stage-owner records that bind live matrices/materials/
  textures and fuse short run setup. Do not retry standalone no-Z specialization.
```

## 2026-07-13 - direct compact animated-water rows

```text
IDEA ID: M5-CI4-COMPACT-DIRECT
HYPOTHESIS:
  A warm 128x128 animated CI4 refresh can write its exact unique rows directly
  into the existing VBlank buffer, removing full-scratch repeated-row expansion
  and immediate unique-row restaging without caching a phase frame.
TARGETED EXCLUSIVE COUNTER:
  Profile-1 texture conversion, queue/staging, stage owner, active work, and
  whole draw median/P95. Profile 2 remains the independent pixel/semantic oracle.
MEASURED UPPER BOUND:
  Control conversion plus staging is 207,712/224,448 ticks; the retained VBlank
  commit is already outside active draw and is intentionally unchanged.
LIVE-TREE RECONCILIATION:
  Started at 47e5c593c after exact persistent stage worlds. User roadmaps,
  reviews, prompts, logs, and decomp trees were preserved; decomp stayed read-only.
FILES/FUNCTIONS CHANGED:
  nds_renderer.c makes the CI4 converter return whether its shared exact
  representative loop targeted compact staging, fills the existing row map in
  first-representative order, and skips the old scratch-to-staging pass only on
  exact-size warm large refreshes. Cold/small/forensic fallbacks are unchanged.
  The GBI fixture pins direct compact output plus the full-scratch fallback.
BUILD TARGET/FLAGS:
  smash64ds-battle-playable-coarse-hwtri profile 1 and canonical profile 0;
  common/scene O2 Thumb, renderer O2 ARM, benchmark mode 0, fast-run mode 3.
BASELINE / EXPERIMENT ROM SHA-256:
  Compile-time compact-off control D1B2C521D2CF71670DDE29437D304272162D7E5B12A89270A59A26C1F35200D4.
  Retained profile-1 DEBA0442CC27F67BE74E950CAC248098CA74AB813A35A11749717411287251CA;
  final profile-0 D8A03238B29DB483BB6083F176C2A104C2FFD834BED51B6542AD79103B0DE582.
FRAME/LOGIC-TICK WINDOW:
  Both profile-1 arms use the identical 128 warm frames 220..347 / logic
  227..354, neutral input, the same emulator/config, zero timer resets, and
  zero conservation error. Their upload-sequence SHA-256 is identical.
A0 MEDIAN/P95:
  Compact off: draw 2,001,600/2,033,664; active 2,038,368/2,070,784; stage
  850,016/864,000; convert 190,400/204,288; stage/queue 17,312/20,160.
B0 MEDIAN/P95:
  Compact on: draw 1,970,304/2,002,880; active 2,006,944/2,039,872; stage
  817,184/831,488; convert 168,064/184,576; stage/queue 4,416/4,480.
WHOLE-DRAW / OWNER DELTA:
  Draw saves 31,296/30,784, active saves 31,424/30,912, stage saves
  32,832/32,512, and conversion plus staging saves 35,232/35,392 ticks.
ACTIVE/WAIT SPLIT:
  Control wait 80,576/414,400; retained wait 110,464/441,280. Both remain
  VBlank-paced at loop 2,240,672-2,240,704 median with 0/0 conservation error.
OWNER SPLIT:
  Only Dream Land owns the changing 32 KiB pond refresh; Mario/Fox remain
  359,872/380,096 and 395,872/402,688 in the retained 128-frame window.
OP/PROGRAM BYTES/FALLBACKS:
  No prepared program, expanded phase frame, new cache, or GX stream was added.
  Queue/upload remains 2/36,864 with zero outside-window/fallback work and VBlank
  lines 192..207. K-RAW remains 45/540 and 47/7/0 bounded fallbacks.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  Canonical/coarse ITCM grows 288 bytes, 20,088 -> 20,376; forensic remains
  18,584. Main/DTCM BSS, stack, texture staging, and scene arena are unchanged.
SEMANTIC TRACE RESULT:
  Profile 2 remains an isolated full-scratch decoder and retains all 828 events
  with zero overflow. Its 18,432 direct CI4 pixels compare with the old exact
  blend formula with zero mismatch; no second GX stream is submitted.
ORACLE/COUNTER/GX RESULT:
  Profile-2 vertex oracle 2,484/0/0; exact 828 triangles, 648/44/126/10 classes,
  121/707/121 batches, 98/730 prepare/reuse, and 36,864 upload bytes. Profile 1
  reports 2 queued/committed refreshes, zero fallback, and the identical upload
  sequence in both A/B arms.
CAPTURE RESULT:
  P1Gate capture 2026-07-13_canonical_fast_021218-4461378-p24268.png passes
  full-screen, stage/fighter region, horizontal-detail, and paired-motion gates.
VERIFIER COMMANDS AND RESULTS:
  Matched 128-frame off/on profile 1, 32-frame profile 0, 8-frame forensic,
  docs/architecture/registry/GBI/dry-run/diff checks, rebuilt DevFast, P1Gate,
  and Boundary 161/162/163 pass. Full Regression is skipped by user request.
DECISION: KEEP AS MEASURED TEXTURE SUBCUT
  It removes 17.0% of the targeted conversion/staging wall with no BSS, exact
  upload identity, and improved P95. This is not the prepared-owner activation;
  that experiment still must clear its independent 50K whole-draw/25% gate.
NEXT MEASURED BOTTLENECK:
  Compile direct coarse records for the largest immutable stage owner, binding
  live matrices/materials/textures through the exact submission path. Do not
  retry standalone no-Z or cache fully expanded water phases.
```

## 2026-07-13 - rejected fixed-shape Dream Land layer-0 scan compiler

```text
IDEA ID: M6-STAGE0-FIXED-SCHEDULE
HYPOTHESIS:
  Replace Dream Land layer 0's 297-command immutable source traversal with 26
  fixed state/VTX/TRI records while retaining the exact triangle helper. The
  candidate must save 50,000 whole-draw ticks or 25% of layer 0.
TARGETED EXCLUSIVE COUNTER:
  Same-ROM profile-1 whole draw, stage owner, and layer-0 wall over 128 warm
  frames; isolated profile-2 semantic and owner-state/cache equality.
MEASURED UPPER BOUND:
  The exact compiler removes source validation, branch dispatch, 90 syncs, 21
  ends, and the generic opcode switch, but saves only 8,448 ticks. Source scan
  is 3.22% of the live 262,592-tick layer-0 wall in this architecture.
LIVE-TREE RECONCILIATION:
  Started from 95b2a426e2 after direct compact CI4 rows. Preserved all untracked
  user roadmaps, reviews, logs, prompts, and goal files; decomp stayed read-only.
FILES/FUNCTIONS CHANGED:
  Temporary renderer/compiler, adapter selection, mode-4 benchmark plumbing,
  and exact diagnostic counters in nds_renderer.c/.h, nds_startup.h,
  reloc_backend_renderer_dl.c, reloc_backend_movement.c, and the fast-run
  benchmark/comparison/verifier scripts. All production changes were removed.
BUILD TARGET/FLAGS:
  Profile-1 coarse O2 Thumb common/scene and O2 ARM renderer; profile-2 forensic
  O2 Thumb common/scene and O2 ARM renderer. Runtime modes 3/4 share each ROM.
BUILD COUNT / TIME TO FIRST MEASUREMENT:
  One initial incremental profile-1 build, two diagnostic renderer rebuilds,
  and one profile-2 incremental build. The first executing 8-frame result came
  after correcting the immutable-span cap to match the generic renderer.
PROFILE-1 ROM SHA-256:
  2C1CDE0817214A4180D4C3A1492C9A88A22BE271C648959672B705051908AEA5.
FRAME/LOGIC-TICK WINDOWS:
  Mode 3 frames 209..336 / logic 216..343; mode 4 frames 215..342 / logic
  222..349. Both contain 128 consecutive warm frames with 0/0 conservation
  error and identical O2 flags/emulator identity.
A0 MEDIAN/P95:
  Mode 3 draw 1,979,680/1,996,928; stage 820,992/837,184; layer 0
  262,592/262,720; Mario 361,728/361,856; Fox 398,752/399,488.
B0 MEDIAN/P95:
  Mode 4 draw 1,971,232/1,988,544; stage 812,544/828,608; layer 0
  254,144/254,272; Mario 361,728/381,504; Fox 398,816/405,504.
WHOLE-DRAW / OWNER DELTA:
  Draw, stage, and layer 0 each improve 8,448 median ticks. Layer 0 improves
  3.22%, far below its 25% gate; draw improves 0.43%, far below 50,000 ticks.
  Candidate draw/stage/layer P95 each improve about 8.4K. Fighter P95 variation
  is unrelated sampling noise because the candidate is stage-layer-0-only.
ACTIVE/WAIT SPLIT:
  Mode 3 active 2,016,320/2,033,984 and mode 4 active
  2,007,936/2,025,600. VBlank wait absorbs the small active-work delta; all
  draw/present/loop residuals stay below 0.8%.
OP/PROGRAM BYTES/FALLBACKS:
  Exact census: 20 root lists plus one branch child, 26 records, 123 live state
  commands, 108 predecoded vertices, 36 TRI commands, and 54 no-Z triangles.
  Program storage is 5,440 bytes. Mode 4 executes 71/594 total fast runs/
  triangles versus mode 3's 45/540; compile/runtime fallback is zero.
ITCM/DTCM/BSS/STACK/ARENA DELTA:
  Experiment profile-1 ITCM was 17,996 versus the accepted 20,376 placement;
  main BSS was 1,881,520, about +6 KiB. DTCM BSS remained 152. No arena,
  per-frame allocation, or expanded texture frame was added. Retained delta is
  zero after the exact reverse patch.
PROFILE-2 ROM SHA-256 / WINDOW:
  D6D0B107C7C8DC2A9573CDB0E254CE79E238490A5014913338AE5B7514CDB334;
  isolated generic and mode-4 frames 45..76 / logic 52..83.
SEMANTIC TRACE RESULT:
  All 32 frames match every 828 semantic events, provenance values, owner entry/
  exit state, persistent 32-slot cache and ownership hashes, resolver/global
  hashes, topology/signature census, and upload sequence. No dual GX submit.
ORACLE/COUNTER/GX RESULT:
  Oracle 2,484/0/0; exact 828 and 648/44/126/10 classes; 121/707/121 batches;
  98/730 texture prepare/reuse; 36,864 upload bytes. Both isolated windows end
  at the same dynamic 732/2,218 GX RAM state.
CAPTURE RESULT:
  None. Exact semantics passed, but the candidate missed the performance gate
  before visual promotion.
VERIFIER COMMANDS AND RESULTS:
  Matched 128-frame profile-1 A/B, 32-frame isolated profile-2 comparison,
  ITCM placement, exact geometry/oracle/upload, and conservation gates pass.
  Broad regression is intentionally skipped at the user's fast-iteration pace.
DECISION: REVERT
  Correct but too small. Do not revive this fixed schedule or another record
  executor that feeds the generic triangle helper.
NEXT MEASURED BOTTLENECK:
  Adopt CODEX_60FPS_FASTEST_PATH_20260712.txt Phase 4: compile the complete
  stage owner into direct records that bind live adapter state and call narrow
  raw/no-Z kernels. A temporary side census measured the exact software
  wallpaper wall at 383,296 ticks and live scale 1.50x..1.70x; retain that as
  a deferred >=300K-class BG2-affine/row-reuse target only after draw <700K.
```

## 2026-07-13 - rejected complete stage no-Z setup kernel

```text
IDEA ID: M7-STAGE-NOZ-SETUP
HYPOTHESIS:
  Keep the exact source state interpreter, but intercept every Dream Land no-Z
  TRI command before the generic classifier/setup path. Prepare transformed XY,
  color, ST, vertex-alpha poly format, and painter depth through one narrow
  command kernel while retaining live texture binds and exact GX writes.
TARGETED EXCLUSIVE COUNTER:
  Same-ROM profile-1 whole draw, stage owner, and layer-0 median/P95, comparing
  retained mode 3 against temporary mode 4 with incremental wallpaper enabled.
MEASURED UPPER BOUND:
  The kernel covered all 126 stage no-Z triangles, but stage improved only
  21,152/22,592 ticks (2.59% median) and whole draw only 13,952/5,248 ticks.
  Bypassing generic class/setup without fusing intervening state/material work
  therefore misses both the 50K whole-draw and 25% owner gates.
LIVE-TREE RECONCILIATION:
  Started from retained wallpaper commit d2f5809a5c. Read the current roadmaps,
  BattleShip grdisplay source, and sm64-nds painter-depth renderer. Preserved all
  user-local files and kept decomp read-only.
FILES/FUNCTIONS CHANGED:
  Temporary renderer/header and benchmark-selector changes added a stage-only
  no-Z command kernel, exact constant/vertex-alpha handling, unique projected
  slot preparation, narrow textured/untextured emission, and mode-4 coverage.
  All production and selector changes were removed after the keep-gate miss.
BUILD TARGET/FLAGS:
  smash64ds-battle-playable-coarse-hwtri; profile 1; common/scene O2 Thumb,
  renderer O2 ARM; benchmark mode 0; incremental wallpaper mode 1.
PROFILE-1 ROM SHA-256:
  3F525296E9CA537CD6D14012DF7116AE4A5501A75CF07492E78D601A72AB37AC
  for both modes; only the GDB runtime selector changed.
FRAME/LOGIC-TICK WINDOWS:
  Mode 3 frames 225..256 / logic 232..263; mode 4 frames 233..264 / logic
  240..271. Both are 32 consecutive warm frames with 0/0 conservation error.
A0 MEDIAN/P95:
  Mode 3 draw 1,861,920/1,916,032; stage 815,168/822,720; layer 0
  262,400/262,464; wallpaper 289,856/362,752.
B0 MEDIAN/P95:
  Mode 4 draw 1,847,968/1,910,784; stage 794,016/800,128; layer 0
  251,936/252,160; wallpaper 297,280/373,632.
ACTIVE/WAIT SPLIT:
  Active improves 1,898,560/1,952,704 -> 1,884,704/1,947,456. VBlank wait
  absorbs the small median delta; residuals remain below 0.9%.
OWNER / COVERAGE RESULT:
  Fast coverage changes 45 runs / 540 triangles -> 120 / 666. Stage coverage
  is 60 -> 186, proving all 126 no-Z triangles joined the existing 60 raw
  triangles; Mario/Fox remain 246/234. Existing raw fallbacks remain bounded.
ORACLE/COUNTER/GX RESULT:
  Profile 1 retains exact 828 triangles, 648/44/126/10 classes, 121/707/121
  batches, 98/730 texture prepare/reuse, and 2/36,864-byte uploads. Profile 2
  and capture were intentionally skipped after the decisive performance miss.
VERIFIER COMMANDS AND RESULTS:
  Incremental build, ITCM placement, two 8-frame coverage smokes, and the
  same-ROM 32-frame mode-3/mode-4 comparison passed their structural and
  conservation gates. The candidate was then fully reverted with apply_patch.
DECISION: REVERT
  A standalone no-Z classifier/setup/emitter is still too small. Do not add
  another triangle-kernel variant around the existing source state stream.
NEXT MEASURED BOTTLENECK:
  Renderer work requires a complete-stage owner design that fuses intervening
  material/state/VTX preparation into coarse direct records. In parallel, the
  live A-normal availability/tap gap remains a higher-impact P1 gameplay issue.
```

## 2026-07-13 - rejected single-affine Dream Land wallpaper

```text
IDEA ID: J1-SINGLE-AFFINE
HYPOTHESIS:
  Replace the exact 256x192 software wallpaper resampler with one DS affine
  B16 background and register-only per-frame updates.
TARGETED EXCLUSIVE COUNTER:
  Exact accepted source X/Y maps versus the DS q8 affine sampler, plus live BG3
  ownership before dedicating or repurposing VRAM D.
LIVE-TREE RECONCILIATION:
  The now-retired 2026-07-13 experiment brief superseded older ordering only
  for this historical preflight. Current priority lives in
  `P1_EXECUTION_BOARD.md`, the renderer contract in
  `optimization/NATIVE_RENDERER_PLAN.md`, and measured decisions in this
  ledger. Canonical battle left BG3 empty across the observed 128-frame window,
  but Results/opening still required the generic BG3 layout.
EXACTNESS RESULT:
  The accepted mapper contains two integer sampling stages. The host/profile
  model found coordinate mismatches in both axes across all 128 observed
  origin/scale states for a single q8 affine transform. No bounded sampling
  difference was approved by the owner.
PERFORMANCE RESULT:
  None. The architecture failed its required exactness/approval preflight, so
  no ROM rebuild or misleading timing run was performed.
DECISION: REVERT / DO NOT IMPLEMENT
  A single affine layer could not meet that experiment's exactness gate. The live
  source tree retained no implementation or diagnostic probe.
NEXT MEASURED BOTTLENECK:
  Test the supplied exact affine-base plus sparse-correction architecture once;
  retain it only if the complete owner falls below the 20K/30K wallpaper gate.
```

## 2026-07-13 - rejected per-root generated fighter representation

```text
IDEA ID: J2-FIGHTER-NATIVE-PER-ROOT
HYPOTHESIS:
  Generate compact fighter roots, state epochs, and triangle runs so Mario and
  Fox bypass source-list traversal while preserving exact renderer semantics.
TARGETED EXCLUSIVE COUNTER:
  Same-ROM profile-1 Mario+Fox owner wall and whole draw; profile-2 828-event,
  owner/cache/state, vertex, class, batch, texture, and upload equality.
GENERATED REPRESENTATION:
  32 roots, 49 epochs, 67 runs, and 626 triangles. Compact typed state and u16
  topology reduced ROM growth to 13,312 bytes after dead-state elimination;
  250/418 runtime state effects were removed, leaving 54 deltas / 168 refs.
PROFILE-1 RESULT:
  Generic Mario+Fox 1,003,232 ticks; generated path 736,864 ticks. Saving is
  266,368 ticks / 26.55%, below the binding >=300K and >=40% activation gate.
  A temporary raw-first variant reached a 305,536-tick timing delta but stalled
  the forensic renderer and was immediately removed; it is not an exact KEEP.
PROFILE-2 RESULT:
  The retained measurable variant matched 828 semantic events, 2,484/0/0
  vertex oracle, 648/44/126/10 submit classes, 121/707/121 batches, 98/730
  texture prepare/reuse, and the exact upload hash with zero trace overflow.
ARCHITECTURE FINDING:
  The generated path still paid one generic triangle entry per root and applied
  residual source-shaped state effects. It was a compact per-root executor,
  not the required complete owner that directly binds live epochs and emits
  long native streams.
MEMORY/DISASSEMBLY:
  Candidate ROM was 12,035,072 bytes versus 12,021,760 baseline. The scene arena
  retained 202,336 bytes of headroom in the measured profile-1 build; generated
  material BSS was 400 bytes. The remaining generic call boundary explains the
  missed reduction gate.
DECISION: REVERT
  Exact but too small. Generator, runtime mode plumbing, generated include, and
  all renderer changes were removed. Do not revive this representation or its
  raw-first kernel.
NEXT MEASURED BOTTLENECK:
  Only a genuine whole-owner fighter path is admissible: preflight the complete
  owner, bind typed material/matrix/texture state once per effective epoch,
  direct-load VTX/MODIFY state, and use no generic scan/VTX/TRI/classifier call.
```

## 2026-07-13 - rejected exact affine-base plus sparse-correction wallpaper

```text
IDEA ID: J1-AFFINE-SPARSE-CORRECTION
HYPOTHESIS:
  Put a fixed 256x220 Dream Land source window in affine BG2 and maintain an
  identity BG3 containing only exact pixels where the affine X or Y sample
  differs from the accepted software mapper.
TARGETED EXCLUSIVE COUNTER:
  Profile-1 wallpaper/whole-draw timing, source-window writes, correction
  density/writes, semantic-map solver time, and sparse-correction maintenance.
  Profile 2 was gated on a >=100K or >=5% win and was not built after the miss.
HOST/FEASIBILITY RESULT:
  The supplied reference model reconstructs exact pixels at representative
  1.50..1.70 scales with about 23-30% corrections. The live eight-frame census
  observed source X 46..232, fixed window origin 44, one initial 65,536-pixel
  upload, zero normal-frame shifts, and 12,746-14,220 current corrections.
FIRST FALSIFYING RESULTS:
  The full +/-8 coefficient search cost more than the retained renderer and
  produced about 1.69M wallpaper ticks. Rounded-slope solving plus correction
  delta tracking still measured 488,992-602,560 wallpaper median in separate
  eight-frame windows, versus the accepted 237,088 baseline. Whole draw rose
  to roughly 2.37M-2.47M instead of falling by the required 180K.
EXCLUSIVE PHASE RESULT:
  Rounded-slope solving remained about 62-66K ticks/frame. BG3 correction
  maintenance alone remained about 480-620K ticks/frame: sparse screen-space
  VRAM stores and the comparisons needed to avoid unchanged stores cost more
  than the complete packed-row/DMA software wallpaper owner.
ROM/MEMORY RESULT:
  Final diagnostic ROM SHA-256 was
  E8983923EBA196E3AE1CB87915155E3700FDC6323EB94ADFF704E0EC3513D91D
  at 11,992,064 bytes. Arena headroom was 218,720 bytes; the temporary affine
  state occupied 1,552 BSS bytes and preserved the reserve.
EXACTNESS STATUS:
  Host reconstruction was exact by construction, but on-device profile-2 pixel
  promotion was intentionally skipped after the decisive negative profile-1
  result. No production exactness claim is made for the removed prototype.
DECISION: REVERT
  The architecture is hundreds of thousands of ticks slower than the retained
  software path and cannot clear the 20K/30K or >=180K gates. Platform API,
  solver, correction code, selectors, verifier fields, and generated pycache
  were fully removed; retained source delta is zero.
NEXT MEASURED BOTTLENECK:
  Return to complete DS-native owners. Do not retry affine correction bitmaps,
  single-affine bias polishing, source-window streaming, or more wallpaper DMA
  tuning without a fundamentally different hardware representation.
```

## 2026-07-13 - retained serialized scene-buffer reuse / paused stage falsifier

```text
IDEA ID: M9-BOOT-MEMORY
HYPOTHESIS:
  Recover the complete-stage candidate's measured 53,808-byte arena overflow
  without weakening its heap reserve by sharing file storage between scenes
  that BattleShip's scene manager serializes.
SOURCE/LIFETIME RESULT:
  Title and opening/action-preview reset the reloc ledger at scene boundaries
  and cannot own their static file stores concurrently. Their existing
  176,000- and 270,000-byte arrays now alias through one aligned union; sizes,
  paths, and cache storage are unchanged.
MEMORY RESULT:
  Main BSS falls exactly 176,000 bytes, from 1,932,688 to 1,756,688. The final
  coarse candidate ELF is 8,539,628 bytes with main/main.rw/BSS
  722,352/124,052/1,756,688.
RUNTIME RESULT:
  The immediate post-memory Profile-1 generic ROM boots with audio, Dream Land,
  and all 828 triangles. Eight-frame draw/stage medians are 2,242,336/865,920
  ticks; this is boot proof, not a performance control. Artifact:
  logs/optimization/native-stage-post-memory-mode0-generic-smoke8.json.
MODE-9 STATUS:
  The first post-memory launch cleanly fell back because the preflight expected
  gcDrawDObjTreeForGObj instead of BattleShip's eight grdisplay layer wrappers.
  Exact wrapper validation is now rebuilt, but the final ROM
  9E7D7114DDE5DCC5FCDD6689AC5EFFD44646F28B53B773942CA0F69AEFE18CAC
  is deliberately unmeasured at the user's pause point.
DECISION: KEEP MEMORY REUSE; MODE 9 PENDING ONE FINAL FALSIFIER
  Resume with the checkpointed eight-frame mode-9 command. Reject immediately
  on another activation failure or unless draw <=1,299,968 and stage <=485,702
  ticks. Preserve the accepted mode-8 owner and this 176,000-byte recovery.
```

## 2026-07-13 - rejected complete-stage owner; retained mode-8 visual proof

```text
IDEA ID: M9-COMPLETE-STAGE-FINAL
HYPOTHESIS / EXCLUSIVE GATE:
  The corrected BattleShip layer-wrapper identities would activate one native
  complete-stage transaction and save at least 300,000 whole-draw ticks while
  reducing stage to <=485,702. Required accounting was exactly 121 runs, 828
  triangles, owners 202+320+306, and zero owner fallbacks.
TIME TO FIRST MEASUREMENT:
  Zero new implementation time at resume: the mandated existing-ROM falsifier
  was the first runtime action. No further activation/debug iteration ran.
LIVE-TREE / BUILD:
  Preserved the accepted mode-8 fighter owner, serialized scene-buffer union,
  decomp read-only state, and all unrelated user files. The falsifier used the
  existing profile-1 coarse ROM `9E7D7114DDE5DCC5FCDD6689AC5EFFD4
  4646F28B53B773942CA0F69AEFE18CAC`; no title or VS-setup build ran.
ACTIVATION / PERFORMANCE RESULT:
  Selector 9 reported `70/686`, owners `60+320+306`, fallbacks `29/0/0` on
  every sampled marker—the exact retained fighter path plus generic stage.
  Required mode-9 activation therefore failed a second time. No benchmark JSON
  or valid timing window exists, so no performance claim is made.
FILES / FOOTPRINT:
  Removed the mode-9 selector/API, native-stage executor and adapter, generator,
  generated table, hooks, verifier branch, and two workspaces. Post-revert
  main/main.rw/BSS are `692456/124052/1738000`; ITCM is `17984/32768`, DTCM
  BSS 152. Versus the failed candidate this removes 29,896 main bytes and
  18,688 aligned BSS bytes. The independent 176,000-byte scene-buffer saving
  remains.
RETAINED MODE-8 REBUILD:
  ROM `D55F80F156875EC9077C679403C6D8F419229657721C18FD56391C4788CA6037`,
  12,036,096 bytes; ELF `22CB3CFA1EA5BF69E1D37AE738A3160C1403F0B8C65ED9A629CE234B90A9B16E`,
  8,443,528 bytes. Frames 193..200 / logic 200..207 report draw
  `1606368/1759680`, stage `812320/930368`, Mario `201344/236288`, Fox
  `235552/235712`, wallpaper `342784/342848`, exact `70/686`, owners
  `60+320+306`, fallbacks `29/0/0`, 828 triangles, two/36,864-byte uploads,
  and zero conservation failure.
CAPTURE RESULT:
  Plain generic capture `2026-07-13_coarse_mode0_plain_launch.png` and
  GDB-confirmed selector-8 capture `2026-07-13_coarse_mode8_verified.png` both
  show Dream Land, Mario, and Fox. Both sampled mode-8 frames pass top-screen,
  named-region, motion, green/detail, and horizontal-texture gates.
INTEGRATED VERIFICATION:
  DevFast, profile-2 forensic, P1Gate, and Boundary 161/162/163 pass. The first
  forensic compile exposed that the generated dense-table declaration type was
  hidden from profile 2; exposing only that 16-byte type declaration fixed the
  build without enabling the production owner there. Forensic then passed
  oracle 2484/0/0 plus exact 828, texture, depth, matrix, semantic, and upload
  gates. Static checks pass.
DECISION: REVERT MODE 9 / KEEP MODE 8 + MEMORY UNION
  Do not revive this complete-stage record executor. Next run the authoritative
  Cut-D complete converted-output cardinality probe for <=30 minutes against
  the measured `197376/199360` conversion wall; require >=120K active saving.
```

## 2026-07-14 - rejected texture residency/overlap; restored coarse ROM

```text
IDEA ID: D0/D1/D2-COMPLETE-TEXTURE-OUTPUT
HYPOTHESIS / EXCLUSIVE BOUND:
  Recurrent exact converted outputs or zero-lag overlap could remove at least
  120,000 active ticks from the measured 197,376/199,360 conversion wall.
TIME TO DECISION:
  One bounded instrumentation/build cycle and one 600-frame run; the candidate
  was rejected from measured cardinality and architectural bounds before any
  cache, custom ARM7 payload, or second render prepass was implemented.
LIVE-TREE / TEMPORARY CHANGES:
  A temporary renderer census recorded complete input/output hashes, sizes,
  frequency, owner, sequence, and reuse distance. The benchmark/verifier gained
  a temporary 600-frame dump path. All census source and script plumbing is now
  removed; the three files match the 20260713_232129 verified lean checkpoint
  byte-for-byte. Decomp and unrelated user files remained untouched.
CARDINALITY RESULT:
  Frames 600; changed frames 457 (76.17%); events 891; complete keys 322;
  overflow 0; final outputs 206; reuse median/P95/max 216/216/216. The 32,768-B
  class has 165 keys, 101 outputs, and 457 events; the 4,096-B class has 157
  keys, 105 outputs, and 434 events. Frequency is 75 keys x2 and 247 keys x3.
  Full output residency is 3,739,648 bytes; a 2-4-slot LRU has effectively zero
  hits. Every first-use owner is stage.
OVERLAP BOUND:
  The exact recipe is first assembled inside ndsRendererHardwareBindTexture,
  immediately before the dependent stage bind. Original update does not expose
  it. ARM7 overlap therefore also needs a custom payload/protocol and a second
  source/render prepass. Against the 197,376-tick ceiling, all prepass, IPC,
  cache, and wait work must fit below 77,376 ticks to clear the 120K gate.
  This cannot close the >1M draw gap and was rejected without implementation.
RESTORED BUILD / ARTIFACT:
  Only profile-1 `smash64ds-battle-playable-coarse-hwtri` was rebuilt: common/
  scene O2 Thumb, renderer O2 ARM, benchmark mode 0. One renderer compile and
  link restored ROM `D55F80F156875EC9077C679403C6D8F419229657721C18FD56391
  C4788CA6037`, 12,036,096 bytes; ELF `C9B57B6C5E53736D8387FEFC15AE60F
  EEB7E193CE540CD893E93C8BACF2E0D10`, 8,443,528 bytes. No title/VS build ran.
PROFILE-1 RESULT:
  Frames 193..200 / logic 200..207: draw 1,606,368/1,705,856; stage
  812,320/911,424; Mario 201,344/201,472; Fox 235,552/235,712; wallpaper
  342,784/342,848; conversion 197,376/199,360; exact mode-8 70/686,
  60+320+306, fallbacks 29/0/0, two uploads/36,864 bytes, conservation 0/0.
CAPTURE / AUDIO RESULT:
  Fresh `2026-07-14_coarse_restored_mode0_plain_launch*.png` and
  `2026-07-14_coarse_restored_mode8_verified*.png` pairs visibly show moving
  Mario/Fox and Dream Land. Full-window paired deltas are 30.965% and 13.224%.
  A same-ROM no-build verifier passes natural BGM start, hardware-timer rate
  near 44.1 kB/s, safe alternating half refills, and zero open/read/unsupported
  failures (`logs/optimization/restored-mode8-audio-smoke8.json`).
MEMORY / RETAINED STATE:
  Retained main/main.rw/BSS are 692,456/124,052/1,738,000; ITCM 17,984/32,768.
  The production fighter owner and 176,000-byte scene-buffer union remain.
INTEGRATED VERIFICATION:
  Docs, architecture, registry, and GBI checks pass. DevFast passes fresh
  canonical capture and `99FF3D2...C901BE30` shipped parity; forensic passes
  2,484/0/0 and exact 828 semantics. P1Gate passes opening, realtime/natural
  battle, one-minute expiry/results, CPU, audio, and memory. Boundary
  161/162/163 passes. Full Regression is intentionally skipped for fast iteration.
DECISION: REVERT D1/D2 / REMOVE PROBE
  Finite residency and exact overlap cannot meet the minimum-calendar-time,
  hundreds-of-thousands gate. The 176,736-tick aggregate matrix ceiling is also
  too small as the next architecture. Next falsify one source-seeded DS display-
  capture/affine Dream Land raster owner against the combined 1,155,104-tick
  wallpaper+stage wall; keep only at >=800K saving and draw <=650K.
```

## 2026-07-14 - rejected Cut F; natural Mode-8 direct-launch checkpoint

```text
IDEA ID: CUT-F-SOURCE-SEEDED-RASTER
PERFORMANCE RESULT:
  The first eight synchronized frames reached draw 467,968/468,992,
  present-active 471,808/472,832, update 137,504/151,552, zero normal-frame
  stage/wallpaper CPU work, and exact 626 fighter triangles (320+306).
FALSIFIER RESULT:
  The captured surface omitted wallpaper pixels, producing navy holes. After
  232 normal frames, legitimate source-camera pan/zoom exceeded the affine
  cache bounds and forced the full 828-triangle generic fallback. The early
  and fallback captures are rejection evidence, not working screenshots.
DECISION:
  Reject and remove all Cut F runtime, API, Makefile, capture, and verifier
  plumbing. Do not iterate margins or seed-camera guesses.
RESTORED DIRECT ARTIFACT:
  The coarse target now compiles NDS_RENDERER_FAST_RUN_DEFAULT=8. ROM
  DC2871F3E6C32C72D6F36516EDA461F25E416E5146947280E11BEFA352E4E3AD
  launches Mode 8 with GDB disabled. Frames 275..282 retain exact 70/686,
  owners 60+320+306, fallbacks 29/0/0, and 828 triangles. Draw is
  1,571,648/1,589,696; stage 775,552/790,720; wallpaper 344,704/347,200.
CAPTURE RESULT:
  2026-07-14_coarse-mode8-direct-settled.png and its _next pair visibly show
  Dream Land and moving Mario/Fox. Both-frame visibility/green/detail, all
  named regions, wallpaper, and three texture-detail crops pass. Registered
  meaningful motion is 11.001%, mean channel delta 9.45, overlap 100%.
DECISION: KEEP DIRECT MODE-8 DEFAULT / REJECT CUT F
  This is a verified launch/fidelity checkpoint, not a 60 FPS claim. Require
  >=300K exclusive saving before the next distinct architecture is coded.
```

## 2026-07-14 - rejected typed stage executor; hardened exact-ROM checkpoint

```text
IDEA ID: PER-CALLBACK-TYPED-STAGE-EXECUTOR
PERFORMANCE FALSIFIER:
  The source-keyed executor covered 799/828 triangles with one stage fallback.
  Candidate stage time was about 877,248 ticks versus 873,344 for the generic
  control. The candidate therefore saved essentially zero stage time; the
  whole-draw delta came from the already accepted native fighter owner.
DECISION:
  Reject and remove the executor, generated stage tables, adapter seam, mode 9,
  and texture-site shortcut. This representation cannot save hundreds of
  thousands of ticks and must not be revived.
BUILD-PATH CORRECTION:
  The 39CD1397... cleanup ROM was produced by an under-specified direct make
  invocation with NDS_DEV_LIVE_INPUT_PREVIEW=0. It is not user-facing evidence.
  The coarse target now intrinsically forces battle_playable_realtime, live
  input, full-rate logic, HW triangles, profile 1, and Mode 8. A plain target
  build reproduces 12,036,096-byte ROM
  DC2871F3E6C32C72D6F36516EDA461F25E416E5146947280E11BEFA352E4E3AD.
MODE-8 RESULT:
  Frames 282..289 retain exact 70/686 runs, 60+320+306 owners, 29/0/0
  fallbacks, and all 828 triangles. Draw is 1,578,720/1,586,368; stage
  784,608/792,832; wallpaper 342,784/344,832. Texture commits remain inside
  VBlank with zero outside/fallback uploads.
PRESENTATION / RUNTIME RESULT:
  Exact-ROM pair 2026-07-14_dc287-mode8-direct*.png passes both-frame 100%
  visibility, green/detail, all named regions, and all horizontal-detail gates.
  Registered meaningful motion is 10.610%, mean channel delta 8.71, overlap
  100%. The earlier alleged partial PNGs are complete on disk; the multi-image
  inspection view displayed unchanged regions as black. A same-ROM no-build
  verifier passes original Fox CPU, natural BGM/refills, reserve, and ITCM.
CHECKPOINT STATUS:
  Workspace automated launch correctness is restored, but this is still only
  about 15.4 FPS. The user's separate exact-ROM no-audio/no-stage manual report
  remains pending controlled retest; hardware/manual acceptance is not claimed.
  Modes 1-7 do not have individual capture proof. No title/VS setup build ran.
INTEGRATED VERIFICATION:
  Docs, architecture, registry, and GBI checks pass. Fresh DevFast passes its
  canonical build, paired motion/detail capture, and exact shipped ROM parity.
  The focused exact coarse benchmark and CPU/audio/memory verifier pass. The
  prior broad checkpoint owns forensic, P1Gate, and Boundary 161/162/163.
```

## 2026-07-14 - rejected full-source scanline-affine wallpaper

```text
IDEA ID: FULL-SOURCE-SCANLINE-AFFINE-WALLPAPER
ARCHITECTURE:
  Profile-1 laboratory mode stored the complete decoded 300x220 BattleShip
  wallpaper in a 512x256 B16 BG, selected X with one affine coefficient, and
  streamed exact source-Y rows into BG2PA..BG2Y during HBlank. It avoided the
  rejected Cut-F camera bound and source-window streaming.
SAME-ROM CONTROL (mode 1, 8 frames):
  ROM 0962F6B4195971EB1E7D34273CCA9F897AAF8FB4DAFD4525DC659DA826F59471
  draw 1,489,696 / 1,548,096; wallpaper 274,144 / 310,016.
CANDIDATE (mode 2, 8 frames):
  draw 1,250,304 / 1,719,360; wallpaper 34,112 / 453,824.
  One 132,000-byte source upload, eight HBlank table arms, zero failures,
  exact 828 triangles, 70/686 runs, 60+320+306 owners, 29/0/0 fallbacks.
FALSIFIER:
  Whole-draw saving was 239,392 ticks and wallpaper saving was 240,032 ticks,
  both below the binding 300K keep threshold. The one-time upload worsened P95.
DECISION: REJECT / REMOVE
  All runtime, API, selector, and verifier plumbing is removed. Do not retry
  full-source affine or per-scanline HBlank wallpaper; wallpaper-only work no
  longer has a credible >=300K ceiling on the current baseline.
RESTORED CHECKPOINT:
  A clean coarse rebuild reproduces exact 12,036,096-byte ROM
  DC2871F3E6C32C72D6F36516EDA461F25E416E5146947280E11BEFA352E4E3AD.
  Its no-build Mode-8 gate passes exact geometry/owner/fallback accounting.
```

## 2026-07-14 - rejected fighter split projection/modelview path

```text
IDEA ID: M2-NATIVE-SPLIT-MATRIX
HYPOTHESIS / KEEP GATE:
  Remove per-root CPU modelview×projection composition from the existing AOT
  Mario/Fox owner by loading one shared GX projection and one world-scaled
  modelview per root. Keep only with at least 100,000 combined fighter ticks
  saved in the first synchronized eight-frame run.
SOURCE / BACKEND BASIS:
  BattleShip ftDisplayMainProcDisplay, gcPrepDObjMatrix, gcDrawMObjForDObj,
  gcDrawDObjTree, and lbCommonSetupFighterPartsDObjs were read before the cut.
  decomp/sm64-nds confirmed separate DS projection/modelview loading, while
  BattleShip row-vector ordering, lighting modelviews, and GX palette restores
  remained authoritative.
IDENTICAL-ROM A/B/A:
  Profile-1 Mode-8 ROM SHA-256
  71316594FD739FFEBFAA73D5A97446C31BB7EB46CD17B2B87BAFE4E6A8AA7727,
  12,042,240 bytes. All runs sampled frames 600..607 / logic 209..216.
CONTROL A0 AND A1 MEDIAN/P95:
  Mario 199,008/219,072; Fox 234,848/241,856; combined median 433,856;
  draw 1,597,152/1,600,384; present 2,095,712/2,127,168; matrix
  177,920/204,608; DL 219,136/219,264. A1 reproduced A0 exactly on every
  listed counter.
TREATMENT B0 MEDIAN/P95:
  Mario 183,136/203,264; Fox 214,528/221,568; combined median 397,664;
  draw 1,561,056/1,564,224; present 1,999,616/2,106,816; matrix
  177,856/204,544; DL 211,072/211,136.
RESULT / SEMANTICS:
  Combined saving was 36,192 ticks (8.34%), draw saving 36,096, and present
  saving 96,096. Exact 70/686 native runs, 60+320+306 owner triangles,
  29/0/0 fallbacks, full 828-triangle class census, two uploads/36,864 bytes,
  CPU activity, timer state, and frame conservation all remained coherent.
  The treatment therefore failed performance, not correctness.
DECISION: REJECT / REMOVE
  The final control closed phase-drift risk. Remove the selector, split GX
  path, public structure field, and benchmark plumbing; retain the pre-existing
  generated owner and synchronized-frame tooling. Do not revive this cut.
NEXT MEASUREMENT:
  Isolate current production-owner execution and matrix-preparation floors
  before selecting another M2 representation. The control window reports
  about 219,136 DL ticks and 177,920 matrix ticks across the fighter path.
```

## 2026-07-14 - rejected CPU-only generated joint preorder

```text
IDEA ID: M2-JOINT-PREORDER-CPU
HYPOTHESIS:
  Replace per-binding hierarchy/cache lookup with generated 25/27-joint
  Mario/Fox preorder schedules while retaining CPU local/world/camera matrix
  construction and the existing production owner.
CONTROL / TREATMENT / CONTROL:
  Control ROM A0/A1 SHA-256
  A0FFA8C2B1F7CFF21DCD36D4399E12C7646593E6195DE26549B2552E75530874;
  treatment B0 SHA-256
  B35892951478AD28C2759750D65243EEC56469AA67F04F4953D814DD728306EB.
  Eight-frame post-GO samples used the same Mode-8 source configuration; the
  final control reproduced the first fighter and matrix medians.
RESULT:
  Control Mario/Fox medians were 188,512 + 220,960 = 409,472 ticks; treatment
  was 183,648 + 215,296 = 398,944, saving only 10,528. Matrix median moved
  176,896 -> 166,464, saving 10,432. Production DL cost was unchanged at
  about 197.4K. Stage/draw medians crossed animation phases and are not used
  for this fighter-only decision.
DECISION: REJECT CPU-ONLY PATH
  The result missed the 100K M2 falsifier by nearly an order of magnitude.
  Remove the CPU-only runtime path. Keep the corrected generated topology,
  parent/binding, and GX-slot data only as input to a distinct whole-skeleton
  hardware owner; do not revive indexed CPU preorder as an optimization.
```

## 2026-07-14 - measured current Mode-8 no-submit floor

```text
IDEA ID: M2-PRODUCTION-TRIANGLE-NOOP-FLOOR
PURPOSE:
  Bound all generated production-run preparation/emission before designing the
  next owner. This is a compile-time nonvisual ablation, not a candidate path.
RESTORED BASELINE:
  Profile-1 ROM 6F00937C1FC022D0CEE85CDAF8FE2ED770E691E26102986909248F76E78E4B66,
  frames 600..607 / logic 209..216. Mario 197,888/218,304; Fox
  233,248/240,384; combined median 431,136; matrix 177,440/204,736; production
  DL 218,336/218,368.
TRIANGLE_NOOP FLOOR:
  ROM 99C29CDFF2A6BF740DDFB9ACDA091D44515801BCB62BCC4F4B628FBA56669531,
  identical frames/logic. It retains all generated fighter accounting at
  67 runs / 626 triangles / 320+306 with zero fighter fallback. Mario
  151,584/171,008; Fox 179,552/186,432; combined median 331,136; matrix
  178,144/204,736; production DL 116,736/116,736.
BOUND / DECISION:
  Removing the complete root-bind/run-prepare/emission boundary saves exactly
  100,000 combined fighter ticks, yet the remaining 331,136 floor still misses
  the 250K ceiling. Submission-only tuning cannot complete M2. The next design
  must jointly reduce the ~178K CPU matrix-preparation wall while preserving
  BattleShip transforms, lighting, materials, and cross-joint semantics.
```

## 2026-07-14 - rejected whole-fighter GX skeleton palette

```text
IDEA ID: M2-WHOLE-FIGHTER-GX-SKELETON
HYPOTHESIS / KEEP GATE:
  Generate the corrected live 25-joint Mario / 27-joint Fox preorder, build
  each exact BattleShip 0x4B local once, and compose geometry through the DS
  GX matrix palette. Retain CPU world/modelview matrices solely for exact
  lighting. Keep only with at least 100,000 same-ROM combined fighter ticks
  saved; check the <=331,000 absolute first-window gate separately with the
  detailed M2 ledger disabled.
SOURCE / BACKEND BASIS:
  BattleShip lbCommonFighterPartsFuncMatrix and ftDisplayMain traversal were
  authoritative for transform order and lighting state. sm64-nds/libnds GX
  store/restore and affine 4x3 multiplication established the DS backend
  mechanism. Cross-matrix corners retained the existing compact slot ABI.
IDENTICAL-ROM A/B/A:
  Profile-1 Mode-8 ROM SHA-256
  03950839A61B7E1C058986AB7D8E1095A20C07AD5E0466DDCB50D8DA3BEEF09B,
  12,047,360 bytes. All three runs sampled frames 600..607 / logic 209..216,
  n=8, melonDS 1.1, JIT disabled, ports 4333/4334. A2 reproduced every A1
  sample and listed median/P95 exactly. Both sides carried the same detailed
  M2 census/timer overhead, so their paired delta is valid; their absolute
  totals are not coarse-profile gate measurements.
CONTROL A1 = A2 MEDIAN/P95:
  Mario 226,112/246,272; Fox 267,584/274,624. Paired per-frame combined
  493,696/520,896. Draw 1,658,528/1,661,568; matrix 199,008/226,304;
  M2 matrix/root subtotal 162,208/189,568; production 243,520/243,648.
TREATMENT B MEDIAN/P95:
  Mario 205,344/225,536; Fox 240,192/247,232. The sum of independent owner
  medians is 445,536; the paired per-frame combined result is 445,568/472,768.
  Draw 1,610,336/1,613,504; matrix 184,320/211,520; M2 matrix/root subtotal
  113,632/140,736; production 242,240/242,304.
RESULT / SEMANTICS:
  Paired combined saving was 48,128 ticks (9.75%); the independent-owner
  median sum saved 48,160. Draw saved 48,192. The treatment removed the
  19,552-tick hash/parent phase and reduced final composition
  30,304 -> 1,920, but exact CPU local/world work remained and production was
  essentially flat. All runs retained 49 lighting epochs, 67 fighter runs,
  626 fighter triangles, the full 828-triangle 648/44/126/10 class census,
  70/686 native run accounting, 29/0/0 fallbacks, two uploads/36,864 bytes,
  zero production failure/overlap/reject, and coherent timer/runtime state.
DECISION: REJECT / REMOVE
  The valid same-ROM saving was 51,872 ticks short of the 100K relative gate,
  which is sufficient to reject the treatment. The 445,568 paired median
  includes detailed-ledger overhead and is not compared with the coarse 331K
  ceiling. All skeleton, selector, public API, generated unique-slot, adapter,
  and verifier treatment code was removed. Corrected 25/27 topology remains;
  the disjoint M2 ledger is now opt-in through
  NDS_RENDERER_M2_DETAILED_LEDGER=1. The historical post-revert instrumented
  profile-1 ROM is
  0192BAFF0B130BC184B2E263D792AEF16CE67851A47CE81FB2DA9496684AE5E9.
CANONICAL ARTIFACT:
  No profile-0 build or publish occurred. The user-facing ROM remains
  D28DFB303EE7381F9209026DD7DFF2370B667AD25A587ECED1D40D5BB87D2198,
  12,043,264 bytes.
NEXT ARCHITECTURE DECISION:
  Do not retry a matrix-only owner while duplicating CPU world/composed
  geometry. The next M2 cut must fuse matrix preparation, exact lighting, and
  production emission at whole-fighter scale: GX owns geometry hierarchy;
  CPU retains only a verified minimal 3x3/light-direction sidecar; immutable
  49-epoch/67-run state and vertex emission is compiled into one coarse owner
  transport rather than per-root/per-run dispatch. Budget matrix preparation
  at <=60-80K and production at <=100-120K so 170-250K remains credible. Use
  the detailed ledger only for phase attribution; run all <=331K and 170-250K
  absolute gates with it disabled.
```

## 2026-07-14 - accepted source-driven native countdown OAM

```text
IDEA ID: IFCOMMON-NATIVE-OAM
BOUNDARY:
  Keep BattleShip ifCommonCountdownThread, GObjs, SObjs, timing, scale, color,
  order, control unlock, timer start, and teardown live. Convert only the exact
  game-status Sprite set once during relocation and draw recognized live SObjs
  through main bitmap OAM; unknown state falls back before partial submission.
IDENTICAL-ROM A/B:
  Integrated profile-1 ROM CCC30624C574FC3C52BE457C761836508262E77D99A55E85965CEA62532FEBAF,
  frames 187..194. Fallback foreground median 1,863,232 ticks; native foreground
  10,336; inclusive native median/P95 11,584/11,584.
CORRECTNESS / LIFETIME:
  Prepare 1/1/0 at frame 0, 93,824 bytes, 16 assets / 59 tiles, zero palette,
  hot conversion, or runtime upload. Active source count 10 SObjs and 24..26
  OAM objects; final clear frame 511; frame 600 idle tax zero. Wait/GO locks,
  timer state, semantic hashes, identical config, complete fallback/native PNGs,
  and post-GO cleanup agree.
DECISION: KEEP
  This removes the bounded 1.86M countdown spike without changing source
  behavior. Bitmap OBJ makes nonzero partial-alpha GO edge texels opaque; keep
  that bounded presentation debt rather than restoring software composition.
```

## 2026-07-14 - retained exact M4 corpus; rejected gameplay NitroFS reader

```text
IDEA ID: M4-PUPUPU-WATER-AOT-DIRECT
SOURCE / OFFLINE CONTRACT:
  The host generator replays the exact BattleShip Pupupu texture/MObj scripts
  for 18,000 frames: period 216, 322 keys, 206 distinct outputs, 3,024,896
  oracle pixels, and zero mismatches. Exact products are a 1,560,960-byte
  payload (SHA fa8bf472...21c1e) and 11,060-byte index
  (SHA 83d2b342...08b6). Retain this generator, fixture, and DevFast gate.
TREATMENT:
  Direct mode held one prepared output per water owner and used fseek/fread on
  a cache miss. It removed live conversion but read the packaged payload on
  12 of synchronized frames 171..202 and shared NitroFS with streaming BGM.
SYNCHRONIZED 32-FRAME RESULT:
  Off inclusive renderer median/P95 1,536,896/1,572,224; direct
  1,537,984/1,573,376 (+1,088/+1,152). Off draw 1,264,960/1,272,512; direct
  1,091,008/1,293,888 (-173,952 median, +21,376 P95). Loop median/P95 was
  1,680,512/1,714,368 off and 1,680,448/1,714,368 direct. The cumulative FPS
  marker's 16.3 -> 16.8 movement is not a synchronized-window win.
MEMORY / CORRECTNESS:
  Audio-adjusted reserve was 153,184 bytes, only 22,112 above the 128 KiB
  floor. The index was integrity-checked, but the same-size payload lacked a
  runtime content check. Frequent gameplay I/O also violates M4's critical-
  path requirement even if conversion counters are zero.
DECISION: REJECT / REVERT RUNTIME; KEEP CORPUS
  Reverted the runtime integration. Do not promote streaming as M4. The next
  falsifier must preload a complete exact DS-native pair-index/palette corpus
  before GO, prove byte-exact oracle expansion, fit measured texture VRAM/RAM,
  and report zero gameplay conversion, preparation, decompression, and I/O.
```

## 2026-07-14 - fixed the M2 hardware parity ownership boundary

```text
IDEA ID: M2-RGB15-T16-HARDWARE-PARITY
PURPOSE:
  Decide whether DS hardware lighting and texture matrices can replace the
  exact CPU work in the next whole-fighter owner before runtime plumbing.
CORPUS / DEVICE CONTRACT:
  BattleShip owner IR/O2R produced 541 dense vertices, 411 unique normals,
  413 unique binding/normal RGB15 cases across 18 bindings, and 99 unique t16
  cases across 9 epochs / 15 runs / 381 corners / 106 dense IDs. The portable
  no-GX C oracle also compiles as a 5,272-byte ARM946E-S freestanding object.
LIGHT RESULT: DEMOTE
  The source-mapped hardware-light reference misses 107/413 cases. Exhaustive
  per-binding best one-light 5-bit material choices still miss 102/413, with
  16/18 bindings failing. Hardware lighting cannot own source-faithful RGB15;
  retain the exact CPU 3x3/light-direction sidecar.
TEXTURE RESULT: SYNTHESIZED ELIGIBLE
  Naive scale-shift texture matrices miss 47/99 canonical t16 cases. Generated
  floor/ceil 20.12 coefficients plus fractional translation bias match 99/99.
  Admit only the synthesized mapping, still gated by live MObj/profile-2 parity.
DECISION:
  Keep the deterministic corpus and gate. The next M2 packet may move geometry
  hierarchy and synthesized texture addressing to GX, but must budget and
  preserve the CPU lighting sidecar. No runtime or performance claim yet.
```

## 2026-07-14 - rejected current-layout M4 indexed residency

```text
IDEA ID: M4-PUPUPU-INDEXED-RESIDENCY
PURPOSE:
  Test whether an 8-bit pair-index plus RGB5A1 palette and periodic maps can
  preload the exact animated water corpus before GO with zero gameplay I/O.
EXACTNESS:
  Phase-aware software expansion matches all 3,024,896 oracle pixels. Literal
  DS RGB256 pair indices have at least 753,481 alpha mismatches because only
  index 0 is transparent while BattleShip's 4x4 dither needs both alpha states
  for the same pair. 32x32 periodic maps miss 1,374,596 output pixels; 64x32
  large / 32x32 small periods still miss 600,358 from clamp/mirror edges.
RESIDENCY:
  Exact compressed archive 232,004 bytes. Current audio-adjusted reserve is
  153,184 bytes, leaving only 22,112 spendable above the floor. Resident pair
  maps/palettes need 645,120 bytes, beyond 524,288 bytes total texture space.
  The DS-visible index-0-normalized exact form is 903,168 resident / 275,008
  compressed bytes.
UPDATE MODEL:
  A hypothetical pair form changes 33 large maps, 41 small maps, and 78
  palettes per 216 frames: 664,576 bytes/cycle, 3,076.7 average bytes/frame,
  and 18,944 peak bytes. Hardware-visible exact maps rise to 975,872/cycle.
DECISION: CURRENT LAYOUT NO-GO
  Do not trade fidelity or reserve for this representation. An exact on-demand
  RGB256 map generator may be measured as an intermediate conversion/upload
  reduction, but it is not M4 completion while gameplay preparation remains.
```

## 2026-07-15 - corrected Mode-8 measurement; rejected per-joint GX hierarchy

```text
IDEA ID: M2-GX-HIERARCHY-FORMAL-RETEST
MEASUREMENT CORRECTION:
  The outer battle-playable verifier did not expose or forward
  RendererFastRunMode, so an apparent 2.65M-tick regression had silently run
  generic mode 0. The wrapper now forwards FastRunMode and the optional M2
  detailed ledger. All focused M2 commands must explicitly request mode 8.
CLEAN BASELINE:
  Profile-1 ROM SHA-256
  3F0ADCF3B89E02370ACFBAB5D2A34B251D6E1FE3CF3EEDB06C961C18A8287475,
  frames 600..607 / logic 209..216. Mario 188,352; Fox 223,872; combined
  412,224; matrix 161,152; production DL 217,312; stage 804,032; draw
  1,575,392; present 1,993,632. Exact mode-8 accounting is 70 runs / 686
  triangles, owners 60+320+306, fallbacks 29/0/0, and two uploads / 36,864
  bytes. The smoke marker is 17.2 FPS.
SAME-ROM A/B/A:
  Selector ROM SHA-256
  7E3D1C36DD38B36509C194C375568BCAE7548003D494F8942F317755D18F0259,
  frames 600..607, detailed ledger off. A0 and A1 reproduced exactly:
  Mario 190,784; Fox 226,688; combined 417,472; matrix 161,408; DL 220,096;
  draw 1,582,816. Treatment B was Mario 186,240; Fox 217,760; combined
  404,000; matrix 143,264; DL 253,216; draw 1,569,344.
RESULT / DECISION:
  The hierarchy saved only 13,472 combined ticks. Its 18,144-tick matrix win
  was offset by 33,120 additional DL ticks. It missed the >=80K keep gate by
  66,528 and missed the <=351K absolute gate by 53K. Remove the runtime/API/
  selector/validator/scratch path; retain only the source-faithful
  syMatrixF2LFixedW 16.16 -> DS 20.12 boundary and generated topology fixtures.
NEXT CUT:
  Compile one packed FIFO transaction per fighter and fuse exact matrix/light
  preparation with 49-epoch/67-run transport. First gate: A1=A0, combined
  <=337,472, matrix+light <=120K, transport <=145K, and unchanged semantics,
  screenshots, geometry, texture traffic, runtime state, and reserve.
```

## 2026-07-15 - rejected whole-owner FIFO copy/patch/DMA transport

```text
IDEA ID: M2-WHOLE-OWNER-FIFO-PACKET
HOST CONTRACT:
  Exact generated Mario/Fox packets remain useful fixtures: 4,034/3,936 words,
  16,136/15,744 bytes, 14/18 roots, 18/31 epochs, 30/37 runs, 320/306
  triangles, and template hashes 033874a6/791eb7a6. The independent checker
  validates every matrix/color/texcoord/epoch association and signed VERTEX16
  boundary.
DETAILED SAME-ROM A/B/A:
  Profile-1 ROM 10AFE933BFDD4E14C32B04AFE7DA0114F3E064F2686F5428AE6CCD5F1D8410E8,
  frames 600..607. Mode-8 A0/A1 coarse and M2 arrays reproduce exactly at a
  477,248-tick combined fighter median. Packet B is 599,040, a 121,792-tick
  regression. B validation is 79,488 versus 4,864; production is 293,088
  versus 245,888. Packet medians are copy/patch 67,296, state 71,552, cache
  flush 3,648, DMA-idle wait 128, DMA wall 20,672, with 2/2 successes.
CLEAN SAME-ROM A/B/A:
  Ledger-off ROM 13506F55B28FFC95EC5C38A4AA90B8B53F5A0B61EF17B63B1E701FE43B98589B,
  same frames. A0 and A1 reproduce exactly at 413,504 median / 413,632 P95;
  packet B is 537,792 / 537,856, +124,288 median. It misses the <=337,472
  first gate by 200,320 and supplies -124,288 rather than the required >=80K
  saving.
CORRECTNESS:
  Both selectors conserve 70 runs / 686 triangles, owners 60+320+306,
  fallbacks 29/0/0, batches 103/725/103, submit classes 648/44/126/10, and
  two uploads / 36,864 bytes. Dated mode-8 and mode-9 visibility captures are
  under artifacts/visibility; they are not synchronized-frame parity proof.
DECISION: REJECT / REMOVE RUNTIME; KEEP HOST FIXTURES
  Remove the Mode-9 selector, adapter, packet timing, and linked device path.
  Do not publish this laboratory ROM. Keep only the exact generator/checker
  evidence for a future design that avoids whole-packet copying and uncached
  validation while retaining source-exact CPU lighting.
```

## 2026-07-15 - retained exact M3 host packet; device path still absent

```text
IDEA ID: M3-PUPUPU-WHOLE-OWNER-HOST-PACKET
HOST RESULT:
  Commit cb742db044 records 8 callbacks, 57 DObjs, 42 bindings, 886 source
  commands, 302 source / 312 dense vertices, 202 triangles, 54 runs, 49
  epochs, four material events, and 10,076 bytes of packet rodata.
DYNAMIC CONTRACT:
  Production preflight must validate callback/link order, DFS topology, live
  draw flags, xobjs, four MObjs, textures, camera, and provenance before the
  first link-4 write. Camera, transforms, materials, colors/texcoords, epoch
  words, projected values, and frame-global no-Z state stay live. Fighters and
  weapons remain interleaved at links 9/14, so callback-sized rebind segments
  are mandatory. No fallback is legal after FIFO ownership begins.
DECISION: KEEP HOST ONLY
  production_linked=0. Extend the existing stage falsifier before adding a
  selector; later device B must save >=300K and reach <=500K before promotion.
```

## 2026-07-15 - retained exact M4 tiled-water host fixture and residency plan

```text
IDEA ID: M4-PUPUPU-TILED-WATER-PRELOAD
EXACT HOST RESULT:
  Commit c85bac721e/checker pins 181,408 bytes and production_linked=0:
  131,072 pair atlas + 16,384 visibility atlas + 20,480 palettes + 8,544
  state tables + 544 cells + 4,384 vertices.
PROPOSED SETUP BUDGET, NOT YET MEASURED:
  Stream/hash 258,048 NitroFS bytes before GO: 167,936 water payload plus
  90,112 deduplicated static pixels, using a reusable 4 KiB chunk. A+B post-GO
  is 241,664/262,144 with 20,480 headroom. Compact GO OAM to 60,544 bytes in E,
  then move the staged water palettes to F/G before control unlock. Estimated
  reserve is 152,080 without or 145,860 with the proposed live-key table;
  these omit new text, padding, allocator, and filesystem overhead, so
  linker/runtime proof is still required.
DECISION: KEEP HOST / IMPLEMENT ONLY BEHIND SETUP-FAIL-CLOSED GATE
  Require zero open/read/seek and zero conversion/upload/allocation/eviction/
  palette DMA after GO, >=128 KiB measured reserve, >=16 KiB A+B headroom,
  moving screenshots, and >=100K owner/draw saving. No gameplay cutover yet.
```

## 2026-07-15 - frozen-114 static residency supersedes animated water

```text
IDEA ID: M4-FROZEN114-STATIC-RESIDENCY
SUPERSEDES: M4-PUPUPU-TILED-WATER-PRELOAD
POLICY:
  Under the project-wide DS visual rule, presentation targets roughly 90%
  overall likeness. Cosmetic exactness gets one measured focused attempt; a
  miss that threatens P1 is replaced by the cheapest recognizable source-
  derived approximation. Gameplay semantics are never relaxed.
HOST:
  Freeze BattleShip frame 0, non-FRAC fraction 114, on original runs 42-43
  and 12 triangles. The corpus has 22 keys, 21 deduplicated outputs, 126,976
  payload bytes, and 131,072 prepared bytes. Large/small water SHA-256 values:
  f3a908659547f360ec9d3b79f80aa4c5dca829cdb36975a5d3a59667d1fdf532
  61b0bb44aa30033d0c8e07d924f6b38ddbafa23807692eb16aab194e57457efe
PUBLISHED DEVICE:
  ROM 3F3AC2E1A20F7D93B0E92419BA642FD5D97A275454ABEC0D1C96EF7742E6BB38.
  Canonical Boundary passes exact M3 8/57/42/54/202 ownership, water 2/0/1,
  22 pinned keys, exact 131,072-byte VRAM-A span, positive pinned hits, zero
  upload/CI4/refresh/evict/fallback/fence work, and exact GO frames 438/439.
  Frame-438 screenshot:
  artifacts/visibility/2026-07-15_canonical_fast_frame438-439_182430-9052820-p35520.png
  SHA-256:
  45DBCD24D2DAC91089A1AAD6AB430C05CB173BB4E3FCFFBACEBE9A323B040922
PERFORMANCE:
  Boundary is 19.8 FPS with about 1.54M present / 1.09M draw ticks. This is a
  semantic/residency keep, not an M3 tick-target or full-match M4 claim.
DECISION: KEEP FROZEN P1 PATH / RETIRE ANIMATED REPLACEMENT
  Never reopen the 167,936-byte/138-triangle animated-water candidate without
  a new user decision and a measured P1-critical regression. M4 still requires
  one-minute GO-to-teardown zero-fence and >=128 KiB reserve qualification.
```

## 2026-07-15 - M3 complete-stage first linked timing gate

```text
IDEA ID: M3-COMPLETE-STAGE-MODE9-LINKED
IDENTITY:
  ROM 2D8B9FFD8FD7BCD9512AFAB5F63F8AF01B6E128D6330006447CB4E988BF0E786,
  14,713,856 bytes. Frames 438-445, profile 1, detailed ledger, static 1,
  hybrid OAM 1, Fox decision/input paused. JSON SHA-256:
  CAD1B8D7925B1C432000FBD3FB6B54F6405166890BE58C1DBF5A1AC6EBC3B159
  Screenshot:
  artifacts/visibility/2026-07-15_m3-stage-mode9-profile1-s8-slot3.png
SEMANTICS:
  Exact 8 callbacks / mask 255 / 57 DObjs / 42 bindings / 54 runs / 202
  triangles / 49 epochs / four material commits / cross 5/10/15. Fast owner
  is 121/828 = 202+320+306 with zero fallback. M4 is 22/131072, armed, full
  masks, positive pinned hits, water 2/0/1, and every sampled fence class zero.
TIMING P50/P95:
  M3 stage exclusive 664,544/664,640; draw 1,183,104/1,183,168; present
  1,536,448/1,537,216; loop 1,680,448/1,680,512; FPS 19.6.
GATE:
  About 140K saved versus the documented ~805K stage baseline, not >=300K.
  Misses the <=500K first gate by 164,544 and remains far above 150-250K.
DECISION: REWORK
  Preserve the semantic packet/evidence while selecting the largest measured
  internal bucket. Do not spend the next increment on cosmetic stage fidelity.
```

## 2026-07-15 - M3 dense preparation reuse missed keep gate

```text
IDEA ID: M3-DENSE-PREPARE-ONCE
IDENTITY:
  Treatment ROM EAE5AD1AC6A054E237FE633AB8B96088DF0A2AEE757E8A0314C561D2975E0B7F,
  frames 438-445, profile 1, static 1, hybrid OAM 1, Fox paused. JSON SHA-256:
  263D147F90E8600EB2B858B172DCA2D0CE3F89657660EA6560E1FC4F252D3DE7.
RESULT P50/P95:
  A stage 664,544/664,640; B stage 555,584/555,776. Saving is
  108,960/108,864, below the required 164,544, and B remains 55,584 above the
  <=500K first gate. Exact 8/57/42/54/202, 121/828, M4 22/131072, zero
  fallback/fence, and the screenshot remained correct.
EVIDENCE:
  artifacts/visibility/m3-dense-prepare-8frames.json
  artifacts/visibility/m3-dense-prepare-frame438.png
  Screenshot SHA-256:
  2A54A1723DFA5C19D10AFA6AA75592E685A1963DE1E92586F7FDDB0F6AE0DE56
DECISION: REJECT / REVERT
  The source and checker changes were fully reverted. Do not retry or widen
  dense-only preparation reuse; choose a different measured M3 bucket.
```

## 2026-07-15 - M2 native-owner ITCM placement missed keep gate

```text
IDEA ID: M2-MODE8-ITCM-PLACEMENT
TREATMENT:
  Place the existing production lighting, run-preparation, and whole-owner
  executor in native-fighter ITCM. No mode, packet, cache, or traversal was
  added. Separately add the missing pre-GX active-animlock/shuffle rejection.
IDENTITY / WINDOW:
  Profile 1, Mode 8, ledger off, Fox paused, frames 600..607, runner 3.
  A/B ROM SHA-256 values are
  0D12911231D389E65A382210A215DBF3A95CB1C1852D4D409AEE216168AD813B /
  9AAA43BFA26F04B15E3888EF352F4E2ADC41DE9445F61C167532A24323CB37AC.
RESULT P50/P95:
  Combined Mario+Fox 416,576/416,704 -> 398,496/398,592, saving only
  18,080/18,112. It misses the required 80K saving by 61,920 and remains
  61,024 above the effective <=336,576 first ceiling. Candidate ITCM was
  25,292 bytes; link placement passed.
CORRECTNESS / VISUALS:
  Both arms preserve 70 runs / 686 triangles, stage/Mario/Fox 60/320/306,
  fallback state/vertex/command 29/0/0, identical upload SHA, and zero
  conservation error. Automated image analysis found 0/123,216 changed stage
  pixels; 13/122,808 HUD pixels differ only in live FPS text.
EVIDENCE:
  artifacts/performance/2026-07-15_m2-itcm-a.json (SHA-256
  847104F1C75038B1D520D6A68ABC1CA50DF1FF712AD9155CB81458798B3206CC)
  artifacts/performance/2026-07-15_m2-itcm-b.json (SHA-256
  88D42770A2743FCCC2CB126D03CF871C36DB0F9143EA3DA0F29B2D519F9F1753)
  artifacts/visibility/2026-07-15_m2-itcm-a-frame607.png
  artifacts/visibility/2026-07-15_m2-itcm-b-frame607.png
DECISION: REJECT PLACEMENT / KEEP SOURCE GUARD
  Revert all three attributes and their checker requirements. Do not retry
  placement alone. Retain the active-animlock/shuffle fail-closed guard because
  it restores the BattleShip matrix-state boundary before native prep/GX.
```

## 2026-07-15 - M3 scaled raw clipping fixes pause orbit and saves 19K

```text
IDEA ID: M3-SCALED-RAW-CLIP
TREATMENT:
  Keep the ten exact stage triangles whose coordinates only slightly exceed GX
  VTX_16 range in the native owner. Submit signed-rounded half coordinates with
  a compensating raw matrix so GX clips the near plane instead of submitting
  CPU-projected, unclipped screen-covering triangles.
IDENTITY / WINDOW:
  Profile 1, Mode 9, static 1, hybrid OAM 1, Fox paused, frames 438..445,
  runner 3. Treatment ROM SHA-256
  03BB711844ECF546562934982E2BBB612FCD9D7783343F53B5135DEFBA1CB42B.
RESULT P50/P95:
  Stage 664,544/664,640 -> 645,248/645,440, saving 19,296/19,200.
  Draw 1,183,104/1,183,168 -> 1,102,656/1,102,720, saving
  80,448/80,448. M3 remains 145,248 above the <=500K first gate.
CORRECTNESS / VISUALS:
  Exact 121 runs / 828 triangles = stage 202 + Mario 320 + Fox 306, zero
  fallback; M4 remains 22/131072 with every sampled post-GO fence class zero.
  Normal/front/+33.6 degree focused captures pass, and the owner accepted the
  repaired view. The synchronized GO frame remains recognizable and intact.
EVIDENCE:
  artifacts/performance/2026-07-15_m3-scaled-raw-clip-b.json (SHA-256
  A28A32E09D4676DC1DCA2C804424EE43B38BC2E047EF42794FE7635D8C6C4F83)
  artifacts/visibility/2026-07-15_m3-scaled-raw-clip-b-frame438.png (SHA-256
  DF967604D1AD4F3915D8612356AB5342791B522BCA0676FD3618535622C91FB2)
  artifacts/visibility/20260715-221450_slot3_p28976_mode163_camera_pause_plus33p6.png
  (SHA-256 8DB894DAB9677A56F1D5937C5E644ADE0712F53E2C84F53D8ADE327BCDFC22BE)
DECISION: KEEP CORRECTNESS FIX / M3 REMAINS REWORK
  The defect is removed and timing improves, but this is not an M3 close.
```

## 2026-07-15 - Correctness note: strict stage painter depths

This is not a performance row and records no timing claim. BattleShip
`grdisplay.c:52-63,86-95,111-118,134-141` confirms no-Z/Z/no-Z/no-Z stage
layers, and `grpupupu.c:637-690` preserves their construction order. The port's
scaled painter counter decremented by one before dividing by six, so six
successive no-Z triangles shared each submitted v16 depth. Decrementing by the
full step restores strict order; 128 endpoint values are reserved on each side
of the clamped real-Z range.

Profile 2 passes the exact immutable 202-triangle order: classes 66 source-Z,
126 no-Z, 10 range; background `4095..4024`, foreground `-3969..-4022`, real
stage Z `3605..3728`, zero collisions/overflow, hash `3BB26905`. Published ROM
SHA-256 `28492257B23E502AA8710C03CE22B0752D58CE3476A5DA86EBD819B1E6A78C4C`
passes frame 438 and moving frame 501 pixel gates. This predates M3; commit
`ef65ef541c` exposed the path but did not introduce the counter behavior.

## 2026-07-16 - Correctness note: fighter root light preambles

This is not a performance row. BattleShip applies fighter display-list state in
`ftdisplaymain.c:753-945,1164-1228`, establishes the model lights in
`ftdisplaylights.c:10-26`, and emits each MObj display list through the material
path in `objdisplay.c:1289-1296`. The generated native fighter IR preserved all
320 Mario triangles and all 306 Fox triangles, but omitted the four leading
`G_MW_LIGHTCOL` commands on each affected root. The right upper pant therefore
inherited the preceding shoe root's dark light state; its blue polygons rendered
nearly black and made the closed underside look incomplete.

The generator now decodes the exact source light prefixes from the hashed O2R
payload and stores a compact per-root selector in the previously reserved byte.
The renderer replays that state at every native-root entry, including hierarchy
preflight and commit. Owner ON/OFF A/B excluded the Mode-8 owner itself; the raw
source census remains symmetric at roots 8/11 (15 triangles), 9/12 (15), and
10/13 (18). Profile 2 preserves 828 total stage/fighter triangles, Mario raw /
cross-matrix classes `284/36`, Fox `298/8`, and zero fallback or M4 fence work.

Candidate ROM SHA-256
`6265772AB02446A1247DB8444129A3040835BDDDBC968A090DA2AA289423ED24`;
pause-orbit evidence is
`artifacts/visibility/20260716-034036_slot3_p10612_mode163_camera_pause_minus33p6.png`.
Both pant legs render blue and the underside is closed in that view. Decision:
KEEP pending the owner's visual confirmation. The camera gate reports 128,528 bytes
of arena headroom, but an isolated unmodified `7abdec43ef` baseline reports the
same value; the 2,544-byte P1 reserve deficit is inherited and is not bundled
into this correctness change.

## 2026-07-16 - M3 no-Z ARM hot-path code generation

```text
IDEA ID: M3-NOZ-HOT-CODEGEN
MEASURED LEAD:
  A one-run Phase-0 bucket probe attributed 174,624/174,720 stage ticks to the
  126-triangle no-Z emitter and counted 146 no-Z matrix loads per frame. The
  emitter was incorrectly marked cold and size-optimized despite running every
  stage frame. The temporary probe was removed before the A/B.
TREATMENT:
  Remove only cold/Os from ndsRendererNativeStageEmitNoZTriangle; retain
  noinline. devkitARM then places the function in normal .text and inlines the
  small vertex emitter. No renderer state, algorithm, packet, or data changes.
IDENTITY / WINDOW:
  Profile 1, Mode 9, static 1, hybrid OAM 1, Fox/countdown iteration switch off,
  frames 438..445. A/B ROM SHA-256 values are
  3075A41597394B74767D6F06B73E4D95062E24F784849E0BE7A2771F243ED186 /
  A732A135CDBA433867B642025512F42480D7AD3694E960992A070C6DF3ED4546.
RESULT P50/P95:
  Stage 624,384/624,512 -> 611,392/611,584, saving 12,992/12,928.
  Draw 1,062,944/1,063,040 -> 1,048,864/1,048,896, saving
  14,080/14,144. FPS remains 19.5; the VBlank-inclusive present/loop window is
  not used to judge this internal bucket.
CODEGEN / SIZE:
  Emitter moves from .text.unlikely to .text and grows 0x188 -> 0x2D8; the
  separate 0x98-byte vertex helper is inlined. Linked ELF grows 4,212 bytes,
  while the padded ROM size is unchanged; no ITCM placement changes.
CORRECTNESS / VISUALS:
  Both arms preserve exact 121/828 runs/triangles, stage/Mario/Fox
  202/320/306, zero fallback, and identical fastRaw/M3 semantic arrays. All DS
  screen pixels are identical; the only 35 screenshot differences are in the
  melonDS title bar. M4 remains prepared at 22/131072 and intentionally
  unarmed under the fast-iteration switch, with zero fences.
EVIDENCE:
  artifacts/performance/2026-07-16_m3-phase0-buckets.json
  artifacts/visibility/2026-07-16_m3-phase0-buckets-frame445.png
  artifacts/performance/2026-07-16_m3-noz-cold-a.json
  artifacts/performance/2026-07-16_m3-noz-cold-b.json
  artifacts/visibility/2026-07-16_m3-noz-cold-a-frame445.png
  artifacts/visibility/2026-07-16_m3-noz-cold-b-frame445.png
DECISION: KEEP
  This is one measured compiler-placement correction. M3 remains above its
  <=500K first gate; continue with the already-measured exact dense-reuse stack.
```

## 2026-07-16 - M3 dense stack rework on hardware no-Z baseline

```text
IDEA ID: M3-DENSE-STACK-20260716
AUTHORIZATION:
  ClaudeFable5_JumpABC_Tasks_20260715_2326.md permits the previously rejected
  dense-index preparation reuse only as a stack base and forbids shipping it
  unless the combined stage P50 reaches <=500K.
CUT 1 - DENSE PREPARE ONCE:
  A 40-byte stack bitset reuses the existing prepared slot for 606 corner
  references -> 312 exact dense vertices. Every reuse fail-closes unless its
  binding, texture epoch, submit class, policy, and flags match the first use.
  No persistent allocation or prepared-struct growth is added.
IDENTITY / WINDOW:
  Profile 1, Mode 9, static 1, hybrid OAM 1, Fox/countdown iteration switch
  off, frames 438..445. A/B ROM SHA-256 values are
  A732A135CDBA433867B642025512F42480D7AD3694E960992A070C6DF3ED4546 /
  98757813F61F8DA935E0192B41786653D244DEA3FB5ABE4BD1BB802FD9B9BC2D.
RESULT P50/P95:
  Stage 611,392/611,584 -> 563,296/563,392, saving 48,096/48,192.
  The smaller result than the old 108,960-tick experiment is expected: the
  current constant-Z GX path has already removed the old 486 CPU projections.
  Exact 121/828 and M3 8/255/57/42/54/202/49/4 remain unchanged; all DS-screen
  pixels are identical and only the melonDS title bar differs.
CUT 2 - INCREMENTAL NO-Z PROJECTION TRANSLATION:
  Tested the only bounded remaining transport reduction: keep painter Z in the
  projection matrix, preserve the existing rounded Z column with an exact
  one-LSB position correction, and reuse position matrices within a run. GBATEK
  and melonDS 1.1 both define the signed 20.12 multiply as a final >>12.
  Treatment ROM SHA-256
  9360EFA1E464F82008BF0222949F3CBA97D358E1BBD289B198BF665D3976CFEE.
  Stage regressed 563,296/563,392 -> 579,712/579,904, costing
  16,416/16,512. Exact counters and every DS-screen pixel remain unchanged.
  The treatment was removed.
STACK TOTAL / GATE:
  From the pre-codegen baseline, the kept codegen plus dense candidate reaches
  624,384 -> 563,296, saving 61,088, but remains 63,296 above <=500K. The task's
  bounded cuts are exhausted: there are zero CPU projection divides left, and
  incremental matrix transport is a measured regression.
EVIDENCE:
  artifacts/performance/2026-07-16_m3-dense-reuse-b.json
  artifacts/visibility/2026-07-16_m3-dense-reuse-b-frame445.png
  artifacts/performance/2026-07-16_m3-noz-projection-b.json
  artifacts/visibility/2026-07-16_m3-noz-projection-b-frame445.png
DECISION: REWORK / REVERT DENSE CANDIDATE
  Honor the stack-only rule: do not ship the 48K dense cut below the combined
  gate. Keep only the independent 12.99K codegen correction from bbe8d3eee2.
  Move to the independent M2 fighter-compute lane instead of widening M3.
```

## 2026-07-16 - M2 Jump C compute bound stopped before code

```text
IDEA ID: M2-JUMPC-COMPUTE-BOUND-20260716
IDENTITY / WINDOW:
  Profile 1, Mode 8 production owner, static 1, hybrid OAM 1, Fox/countdown
  iteration switch off, frames 600..607. ROM SHA-256 is
  49CEE7E87B709EFA49B8DC844467841525E941C14B70F6076815E42A37DC7846.
PHASE-0 RESULT P50/P95:
  Local matrix construction is 53,824/54,144 ticks; world-affine fixed-point
  composition is a separate 22,464/22,720 bucket. Lighting is 80,384/80,640.
  The locked ledger-off first-gate baseline remains 416,576/416,704.
SOURCE / CODEGEN FINDING:
  The Mode-8 fighter-parts path calls syMatrixTraRotRpyR or
  syMatrixTraRotRpyRSca at reloc_backend_renderer_dl.c:584-616. BattleShip
  matrix.c:966-1013 and 1023-1140 already implement their rotations with
  gSYSinTable and integer products; the live path does not call the remaining
  PYR sinf/cosf wrappers. The production lighting loop at nds_renderer.c:
  13649-13703 already uses the prepared direction and 128-entry exact shade
  LUT at 2159-2183. Its remaining work is the required signed normal dot
  product, LUT index, and exact material packing, not an unconverted color
  ramp.
BOUND:
  Even deleting the complete 53,824-tick eligible local-builder bucket and
  re-adding the previously measured 18,080-tick ITCM gain saves at most 71,904
  ticks, 8,096 short of the required 80,000. Lighting exposes no eligible
  residual under CUT 2; replacing the exact dot product or adding another
  speculative cache would manufacture a different lighting path. The fixed
  20.12 world-affine multiply is outside this task's source-table conversion
  cut and is not counted as hypothetical savings.
ITERATION CHECKER:
  Modes 7-9 now consistently accept the intentionally prepared-but-unarmed M4
  corpus and idle TEXEL0/TEXEL1 markers when the shared Fox/countdown switch is
  off. Strict M4 arming and texture-fence proof remain unchanged.
EVIDENCE:
  artifacts/performance/2026-07-16_m2-jumpc-a.json
  artifacts/visibility/2026-07-16_m2-jumpc-a-frame607.png
DECISION: REWORK / STOP BEFORE CODE
  No renderer candidate was built. The task's pre-code feasibility gate fails;
  retain the current exact source-table matrix and shade-LUT paths.
```

## 2026-07-16 - User withdrew intermediate discard gates; M3 dense reuse kept

```text
IDEA ID: M3-DENSE-RESTORE-20260716
POLICY:
  The <=500K Jump A stack gate and >=80K / <=336,576 Jump C pre-code keep
  gates are withdrawn. They remain milestone targets. Any repeatable
  correctness-preserving gain now accumulates instead of being reverted solely
  for missing an intermediate target.
IDENTITY / WINDOW:
  Mode 163, profile 1, mode 9, static 1, bitmap OAM, Fox/countdown iteration
  switch off, frames 438..445. A/B ROM SHA-256 values are
  426B821A8AE3CE91F1DDB0BA79ABD6B0EFEC31D3A11D029BDE5A42F67DDC5791 /
  C6EDC9B186A1B898E8672E82F4BAB70F9FB7A8D848E9785B17AD43E64ADB87D9.
RESULT P50/P95:
  Stage 619,744/619,904 -> 577,440/577,536, saving 42,304/42,368.
  Draw 1,057,856/1,057,920 -> 1,013,760/1,013,824, saving 44,096/44,096.
CORRECTNESS / VISUALS:
  A 40-byte frame-stack bitset prepares 606 references as 312 unique dense
  vertices. The host checker proves zero binding/epoch/class/policy/flags
  conflicts and 408 projected references -> 246 unique. Exact 121/828 and
  57/42/54/202/49/4 remain unchanged with zero fallback/fence work. The valid
  B2 capture is exactly equal to A over all 120,000 top-screen pixels; the
  first B capture was discarded as an offset/black window-capture failure.
EVIDENCE:
  artifacts/performance/2026-07-16_m3-dense-restore-a.json
  artifacts/performance/2026-07-16_m3-dense-restore-b2.json
  artifacts/visibility/2026-07-16_m3-dense-restore-a.png
  artifacts/visibility/2026-07-16_m3-dense-restore-b2.png
DECISION: KEEP
  Accumulate the 42.3K stage reduction and continue with Jump C.
```

## 2026-07-16 - M3 signed-16 rounding codegen rejected

```text
IDEA ID: M3-S16-ROUNDSHIFT-20260716
MEASURED LEAD:
  The retained no-Z triangle path expands signed-16 coordinate rounding through
  the generic signed-64 helper. In the ARM object, NativeStageVertexShift was
  0x15C bytes and NativeStageEmitNoZTriangle was 0x2D8 bytes.
TREATMENT:
  Use equivalent unsigned-magnitude 32-bit rounding only for the native-stage
  signed-16 vertex-shift and emit calls. No packet, matrix, or renderer state
  changed.
IDENTITY / WINDOW:
  Profile 1, Mode 9, static 1, hybrid OAM 0, Fox/countdown iteration switch
  off, frames 438..445. A/B ROM SHA-256 values are
  A982E5E9CC45BD3303EB363CBB6998EE4BCCE66C94161EC51CBC706FE463D8E6 /
  CF4F0E7364391C7DDFD6681360742D81C2FA7B8BE079D096897119ED8BD045D4.
RESULT P50/P95:
  Stage 612,832/612,928 -> 610,784/610,944, saving 2,048/1,984. Draw saved
  only 128 ticks while the whole loop changed by +32 ticks. The local result is
  roughly 110.8K short of the <=500K first gate.
CODEGEN / SIZE:
  NativeStageVertexShift shrank 0x15C -> 0xD0 and
  NativeStageEmitNoZTriangle shrank 0x2D8 -> 0x250. The lab ROM shrank 1,024
  padded bytes. This confirms the wide-shift lead but not a useful runtime win.
CORRECTNESS / VISUALS:
  Both arms retained exact 121/828 runs/triangles, stage/Mario/Fox
  202/320/306, zero fallback, and the complete M3 census. The B screenshot was
  mostly black instead of matching A, so the visual packet was internally
  inconsistent. No A2 was run because the measured saving already missed the
  gate decisively.
EVIDENCE:
  artifacts/performance/20260716_m3-s32-roundshift-a.json
  artifacts/performance/20260716_m3-s32-roundshift-b.json
  artifacts/visibility/20260716_m3-s32-roundshift-a.png
  artifacts/visibility/20260716_m3-s32-roundshift-b.png
DECISION: REVERT
  The source treatment is fully removed. Do not retry signed-16 round-shift
  codegen without a new attributable bound large enough to close M3.
```

## 2026-07-16 - M4 Whispy source-frame residency repair

```text
IDEA ID: M4-WHISPY-SOURCE-FRAME-REUSE-20260716
CLASS: correctness / critical-path residency, not an M3 optimization
ROOT CAUSE:
  Exact CPU-on phase sampling reached frame 1398 and falsified the prior
  one-minute M4 claim. Native-stage run 28, binding 22, root 0x1630, MObj
  0x13D8 selected a new Whispy mouth source image. The 22-key AOT pack excludes
  animated actors; the resident lookup rejected, M3 aborted, GL_DELETE became
  the first fence class, and generic fallback converted/uploaded 40 textures.
SOURCE:
  BattleShip grpupupu.c:250-347 drives Whispy wind state, :565-623 applies the
  live mouth/eye animations and texture selections, and :663-669 constructs
  the four map GObjs. StagePupupuFile3.c:463-483 defines mouth MObj 0x13D8 and
  :521-543 binds display-list root 0x1630.
TREATMENT:
  Native-stage resident preflight may reuse an unpinned pre-GO source-frame
  texture only when the primary image pointer is the sole difference in the
  complete 59-word renderer key. All state, geometry, timing, palette, combine,
  tile, dimensions, and sampling fields remain exact. Any other difference
  still fails closed. No asset, conversion, upload, or gameplay state changed.
IDENTITY / WINDOWS:
  Profile 1, Mode 9, static 1, hybrid OAM 0, CPU/countdown on. ROM SHA-256
  426B821A8AE3CE91F1DDB0BA79ABD6B0EFEC31D3A11D029BDE5A42F67DDC5791.
  Exact frames 1398..1405 and same-ROM late frames 3300..3307.
RESULT:
  Both windows retain 121/828 runs/triangles, stage/Mario/Fox 202/320/306,
  M3 57/42/54/202/49/4, zero post-arm fallback, and all ten post-GO texture
  fence classes zero. The late window also keeps zero premature teardown.
  Frame-1398 stage P50/P95 is 628,512/628,672; late stage is
  621,056/621,120. This repairs the multi-million-tick conversion cliff but is
  not credited as an M3 target win.
EVIDENCE:
  artifacts/performance/20260716_m4-source-frame-freeze-frame1398.json
  artifacts/performance/20260716_m4-source-frame-freeze-late3300.json
  artifacts/visibility/20260716_m4-source-frame-freeze-frame1398.png
  artifacts/visibility/20260716_m4-source-frame-freeze-late3300.png
DECISION: KEEP
  Accept the first-source-frame image as the smallest recognizable DS visual
  approximation. Refresh the natural teardown once at release qualification;
  do not stack another verifier now.
```

## 2026-07-16 - P1 hard feasibility phase packet

```text
IDEA ID: P1-PHASE-FEASIBILITY-20260716
CLASS: decision gate, not an optimization
QUESTION:
  Does the current source-faithful CPU-on renderer have a credible path to one
  VBlank (~560K ticks) in every material match phase by July 19?
IDENTITY:
  One exact profile-1 Mode-163 ROM, SHA-256
  426B821A8AE3CE91F1DDB0BA79ABD6B0EFEC31D3A11D029BDE5A42F67DDC5791;
  Mode 9 / static 1 / bitmap OAM / Fox CPU and countdown on. Eight synchronized
  completed frames per window; no rebuild between windows.
RESULT:
  countdown 438..445: active P50/P95 1,435,424/1,441,088, 19.6 FPS
  early 600..607:      active P50/P95 1,415,424/1,416,064, 19.2 FPS
  mid 1398..1405:     active P50/P95 1,616,000/1,617,920
  late 3300..3307:    active P50/P95 1,240,832/1,414,912
  Exact geometry stays 121 runs / 828 triangles and M4 remains armed with zero
  post-GO fence work in every sampled window.
EVIDENCE:
  artifacts/performance/20260716_p1-fixed-phase-countdown-frame438.json
  artifacts/performance/20260716_p1-fixed-phase-early-frame600.json
  artifacts/performance/20260716_m4-source-frame-freeze-frame1398.json
  artifacts/performance/20260716_m4-source-frame-freeze-late3300.json
  artifacts/visibility/20260716_p1-fixed-phase-countdown-frame438.png
  artifacts/visibility/20260716_p1-fixed-phase-early-frame600.png
  artifacts/visibility/20260716_m4-source-frame-freeze-frame1398.png
  artifacts/visibility/20260716_m4-source-frame-freeze-late3300.png
DECISION: STOP 60-FPS PROMOTION
  P95 is 2.53-2.89 times the one-VBlank budget. The retained M2/M3 bounds do
  not close the 855K-1.058M deficit, so request approval for a stable 20 FPS
  presentation target. Do not silently claim P1 complete at a lower rate.
```

## 2026-07-16 - M2 native-fighter ITCM gain restored and kept

```text
IDEA ID: M2-MODE8-ITCM-RESTORE-20260716
POLICY:
  The old 80K / <=336,576 intermediate discard gate is withdrawn. The final
  170-250K milestone target remains; repeatable correct gains accumulate.
IDENTITY / WINDOW:
  Mode 163, profile 1, Mode 8, static 0, bitmap OAM, Fox/countdown iteration
  switch off, frames 600..607. A/B ROM SHA-256 values are
  6F6AC367CB576A129CD4A17DA7E62C3CD011B3B53CE2F7163062A57F172CD780 /
  0C22A7FC41C0D6997940D931B7B6E67758A7A4E499E8C9C59EF45D60F49D0553.
TREATMENT:
  Place the existing native-fighter shading, run-preparation, and production
  executor functions in the existing ARM/O3 native-fighter ITCM section. No
  packet, traversal, cache, mode, allocation, or renderer state changed.
RESULT P50/P95:
  Combined fighter 419,328/419,392 -> 402,560/402,624, saving 16,768/16,768.
  Draw 1,245,024/1,247,616 -> 1,230,336/1,232,832, saving 14,688/14,784.
  ITCM is 25,384/32,768 bytes; placement checker accounts 22,868 renderer bytes.
CORRECTNESS / VISUALS:
  Exact 70 runs / 686 triangles and 60/320/306/29/0/0 fast partition remain;
  conservation is zero. All 120,000 top-screen pixels are identical. The
  benchmark wrapper exported both full packets before its unrelated unarmed
  TEXEL assertion; no verifier was weakened or new mode added.
EVIDENCE:
  artifacts/performance/2026-07-16_m2-itcm-restore-a.json
  artifacts/performance/2026-07-16_m2-itcm-restore-b.json
  artifacts/visibility/2026-07-16_m2-itcm-restore-a.png
  artifacts/visibility/2026-07-16_m2-itcm-restore-b.png
DECISION: KEEP
  Accumulate the gain. Next take one source-backed cut against the measured
  53,824-tick local-matrix builder; do not require the removed 80K gate.
```

## 2026-07-16 - M3 AOT coordinate shift removed runtime search

```text
IDEA ID: M3-AOT-COORDINATE-SHIFT-20260716
RECONCILIATION:
  the owner's Jump A Cut 3 constant-depth GX painter mechanism already landed with
  the pause/depth work. Its normal M3 path no longer calls the old CPU div64
  projector, so duplicating Cut 2 or Cut 3 would be incorrect. The retained
  Phase-0 bucket is 174,624/174,720 no-Z emitter ticks and 146 matrix loads.
TREATMENT:
  The generator packs each immutable source vertex's exact GX coordinate shift
  into unused high bits of its source cache byte. The emitter reads that value
  directly instead of searching shifts 0..5 for every triangle. Cache-slot low
  bits, matrix binding, geometry, painter depth, and the 12,663-byte slab stay
  unchanged. Shift census is 257 at zero and 55 at one.
IDENTITY / WINDOW:
  Mode 163, profile 1, Mode 9, static 1, bitmap OAM, Fox/countdown iteration
  switch off, frames 438..445. A/B ROM SHA-256 values are
  C4FBEEE4A478832E24E1D0E4C107AE61FB92238E68E8BCE0D349622AB002AC87 /
  090680BAE84A9D8B425DE5F7B8942DD58E0FC8EDF2672CB2B54B7D59C92F4A34.
RESULT P50/P95:
  Stage 578,272/578,560 -> 556,256/556,352, saving 22,016/22,208.
  Draw 997,440/997,504 -> 975,360/975,488, saving 22,080/22,016.
CORRECTNESS / VISUALS:
  Exact 121/828, 57/42/54/202/49/4, cross 5/10/15, zero fallback,
  conservation, and all M4 fence classes hold. Both 120,000-pixel DS screen
  regions are identical. Generated slab size and runtime RAM are unchanged.
EVIDENCE:
  artifacts/performance/2026-07-16_m3-aot-shift-a.json
  artifacts/performance/2026-07-16_m3-aot-shift-b.json
  artifacts/visibility/2026-07-16_m3-aot-shift-a.png
  artifacts/visibility/2026-07-16_m3-aot-shift-b.png
DECISION: KEEP
  Accumulate the gain and continue against the same measured no-Z path.
```

## 2026-07-16 - M3 zero-shift matrix builder specialization

```text
IDEA ID: M3-ZERO-SHIFT-RAW-MATRIX-20260716
TREATMENT:
  Of 146 no-Z matrix loads, 118 use the exact zero coordinate shift. Route
  those through the existing raw matrix builder; retain the shifted builder
  unchanged for the remaining 28 loads. Geometry and packet data do not move.
IDENTITY / WINDOW:
  Mode 163, profile 1, Mode 9, static 1, bitmap OAM, Fox/countdown iteration
  switch off, frames 438..445. A is the committed AOT-shift baseline ROM
  090680BAE84A9D8B425DE5F7B8942DD58E0FC8EDF2672CB2B54B7D59C92F4A34;
  B is 84569EEF0EE5301FD72053CFACDC35C78B6C8B057A1E9D65B424C9FB67891E67.
RESULT P50/P95:
  Stage 556,256/556,352 -> 541,952/542,272, saving 14,304/14,080.
  Draw 975,360/975,488 -> 962,816/962,880, saving 12,544/12,608.
CORRECTNESS / VISUALS:
  Exact 121/828, 57/42/54/202/49/4, cross 5/10/15, zero fallback,
  conservation, and all M4 fence classes hold. The 120,000-pixel top gameplay
  region is identical; the bottom content is identical except the final
  eight-row emulator-window edge outside the DS HUD image.
EVIDENCE:
  artifacts/performance/2026-07-16_m3-aot-shift-b.json
  artifacts/performance/2026-07-16_m3-zero-shift-fast-b.json
  artifacts/visibility/2026-07-16_m3-aot-shift-b.png
  artifacts/visibility/2026-07-16_m3-zero-shift-fast-b.png
DECISION: KEEP
  Accumulate the exact gain. M3 remains above its final 150-250K target.
```

## 2026-07-16 - M2 exact power-of-two matrix quantization

```text
IDEA ID: M2-JUMPC-POW2-QUANTIZE-20260716
PHASE 0:
  Mode 8, profile 1 detailed ledger, static 1, hybrid OAM 1, Fox/countdown
  iteration switch off, frames 600..607. Local matrices are 53,024/53,120
  ticks and lighting is 67,808/68,032. The owner performs 50 local builds.
SOURCE / CODEGEN:
  BattleShip matrix.c:966-1140 already uses gSYSinTable and integer rotation
  products. Actual ARM code still calls __aeabi_fmul plus __aeabi_f2iz for
  FTOFIX32 and power-of-two scale boundaries: syMatrixF2LFixedW is 0x134 bytes,
  syMatrixRotRpyR 0x19c, syMatrixTraRotRpyR 0x1dc, and the scaled form 0x248.
TREATMENT:
  The fighter adapter preserves the exact source 16.16 Mtx layout while
  converting finite power-of-two float scales from IEEE-754 fields directly.
  Cached source float matrices avoid syMatrixF2LFixedW soft-float calls;
  unscaled RPY matrices use the source rotation and exact translation pack.
  Unsupported values fall back to the original source functions. Profile 2
  retains the original functions as the shadow oracle.
IDENTITY / WINDOW:
  Mode 163, profile 1, Mode 8, static 1, hybrid OAM 1, Fox/countdown iteration
  switch off, frames 600..607. A ROM is
  0C22A7FC41C0D6997940D931B7B6E67758A7A4E499E8C9C59EF45D60F49D0553;
  B ROM is ECDE537C0081B55A1A80474AC5A2B15FC339E01626AE69413311F4188EEC888C.
RESULT P50/P95:
  Combined Mario+Fox 402,560/402,624 -> 398,048/398,144, saving 4,512/4,480.
CORRECTNESS / VISUALS:
  Exact 70/686 and 60/320/306/29/0/0, zero fallback/texture fence, and zero
  conservation error hold. the owner inspected the candidate PNG and confirmed it
  is visually normal. No allocation, packet, or ITCM change is introduced.
EVIDENCE:
  artifacts/performance/2026-07-16_m2-itcm-restore-b.json
  artifacts/performance/2026-07-16_m2-jumpc-pow2-b.json
  artifacts/visibility/2026-07-16_m2-itcm-restore-b.png
  artifacts/visibility/2026-07-16_m2-jumpc-pow2-b2.png
DECISION: KEEP
  Accumulate the exact compute gain. Lighting has no eligible residual; move
  to the measured production emit/account path.
```

## 2026-07-16 - M2 complete raw-emitter loop specialization

```text
IDEA ID: M2-RAW-EMITTER-SPECIALIZE-20260716
RECONCILIATION:
  Jump C's 246-Mario/234-Fox KRAW premise predates the Mode-8 production owner.
  The current owner already submits all 582 raw fighter triangles plus 44
  cross-matrix triangles. No coverage extension or new interpreter is needed.
TREATMENT:
  Select raw versus cross-matrix and textured versus untextured once per run,
  outside the raw corner loop. Packed corners, dense vertices, colors, texture
  coordinates, GX writes, cross-matrix path, and accounting remain unchanged.
IDENTITY / WINDOW:
  Mode 163, profile 1, Mode 8, static 1, hybrid OAM 1, Fox/countdown iteration
  switch off, frames 600..607. A/B ROM SHA-256 values are
  ECDE537C0081B55A1A80474AC5A2B15FC339E01626AE69413311F4188EEC888C /
  C808F40EB1F7240DF07B6B4867ED357F0FDD1F1DF4BB7E7554B15C0B49F5819B.
RESULT P50/P95:
  Combined Mario+Fox 398,048/398,144 -> 397,248/397,312, saving 800/832.
CORRECTNESS / VISUALS:
  Exact 70/686 and 60/320/306/29/0/0, zero fallback/texture fence, and zero
  conservation error hold. The deterministic native 256x192 top-screen compare
  is 0/49,152 changed pixels. ITCM grows 128 bytes to 25,512/32,768.
EVIDENCE:
  artifacts/performance/2026-07-16_m2-jumpc-pow2-b.json
  artifacts/performance/2026-07-16_m2-raw-emitter-specialize-b.json
  artifacts/visibility/2026-07-16_m2-jumpc-pow2-b2.png
  artifacts/visibility/2026-07-16_m2-raw-emitter-specialize-b.png
DECISION: KEEP
  Accumulate the exact gain under the user-approved no-discard policy.
```

## 2026-07-16 - M2 production-owner accounting batch

```text
IDEA ID: M2-PRODUCTION-ACCOUNT-BATCH-20260716
TREATMENT:
  Preserve every raw/cross triangle and reuse increment, but accumulate the
  complete Mode-8 production-owner counts during traversal and apply each
  accounting class once. The fail-closed exit flushes partial totals before
  returning, so rejected-owner diagnostics retain their prior meaning.
IDENTITY / WINDOW:
  Mode 163, profile 1, Mode 8, static 1, hybrid OAM 1, Fox/countdown iteration
  switch off, frames 600..607. A/B ROM SHA-256 values are
  C808F40EB1F7240DF07B6B4867ED357F0FDD1F1DF4BB7E7554B15C0B49F5819B /
  B7E6A95300AA38431CA4C3444CCA2983CB62E1BBB265C04E359269D4463647B0.
RESULT P50/P95:
  Combined Mario+Fox 397,248/397,312 -> 395,264/395,328, saving 1,984/1,984.
  Draw 1,025,152/1,025,216 -> 1,022,944/1,022,976, saving 2,208/2,240.
CORRECTNESS / VISUALS:
  Exact 70/686 and 60/320/306/29/0/0, zero fallback/texture fence, and zero
  conservation error hold. The deterministic native 256x192 top-screen compare
  is 0/49,152 changed pixels. ITCM is 25,972/32,768.
EVIDENCE:
  artifacts/performance/2026-07-16_m2-raw-emitter-specialize-b.json
  artifacts/performance/2026-07-16_m2-batched-account-b.json
  artifacts/visibility/2026-07-16_m2-raw-emitter-specialize-b.png
  artifacts/visibility/2026-07-16_m2-batched-account-b.png
DECISION: KEEP
  Accumulate the exact gain and continue against production emission.
```

## 2026-07-16 - M2 AOT-packed GX vertex coordinates

```text
IDEA ID: M2-AOT-GX-VERTEX16-20260716
SOURCE / REPRESENTATION:
  BattleShip objdisplay.c:1560-1603 preserves DObj traversal and display-list
  order; gbi.h:1802-1830 and :2018/:2170 define source vertex loads and
  triangles. The DS backend reference submits native v16 coordinates directly
  at decomp/sm64-nds/src/nds/nds_renderer.c:386-401. The existing owner
  generator's pack_fifo_vertex16 oracle already range-checks and encodes the
  exact GX words.
HOT SYMBOL / CALLERS:
  ndsRendererNativeEmitProductionRun.constprop.0, called by the Mode-8
  production owner for 1,878 fighter corners per frame.
BEFORE ADDRESS / SIZE / SECTION / STACK:
  0x01FFBEB8 / 0x270 / ITCM / 32-byte saved-register frame. Executor is
  0x9C8 with a 112-byte frame. Object/ELF SHA-256 values are
  72C5F02F1D0294529E812FA46618477C06187C3E13325F65FE32E9C2EC2A6164 /
  4279639E14EDA7E4109A543730DF31983E8238034D600C8797A9370422F97123.
BEFORE EXPENSIVE SEQUENCE:
  Every corner loaded raw s16 x/y/z, shifted each coordinate into v16, packed
  x/y with ORR, then wrote the same two GX FIFO words.
ONE CHANGE:
  Replace the same six raw-coordinate bytes in NDSNativeDenseVertex with the
  generator's exact u32 xy and u16 z words. Runtime performs two direct loads
  and two FIFO stores. The record remains 16 bytes; no cache or second path.
AFTER ADDRESS / SIZE / SECTION / STACK:
  0x01FFBE7C / 0x218 / ITCM / unchanged 32-byte saved-register frame. Executor
  remains 0x9C8/112 bytes. Object/ELF SHA-256 values are
  E26CC6353C6324E03F69430A34F314C72E63C7342B4D647FF229D64B222941B8 /
  7A91E2BB0DB9CA9459211F818C6DCD3700CE90BF0BC8366503E1F13B34C32C46.
MAP / TCM / ROM DELTA:
  Emitter shrinks 88 bytes; total ITCM falls 25,972 -> 25,824. The generated
  dense slab and ROM byte size are unchanged.
IDENTITY / WINDOW:
  Mode 163, profile 1, Mode 8, static 1, hybrid OAM 1, Fox/countdown iteration
  switch off, exact-aspect 416x664 window, frames 600..607. A/B ROM SHA-256:
  B7E6A95300AA38431CA4C3444CCA2983CB62E1BBB265C04E359269D4463647B0 /
  11048184FEF9390B0EE21608B39D4A38A5016E8318D244AADAA8636AAC61263D.
P50/P95 RESULT:
  Combined Mario+Fox 395,264/395,328 -> 386,880/386,944, saving 8,384/8,384.
  Draw 1,022,944/1,022,976 -> 1,014,560/1,014,592, saving 8,384/8,384.
EXACTNESS GATES:
  Generator and 32-root/49-epoch/67-run/626-triangle hierarchy checks pass.
  Exact 70/686 and 60/320/306/29/0/0, zero fallback/texture fence, and zero
  conservation error hold. Native top-screen delta is 0/49,152 pixels.
EVIDENCE:
  artifacts/performance/2026-07-16_m2-aot-vertex16-a.json
  artifacts/performance/2026-07-16_m2-aot-vertex16-b.json
  artifacts/visibility/2026-07-16_m2-aot-vertex16-a.png
  artifacts/visibility/2026-07-16_m2-aot-vertex16-b.png
KEEP / REWORK / REVERT: KEEP
  Accumulate the exact AOT representation gain. No A2 is warranted.
```

## 2026-07-16 - M2 co-located GX output record

```text
IDEA ID: M2-HOT-OUTPUT-RECORD-20260716
BOUND:
  Source-proven write elision is not viable: only 63/1,878 consecutive corners
  repeat color, and textured runs repeat UV only 20 times. A per-corner branch
  would cost more than the removed FIFO writes.
HOT SYMBOL / CALLERS:
  ndsRendererNativeEmitProductionRun.constprop.0, called by the Mode-8 owner
  for all 1,878 fighter corners each frame. The prior loop indexed separate
  12-byte prepared and 16-byte immutable geometry records.
ONE CHANGE:
  Keep source metadata in a 12-byte record and generate one mutable 16-byte
  output record containing the immutable AOT GX xy/z plus prepared color/UV.
  The raw loops now calculate one power-of-two record address. Source order,
  shading, texture coordinates, GX values, and the cross-matrix path remain
  unchanged.
CODEGEN / MEMORY:
  Emitter 0x01FFBE7C/0x218 -> 0x01FFBE98/0x224 in ITCM; total ITCM
  25,824 -> 25,864. Dense/prepared RAM remains exactly
  6,492+8,656 = 15,148 bytes versus 8,656+6,492 = 15,148 before. The ROM grows
  6,144 bytes because the AOT output record is initialized. Before/after ELF
  SHA-256 values are 7A91E2BB... / 96A4DF59...; after renderer object SHA-256
  is B16ED314596A4180D72C3EA038C2FA6E8F12E2C1492BDA2B808219F4CEE0931A.
IDENTITY / WINDOW:
  Mode 163, profile 1, Mode 8, static 1, hybrid OAM 1, Fox/countdown iteration
  switch off, exact-aspect 416x664 window, frames 600..607. A/B ROM SHA-256:
  11048184FEF9390B0EE21608B39D4A38A5016E8318D244AADAA8636AAC61263D /
  3360DA58F8D24D8A9575CE97BB5A81AC9D3686B3A4421F2A14DB130A19423A63.
P50/P95 RESULT:
  Combined Mario+Fox 386,880/386,944 -> 384,000/384,000, saving 2,880/2,944.
  Draw 1,014,560/1,014,592 -> 1,011,904/1,012,032, saving 2,656/2,560.
EXACTNESS GATES:
  Generator and 32-root/49-epoch/67-run/626-triangle hierarchy checks pass.
  Exact 70/686 and 60/320/306/29/0/0, zero fallback/texture fence, and zero
  conservation error hold. Native top-screen delta is 0/49,152 pixels.
EVIDENCE:
  artifacts/performance/2026-07-16_m2-hot-output-a.json
  artifacts/performance/2026-07-16_m2-hot-output-b.json
  artifacts/visibility/2026-07-16_m2-hot-output-a.png
  artifacts/visibility/2026-07-16_m2-hot-output-b.png
KEEP / REWORK / REVERT: KEEP
  Accumulate the exact gain. A2 is not warranted.
```

## 2026-07-16 - M3 bounded signed-16 rounding retake

```text
IDEA ID: M3-S16-ROUNDSHIFT-RETAKE-20260716
RECONCILIATION:
  The earlier treatment was reverted under the withdrawn <=500K discard gate
  and a viewer-side screenshot false alarm. the owner confirmed that PNG looked
  normal; deterministic native-pixel comparison now owns the visual decision.
SOURCE / BOUND:
  Generated NDSNativeStageDenseVertex coordinates are signed 16-bit and encode
  only shifts 0..7. Exhaustive host comparison proves the bounded formula
  equals the generic signed-64 nearest-round result for all 65,536 values and
  all eight shifts. BattleShip objdisplay.c:1557-1603 remains authoritative for
  DObj/display-list order; no source-visible order or geometry changes.
HOT SYMBOL / CALLERS:
  ndsRendererNativeStageEmitNoZTriangle, once per 126 no-Z stage triangles,
  reached by ndsRendererCommitNativeStageSegment across the eight exact stage
  owners. The emitted vertex helper performs three coordinate shifts.
BEFORE ADDRESS / SIZE / SECTION / STACK:
  EmitNoZTriangle 0x0200FF08 / 0x2D4 / main RAM text / 64-byte saved-register
  plus local frame. CommitNativeStageSegment is 0x8CC. Lab ELF SHA-256 is
  5C0BFF706A8FC4BC0BA120688A0F29D0F8AEC687013E6CAF4DD4C9B89F50B2E5.
BEFORE EXPENSIVE SEQUENCE:
  Each signed-16 coordinate routes through ndsRendererRoundShiftS64, expanding
  to multi-register 64-bit sign, add/borrow, shift, and merge sequences.
ONE CHANGE:
  Use one bounded signed-16 magnitude rounder at the three stage vertex seams.
  Generic matrix and clip-space signed-32/64 rounding remains unchanged.
AFTER ADDRESS / SIZE / SECTION / STACK:
  EmitNoZTriangle 0x0201099C / 0x23C / main RAM text / unchanged stack.
  CommitNativeStageSegment shrinks to 0x868. The helper is fully inlined and
  the 64-bit sequences disappear. Renderer object SHA-256 is
  78B64C5230E0644377B141A5D1853117612AB2043FCA9F6829F89FE88160ADDC;
  lab ELF SHA-256 is
  A4A852EE524122E04FC90F2FA6AE1ED8D403B31BE7A5CBA9E83FBF4764CC513A.
MAP / TCM / ROM DELTA:
  Main-RAM text shrinks; ITCM is untouched. The ELF shrinks 2,868 bytes and
  the padded lab ROM remains 14,760,960 bytes.
IDENTITY / WINDOW:
  Mode 163, profile 1, Mode 9, static 1, bitmap OAM, Fox/countdown iteration
  switch off, exact-aspect window, frames 438..445. A/B ROM SHA-256 values are
  E4E77B1E45DC4E9580C5DC6AC943B5FE41F54B146D88AFC40E733286BDEEF718 /
  AF6303BCD0D7A8FBBD23EC92A14C96C11A4B24B15F19992317C51909A500D5F3.
P50/P95 RESULT:
  Stage 545,440/545,536 -> 536,032/536,256, saving 9,408/9,280.
  Draw 949,824/949,824 -> 936,704/936,832, saving 13,120/12,992.
  Active 954,560/954,624 -> 941,472/941,568, saving 13,088/13,056.
EXACTNESS GATES:
  Generated stage and host packet checks preserve 8/57/42/54/202/49/4 and
  cross 5/10/15. Runtime preserves exact 121/828 and 202/320/306 with zero
  fallback, fence, or conservation error. Native top-screen delta is
  0/49,152 pixels.
EVIDENCE:
  artifacts/performance/2026-07-16_m3-s16-retake-a.json
  artifacts/performance/2026-07-16_m3-s16-retake-b.json
  artifacts/visibility/2026-07-16_m3-s16-retake-a.png
  artifacts/visibility/2026-07-16_m3-s16-retake-b.png
KEEP / REWORK / REVERT: KEEP
  The old visual rejection is superseded. A2 is not warranted.
```

## 2026-07-16 - Task 6 R0 and M3 Phase-0 decomposition

```text
IDEA ID: TASK6-R0-M3-PHASE0-20260716
RECONCILIATION:
  The frozen uninstrumented profile-1 mode-9 baseline ROM is
  58554D8361E77B6988F8F6C94F2BDB8A8F6FC81EE04D74B19FF08AC46E8E03B1.
  It uses static residency 1, bitmap OAM, live level-3 Fox, and the original
  countdown path. Generated stage topology is 312 dense vertices and 408
  projected references resolving to 246 unique references: 226 no-Z plus 20
  range, with 162 repeated references removed by the retained dense owner.
REAL COMBAT BASELINE, FRAMES 600..607, P50/P95:
  Two-update batch is 314,144/315,520 ticks. Draw is
  1,296,992/1,297,920, flush is 64/64, stage is 539,616/539,904, and active is
  1,301,728/1,302,720. The requested real draw+flush P50 is therefore
  1,297,056 ticks, not the earlier 936K-1,014K cross-build estimate. It is
  507,056 ticks above the approximate 790K full-speed target.
INSTRUMENTED IDENTITY:
  The default-off profile-1-only Phase-0 ROM is
  228381E6E81971022CC37B6E1245A006E720C6146EAFB07D7318E7C8EFE1DB5D.
  It records 1,319 timer reads and 651 nested spans per frame; 16 calibration
  intervals measure 64/64 ticks per read. Stopwatch buckets below are not
  additive because preflight, prepare, no-Z, and commit are nested.
PHASE-0 BUCKETS, FRAMES 600..607, P50/P95 TICKS:
  Preflight 327,296/327,360; prepare-runs 255,808/255,936; raw attribute-
  exclusive 118,336/118,528; near transform 34,432/34,688; prepare residual
  103,136/103,552; preflight residual 71,520/71,872; begin/bind
  39,936/40,192; 66-raw emit 9,088/9,088; 10-range emit 2,112/2,112;
  126-no-Z inclusive 98,816/99,008, including matrix 50,528/50,880 and
  exclusive emission 48,384/49,088; accounting 7,840/8,064; commit
  170,528/170,688 with residual 12,832/13,184.
CALIBRATED BOUNDS:
  Removing measured timer-read cost gives a 100,416-tick P50 upper bound for
  attribute work and about 32,096 ticks for no-Z matrix work. These are bounds,
  not promised savings. Counts are exactly 312 prepared dense vertices, 226
  near transforms, and 146 no-Z matrix preparations.
CORRECTNESS:
  Both synchronized windows preserve owner 121/828, stage
  8/255/57/42/54/202/49/4, cross 5/10/15, M4 22/131072, and zero fallback,
  post-GO texture-fence work, or conservation error.
EVIDENCE:
  artifacts/performance/2026-07-16_task6-r0-fighter600.json
  artifacts/performance/2026-07-16_task6-r0-stage438.json
  artifacts/performance/2026-07-16_task6-phase0-fighter600.json
  artifacts/performance/2026-07-16_task6-phase0-stage438.json
DECISION: PROFILE / PROCEED
  The calibrated attribute surface is real enough for a narrow source-backed
  cut. No FIFO packet caching, new interpreter, ordering change, or polygon/
  translucency change is authorized.
```

## 2026-07-16 - Task 6 Cut C first-use attribute preparation

```text
IDEA ID: TASK6-CUT-C-FIRST-USE-ATTRIBUTE-20260716
BOUND:
  Phase 0 places the calibrated attribute upper bound at 100,416 ticks P50.
ONE CHANGE:
  Classify run alpha once while retaining exact per-corner source-alpha checks
  for vertex-alpha runs. Consult the 312-dense first-use mask before building
  the full input record, so repeated projected references do not reconstruct
  attributes. Generated packet data, source order, run boundaries, submit
  classes, matrix transport, texture state, alpha, poly ID, and fallback are
  unchanged.
IDENTITY:
  A/B ROM SHA-256 values are
  58554D8361E77B6988F8F6C94F2BDB8A8F6FC81EE04D74B19FF08AC46E8E03B1 /
  078AD28EED968E8E7355A8971D567887677A7F56BDE5FA935906B65B5588945F.
COMBAT WINDOW 600..607, P50/P95:
  Stage 539,616/539,904 -> 497,632/497,792, saving 41,984/42,112.
  Draw 1,296,992/1,297,920 -> 1,257,440/1,258,304, saving 39,552/39,616.
  Active 1,301,728/1,302,720 -> 1,262,208/1,263,040, saving 39,520/39,680.
  Draw+flush P50 becomes 1,257,504 ticks.
STAGE WINDOW 438..445, P50/P95:
  Stage 539,520/539,712 -> 497,472/497,600, saving 42,048/42,112.
  Draw 1,609,056/1,609,792 -> 1,568,160/1,569,024, saving 40,896/40,768.
  Active 1,613,824/1,614,592 -> 1,572,896/1,573,760, saving 40,928/40,832.
EXACTNESS GATES:
  Both windows retain owner 121/828, stage 8/255/57/42/54/202/49/4, cross
  5/10/15, M4 22/131072, and zero fallback, fence, or conservation error.
  The deterministic native 256x192 top-screen A/B is raw 0/49,152 and
  meaningful 0/49,152 changed pixels.
EVIDENCE:
  artifacts/performance/2026-07-16_task6-cut-c-fighter600.json
  artifacts/performance/2026-07-16_task6-cut-c-stage438.json
  artifacts/performance/2026-07-16_task6-cut-c-phase0-fighter600.json
  artifacts/performance/2026-07-16_task6-cut-c-visual-{a,b}.json
  artifacts/visibility/2026-07-16_task6-cut-c-{a,b}.png
KEEP / REWORK / REVERT: KEEP
  Bank the repeatable exact gain under the no-discard override.
```

## 2026-07-16 - Task 6 Cut D valid-color stage seam and stop

```text
IDEA ID: TASK6-CUT-D-VALID-COLOR-SEAM-20260716
BOUND:
  Cut C's refreshed profile leaves 75,456 raw attribute-exclusive ticks, or a
  calibrated 57,536-tick P50 upper bound. Every stage dense vertex has valid
  immutable RGBA, making only the generic invalid-vertex lighting fallback
  unreachable at this owner boundary.
ONE CHANGE:
  Share the existing resolved-color tail between generic and stage callers.
  The generic caller keeps invalid-vertex lighting fallback; the stage owner
  passes valid dense RGBA directly and constructs an input record only for the
  226 no-Z transforms that consume XYZ. Material-only, white, vertex/material
  modulation, source alpha, texture, packet, ordering, matrix, poly ID, and
  translucency behavior remain unchanged.
IDENTITY / CODEGEN:
  A/B ROM SHA-256 values are
  078AD28EED968E8E7355A8971D567887677A7F56BDE5FA935906B65B5588945F /
  2868DEC6573EB9FE2347FB4349AD2C70E54590AE8AD47A7C1523ABB62F508ECC.
  The inlined prepare owner grows from 0xCB0 to 0xD70 bytes; measured end-to-
  end timing, rather than symbol size, decides the cut.
COMBAT WINDOW 600..607, P50/P95:
  Stage 497,632/497,792 -> 489,184/489,536, saving 8,448/8,256.
  Draw 1,257,440/1,258,304 -> 1,245,600/1,246,464, saving 11,840/11,840.
  Active 1,262,208/1,263,040 -> 1,250,432/1,251,264, saving 11,776/11,776.
  Draw+flush P50 becomes 1,245,664 ticks.
STAGE WINDOW 438..445, P50/P95:
  Stage 497,472/497,600 -> 488,992/489,344, saving 8,480/8,256.
  Draw 1,568,160/1,569,024 -> 1,554,784/1,555,648, saving 13,376/13,376.
  Active 1,572,896/1,573,760 -> 1,559,616/1,560,448, saving 13,280/13,312.
EXACTNESS GATES:
  Both windows retain owner 121/828, stage 8/255/57/42/54/202/49/4, cross
  5/10/15, M4 22/131072, and zero fallback, fence, or conservation error.
  Against Cut C, the deterministic native 256x192 top-screen comparison is
  raw 0/49,152, meaningful 0/49,152, and mean channel delta 0.00.
REFRESHED PROFILE / STOP:
  Instrumented ROM SHA-256 is
  85F455E29CCC1C131EF7AA6DF5D089971C4DBC635F094AA2A21075851E663E11.
  Preflight is 286,848/286,912, prepare 215,616/215,936, raw attribute-
  exclusive 74,368/75,264 (calibrated P50 upper bound 56,448), near transform
  33,536/36,096, begin/bind 39,808/40,128, and no-Z matrix
  50,432/50,880. The calibrated attribute bound moves only 1,088 ticks from
  Cut C; the larger uninstrumented win came from code layout and avoided stack/
  input work. Remaining measured work is required state validation, texture/
  color preparation, transforms, matrix transport, and ordered GX emission.
EVIDENCE:
  artifacts/performance/2026-07-16_task6-cut-d-fighter600.json
  artifacts/performance/2026-07-16_task6-cut-d-stage438.json
  artifacts/performance/2026-07-16_task6-cut-d-phase0-fighter600.json
  artifacts/performance/2026-07-16_task6-cut-d-visual.json
  artifacts/visibility/2026-07-16_task6-cut-d.png
KEEP / REWORK / REVERT: KEEP / STOP
  Task 6 banks 51,392 draw+flush ticks P50 from R0, ending at 1,245,664.
  That remains 455,664 ticks above the approximate 790K full-speed target.
  Further speculative cutting would cross the forbidden packet transport,
  interpreter, source-order, polygon-ID, or translucency boundaries, or chase
  nested-profiler noise. Close optimization and qualify the canonical ROM.
CANONICAL QUALIFICATION:
  Published battle ROM SHA-256 is
  7AB28684930899D5A4F5165E1CE85DDA7A93FC7F3CB06D44062283536507BFAD;
  ITCM is 26,104/32,768. Locked-30 smoke passes at 19.5 presents/s and 38.9
  updates/s. The no-build natural CPU-on match passes with 4,084 updates / 2,042
  presents, phase rates 39.9/38.1/39.6/n.a./58.2 updates/s, phase slips
  196/1036/925/0/3, Time Up -> Results, one normal M4 teardown, all ten post-GO
  fence counts zero, and 140,816 bytes reserve after the 65,536-byte BGM ring.
  The generic smash64ds.nds was not rebuilt and remains SHA-256
  009211A8ACC4BCC8DD473A14327AFFF023EC9F33EADBA786B67E0735D7077AB7.
```

## 2026-07-17 - Task 8 Cut F exact fighter RPY matrices

```text
IDEA ID: TASK8-CUT-F-FIGHTER-RPY-EXACT-20260717
BOUND / CENSUS:
  The detailed M2 window records 40 eligible unscaled mode-0 fighter local
  matrices per frame. Their owner bucket is 50,272/50,624 ticks P50/P95 before
  this cut. Cached and scaled matrix paths are not candidates.
SOURCE AUTHORITY:
  BattleShip macros.h:28 defines the radians-to-table-index multiply/truncate.
  BattleShip sys/matrix.c:966-1012 defines the gSYSinTable lookup, RPY integer
  products, shifts, packing, and fixed-W matrix layout. No decomp source changed.
ONE CHANGE:
  At the display adapter's unscaled fighter seam, replace the original source
  call with one ARM-state exact builder. It reproduces the source binary32 index,
  uses the imported source table and integer formulas, and packs translation
  with the existing exact 16.16 converter. Unsupported values fail closed to
  syMatrixTraRotRpyR before use. Gameplay, cached, scaled, generic fallback,
  matrix layout, and source files remain unchanged.
HOST / CODEGEN PROOF:
  scripts/check-fighter-matrix-angle-index.ps1 compares 234,881,492 binary32
  inputs against the literal source float multiply and truncation. It exhausts
  both signs and every mantissa for the nontrivial exponent classes 117..130;
  lower accepted classes are covered by signed zero/subnormal endpoints and the
  proven product-magnitude-below-one bound. It passes with zero mismatches. Both rebuilt
  profile-1 and profile-2 builders are ARM symbols with three UMULL instructions
  and zero __aeabi_* calls. DevFast now owns this regression.
IDENTITY / WINDOW:
  Mode 163, profile 1, detailed M2 ledger, Mode 9, static AOT 1, strict post-GO
  fence, bitmap OAM, live Fox, incremental wallpaper, frames 600..607.
  Cut E / Cut F ROM SHA-256 values are
  5E7E56D401665F0EA259F04C812EF057A5BFDABE5AD7AC698E7C223C702027B9 /
  5784EE4F7C3C213557E1A3AEEE43549794F465F7C831BB70CB0F2639A969A725.
P50/P95 RESULT:
  Combined fighter local-matrix ticks 50,272/50,624 -> 49,344/49,472, saving
  928/1,152. This is a local owner gain, not a locked-30 claim.
EXACTNESS / RUNTIME GATES:
  Profile 2 shadows each eligible matrix with BattleShip syMatrixTraRotRpyR and
  byte-compares the full Mtx before conversion. The canonical static-off live-
  Fox run passes frames 600..607 with RENDER_ORACLE=2484/0/0, stable 121/828
  owner census, 202/320/306 stage/Mario/Fox triangles, zero clipping, and a
  normal content/detail-gated frame 607. The rebuilt profile-2 ROM remains
  byte-identical to the exercised ROM at SHA-256
  796765A83CD796AB065B0FC634CE177C333FC649FF2C43A603ED1DD09BCF9CD0.
  The 2,484 sample field belongs to the vertex oracle; fighter matrix shadows
  share its zero mismatch counter but do not inflate that sample field. A
  static-on exploratory profile-2 run also reached 2484/0/0 but failed an
  existing texture-format accounting assertion; it is not used as the full
  forensic gate. The supported static-off forensic identity passed end to end.
  GBI fixtures, renderer ITCM placement, and both profile builds pass. Removing
  the now-unused predecessor translation helper leaves both exercised ROM
  payloads byte-identical; only ELF debug identities change.
EVIDENCE:
  artifacts/performance/2026-07-17_task8-cut-e-m2-fighter600.json
  artifacts/performance/2026-07-17_task8-cut-f-candidate-fighter600.json
  artifacts/performance/2026-07-17_task8-cut-f-forensic-static-off-fighter600.json
  artifacts/visibility/2026-07-17_task8-cut-f-candidate-frame607.png
  artifacts/visibility/2026-07-17_task8-cut-f-forensic-static-off-frame607.png
  .tura/task8-cut-f-candidate-runtime.log
  .tura/task8-cut-f-forensic-static-off-runtime.log
KEEP / REWORK / REVERT: KEEP
  Bank the exact 928-tick P50 owner-local gain under the no-discard rule.
```

## 2026-07-17 - Task 8 Cut G2 idempotent GX state

```text
IDEA ID: TASK8-CUT-G2-IDEMPOTENT-GX-STATE-20260717
BOUND / FIRST CANDIDATE:
  The refreshed begin/bind bucket was 39,808 ticks. Instrumentation found, per
  frame, texture-param 44 writes / 13 repeats, matrix-mode 164 / 0, and
  polygon-format 69 / 34. Shadowing all three in production therefore compared
  matrix mode 164 times without one skip. That first candidate regressed draw
  by 3,616/3,520 and active by 3,520/3,456 ticks P50/P95 and was rejected.
ONE KEPT CHANGE:
  Retain one renderer-owned shadow only for texture parameters and polygon
  format. Bind, upload, delete, and frame handoff invalidate the affected
  state; all renderer writes route through that owner. Matrix-mode accounting
  remains lab-only and production still emits the direct write.
IDENTITY / A-B RESULT, FRAMES 600..607:
  Two independent controls are byte-identical at ROM SHA-256
  9BD486DF9C7B833636289182A5D7E0E77FFC58648C9C260F79DE8B57EB74C1DC.
  The lean candidate is
  E80F1828FD9243F1FE9D6606167D1581C524F5AFEBFE74F371D6720AAA47C1D5.
  Draw 1,154,496/1,190,400 -> 1,154,144/1,190,016, saving 352/384;
  active 1,159,360/1,195,264 -> 1,158,912/1,194,816, saving 448/448.
  This is a small global draw gain, not a locked-30 claim.
FINAL-STATE / EXACTNESS PROOF:
  The final instrumented ROM is
  19EDF8AC689EFF001E3B2053D02168BF003A3858D56835ACEE7BED8FF70D8651.
  It reproduces texture 44/13, matrix 164/0, and polygon 69/34 on all eight
  frames. Owner 121/828, stage 8/255/57/42/54/202/49/4, cross 5/10/15,
  M4 22/131072, and every fallback, fence, and conservation gate pass. The
  production top-screen A/B is exactly 0/49,152 changed pixels, mean delta 0.
EVIDENCE:
  artifacts/performance/2026-07-17_task8-g2-control-fighter600.json
  artifacts/performance/2026-07-17_task8-g2-control2-fighter600.json
  artifacts/performance/2026-07-17_task8-g2-candidate-fighter600.json
  artifacts/performance/2026-07-17_task8-g2-candidate2-fighter600.json
  artifacts/performance/2026-07-17_task8-g2-final-lab-fighter600.json
  artifacts/visibility/2026-07-17_task8-g2-control2-frame607.png
  artifacts/visibility/2026-07-17_task8-g2-candidate2-frame607.png
KEEP / REWORK / REVERT: KEEP LEAN / REVERT FULL SHADOW
  Bank the 47 productive repeat-write skips; do not ship the zero-hit matrix
  comparison.
```

## 2026-07-17 - Task 8 Cut H and update-pair audit

```text
CUT H PROFILE / DECISION:
  Final Phase-0.5 records the lower HUD at only 1,184/1,216 ticks P50/P95.
  ndsPlatformRenderBattleTextHud already fingerprints the displayed state and
  returns unchanged; FPS text is likewise change-driven, and consoleClear is
  limited to initialization/teardown. The proposed full-line dirty rewrite is
  not a measured cut. No code change.
UPDATE-PAIR AUDIT:
  Mode 163 runs exactly two ndsRunMarioFoxProofUpdate calls before one
  ndsBattlePlayablePresentRealtimeFrame. The update reaches gcRunAll, whose
  source implementation executes only func_run and process callbacks; it never
  calls proc_display. Display callbacks and renderer submission are reached
  once through gcDrawAll inside ndsBattlePlayablePresentFrame, after the pair.
  Post-update recorders read gameplay/proof state only. No draw-side derivation
  or per-present work was found inside the update pair, so there is no bounded
  cut row to add.
DECISION: NO CUT / AUDIT PASS
```

## 2026-07-17 - Task 8 end-gate reserve recovery

The integrated one-minute gate exposed a post-BGM reserve of only
`178,960 - 65,536 = 113,424` bytes, 17,648 below the 128 KiB floor. The
following table owns the bounded lifetime change used to recover it.

| Item | Producer | Consumer | Bytes | Storage/bank | Lifetime | Invalidation key | Changed bytes | Transfer/clear bytes | Active ticks P50/P95 | Wait P50/P95 | Fallback/stale | Reserve impact |
|---|---|---|---:|---|---|---|---:|---:|---:|---:|---|---:|
| Fighter DL state scratch | `ndsFighterMarioFoxDLAllDrawForSlot` | list executor; forensic rasterizer | 54,272 -> 27,136 | ARM9 main BSS | one serial fighter display callback | overwritten, or cleared before detailed output | unchanged | profile-0 hot clear remains 0 | no tick claim | 0/0 | exact fallback and profile-2 oracle retained | +27,136 BSS |
| IFCommon overlays | source prepare | OBJ/GX | 31,168 OBJ + 57,344 texture + 608 palette | OBJ VRAM / texture VRAM | battle scene | scene generation; no post-GO mutation | unchanged | zero post-GO | 0/0 hot | 0/0 | fence 0; teardown 1 | unchanged |
| BGM ring | ARM9 stream refill | audio channel | 65,536 | ARM9 main BSS | battle/results transition | audio teardown | unchanged | unchanged | unchanged | unchanged | normal teardown | unchanged |

```text
MEASURED WALL:
  The first current-ROM lifecycle completed gameplay and Results but failed the
  reserve assertion at 113,424 bytes after BGM.
REPRESENTATION / LIFETIME CHANGE:
  The two 27,136-byte fighter display-list state arrays never overlap: each is
  produced, consumed, optionally rasterized, and discarded inside one serial
  display callback. Reuse one array for both slots; retain per-slot stats and
  clean ledgers.
BEFORE / AFTER STATIC STORAGE:
  sNdsFighterDLAllDrawStates 54,272 -> 27,136 bytes; .main.bss
  1,785,200 -> 1,758,064; __heap_start_ntr 0x022921F0 -> 0x0228B7F0.
VRAM / CACHE / DMA / AUDIO CONTRACT:
  No VRAM bank, texture, palette, DMA, cache-maintenance, ARM7, BGM, FGM, or
  reloc ownership changed. Canonical ITCM remains 26,176/32,768.
OUTPUT / RUNTIME PROOF:
  The synchronized frame-607 comparison is exactly 0/49,152 changed pixels,
  mean channel delta 0. Owner 121/828, stage 8/255/57/42/54/202/49/4,
  M4 22/131072, and zero fallback/fence/conservation remain intact.
RESERVE HIGH-WATER / LIFECYCLE:
  The rerun passes at 203,536 bytes before and 138,000 after the 65,536-byte
  BGM ring, 6,928 above the floor. It reaches Time Up -> Results with 4,084
  updates / 2,042 presents, phase rates 39.9/38.0/39.6/n.a./58.2 updates/s,
  slips 196/1039/925/0/3, one normal M4 teardown, and zero stale, safety,
  eviction, or post-GO fence counts.
IDENTITY / EVIDENCE:
  Canonical ROM SHA-256 is
  162212F4093BFFF9B8C9AA47678019DD627766A5BD66CFBB1BB44FE9B4284DC5.
  artifacts/performance/2026-07-17_task8-shared-dl-scratch-fighter600.json
  artifacts/visibility/2026-07-17_task8-shared-dl-scratch-frame607.png
  artifacts/visibility/2026-07-17_task8-g2-candidate2-frame607.png
KEEP / REWORK / REVERT: KEEP
  This restores the required reserve by matching storage to its actual serial
  lifetime; it is not a renderer tick claim.
```

## 2026-07-17 - Task 9 bit-exact soft-float ITCM promotion

```text
IDEA ID: TASK9-LIBGCC-FLOAT-ITCM-20260717
R0 / MEASUREMENT IDENTITY:
  Mode 163 battle_playable_realtime, renderer profile 1, Mode 9, static AOT 1,
  bitmap OAM 0, live Fox CPU, frames 600..607 / logic updates 808..822. Common
  code is GCC 15.2.0 -O2 -march=armv5te -mtune=arm946e-s -mthumb; only the
  established renderer translation unit adds -marm. R0 and its retake are
  byte-identical: ROM/ELF 803747713DA1FBA66EF68CD66F704FCDDE244E25A8791DF3EEC0DE89568D78C6 /
  00912357A3C670D206634FC6C5E5A2185A89366D780A7157ABA60B3874A14888.
  Artifact SHA-256 values are ABC86221A5E85767D1E127993464BA448F30E9726831178705F4FCB8F609F0A9
  and B3BD6860FED0302AA793F29ABAA110B3A1428C1972175C2408A89823532EF43C.

PHASE 0 CENSUS:
  Lab-only --wrap shims count the exact scVSBattleFuncUpdate scope; audio is
  excluded. The selected Thumb multilib archive is GCC's stock
  C755ADC33ECA252260360327904591B8462CCE5C25E48B0E881AC0B295953F48.
  Objdump proves the linked nonzero helpers are ARM-state routines in waitstated
  .main despite the caller's Thumb mode. The artifact below owns all 35 rows;
  the compact table lists every routine observed in the natural window.

  Routine     mean calls/update   median ticks/call   observed max tick bound
  fadd              821.063              24                   231116
  fsub              576.375              39                   148656
  fmul             1172.375              21                   395504
  fdiv               51.250              80                    29120
  fcmpeq            556.250              40                   127148
  fcmplt             80.000              48                    22420
  fcmple            111.813              40                    30172
  fcmpge             46.250              39                     6156
  fcmpgt             94.125              47                    12844
  fcmpun              8.000              22                     2964
  f2iz                2.000              17                     5092
  f2uiz               4.000              15                      532
  i2f               174.438              34                    50160
  f2d                 4.000              16                      456
  d2f                 4.000              19                      380
  dmul                4.000              48                      380
  ddiv                4.000             355                     1980

  Observed zero: frsub, ui2f, l2f, ul2f, dadd, dsub, drsub, dcmpeq,
  dcmplt, dcmple, dcmpge, dcmpgt, dcmpun, d2iz, i2d, ui2d, l2d, ul2d.
  Mean count x measured median cost totals 115,648 ticks/update; the top ten
  rows account for 113,626. Census artifact SHA-256 is
  E302383C8635E8AECF8D28278A18C0E34147A35BA807A1B9E390B1D97381FA27.

PHASE 1 / EXACT CODE IDENTITY:
  One change only: extract six stock objects, rename .text to .itcm, and link
  them ahead of libgcc. No instruction, helper ABI, float rule, renderer ITCM,
  packet, gameplay, or source file changes. The verifier byte-compares each
  relocated code section with a fresh private extraction.

  Object                 bytes   machine-code SHA-256
  _arm_addsubsf3.o          684   E1F79C55786F2323E18924D71A1268945789DD33C3C8C8861042F41A5CCB4A91
  _arm_muldivsf3.o          760   C313BBE04B3484CDBE9E6B14BCB14B0828F308B076616D080AA89204FDFD940A
  _arm_cmpsf2.o             276   2B656E12FDE0F34CEB17395A5FF8FCC1EF0CEBBF94F8EDA3725BD26C0B3C2884
  _arm_unordsf2.o            56   7B4D5CFBE032C0D80470495D888A2598C8D77BD434831B0F28488C0070A60407
  _arm_fixsfsi.o             92   73908D204E4625C30FC21DFF9A2F817E26C53D161164A032BD229A9DCDD6EDEE
  _arm_fixunssfsi.o          84   1BFBAB489E0E7B2166376F736F04F891A50F7BF04191CFB8731E5294B752D66F
  Total                    1,952

  Current canonical symbols are .itcm ARM functions: fsub/fadd
  01FF8028/01FF802C (0x1C0/0x1BC), ui2f/i2f 01FF81E8/01FF81F0
  (0x28/0x20), fmul 01FF82CC (0x198), fdiv 01FF8464 (0x160), compare
  aliases 01FF8660..01FF86D8 (0x18 each; unordered 0x38), and
  f2iz/f2uiz 01FF8710/01FF876C (0x5C/0x54).

TIMING / KEEP:
  R0 coarse update P50/P95 313,888/581,376 -> 263,040/527,808. The stable
  source-update owner moves 311,744/312,960 -> 260,192/261,312, saving
  51,552/51,648 ticks, or 16.54%/16.50%. Phase-1 ROM/ELF are
  32C957ADBB0D61F031DC9FF743BE4983175CCBBAE84924788C48FA67C8ECB154 /
  638778926805CBFD38140C8CB4C897C17C3BADAAD4B68E472A26AB823F53C511.
  Timing artifact SHA-256 is
  D6284625227D423C31DB89B84122AB2C273A376855706D179D08E65442D791A1.

SUPREME STATE GATE:
  The lab hash records match-local RNG/heap-relative state, battle, scene,
  camera, ground, controllers, collision, active GObjs/processes and their
  DObj/SObj/CObj/XObj/AObj/MObj trees, plus fighter/item/weapon/effect state.
  Pointer normalization is relative/canonical and bounded; overflow is fatal.
  R0 and Phase 1 each produced 3,892 post-update rows through Results, overflow
  0, with exact row equality. Baseline ROM/ELF are
  8E0629C2924B6FFAEE9F0A6423128CAC0E3CE53C957DA7B50193797EACF178CE /
  27E04474190AE33ACEAB669B51C7BA92B3259AF40BF860348750F155EC706FB3;
  candidate ROM/ELF are
  4744FA5F2D19086782390B3EA6DC9204C35A32BC556F76024C9298AB9FB98D60 /
  AA10D8ACC4D00FA84574E29C32E006C0A025D35A76A15869804D9A43E9DAE39B.
  Row-artifact SHA-256 values are
  3DC139481FF70EABAE75981EF7CA05D022714BD999CB4C79B09C295C8A431F1A /
  D8E6C4966B22F811BEA4A8ED9265DD47C56AA2C60E02F8BD40EB795E926EB363.

PHASE 2:
  SKIP. Unchanged libgcc already gives a decisive 16.54% owner gain and exact
  3,892-update identity. A custom IEEE-754 implementation and its exhaustive
  proof cost are not justified.

STACK REPORT ONLY:
  The linker boot/user stack is DTCM at __sp_usr=02FF3E80, but gameplay runs on
  the port coroutine's 16 KiB malloc-backed main-RAM stack. Runtime owner base /
  top are 0228F200/02293200; update-600 SP is 02292FE8. The fmul ITCM path was
  observed at PC 01FF82CC with SP 02292EF8; ddiv remains main-RAM at 020867D8
  with SP 02292F08. The deepest sentinel touch was 0229125C: 8,100 bytes used,
  8,284 headroom. Therefore the requested no-main-RAM-spill premise is false;
  no stack change was made.

FINAL GATES / MEMORY:
  DevFast and Current pass. Canonical ITCM is 28,128/32,768, leaving 4,640;
  renderer ownership remains 23,640 bytes. Normal ITCM is 8,612, leaving
  24,156. The isolated one-minute proof uses 27,980, leaving 4,788. It completes
  4,084 updates / 2,042 presents, timer 3,600 -> 0, CPU decisions 3,601,
  phase rates 39.9/37.4/39.3/n.a./58.2 updates/s, slips 196/1088/946/0/3,
  Results, one teardown, reserve 232,208 - 65,536 = 166,672, and zero stale,
  safety, eviction, or post-GO fence counts. Its ROM is 9C35F4B3...; the public
  A89F143B... ROM remains byte-identical across the isolated proof.

TOOLING / CHECKER HARDENING:
  Never redirect `ar p` binary output through PowerShell text streams and never
  extract from the installed archive. More importantly, never declare an
  installed .a as a Make prerequisite: Current's `make -B` invokes an implicit
  archive rebuild and reduced libgcc.a to the 8-byte empty archive
  F0A17A43C74D2FE5474FA2FD29C8F14799E777D7D75A2CC4D11C20A6E7B161C5.
  The build now keeps libgcc out of Make's target graph, hard-checks its package
  hash, makes one private copy, and performs one grouped extraction. State-range
  normalization likewise checks value <= last before unsigned subtraction so
  high-bit gameplay values cannot masquerade as pointers.

EVIDENCE:
  artifacts/performance/2026-07-17_task9-phase0-float-census.json
  artifacts/performance/2026-07-17_task9-{r0,r0-retake}-fighter600.json
  artifacts/performance/2026-07-17_task9-phase1-float-itcm.json
  artifacts/performance/2026-07-17_task9-state-{r0,phase1-itcm}.json
KEEP / REWORK / REVERT: KEEP PHASE 1 / SKIP PHASE 2
```

## 2026-07-17 - M2 production emitter run-class split

```text
IDEA ID: M2-PRODUCTION-EMITTER-RUN-SPLIT-20260717
MEASUREMENT IDENTITY:
  Mode 163 battle_playable_realtime, profile 1, detailed M2 ledger, Mode 8,
  static AOT 1, hybrid OAM 1, Fox decisions paused, frames 600..607. A and A2
  are byte-identical: ROM/ELF
  5F7C2A53FA6B931B21CDE8F0669DDD51D3CD859F44A2771FC0194620345F6D48 /
  08589A0AADEA17A18090FA5DDDE7F6C285705D88558F011E412DE852DFFB2151.
  Their renderer object is
  DBBCC6D7524BC6C97128F43063F62091DC4FFE16FE4F70B17B19A27B9E35F42C.

HOT SYMBOL / FREQUENCY / BEFORE CODEGEN:
  ndsRendererNativeEmitProductionRun is the measured 48,000/48,192-tick
  emit/account child of the 212,416/212,480 production bucket. The frame has
  67 calls: 54 raw and 13 cross, spanning 626 triangles / 1,878 corners, of
  which 381 corners are textured. Before: object 0xFC/0x224, ELF
  0x01FFC728/0x224, and push/pop {r4-r9,lr}, seven saved registers on the
  malloc-backed main-RAM coroutine stack. The production owner is
  0x01FFDD70/0xE28 with a 116-byte frame.

HYPOTHESIS:
  The already-proved run class is invariant for each call. Separate raw and
  cross callees can remove cross-only arguments/register pressure from the 54
  common raw calls without changing an emitted GX word.

REJECTED FULL INLINE:
  always_inline removes the 0x224 callee and locally moves production to
  211,584/211,648, but grows the owner 0xE28 -> 0x1028, its frame 116 -> 124,
  and regresses draw 1,061,888/1,061,952 -> 1,061,984/1,062,080 and active
  1,066,208/1,066,816 -> 1,066,304/1,066,880. REVERT.

RETAINED CHANGE / AFTER CODEGEN:
  Call sites dispatch directly to one raw or cross function. Raw is
  0x01FFC62C/0xD0 and saves {r4-r7,lr}; cross is
  0x01FFC7F8/0x164 and saves {r4-r9,sl,lr}. The 54 raw calls save two words on
  both entry and exit while 13 cross calls add one, eliminating 190 stack word
  transfers per frame. Candidate object/ELF are
  3753EE91FA98D05EC0FD5208D800601CA66FB47D6E2260DEBF8953EB4336F53C /
  5D12AA2381958ED0271DF11883C445BFDEEC4CFD02A2FB039184495518D8921A.

SYNCHRONIZED A/A2/B P50/P95:
  Combined fighter  433,472/433,536 -> 432,384/432,448  (-1,088/-1,088)
  Production        212,416/212,480 -> 211,328/211,392  (-1,088/-1,088)
  Emit/account       48,000/48,192  ->  47,456/47,616    (-544/-576)
  Draw            1,061,888/1,061,952 -> 1,060,928/1,060,992 (-960/-960)
  Active          1,066,208/1,066,816 -> 1,065,280/1,065,856 (-928/-960)

TAIL-DISPATCH FALSIFIER:
  A shared noinline dispatcher reclaims 648 detailed-build ITCM bytes, but
  gives back 864 of the 960 draw ticks: draw becomes
  1,061,792/1,061,824 and production 212,288/212,352. REVERT; direct dispatch
  wins under the measured P1 performance priority.

EXACTNESS / MEMORY / CHECKPOINT:
  A/A2/B preserve 70/686 and 60/320/306/29/0/0, all fence/conservation fields,
  and change 0/120,000 pixels in the 400x300 gameplay viewport. Owner packet,
  hierarchy, GBI fixtures, ITCM placement, two-ROM publication, and Boundary
  pass. Canonical ITCM is 28,608/32,768 with 24,104 renderer bytes, up
  480/464 from the Task-9 checkpoint; public ROM is 14,613,504 bytes,
  SHA-256 B54D16DB43844C63D88F8CD3E635A5A53DB0818CDC5F4517724BA577EF621753.
  A fresh ledger-off window is 372,096/372,160 combined fighter ticks.

EVIDENCE:
  artifacts/performance/2026-07-17_m2-emit-{a,a2,inline-b,split-b,
    tail-dispatch-c,split-ledger-off}.json
  artifacts/visibility/2026-07-17_m2-emit-{a,a2,split-b}-frame607.png
  builds/m2-emit-split-candidate/{nds_renderer.o,
    smash64ds-battle-playable-coarse-hwtri.elf}
KEEP / REWORK / REVERT: KEEP DIRECT SPLIT / REVERT INLINE AND TAIL DISPATCH
```

## 2026-07-17 - Mario generic/fast forensic parity is red

```text
IDEA ID: PROFILE2-MARIO-GENERIC-FAST-PARITY-RED-20260717
FINDING:
  The first 30-second attempt timed out in the slow generic forensic arm. The
  comparison wrapper now forwards RendererBenchmarkTimeoutSeconds; the
  documented 120-second run captured generic and Mode 8 at identical frames
  180..187 from ROM 7ABCAD1B6D3321062F50E6F2FDFBE42F21D16471A78976D386413BB256A6A73D.

RESULT:
  Stage, Fox, owner counts, geometry, and upload provenance match. Mario does
  not: all eight rows differ in exit vertex-cache and semantic hashes, and its
  light signature is generic 3347397109 versus fast 2758001273. This is a real
  red gate, not a timeout waiver.

OWNERSHIP / FIRST-BAD BOUND:
  artifacts/performance/2026-07-17_task8-cut-f-forensic-static-off-fighter600.json
  already has the generic 3347397109 signature. The older
  artifacts/performance/2026-07-16_mario-light-prefix-profile2.json already has
  the fast 2758001273 signature. The forensic ELF contains no production
  emitter or native production/hierarchy owner symbol, so the new emitter split
  cannot cause this divergence. Repair the shared Mario matrix/light seam
  before another M2 experiment.

EVIDENCE:
  artifacts/performance/2026-07-17_m2-forensic-generic-profile2.json
    SHA-256 2DD5FC378ABE1ACFDDF2EAE3628F51C38856BFDFB49898110EFED520D4DFA8CF
  artifacts/performance/2026-07-17_m2-forensic-fast-mode8-profile2.json
    SHA-256 7E2D559DA1139FFF6DE246DAD66AACE2AD32671D4AF9848C3B952B0139D096EF
KEEP / REWORK / REVERT: RED / REWORK SHARED MARIO MATRIX-LIGHT PARITY
```

## 2026-07-17 - Restored exact fighter light parity

```text
IDEA ID: PROFILE2-FIGHTER-LIGHT-PARITY-RESTORE-20260717
ROOT CAUSE:
  The shared F3DEX2 G_MOVEWORD decoder used the wrong field layout, and the
  live material emitter encoded that same wrong layout. The owner generator's
  exact O2R scan found 148 static fighter G_MW_LIGHTCOL commands: 120 are the
  already-compact root prefixes, while 28 are source-significant intra-root
  changes that the generated state sequence had omitted.

RETAINED CHANGE / RECURRENCE GATE:
  Decode and emit index bits 16..23 and offset bits 0..15 while preserving the
  separate SPECIAL_1 layout. Keep the 120 root prefixes compact and add only
  the 28 intra-root changes to epoch state spans. The generated tables contain
  70 state deltas and 196 state-sequence entries; the owner remains 32 roots,
  49 epochs, 67 runs, 626 triangles, and the hierarchy remains 49 bindings /
  1,878 corners. Generator checks now census exact command identity, ordering,
  and material boundaries. Native-state validation accepts effect 14 only for
  an exact G_MW_LIGHTCOL opcode/index/offset tuple.

PROFILE-2 EXACTNESS:
  A fresh synchronized generic/Mode-8 comparison on forensic ROM
  940F1A8D14776F6127AE644AAC6AA0E01E51D30274D6B08CDEDD1EC107B3EC26
  covers frames 180..187. All eight frames have zero semantic, owner, and
  geometry mismatches and both arms retain 686 triangles. These are forensic
  builds, so their timing is not a production performance claim.

BOUNDARY CHECKPOINT:
  Public ROM A7E607BB03550E7D7379EE3114040E86A823B300F708839C0A946D86302DDBEB
  is 14,613,504 bytes and passes Boundary with 121 runs / 828 triangles, zero
  fallback, 22 resident textures / 131,072 bytes, 646 cache hits, and zero
  post-GO fence work. Canonical ITCM is 28,040/32,768; renderer ownership is
  23,536 bytes. Locked-30 full-speed remains unmet and needs a separate
  production measurement.

EVIDENCE:
  artifacts/performance/2026-07-17_m2-forensic-light-parity-generic-profile2.json
    SHA-256 2E9F2CA40038271EB08898849BE56DDA4F2244F6087B8C39B1675B2E42F82757
  artifacts/performance/2026-07-17_m2-forensic-light-parity-fast-mode8-profile2.json
    SHA-256 60F10268CA3A4BBF241246EE184112E506ED19AC77A85983E96A68DFE6651F7F
KEEP / REWORK / REVERT: KEEP EXACT LIGHT STATE / RESUME MEASURED M2 WORK
```

## 2026-07-17 - Reused the fully overwritten fighter capture arena

```text
IDEA ID: M2-DISPLAY-CONTRACT-SCALAR-RESET-20260717
MEASUREMENT IDENTITY:
  Mode 163 battle_playable_realtime, profile 1, detailed M2 ledger, Mode 8,
  static AOT 1, hybrid OAM 1, Fox decisions paused, frames 600..607. Control
  ROM/ELF are
  3C5B83FC1C1B2DB80E8D954733F3B9A19A939AD087AE7F84BDAE15CBA14CB997 /
  51B164559149288AAF60918161F327F7313E4CA3BAF3F57B3625705C18719A1B;
  candidate ROM/ELF are
  6783F4B2F7C47A54499A5A2CB924B2483F907DAF6AA9039A54BF4EC43FCEDAEB /
  8100515035F671BCDD5E2C887573736F1C8566BEBA7A56EE5D34388A189BE581.

HOT SYMBOL / CALLERS:
  ndsFighterDisplayContractCapture is inlined into
  ndsFighterDisplayContractSubmit. The unchanged source callback remains
  ndsBaseFTDisplayMainProcDisplay, translated from BattleShip
  ft/ftdisplaymain.c.
DYNAMIC FREQUENCY:
  Two captures per presented frame: one Mario and one Fox.
BEFORE ADDRESS / SIZE / SECTION / STACK:
  Submit is 0x02044C04/0x298 in Thumb .main with a 128-byte maximum frame.
  scene_backend.o is 4,134,116 bytes, SHA-256
  D229F48DCE4C86CEC0ABE75A5D60E35CF1C29C710C8854C87935CF290E488269.
BEFORE EXPENSIVE SEQUENCE:
  At 0x02044C82 the inlined capture loads 6,240 and calls main-RAM memset at
  0x0209CEA4, clearing the complete contract before every fighter traversal.

CAUSAL HYPOTHESIS:
  Selection overwrites every consumed event field, playback visits only
  event_count entries, and the source display-list macros rewrite scratch from
  reset heads. Clearing stale event/scratch bytes therefore cannot affect live
  output; only scalar guards and current light/material state need reset.
ONE CHANGE:
  Replace the full-struct clear with explicit live-state resets. Preserve the
  16-byte Light zero initialization and all source callback/event selection.
  The GBI fixture checker forbids restoration of the broad clear and requires
  the complete scalar reset sequence.
AFTER ADDRESS / SIZE / SECTION / STACK:
  Submit remains at 0x02044C04 in Thumb .main, grows to 0x2D8, and retains the
  128-byte frame. scene_backend.o is 4,134,324 bytes, SHA-256
  7F4739D0A6FEA20C6B6FF48053952C79C44D2F743F59119AA941280E165314FF.
AFTER SEQUENCE:
  The 6,240-byte memset is gone. One 16-byte memset remains for exact Light
  zeroing; event and scratch storage are reused without being read stale.
MAP / TCM / ROM DELTA:
  .main grows 64 bytes; object/ELF grow 208/188 bytes. Detailed-build ITCM is
  unchanged at 29,384/32,768. Both ROMs remain 14,621,696 bytes. Candidate map
  SHA-256 is
  3BA36BB628426093E47A04EF11FA9DD198E31CC08E884BCEF8E5C873E1790619.

SYNCHRONIZED A/B P50/P95:
  Capture              47,296/47,360     -> 41,152/41,152     (-6,144/-6,208)
  Combined fighter    452,640/455,808    -> 446,464/449,600   (-6,176/-6,208)
  Production          232,768/235,968    -> 232,736/235,840      (-32/-128)
  Draw              1,077,568/1,080,832  -> 1,071,488/1,074,816 (-6,080/-6,016)
  Active            1,081,824/1,094,144  -> 1,075,872/1,079,040 (-5,952/-15,104)

EXACTNESS GATES:
  All eight frames preserve 70/686 and 60/320/306/29/0/0, renderer geometry,
  M2 state/conservation, static texture and fence fields, and 0/49,152 changed
  top-screen pixels. Current profile-2 generic/Mode-8 ROM A4D84254... is exact
  on all eight frames 180..187 for semantic hashes, provenance, owner state,
  geometry, and upload sequence; both arms retain 686 triangles. Boundary
  passes on public ROM
  36218F25F3C69929CEBF4F62B8E2BEBFFCCC13739DBBE5B5D28376352677A686
  with canonical ITCM 28,040/32,768 and a dated capture. Current source-light-
  exact ledger-off combined fighter timing is 386,016/389,184; profile-0 smoke
  is 22.3 FPS, so stable locked 30 remains unmet.

TOOLING / RECURRENCE:
  compare-renderer-fast-raw.ps1 now forwards its capture timeout, parses both
  integer-array and space-delimited JSON rows, and fails closed on missing
  projected fields. A transient runner disconnect was rerun on another repo
  slot; no disconnected sample was admitted.

EVIDENCE:
  artifacts/performance/2026-07-17_m2-contract-reset-{a,b}.json
  artifacts/performance/2026-07-17_m2-contract-reset-ledger-off.json
  artifacts/performance/2026-07-17_m2-contract-reset-{generic,fast}-profile2.json
  artifacts/visibility/2026-07-17_m2-contract-reset-{a,b}-frame607.png
  artifacts/visibility/2026-07-17_m2-contract-reset-ledger-off-frame607.png
  builds/m2-contract-reset-{control,candidate}/
KEEP / REWORK / REVERT: KEEP SCALAR RESET / CONTINUE M2 FULL-SPEED WORK
```

## 2026-07-17 - Plain dense IDs remove raw-corner masks

```text
IDEA ID: M2-RAW-CORNER-PLAIN-DENSE-ID-20260717
MEASUREMENT IDENTITY:
  Mode 163 battle_playable_realtime, profile 1, ledger off, Mode 8, static AOT
  1, hybrid OAM 1, Fox decisions paused, exact frames 600..607. Control ROM/ELF
  are E24E7D12FC0EA46811BE277C82A496CAA7C82D72E29958C2A35DE2866549BFBF /
  7BCE5EB2143674CECD442FE7C08FCF016F903B4D4C38698239C6AD0455A31B87;
  candidate ROM/ELF are
  2ADB8879272642547FDFFD4E0DB76698CDB29409FBE69BD6AF719D39355BB57D /
  22F311112915CB9F60CA6DA3533B1E8554523C229158F98DAB82FCFC82F670A5.

HOT SYMBOL / EXACT CHANGE:
  ndsRendererNativeEmitProductionRawRun is called for 54 raw runs per frame.
  The immutable owner contains 1,878 u16 corners: 1,746 raw corners whose
  binding is already fixed by the run root, and 132 cross-run corners that need
  their packed GX slot. Emit raw corners as plain 10-bit dense IDs, retain the
  packed cross ABI, and load raw IDs without & 0x3FF.

ARM CODEGEN:
  The ARMv5TE ITCM raw emitter shrinks 0xD0 -> 0xBC. Both dynamic AND sites and
  the 0x000003FF literal disappear; the selected raw loop executes 1,746 fewer
  ALU instructions per frame and saves one register at each raw-run entry/exit.
  Cross remains 0x164. Canonical ITCM moves 28,040 -> 28,020 / 32,768 while
  renderer ownership remains 23,536 bytes.

SYNCHRONIZED A/B P50/P95:
  Mario             173,600/175,168   -> 173,152/174,720   (-448/-448)
  Fox               213,632/214,016   -> 213,152/213,504   (-480/-512)
  Combined fighter  386,016/389,184   -> 385,088/388,224   (-928/-960)
  Draw            1,011,648/1,014,976 -> 1,010,688/1,014,016 (-960/-960)
  Active          1,015,776/1,018,944 -> 1,014,848/1,017,984 (-928/-960)
  Present         1,680,448/1,680,512 -> 1,680,448/1,680,512 (unchanged)

EXACTNESS / RECURRENCE GATES:
  All eight frames retain 70/686 and 60/320/306/29/0/0, zero conservation
  error, unchanged texture/fence state, and 0/49,152 changed top-screen pixels.
  The hierarchy checker rejects any raw corner with high slot bits and still
  proves the exact cross binding map. Packet, GBI, parity-corpus, and fresh
  generic/Mode-8 profile-2 frames 180..187 pass with zero semantic, owner, or
  geometry mismatch and 686 fast triangles.

REJECTED PRECEDING EXPERIMENT:
  Restoring BattleShip camera kind 0x4C at gcPrepCameraMatrix and deleting the
  two capture-local gmCameraLookAtFuncMatrix calls built and passed its host
  source contract, but two identical 120-second runtime attempts timed out
  after scVSBattleStartBattle while still in NitroFS reads. No timing sample
  was admitted and the change was fully reverted. Do not retry that hoist
  without first proving the startup/runtime ownership fault.

EVIDENCE:
  artifacts/performance/2026-07-17_m2-camera-prep-a.json
  artifacts/performance/2026-07-17_m2-raw-corner-b.json
  artifacts/performance/2026-07-17_m2-raw-corner-{generic,fast}-profile2.json
  artifacts/visibility/2026-07-17_m2-{camera-prep-a,raw-corner-b}-frame607.png
  Boundary publishes FE0C8893C37F43934DB4BEEB8169F52BB0AADEB97C5C4BA07B69416A37B743A9
  at 14,613,504 bytes with ITCM 28,020/32,768 and capture
  artifacts/visibility/2026-07-17_canonical_fast_125115-2819034-p54940.png
KEEP / REWORK / REVERT: KEEP RAW DENSE IDs / REVERT SHARED CAMERA HOIST
```

## 2026-07-17 - Split raw emitter by immutable texture class

```text
IDEA ID: M2-PRODUCTION-RAW-TEXTURE-SPLIT-20260717
MEASUREMENT IDENTITY:
  Mode 163 battle_playable_realtime, profile 1, ledger off, Mode 8, static AOT
  1, hybrid OAM 1, Fox decisions paused, exact frames 600..607. Control ROM/ELF
  are 6DAA92BF2F731B5423A81DFF7497B355301CA62444D31594079497C508CD3F55 /
  A0C684BB70D6D7B566FC4564C73EE1FF8A5F67D6898FA31C7E46F99217A00262;
  candidate ROM/ELF are
  F7CE466C1E993AF8E62EBE45A39CEC61FB2B6658C1E3628E4C377EFF12C7F7FF /
  8A142D37FF18926E242BA72BB082144BC91E4F5BBB4108278797855C9130B30F.

BOUND / ONE CHANGE:
  The existing generated parity corpus proves 15 textured runs: 11 of the 54
  raw runs and 4 of the 13 cross runs. Split the already-specialized production
  raw loops into textured and untextured callees and dispatch from the two
  existing call sites. No table, generator, state, GX word, or harness changes.

ARMV5TE CODEGEN:
  The control 0xBC raw symbol pushes/pops {r4,r5,r6,lr} on both paths. Candidate
  textured is 0x74 and retains {r4,r5,r6,lr}; untextured is 0x64 and uses only
  {r4,lr}. The 43 common untextured calls remove four stack word transfers each,
  172 per frame. The production owner grows 0xB98 -> 0xB9C; cross stays 0x164.
  ITCM grows 28,020 -> 28,052 / 32,768 and renderer ownership 23,536 -> 23,540.
  Control/candidate object SHA-256 values are
  6A224B07EF3163706A532DB887BEA440D9B1DE027F192FADB8F2238DED1B425D /
  467794AF644CFCABC4E4496396BEE979F4F6A71E7FE030ED250977F3611A9CEB.

SYNCHRONIZED A/B P50/P95:
  Mario             173,792/175,360   -> 173,248/174,848   (-544/-512)
  Fox               214,048/214,464   -> 213,280/213,632   (-768/-832)
  Combined fighter  386,624/389,824   -> 385,312/388,480 (-1,312/-1,344)
  Draw            1,011,648/1,014,976 -> 1,009,824/1,013,120 (-1,824/-1,856)
  Active          1,015,680/1,018,880 -> 1,014,048/1,017,088 (-1,632/-1,792)
  Loop            1,680,448/1,680,512 -> 1,680,448/1,680,512 (unchanged)
  Fixed-two VBlank wait absorbs the saved active work: present moves
  1,434,624/1,438,784 -> 1,436,192/1,440,256 and smoke remains 19.7 FPS. This is
  a CPU-cost KEEP, not a full-speed promotion.

EXACTNESS / DECISION:
  Every paired frame saves 1,280..1,344 combined fighter ticks. All eight retain
  70/686 and 60/320/306/29/0/0, zero conservation and texture-fence faults, and
  0/49,152 changed top-screen pixels. A2 is unnecessary because timing and all
  guards agree. Owner hierarchy/packet, parity corpus, and GBI fixtures pass.
  Profile-2 ROM 75A731E10BCB1357FFD39129AA41D08C58352CDB17F00DCC10DB5BEC22166A4B
  is exact on generic/Mode-8 frames 180..187 with 686 triangles in both arms and
  zero semantic, owner, or geometry mismatch.

TOOLING NOTE:
  The first control wrapper allowed only 240 seconds and ended during the full
  rebuild before exporting a sample; it was excluded. Full-rebuild wall time
  and the runtime capture timeout are separate allowances in future cycles.

INTEGRATION / DISCONNECTED CAPTURE:
  The full Latest build passed normal runtime, mode-163 smoke, CPU
  setup/proc/target 1/33/33, ITCM, and two-ROM publication, then Windows session
  1 entered `WinDisc` and `CopyFromScreen` failed with an invalid handle on two
  runner slots. `capture-melonds.ps1` now falls back to native `PrintWindow`
  only after that exception; `check-harness-registry.ps1` preserves both paths.
  A clean full Latest no-build profile passes in 201.2 seconds on battle ROM
  DA8282BBBD9872DC29F7442CC6ED3E0029967A7AB1AA0E94F9EDBED172981F04
  and unchanged normal ROM
  D06323485C866D74BA5D82F87B58182C82A3D7FBE5E9AAC08B83807583171A9E.
  Capture `2026-07-17_canonical_fast_212428-0396036-p48468.png` and its paired
  frame retain 100% overlap, 1,844/49,152 meaningful changes, and every
  visibility, named-region, horizontal-detail, and required-region gate.

EVIDENCE:
  artifacts/performance/2026-07-17_m2-raw-texture-split-{a,b}.json
  artifacts/performance/2026-07-17_m2-raw-texture-split-{generic,fast}-profile2.json
  artifacts/visibility/2026-07-17_m2-raw-texture-split-{a,b}-frame607.png
  builds/m2-raw-texture-split-{control,candidate}/
KEEP / REWORK / REVERT: KEEP RAW TEXTURE-CLASS SPLIT / CONTINUE P1
```

## 2026-07-17 - Task 10 hardware-calibration loop

```text
IDEA ID: TASK10-HARDWARE-CALIBRATION-20260717
BOUND:
  Add a compile-time profile-1 phase HUD and a standalone calibration ROM.
  Profile 0 retains the same source graph and binary. The lab is a Make target,
  not a harness-registry mode, and none of its symbols link into production.

MELONDS IDENTITY:
  melonDS 1.1, ARM interpreter, software renderer, unthrottled LimitFPS=false,
  muted host audio, canonical 416x664 window. Config SHA-256 is
  695273E6AC92305B1BDCA71FC72D44A8BE23F05B12517FD7C04532F0576C5B25.

STANDALONE LAB TICKS:
  ALU-ITCM   17,501,568  (1M dependent chains, 32 adds each, ARM .itcm)
  MEM-THMB   33,557,632  (8M loads, 256 KiB stream, Thumb state)
  MEM-ARM    37,752,448  (identical load loop and buffer, ARM state)
  CACHE4K    33,565,696  (same total loads, 4 KiB control)
  GX-BRST     2,729,728  (10K immediate triangles, swap every 2,048)
  CARD              TBD  (optional; no media-independent result admitted)

PROFILE-1 HUD PROOF:
  Focused frozen-Fox frames 600..607 retain exact 828 triangles and
  60/320/306 stage/Mario/Fox ownership. P50/P95 ticks are source update
  239,520/258,688; draw 1,011,840/1,015,232; active
  1,016,032/1,065,856; loop 1,680,448/1,680,512. The half-second formatter
  costs 384/50,880 HUD ticks and reports nonzero ACT plus zero slip in the
  accepted capture.

HARDWARE PROJECTION METHOD:
  Measure the exact same lab ROM on melonDS and retail hardware, then compute
  M_ITCM = hardware ALU-ITCM / melonDS ALU-ITCM,
  M_MAIN = hardware MEM bench / melonDS matching MEM bench, and
  M_GX = hardware GX-BRST / melonDS GX-BRST.

  For a measurement whose exclusive owners are known:
    projected hardware ticks = ITCM ticks * M_ITCM
                             + main-RAM ticks * M_MAIN
                             + GX ticks * M_GX

  Keep mixed or unattributed time unprojected and state that uncertainty. The
  CACHE4K control diagnoses locality; it is not a replacement for M_MAIN.
  Never apply the observed 0.75x throughput as one universal tick multiplier.
  Retail results are now recorded in the board's Hardware reality table. The
  admitted buckets are ITCM x1.03, streaming Thumb x1.50, streaming ARM x1.20,
  cache-resident control x0.88, and GX transport x0.87; the workload still
  requires direct device A/B because melonDS has no useful cache model.

EVIDENCE:
  artifacts/visibility/2026-07-17_230709-6847521_task10-hardware-calibration.png
  artifacts/visibility/2026-07-17_task10-phase-hud-profile1-pass.png
  builds/build-task10-hardware-calibration/
  Lab ROM SHA-256:
    builds/build-task10-hardware-calibration/smash64ds-task10-hardware-calibration.nds
    04F32CA76045B821A5404D55F740808DF13B3A0BA2261CC8D7AD5DB3459D263C
  Profile-1 HUD ROM SHA-256:
    builds/task10-hardware-packet/smash64ds-task10-phase-hud-profile1.nds
    37AD2C8A4C388F3969A52E048D4921B524F8F579516A6973944CE5A7D9E81B60
  Retail photo SHA-256:
    5AF90D991C83ABA72F5D8A17403BA0555A332D243365B6D1A342669C6DF9E210
KEEP / REWORK / REVERT: KEEP TOOLING / HARDWARE CALIBRATION COMPLETE
```

## 2026-07-17 - Task 9 Phase 2 exact ARM `fcmpeq`

```text
IDEA ID: TASK9-PHASE2-FCMPEQ-ARM-20260717
RECONCILIATION / BOUND:
  The earlier Phase-1 row stopped before custom code because its 16.54% owner
  win was already decisive. the owner explicitly resumed the staged Phase 2. The
  census names fcmpeq as the safest next exact cut: 556.25 calls/update,
  40 median ticks/call, and a 127,148-tick observed bound. This ratchet changes
  only that helper; fadd/fmul and conversion helpers retain stock libgcc.

CODE / LINK IDENTITY:
  The candidate is a 36-byte, nine-instruction, stackless and call-free ARM
  leaf in .itcm. Machine-code SHA-256 is
  07B3147B9CF599BDD408AF922A4B9F6891734B4C3AB7DE7C3A700DDE92B6FBE2.
  The selected GCC 15.2.0 Thumb-multilib libgcc archive remains
  C755ADC33ECA252260360327904591B8462CCE5C25E48B0E881AC0B295953F48.
  Phase 2 retains the stock _arm_cmpsf2.o text byte-for-byte (276 bytes,
  2B656E12FDE0F34CEB17395A5FF8FCC1EF0CEBBF94F8EDA3725BD26C0B3C2884)
  and renames only its public fcmpeq symbol to the linked golden
  __nds_task9_libgcc_fcmpeq_golden. Phase 1 proves that golden is absent.
  ITCM moves 28,052 -> 28,088 / 32,768 bytes, leaving 4,680 bytes free.

HOST AND ARM9 SEMANTIC PROOF:
  Host differential proof passes 16,777,216 directed pairs covering zeros,
  subnormals, normals, infinities and NaN payload/sign classes, then
  100,000,000 deterministic randomized pairs. Final PRNG seed is
  64F086E1A4420059 and mismatches are zero.
  A standalone ARM9 melonDS ROM compares candidate and the linked stock golden
  over 54 directed values / 2,916 pairs with zero mismatches. ROM/ELF SHA-256:
  8B64932AF9D8DA35D2E7114AE96B4353B72A472BDFF457E2352740B99C6AFBDF /
  596C8916949883AD7C7FCFC1CFF26987B64A226E2CC7ADC7603C31936C2EFF2A.

SUPREME STATE GATE:
  Phase 1 (ITCM/Phase2/Hash 1/0/1) and Phase 2 (1/1/1) each complete the
  one-minute CPU-on mode-163 lifecycle. All 3,892 post-update full active-game
  state rows match exactly and both overflow counts are zero. The verifier
  now rejects wrong mode identities or reused ELF/ROM identities before it
  compares rows. JSON SHA-256 values are
  D322B55DA1923EC43F66203F93DB91DFBDFDED4873693328959A500AEA69C75F /
  7EC98FCD359E97F2C21DB0B8E38CC3AC88D9A9BFD827F0C6B6521EC794577C78.

SYNCHRONIZED EIGHT-FRAME A/B:
  Mode 163, profile 1, fast mode 9, static AOT 1, incremental wallpaper 1,
  live Fox, identical frames 438..445 and controls. A is Phase 1 ROM/ELF
  1D55C7F38B1E987488882CFC51FA36B5DE7E9F457F24627D231BC6DBD8F11018 /
  3432C2B3A44D76C03A966BA3D90713D1941A4381C1EEF22C203AADF1D1FCAD99.
  B is Phase 2 ROM/ELF
  A517F627A1BA836DE85688A9F71274F32D1551BCD06ED3C30238622F5299E062 /
  3E46FA161228CCFA60F6DAC7E68F1AA5EE23347A62DECC89256E3316D86EE0E5.

  Owner / frame       Phase 1 P50/P95      Phase 2 P50/P95      Delta
  source update       236,640 / 238,016    215,680 / 217,024    -20,960 / -20,992
  stage               476,032 / 476,352    476,000 / 476,160        -32 /    -192
  draw              1,154,144 / 1,225,920 1,151,808 / 1,223,616  -2,336 /  -2,304
  active            1,159,008 / 1,230,784 1,156,704 / 1,228,416  -2,304 /  -2,368

  The stable source owner gains 8.86% / 8.82%. All eight frames retain exactly
  828 triangles, stage/Mario/Fox 202/320/306, and zero fallback, texture-fence,
  conservation, or benchmark-identity faults. The stable owner and decisive
  expected result make a third A unnecessary.

EVIDENCE:
  scripts/check-task9-phase2-fcmpeq.ps1
  scripts/verify-task9-phase2-fcmpeq-arm9.ps1
  scripts/verify-task9-state-hash-ab.ps1
  artifacts/performance/2026-07-17_task9-phase2-fcmpeq-phase1-a.json
  artifacts/performance/2026-07-17_task9-phase2-fcmpeq-candidate-b.json
  artifacts/performance/2026-07-17_task9-state-phase1-itcm.json
  artifacts/performance/2026-07-17_task9-state-phase2-fcmpeq.json
KEEP / REWORK / REVERT: KEEP PHASE 2 FCMPEQ / INTEGRATED BOUNDARY PENDING
```

## 2026-07-18 - Task 18 production KO falsifier

```text
IDEA ID: TASK18-PRODUCTION-KO-FALSIFIER-20260718
BOUND / IDENTITY:
  Falsify the cited KO wallpaper spike on the retained production path; make no
  runtime change. The isolated checkout is pinned to
  19fdafa48d5e4dca23d2d0af212737a756001af8. Mode 163 uses profile 1, fast
  mode 9, static AOT 1, live Fox, WallpaperIncrementalMode=1, and Phase 0.5.
  ROM/ELF/map SHA-256 values are
  3E4A909AE0D6C87FE3728206BF611BDA6DDE08B9B3628C20D539369612363A1B /
  1DBA648A3F7E4581A24F71AFEB52998CC393327F2EC450B528ADA206F8ADBCC8 /
  ECDA8D11D560EBA77ADCF5CA1D82D96920A859D04BF85536BA99154CB76C109A.
  melonDS 1.1 is interpreter/software-renderer/unthrottled; executable/config
  SHA-256 values are
  04738277BA1D7EA0B7408608755D746193A68A9BFA628E2759140A9F2D5AB109 /
  B4AA5C791B4D8CC94E7EBC4AC77514CCE3B83D5976A726E46FD5FF29B714B2F3.

SAME-ROM PRODUCTION FALSIFIER:
  Natural KO frames 708..715 (logic 1024..1038) measure wallpaper
  302,880/357,824 P50/P95 ticks. Steady frames 438..445 measure
  292,224/360,000, so KO is +10,656/-2,176 ticks (+3.65%/-0.60%). Every KO
  frame retains 828 source triangles plus 16 transient death-effect triangles,
  zero tile/texture uploads, and the resident source cache. Decode/build is 1,
  decode hits are 714, fast draws/fallbacks are 715/0, and all eight KO frames
  change the camera-derived final key while incremental remapping writes only
  the changed pixel subset after the first frame.

CLOSURE:
  The cited 547,584/547,648 wallpaper row is
  WallpaperIncrementalMode=0, a forced full-raster oracle, not production. Its
  source JSON SHA-256 is
  C6926AAD99218EF3E8282D3D85839CAE175B4273AAB05B937987E18EB89C125A.
  Task 18 closes as a bad full-raster-oracle baseline/documentation defect; no
  runtime fix is justified. The affine lab is disabled and unreachable in the
  measured ROM, and affine remains retired.

EVIDENCE:
  artifacts/performance/2026-07-18_task18-production-ko-incremental-phase05.json
    AC45C244FC4067C29615F715D7E710C65D19AC75E4D1AA79F41FDCFAC0E63FC8
  artifacts/performance/2026-07-18_task18-production-steady438-445-phase05.json
    29A3104F61DEEA6C883B891F1BDBA6FE7014F6348E4B0DD15DB3A03F3C62AAA1
  artifacts/visibility/2026-07-18_task18-production-ko-incremental-frame715.png
    7EB1B82516BCB96BC70DBCA9CDCA9CE75549C05CA09D7B890CD1E858FA64ECCF
KEEP / REWORK / REVERT: CLOSE BAD BASELINE / AFFINE REMAINS RETIRED
```

## 2026-07-18 - Task 13 fighter decimation pack

```text
IDEA ID: TASK13-FIGHTER-DECIMATION-PACK-20260718
BOUND / IDENTITY:
  Test the requested deterministic derived-asset LOD substitution on the
  natural mode-163 Mario/Fox window. The generator admitted 17 of 32 parts and
  reduced the pair's fighter payload from 626 to 402 triangles. Pack/manifest
  SHA-256 values are
  5DAC5E5C518284D4EA464AAF3E5B81E05A9740CA30D3B04E9675ECECC264A67B /
  EB80B3D4ACABF49C60E74D8D59A40A2D3B3C698BFFB824A92FA6E4EABF2239B0.
  The aligned control ROM SHA-256 is
  D9072000D678DA2780A52B17CB5F720D06972822D57FF3980B83831901FC4446.

SYNCHRONIZED EIGHT-FRAME A/B, FRAMES 438..445:
  The aligned source control measures draw 1,743,200/1,833,984 P50/P95,
  active 1,748,032, stage 791,872, Mario 299,200, Fox 371,072, and 828
  total triangles. Enabling the runtime with part mask 0 changes no geometry
  but measures draw 1,748,320, active 1,753,312, stage 794,432, Mario
  300,480, and Fox 372,480: a fixed +5,120 draw-tick loss.

  The best three one-run Mario parts, mask 0x89, remove 64 triangles
  (626→562) but measure draw 1,746,368/1,837,184, active 1,751,360,
  stage 794,464, Mario 298,368, and Fox 372,480. Against control that is
  +3,168 draw P50 and +3,328 active despite the local Mario reduction.
  Adding a fourth part (mask 0x99, 98 triangles removed) regresses draw by
  4,192; the full pack regresses it by 7,360. Moving cold support code out of
  ITCM and skipping the unused Fox lookup still leaves every best-three paired
  frame 3,136–3,264 ticks slower. The design costs about 3,216 ITCM bytes.

DECISION:
  REVERT. More removed geometry monotonically increases the loss and the
  smallest useful subset cannot repay the fixed hash-lookup/substitution cost.
  The decisive performance gate failed before the owner visual sign-off; no
  default-on or user-facing flag-ON ROM is warranted. All Task 13 runtime,
  tooling, generated pack/manifest, and generated include changes were removed.

EVIDENCE:
  artifacts/performance/2026-07-18_task13-control-aligned-438-445.json
    D99D77A0E6463B25BA021AEEE2585F1514B2C30221A336D5346ED3054A138681
  artifacts/performance/2026-07-18_task13-lod-mask0-438-445.json
    27E7546BE2EEB55DC023CDA46DAC281427FAF124BDF21857AE74809205613AE2
  artifacts/performance/2026-07-18_task13-lod-best-three-438-445.json
    D4989476403A61858E3D3BDAFFD0341366BAEB8B280F54510F49DDE1D8A83B20
  artifacts/performance/2026-07-18_task13-lod-best-three-placed-438-445.json
    1274FC75717F076960278D9FCD2A5EC3F63C2F96C34A542470834361D9E21590
KEEP / REWORK / REVERT: REVERT / RUNTIME, TOOLS, AND DERIVED ASSETS REMOVED
```

## 2026-07-18 - Task 14 generation-gated dense first-visit plan

```text
IDEA ID: TASK14-DENSE-FIRST-VISIT-PLAN-20260718
BOUND / LIVE-STATE SPLIT:
  Extend the existing CUT E topology cache with only immutable run offsets and
  the exact first-visit permutation of the 312 generated dense vertices. The
  full validator constructs the plan behind generation/stamp validation and
  publishes valid last. Every frame still recomputes matrices, materials,
  texture selection, alpha, color, UV, and near-plane transforms. The cut
  removes only the 606-corner scan and 312-bit per-frame first-use mask.

SYNCHRONIZED EIGHT-FRAME A/B, FRAMES 438..445:
  Control/candidate are mode 163, profile 1 + identical Phase-0 observation,
  fast mode 9, static AOT 1, live Fox, incremental wallpaper, interpreter,
  software renderer, and the same melonDS config hash CE818A04.... ROM hashes
  are 3E4A909A... / 09586578....

  Metric                 control P50/P95       candidate P50/P95     delta
  stage                  904,928 / 905,088     895,872 / 896,000   -9,056 / -9,088
  draw                 1,886,688 / 1,976,960 1,877,408 / 1,968,256 -9,280 / -8,704
  active               1,892,992 / 1,983,296 1,883,744 / 1,974,592 -9,248 / -8,704
  preflight              424,064 / 424,064     414,720 / 414,784   -9,344 / -9,280
  prepare-runs           350,592 / 350,848     342,080 / 342,272   -8,512 / -8,576
  attribute-exclusive     87,968 /  88,384      78,592 /  78,912   -9,376 / -9,472

  Per-frame stage deltas are -8,768, -9,216, -8,960, -9,024, -8,832,
  -9,024, -9,152, -9,152. Draw and active also improve on every paired frame.
  This is a repeatable correctness-preserving main-RAM CPU gain; no withdrawn
  minimum threshold is applied.

CORRECTNESS / FAIL-CLOSED RESULT:
  Exact packet/census remains 8/255/57/42/54/202/49/4, cross 5/10/15,
  owner 121/828 with 202/320/306 partition, dense/near/matrix 312/226/146,
  and zero fallback/fence/conservation faults. The native frame-445 crop is
  raw and meaningful 0/49,152 changed pixels, mean delta 0.00.

  The lab mutation records full2, hits436..443, mismatch1, inject1,
  revalidate1. A separate clean production build records full1, hits437..444,
  mismatch/inject/revalidate 0/0/0; nm finds zero lab-fault symbols. Host proof
  pins the exact 55 offsets from 0 through 312, one 312-ID permutation, uniform
  source alpha for all 54 runs, 606 corners, and 12 fail-closed perturbations.
  The static fixture requires the unique-bit guard, terminal offset/count, and
  valid=FALSE -> checked full validation -> generation/stamp -> valid=TRUE
  publication order. GBI fixtures and the renderer parity corpus pass.

SIZE / PLACEMENT:
  BSS grows 736 bytes (734-byte arrays plus 2-byte alignment), text grows 568
  bytes, and the observed lab ROM grows 1,024 bytes. ITCM remains exactly
  28,132 bytes in the instrumented pair and 28,088 in production; renderer
  ITCM remains 15,396. The 40-byte mask moves from every hit-path owner stack
  to miss-only full validation, so validation peak is unchanged and the normal
  hit path uses 40 fewer stack bytes. No arena or DTCM change.

EVIDENCE:
  artifacts/performance/2026-07-18_task14-control-phase0-438-445.json
    D1376B8F3671301EC64381A3A652348BA18B5E45B55A5B6FBDEBA15899258868
  artifacts/performance/2026-07-18_task14-candidate-phase0-438-445.json
    5235AEC60CBCF36F4503784F736A61F3D9DE22A6CBE25CCBBC7F3C00B2EAB0EF
  artifacts/performance/2026-07-18_task14-candidate-production-438-445.json
    DA4169BA8D28C110652E9AE5B3FCC73AB1BF6A4FD959AA294A4E7F3F075208AE
  artifacts/visibility/2026-07-18_task14-control-frame445.png
    48758530DF00AA8B038DF626708465E84D608EA784BC6BA54644BFC4665354FA
  artifacts/visibility/2026-07-18_task14-candidate-frame445.png
    B79765A7A1040FC27253FACCC93D22477D6ABFD7D2FB71BA9F0F6B5DB4825D0E
KEEP / REWORK / REVERT: KEEP
```

## 2026-07-18 - Task 25R authoritative stable-30 baseline

```text
IDEA ID: TASK25R-AUTHORITATIVE-STABLE30-BASELINE-20260718
BOUND / IDENTITY:
  Measurement, verifier, and report tooling only. The production profile-0
  executor has no Task-25R timing symbols, stores, or layout change. One
  profile-1 ROM supplies all seven detailed eight-frame windows and one
  profile-0 sibling supplies the complete canonical lifecycle. Both use mode
  163, renderer mode 9, static AOT 1, incremental wallpaper, live Fox, Task 9
  1/1, and Task 16 compare/i2f/add-sub 1/1/1.

  Measured source HEAD:
    f088db98de272e9788405c2181029ad4a4c353ba
  Profile-1 ROM / ELF:
    6E90D4140E6332E8F37BB05CB8B35ED192AAB448B26E110916992F2C15701921
    55CC8EF067E68310441F7025978FCF7569E95E51A3B6B937E7811A7D25B44C06
  Profile-0 ROM / ELF:
    E685C034D301D3C6881D398B14820D0D60A112FA259657A6B05408C90683C5CE
    E9428051194F9E6917427DCD54BE44838C049E856ECBAFD6A702D605A34A1A96

SAME-ROM PROFILE-1 WINDOWS (P50/P95/MAX ARM9 TICKS):
  Phase             frame/source       whole loop                    stage P95  Mario+Fox P95  2/3/4/5+
  Countdown / GO    438-445/484-498    1680448/1680512/1680512       464320     380928          0/8/0/0
  Early combat      600-607/808-822    1680448/1680448/1680448       464448     207040          0/8/0/0
  Whispy            1398-1405/2404-18  1680448/2240640/2240640       464576     381312          0/6/2/0
  Late combat       1846-53/3300-14    1120256/1120320/1120320       464384     380416          8/0/0/0
  Natural KO        566-573/740-754    1120256/1680512/1680512       468480     205440          6/2/0/0
  Natural rebirth   589-596/786-800    1680448/2240640/2240640       464192     207616          0/7/1/0
  Time Up / Results 1988-95/3584-98    1120256/1680448/1680448       464320     381632          7/1/0/0

  Countdown is the only phase that satisfies the strict fixed-both-fighter
  geometry and zero post-GO fence contract. Early, KO, and rebirth have Mario
  source-owned nonrendering with zero fallback; their observed 91/508 geometry
  is 202+0+306 rather than the fixed 202+320+306 expectation. Whispy, late,
  and Time Up fail the post-GO residency fence. These are reported exactly,
  not normalized away.

PROFILE-0 LIFECYCLE / STABLE-30 DEFICIT:
  Source updates / presentations / teardown: 4,084 / 2,042 / 1
  Presentations/s / updates/s:             18.6 / 37.3
  Interval histogram 2/3/4/5+:            61 / 1,547 / 396 / 38
  Intervals >=3 VBlanks / slip events:     1,981 / 2,457
  Net reserve / floor:                     166,672 / 131,072 PASS
  Synchronized native pixels:              0 / 49,152 changed

CORRECTNESS DEBTS:
  The post-GO texture fence first trips at class+1/frame 10/1111 with counts
  1,1,1,0,1,1,0,0,0,1. The natural Mario-KO source sequence is exactly
  439/292/154 with mask 0x13, but playback/generation failure counts are 1/1,
  so the exact KO FGM gate is false. Normal verifier paths remain strict.
  -RequireStable30 correctly fails on pacing, fixed-window exactness, audio,
  and the fence while reserve and synchronized pixels pass.

PRIORITY:
  The same-ROM leaf-owner comparison selects M3-first. Maximum stage P95 is
  468,480 ticks at natural KO, versus 380,544 for the largest combined fighter
  pair at Countdown. Next queue is Task 23R Phase 0 -> Task 26 -> only the
  residual Task 23R Phase-1 work Task 26 leaves. No runtime optimization is
  promoted by this row.

EVIDENCE:
  artifacts/performance/2026-07-18_task25r-phase-matrix.json
    DDC65C1507FBBC81EEBDE15E3681AE65A97926F945BCBCA1ED491A6D53D9F343
  artifacts/performance/2026-07-18_task25r-phase-matrix.md
    54B60555DE32E002C413A3A438738F074D7024DF00CDFC986D0B9942BDF37601
  artifacts/performance/2026-07-18_task25r-identity/identity.json
    3EABC8119CF8D1AADC911BEEEAB3F90EC6E33FA02CD259DFAF056CB7F9755314
  artifacts/visibility/2026-07-18_task25r-profile1-frame607.png
    ECEE8B8720C382A1123F2F278F35D5CA5AE64D080F2E171AFF7ED040F142BCA0
  artifacts/visibility/2026-07-18_task25r-profile0-frame607.png
    8FE8473D3854B25B28EC6FD7B68F60D8E9927DAEE597CCF31ED812C2E7F155A6
  Per-phase JSON, lifecycle JSON, nm/readelf/size/toolchain, compressed map,
  and compressed disassembly share the same task25r evidence prefix.
KEEP / REWORK / REVERT: KEEP TOOLING + EVIDENCE / STABLE-30 FAIL / M3 FIRST
```

## 2026-07-18 - Task 23R Phase 0 consumed-field certificate

```text
IDEA ID: TASK23R-PHASE0-CONSUMED-FIELD-CERTIFICATE-20260718
BOUND / IDENTITY:
  Host generation/checking and debugger-only census; no runtime reuse, cache,
  code, data, stack, reserve, or DS working-set change. All eight windows use
  mode 163, M3 owner 9, static AOT 1, incremental wallpaper, live Fox, Task 9
  1/1, and Task 16 compare/i2f/add-sub 1/1/1.

  Measured source HEAD:
    1d381c447f06deed04b7749bffe6d5bb1259b303
  Profile-1 ROM / ELF:
    88EF4931A24151A70FA05CF0AE2E6B69501B708DE405584D4716657A6DA56249
    FEC9EB30BBD628F0E9DDB502C0A7DF0752BC1B0E1E4C227C39B99EA25AAB1FDA

CONSUMED-FIELD CLOSURE:
  The deterministic generated manifest binds 588 pointer-field accesses across
  36 production closures: immutable generation 140, live camera-dependent 43,
  live camera-independent 260, callback-visible mutation/output 145. Exact
  closure-policy equality makes any new or removed pointer-base access fail
  until classified; the falsifier introduces a wholly new pointer base. Twelve
  fail-closed packet perturbations and that unclassified-read falsifier pass.
  Exact stage counts remain 8/255/57/42/54/202/49/4, cross
  5/10/15, and dense/near/matrix 312/226/146.

  Task 26 live operands are ordered asset_bases[4], binding_composed[42],
  materials[4], config. Generation/stamp admission, all matrices and near-plane
  work, material rebuild, callback restoration, and fallback-before-GX remain
  live. A second topology cache, cached camera results, generic runtime scan,
  or post-GX fallback is forbidden.

EIGHT SYNCHRONIZED EIGHT-FRAME WINDOWS:
  Phase             profile frames  segment s0-s7  material m0-m3  boundary
  Countdown / GO    438-445         each 7/0       each 7/0       unknown
  Early combat      600-607         each 7/0       each 7/0       unknown
  Whispy transition 672-679         each 7/0       each 7/0       exact 675->676
  Whispy steady     1398-1405       each 7/0       each 7/0       unknown
  Natural KO        566-573         each 7/0       each 7/0       unknown
  Natural rebirth   589-596         each 7/0       each 7/0       unknown
  Late combat       1846-1853       each 7/0       each 7/0       unknown
  Time Up / Results 1988-1995       each 7/0       each 7/0       unknown

  Counts are adjacent prepared-output hits/changes only. Eight samples create
  seven opportunities; the first sample and cross-window boundary are not
  counted. The synchronized transition rows change Whispy source
  status/wait/duration/blink from 1/1/0/65 at frame 675 to 3/0/0/63 at frame
  676 while the exact G2 tuple changes 44/13/166/0/70/34 ->
  44/13/166/0/70/36. BattleShip starts the Whispy material animation at this
  Wait-to-Open edge (`grpupupu.c:250-275`, `:565-597`). Prepared segment and
  material snapshot lanes remain 7/0; that observed stability is not promoted
  as a residual-cache hit. Task 23R Phase 1 must remeasure the exact residual
  key after Task 26.

CORRECTNESS / CHECKPOINT:
  Early frame 607 is exactly 0/49,152 changed native pixels versus Task 25R,
  mean delta 0.00 and overlap 100%. The phase-matrix harness now pins exact
  phase-specific G2 tuples and accepts only exact gameplay-active wallpaper
  rows or exact Late/TimeUp inactive 0-row/0-write state while preserving
  row/store conservation. Boundary passes in
  77.8 seconds with the manifest checker in the normal GBI fixture path.
  The unchanged Task 25R reserve baseline remains 166,672 bytes.

NEXT:
  Task 26. First candidate is segment 0 / layer0 generated preflight/control
  only: bindings 0-19, runs 0-25, 54 triangles, 22 epochs, all projected-no-Z,
  no material event. Reuse existing prepared storage, commit loop, and GX
  emitters. Do not begin Task 23R Phase 1.

EVIDENCE:
  docs/optimization/NDS_NATIVE_STAGE_CONSUMED_FIELDS.generated.json
    0A9BB01791F05BB1DF78FA8F358205DF5BEA371EE7268A10D291042722CDA205
  artifacts/performance/2026-07-18_task23r-phase0.md
    854D4CCA580670728B4368B1D5D3A8632B45F1D7C757F0447EABE7D9CE5D48AE
  artifacts/visibility/2026-07-18_task23r-phase0-profile1-frame607.png
    BDEC00014752795F8923CAC5046F830231AF23B5AFD2696C41D9A5132C516ACE
  Eight per-window JSON captures share the task23r-phase0 evidence prefix.
KEEP / REWORK / REVERT: KEEP PHASE-0 CERTIFICATE / NO RUNTIME REUSE / TASK 26 NEXT
```

## 2026-07-18 - Task 26 exact generated M3 segment 0

```text
IDEA ID: TASK26-GENERATED-M3-SEGMENT0-20260718
BOUND / IDENTITY:
  One fixed generated program for Dream Land segment 0 / layer0 only: 21
  DObjs, bindings 0-19, runs 0-25, 54 triangles, 22 epochs, 108 dense
  vertices, 123 immutable state effects, and 90 synchronization effects.
  Matrices, clipping, materials, texture/color/alpha/UV selection, validation,
  current commit/GX emission, and fail-closed fallback before GX remain live.
  There is no runtime opcode scan, packet copy/patch, per-frame list build,
  sorting, second topology cache, or post-GX fallback.

  Implementation base HEAD:
    4e8ecbf0406cac77ab69b9f9980a9885f4bd7df1
  Published battle ROM / ELF:
    757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4
    443475D101D79DC6C069EFD9AFBF537CD363CA00CF587CD7AF7A967EC5084631
  Published ROM size: 14,681,088 bytes

EXACTNESS:
  Frames 438-445 compare all 2,775 CPU-preparation words and 26 rows with zero
  differences. Owner-word conservation is exactly 15,126 = 7,011 stage +
  4,130 Mario + 3,985 Fox. The live-mutation falsifier reports
  inject/mismatch/revalidate 1/1/1 and zero ARM faults, before GX. Exact
  8/255/57/42/54/202/49/4, 121/828, and 202/320/306 contracts remain, with
  zero fallback, fence, or conservation failure. Synchronized native pixels
  are 0/49,152 in both production and hardware-style pairs.

TASK-25R-CONTROL PHASE A/B (P50/P95 ARM9 TICKS):
  Phase               stage control -> candidate           stage delta       draw delta
  Countdown / GO      464128/464320 -> 460544/460608       -3584/-3712       -3296/-3264
  Early combat        464352/464448 -> 460736/461056       -3616/-3392       -3680/-3648
  Material edge       464480/467712 -> 460928/464064       -3552/-3648       -3488/-3584
  Whispy steady       464288/464576 -> 460864/461056       -3424/-3520       -3104/-3200
  Natural KO          468256/468480 -> 464640/464896       -3616/-3584       -3552/-3520
  No relevant phase or P95 regression occurs.

CURRENT HARDWARE-STYLE MELONDS PAIR:
  Countdown stage 465472/465600 -> 455232/455232 (-10240/-10368), draw
  1148864/1217088 -> 1137632/1205888 (-11232/-11200), and active
  1152896/1221184 -> 1141728/1210048 (-11168/-11136). Loop P50 remains
  1680448 and P95 moves +64 ticks inside the same VBlank bucket.

SINGLE RETAIL OBSERVATION (USER SUPPLIED; NO REPEATABILITY CLAIM):
  generated: UPD 330944, DRW 1706688, ACT 1709440, LOOP 2240384
  control:   UPD 1547072 spike, DRW 1728256, ACT 1705152, LOOP 2240768
  delta:     UPD excluded, DRW -21568 (-1.25%), ACT +4288 (+0.25%),
             LOOP -384 (-0.02%, same VBlank bucket)
  ACT below DRW in the control photo is valid because HUD rows refresh at
  staggered times. The user declines repeats; do not infer a stable cache or
  layout effect from this one pair.

FOOTPRINT / PLACEMENT:
  Canonical owner/helper are ARM, 8,292 / 1,472 bytes; the compact hot-run
  table is 52 bytes. Canonical local frames including saved registers are
  192 / 168 bytes. Instrumented loaded footprint grows 7,748 bytes (+20 ITCM,
  +7,696 main text/rodata, +32 BSS); the hardware-style pair ROM grows 6,144
  bytes. Canonical ITCM is 28,820 / 32,768, leaving 3,948 bytes.

INTEGRATED LIFECYCLE REPAIRS:
  Exact static residency adds runtime-observed Whispy asset 152 and late Fox
  asset 313: 24 keys, 23 outputs, 132,096 payload and 136,192 prepared bytes
  across VRAM A+B. All ten post-GO fence counters stay zero. An exactly
  completed Calico hardware channel now retires its stale software audio owner
  before reuse; inconsistent ownership remains fail-closed. The refreshed
  lifecycle passes the exact 439/292/154 KO sequence, 4,084/2,042 fixed-two
  pacing, one teardown, Results, and 166,672-byte net reserve. Stable-30
  remains correctly red.

DECISION / NEXT:
  KEEP the exact positive segment-0 slice. STOP broader Task-26 expansion
  because the generated working-set effect cannot be device-falsified without
  another retail A/B, which the user declines. This expansion gate does not
  revert the smaller exact win. Task 23R Phase 1 next measures only residual
  work and may retain a cache only at >=20% exact complete-key hits with key
  cost below half the avoided work.

EVIDENCE:
  artifacts/performance/2026-07-18_task26-generated-segment0.md
  artifacts/performance/2026-07-18_task26-segment0-cpu-prep-trace-e0.json
    14F263B7C51A468562C74E7AFCB0508EAC455BECE21F72A2CC6C127A7983FD9A
  artifacts/performance/2026-07-18_task26-segment0-cpu-prep-trace-e1.json
    486464C680B4CD13479E5A8324FA4AA0297A0B7EE7EEC720786B6FE77D4CD5DC
  artifacts/performance/2026-07-18_task26-hardware-control-melonds.json
    E2D37CFC3C6F58AE5ED48056498FB48F90A274009E15FFFC7E6BA6B0F06830CE
  artifacts/performance/2026-07-18_task26-hardware-generated-melonds.json
    620BA8E56785A30A63D0FF7C094DDB42FA1B56EB9BCF39FCBC53A1BF8E7EA443
KEEP / REWORK / REVERT: KEEP SEGMENT 0 / STOP EXPANSION / TASK 23R PHASE 1 NEXT
```

## 2026-07-19 - Task 23R Phase 1 residual census and cache falsification

```text
IDEA ID: TASK23R-PHASE1-RESIDUAL-DENSE-CACHE-20260719
BOUND / IDENTITY:
  Post-Task-26 residual owner only. Five synchronized eight-frame census
  windows use mode 163, generated segment 0, static residency, incremental
  wallpaper, live Fox, Task 9 1/1, and Task 16 1/1/1.

  Census ROM / ELF:
    D0B6C5781135A83D8CADA939DEB1D25027CDD9B12EE3B3E8D0FF59788E6D2B18
    7F0220052975A964067243627801BAD2E07568AC60C8294A82C7D1A3D11710FD
  Same-ROM candidate/control ROM / ELF:
    A1E9D574552ED004DA8E6F3D5295A7505CEFADA9BC1F45EA665FD15E47E3D77D
    B44EC74AB267EEF14B261937ED78DDFB8EAC49E69D72C08676FAB6ECB987539A

CENSUS:
  Countdown, early, material-transition, natural-KO, and natural-rebirth
  windows produce 40/40 hits for the collision-free 484-byte complete key.
  Worst key P95 is 896 ticks. Residual prepare is 102,784-103,168 ticks and
  the avoided upper bound is 85,376-86,272 ticks, so the hit/cost experiment
  gate clears. Exact stage/cross/dense/near/matrix, Task-26 shadow, texture,
  fallback, and ownership gates pass in all five windows.

ONE NARROW SAME-ROM CANDIDATE:
  Reuse only the 204 residual dense packed color/UV values. Keep every state
  span, texture resolve, material progression, matrix, and all 118 near-plane
  transforms live. Candidate markers report 8/8 hits, 204 reused values per
  sample, and forced key mutation inject/reject/revalidate 1/1/1. The exact
  sampled window records zero recomputes.

  Metric              control P50/P95       candidate P50/P95     delta
  residual prepare    108480/108672         95360/95616          -13120/-13056
  residual vertex      52832/53056          39552/39872          -13280/-13184
  stage               750112/750528        738336/738432        -11776/-12096
  draw               1406912/1431040      1394944/1419072      -11968/-11968
  active             1413312/1437376      1401312/1425408      -12000/-11968
  loop               1680448/2240640      1680448/2240640           0/0
  VBlank interval       3/4/4                3/4/4                  0/0/0

  Synchronized frame 607 is exactly 0/49,152 changed native pixels, mean
  delta 0.00, overlap 100%. The lab candidate adds 548 BSS bytes, 360 bytes
  for two out-of-line helpers, +1,024 ROM bytes, and +6,484 ELF bytes versus
  the census build.

DECISION / NEXT:
  REVERT CACHE / KEEP CENSUS. The exact emulator-local compute deletion is
  real, but this code/data/record-layout change is cache-sensitive on retail
  DS. No retail falsification will be collected, the interval histogram does
  not improve, and the candidate window does not exercise profile-2 sampled
  recomputation. Remove every selector, helper, global, BSS byte, and runtime
  cache path. Retain only compile-gated census/checker/harness exports with
  zero production footprint. Continue with Task 21R -> Task 27.

EVIDENCE:
  artifacts/performance/2026-07-18_task23r-phase1.md
  candidate/control JSON:
    5417D1C284495CBB010C9AB84FEC35B7D0514820FDFCFBFEC39BF548812BCD6A
    0887E2D914C3D1053134EB2146241E2C5052CCAE70A1503B503E7AF7E561507B
  candidate/control screenshots:
    8DE92BDF28F28228478231E3B7BF0E04EC1F420AC2790803F199ED614927B3CA
    DA2368F56C81476C88D0003FBC286979E3E40F3CFCC7F84A6C8862A57AEA726F
KEEP / REWORK / REVERT: KEEP CENSUS / REVERT CACHE / TASK 21R NEXT
```

## 2026-07-19 - Task 21R M2 structural foundation

```text
IDEA ID: TASK21R-M2-STRUCTURAL-FOUNDATION-20260719
BOUND / IDENTITY:
  Canonical mode-8 owner, profile 1, frames 600..607, static residency,
  incremental wallpaper, hybrid OAM, Task 9 phase 2, and Task 16 1/1/1.
  Control ROM / ELF:
    C777D2D1CE323769706A51E913CE4A3014A05E6F23CF1F9080CEA9C64A1BA0A4
    25953C4345722EC1CC853C00A982AAA4455CD3517087D6715F9BBB7614E6AAF0
  Source owner remains BattleShip ftdisplaymain/ftdisplaylights. No retained
  production renderer or gameplay change.

PHASE 0 / CUT 21A:
  Current owner is 5,812 ARM main-RAM bytes with a 308-byte local frame;
  prepared-run workspace is 49*56=2,744 bytes inside the unchanged 8,800-byte
  owner BSS. Generated control remains 32 roots, 49 epochs, 67 runs, 626
  triangles, 1,878 corners, 52 u16 schedule entries, and 32 u8 binding indices.
  Preserve the prior 16/392 (4.08%) exact-hit census and no-cache verdict.

CUT 21B:
  A temporary 0xA5 poison run proves every consumed prepared-run field is
  assigned and keeps 0/49,152 changed pixels. Removing the 49 clears and dead
  vertex_flags store saves matrix 1,344/960 P50/P95 and 12 ITCM bytes, but
  regresses Mario +544/+576, Fox +608/+576, draw +640/+576, and active
  +576/+448. Loop remains 1,680,448. REVERT the entire runtime cut and layout.
  Three attempted A2 launches failed before measurement when melonDS did not
  open the GDB listener; they are excluded from evidence.

CUT 21C SAME-SLOT A/B:
  One compact consumer replaces 463 pointer comparisons with 32 checked direct
  binding indices. Candidate ROM:
    F7F78ECF833E30AA662C013812AF3372B07EFE036BFE05737FE6C9A3A8B6F02C
  Matrix improves 158,464/158,528 -> 157,024/157,632 (-1,440/-896), but
  complete draw regresses 1,002,496/1,005,824 -> 1,003,872/1,007,168
  (+1,376/+1,344), Mario +640/+576, Fox +768/+704, and active P95 +9,344.
  Loop is unchanged. Synchronized pixels are exactly 0/49,152 and all
  70/686 run/triangle, 320/306 ownership, fallback, fence, and conservation
  gates pass. The candidate adds 120 main text/rodata bytes. REVERT its API,
  adapter consumer, code, and symbols.

RETAINED FOUNDATION / NEXT:
  KEEP the already-valid compact root/epoch/run tables, u16 schedule, u8
  binding indices, source order/provenance, exact widths, and generated
  six-closure consumed-field/invalidation manifest. The checker proves full
  prepared-run assignment, immutable/live/camera/callback ownership, and
  32/49/67/626/1,878 cardinality. This is Task 27 input only, not a completed
  generated fighter and not permission to restore either rejected runtime cut.

CLOSEOUT VERIFICATION:
  Generation, hierarchy, packet, GBI, parity-corpus, registry, Task-9 float
  ITCM, and renderer-ITCM checks pass. Restored published ROMs are byte-identical
  to the 00:25 full-Boundary checkpoint: battle 757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4,
  public D06323485C866D74BA5D82F87B58182C82A3D7FBE5E9AAC08B83807583171A9E.
  Two independent no-build runs pass canonical lifecycle smoke and publication,
  then fail only when unattended melonDS exposes no capturable window. The
  already-retained synchronized A/B proves 0/49,152 changed pixels; exclude the
  duplicate host UI capture failure and do not request another repeat.

EVIDENCE:
  artifacts/performance/2026-07-19_task21r-m2-foundation.md
  docs/optimization/NDS_NATIVE_FIGHTER_CONSUMED_FIELDS.generated.json
  artifacts/performance/2026-07-19_task21r-21b-control.json
  artifacts/performance/2026-07-19_task21r-21b-poison.json
  artifacts/performance/2026-07-19_task21r-21b-candidate.json
  artifacts/performance/2026-07-19_task21r-21c-control.json
  artifacts/performance/2026-07-19_task21r-21c-candidate.json
KEEP / REWORK / REVERT: KEEP CENSUS + COMPACT MANIFEST / REVERT 21B + 21C RUNTIME / TASK 27 NEXT
```

## 2026-07-19 - Task 27 generated M2 Mario root/epoch program

```text
IDEA ID: TASK27-GENERATED-M2-MARIO-20260719
BOUND / IDENTITY:
  Source checkpoint c6b0d6695e22a6d11f6d6acf56b3da4d541cde42. Canonical
  mode-8 owner, profile 1, frames 600..607 / updates 1206..1220, static AOT,
  incremental wallpaper, hybrid OAM, Task 9 phase 2, and Task 16 1/1/1.
  Task 25R sets the Mario P95 ceiling at 171,520 ticks. Continue broad
  generation only for >=8,000 combined-fighter P50 ticks or a credible
  >=35,000 two-fighter projection.

PHASE A CERTIFICATE — KEEP:
  The hashed Mario O2R and Task-21R compact tables deterministically produce
  25 joints, 14 roots, 18 epochs, 30 runs, 21 raw + 9 cross runs, 320
  triangles, 960 corners, 48 root-prefix + 4 intra-root light commands, and 62
  immutable state effects. Source/table/event checksums are
  791C164E/D78DB920/BF8DCCFD. Roots 0..13, epochs 0..17, and runs 0..29 are
  exact source order. Rebuild determinism, scanner/packet exclusion, a table
  checksum mutation, and a rejected run-class mutation pass. Retain the
  generated manifest/checker; it has no production runtime footprint.

MARIO-ONLY RUNTIME A/B — REVERT:
  Candidate ROM/ELF:
    207588F9640B4312FCD0ED08223F8AF7C3327E268668B4E704564741349F82AC
    A6A8CCC927C4422974199C50108068373C6817F7D940B9B6AD8016210468DF2
  Complete current preflight and live material/light/texture preparation ran
  before GX; the generated program owned Mario only and Fox remained control.

## 2026-07-19 - BGM-stall falsifier setup (device A/B pending the owner)

```text
IDEA ID: BGM-STALL-FALSIFIER-20260719
GOAL: prove or clear synchronous ARM9 BGM refill (nds_audio_bgm.c:278 fread +
      :289 DC_FlushRange, in-frame via ndsAudioBackendUpdate at
      taskman_seam.c:4358 after the UPD counter closes) as the source of the
      retail 5-VBlank dips that read as ~12 FPS.
INSTRUMENT (committed, profile-1 only):
  - gNdsBattlePlayablePacingPresentIntervalBucket[6] populated at
    taskman_seam.c next to Min/Max; indices 2/3/4 + 5+ bucket.
  - HUD rows 21-22: VBI 2:n 3:n 4:n / 5+:n max:n BGM last/max [OFF]
  - gNdsAudioBgmRefillTicksLast/Max around ndsAudioBgmRefillHalf.
FALSIFIER (committed):
  - NDS_BGM_FALSIFIER_OFF flag (default 0, never in a published target).
    Under it ndsAudioBgmPlay skips open/read/play but keeps every BGM state
    word and counter; ndsAudioBgmUpdate short-circuits on sNdsAudioBgmFile ==
    NULL, so no refill/fread/flush runs.
A ROM (BGM on, profile-1 coarse-hwtri):
  builds/build/smash64ds-battle-playable-coarse-hwtri.nds
  14,686,208 bytes
  CD0F0F92A2552BE926C76DBC9D401E6EA38B8E5CE2244202D6A620E59D12E234
B ROM (BGM off, profile-1 bgm-off-hwtri):
  builds/build-bgm-off-hwtri/smash64ds-battle-playable-bgm-off-hwtri.nds
  14,686,208 bytes (same length; no data layout change)
  91953C0CC8CCAA49F01C011FAF5C4FBCA9F6077849365D5AFBE156A3730088DF
  Distinct hashes confirm the flag changed the binary.
VERDICT: PENDING the owner device run (same heavy-combat minute on each, photograph
  HUD rows 12-22).
  - Dips vanish under B AND refill-tick-max spikes correlate with 5-VBlank
    intervals under A => BGM I/O confirmed; then check Calico ARM7 blkdev,
    prefer block-aligned IMA ADPCM over ring growth.
  - Dips persist under B => BGM cleared; fold into affine re-plumb evidence.
```
  A one-time 106-event validator failed before GX and no post-GX fallback
  existed. ITCM remained 28,808/32,768 with 3,960 free.

  Against the retained same-slot Task-21R control, matrix improves
  158,464/158,528 -> 155,328/155,520 (-3,136/-3,008 P50/P95), but Mario
  regresses 169,792/171,392 -> 169,920/171,520 (+128/+128), Fox layout
  regresses +224/+192, and complete draw regresses
  1,002,496/1,005,824 -> 1,005,120/1,008,384 (+2,624/+2,560). DL is
  +704/+704 and whole-loop P95 is +64. Both sides retain exact 70/686,
  60/320/306, 29/0/0 fallback state/vertex/command, and zero conservation
  error. The candidate is 8,128 ticks short of the continuation threshold.
  Timing is decisive, so no pixel promotion run was spent.

DISPOSITION:
  Remove all selector, renderer, validation, and runtime executor code. Do not
  attempt Fox. Keep only the Phase-A provenance/source-order/state-effect
  manifest and falsifiers as a bounded architecture record. No emulator or
  retail repeat is requested.

CLOSEOUT VERIFICATION:
  Generation/check mode, eight hierarchy falsifiers, packet census, GBI,
  Task-9/16 float placement, renderer ITCM, canonical lifecycle smoke, and the
  two-ROM publication contract pass. The restored production ROMs are
  byte-identical to the already-green Task-21R Boundary pair: battle
  757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4 and
  public D06323485C866D74BA5D82F87B58182C82A3D7FBE5E9AAC08B83807583171A9E.
  The full wrapper then stops only because unattended melonDS again exposes no
  capturable top-level window. Treat that duplicate host UI failure as
  screenshot transport; binary identity proves the retained certificate has
  zero production and pixel effect, so do not request another capture or
  retail repeat.

EVIDENCE:
  artifacts/performance/2026-07-19_task27-generated-m2.md
  artifacts/performance/2026-07-19_task27-phasea-mario-bound.json
  artifacts/performance/2026-07-19_task27-mario-generated-candidate.json
  artifacts/performance/2026-07-19_task21r-21c-control.json
  docs/optimization/NDS_NATIVE_FIGHTER_CONSUMED_FIELDS.generated.json
KEEP / REWORK / REVERT: KEEP PHASE-A CERTIFICATE / REVERT MARIO RUNTIME / STOP BEFORE FOX / TASK 20R NEXT
```

## 2026-07-19 - Task 20R DTCM capacity and ownership closeout

```text
IDEA ID: TASK20R-DTCM-SCRATCH-STACK-20260719
BOUND / IDENTITY:
  Source start 0102a741d6faf8650396d9b101e4ecced2bc39df. Profile-0
  battle ROM / ELF:
    757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4
    5F77112C0487DFF4B053AAA4227279B2472D31F72F70D7CE7B57CF0617046639
  No production placement or Task-20 symbol exists.

CURRENT LAYOUT:
  .dtcm/.dtcm.bss are 0/152 bytes. The only owners are __irq_table at
  02FF0000+128 and __sched_state at 02FF0080+24. __dtcm_bss_end is 02FF0098;
  __sp_usr/__sp_irq/__sp_svc are 02FF3E80/02FF3F80/02FF3FC0. The shared
  data/user-stack gap is 15,848 bytes; IRQ/SVC/BIOS reserves are 256/64/64.
  The linker rejects static data past __sp_usr. A new post-link checker now
  rejects every unreviewed application DTCM owner and prints the exact bounds.

PHASE C STACK — STOP BEFORE IMPLEMENTATION:
  The unchanged gameplay allocation is 16,384 bytes. Capacity plus one
  64-byte guard needs 16,448 bytes, NO_FIT by 600 even with zero live user
  stack. Shrinking from an incomplete route is forbidden. The earlier
  provisional 13,044/3,700 high-waters replay as 16,808 guarded bytes,
  NO_FIT by 960, or 18,856 with two 1,024-byte margins, NO_FIT by 3,008.
  Three current lifecycle launches failed before ROM execution because the
  ARM9 GDB listener never opened; no result is inferred from transport.

PHASE B SCRATCH — NO PROMOTION:
  The only credible bounded object is the 2,048-byte, 32-matrix
  sNdsRendererAdapterNativeOwnerModelviews at 02217F80. It is completely
  overwritten and synchronously consumed by ARM9 each presentation, with no
  DMA/IPC/ARM7/audio/asynchronous-GX pointer escape. It is nevertheless a
  DTCM/cache-layout candidate and has no existing retail A/B. The user
  declined repeats, so emulator timing cannot promote it. No move was made.

DMA / POINTER CLOSURE:
  The gameplay stack pointer is retained only by PortCoroutine, OSThread, and
  the synchronous CPU scheduler. Renderer DMA reads texture staging, and ARM7
  audio reads the BGM ring/FGM pack; neither candidate reaches those paths.
  Since profile 0 has zero application DTCM objects, forbidden direct DMA
  references are zero by construction. Any future third owner fails closed.

VERIFICATION / EVIDENCE:
  Production and profile-1 lab post-link checks pass; the guarded evidence
  replay prints the exact NO_FIT margins. GBI fixtures pass with the new
  canonical-verifier hook. Boundary also passes registry, Task-9/16, renderer,
  and DTCM placement after rebuilding the production ROM byte-identically,
  then exits 1 in emulator launch transport without a new runtime artifact.
  Do not infer game behavior or spend another duplicate launch.
  artifacts/performance/2026-07-19_task20r-dtcm-audit.md
KEEP / REWORK / REVERT: KEEP CHECKER + CENSUS / NO SCRATCH MOVE / NO STACK MOVE / TASK 22R NEXT
```

## 2026-07-19 - Task 22R wallpaper writer final disposition

```text
IDEA ID: TASK22R-WALLPAPER-FINAL-DISPOSITION-20260719
IDENTITY / RETAINED SURFACE:
  The same-ROM profile-1 A/B remains ROM 285867B3... / ELF 8177EE12....
  The profile-2 oracle pair is B5E03AD6... / 85B282CE.... The post-revert
  census is 3556D08F... / 1CA31FDE.... Current production is the unchanged
  757ED786... / 5F77112C... pair and has zero Task-22/profile/oracle symbols.

PHASE A / B:
  Retain the neutral setup/X-map/Y-map/write/commit and maximal-run/store
  census. Across each eight-frame window, full+incremental rows close at
  1,536 and scalar+2*packed+DMA+copy equals final writes. Production already
  has a complete 16-field final key over source identity/generation/layout,
  overlay epoch, origin, scales, combine mode, mapping version, and map slot.
  Exact hits skip all map/write work; changed-camera source hits alternate the
  existing two exact X/Y map slots. Do not add a second axis cache.

PHASE C:
  The threshold-4 writer improves Countdown, early, and Whispy wallpaper P95
  by 14,592 / 27,136 / 14,528 ticks. Natural-KO wallpaper regresses
  366,784 -> 370,944 (+4,160) while its writer changes only
  316,160 -> 315,968 (-192). This exceeds the +2,000 gate and forces REVERT.
  Profile 2 recorded 136,192 map and 14,942,208 pixel checks with zero
  mismatch. Candidate writer, threshold, selector, scratch, and plumbing are
  absent. The current verifier exports/fails on the oracle row; the older
  profile-2 JSON binds identity but predates that JSON field.

PHASE D:
  NOT ADMISSIBLE. DMA testing requires a retained CPU writer. Retail is the
  performance referee, no device A/B exists, and the user declined repeats.
  Do not infer a crossover from melonDS or revive many-small-job DMA.

EVIDENCE / CLOSEOUT:
  artifacts/performance/2026-07-19_task22r-wallpaper-closeout.md
  The ten committed Task-22 JSONs and SHA-256 values in the reconciliation
  entry remain authoritative. Post-revert frame 445 conserves 10,570 scalar
  + 10,496 DMA = 21,066 final pixels across 192 rows. GBI fixtures pass and
  current production remains byte-identical.
KEEP / REWORK / REVERT: KEEP PRODUCTION KEY + CENSUS / REVERT WRITER / SKIP DMA / TASK 28 NEXT
```

## 2026-07-19 - Task 28 bounded ARMv5TE matrix leaf

```text
IDEA ID: TASK28-ARMV5TE-MATRIX-S32-20260719
BOUND / IDENTITY:
  Source d06c01497b673a679adb08b6ffd04e6271575880. Production ROM / ELF:
    757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4
    5F77112C0487DFF4B053AAA4227279B2472D31F72F70D7CE7B57CF0617046639
  Task-21R proves 52 scheduled joints plus 32 binding roots, so the leaf is
  called 84 times / 756 three-term dots per presentation. The existing matrix
  owner is 158,464/158,528/158,528 P50/P95/max ticks.

CURRENT -> CANDIDATE CODEGEN:
  Exact profile-1 control/candidate configs match: mode 163, profile 1, M2
  detailed 1, Task-26 segment 0, Task-16 1/1/1, renderer ARM. The linked leaf
  changes 256 -> 248 bytes, 52 -> 40 stack bytes, and 64 -> 62 static
  instructions. Its dot changes one SMULL + two SMLAL into one MUL + two MLA;
  both leaves have zero helper calls, divides, and veneers. ITCM/text.hot/BSS/
  DTCM are unchanged; the full candidate .main grows 40 bytes because of the
  finite-domain admission and pre-GX fallback.

ARITHMETIC PROOF:
  Admit every matrix element only in [-16384,16383]. Three signed products
  have magnitude <=805,306,368, so product, sum, negation, and the 2048
  rounding bias remain signed-32 exact and current s64 saturation cannot fire.
  Outside that partition the literal current owner remains the pre-GX
  fallback. A 100,000,000-vector deterministic run with seed 6D2B79F5 plus
  directed signed/range boundaries reported zero mismatches.

FAILED PROMOTION GATES:
  Two exact control attempts in runner slots 7 and 2 completed static checks
  but melonDS never listened on 127.0.0.1:4333, so no control JSON or candidate
  runtime was produced. There is no natural-owner/microbench timing, literal
  ARM9 golden corpus, state/matrix/geometry/material/audio rows, synchronized
  pixel proof, runtime reserve, or retail ARM/cache/layout evidence. The user
  declined retail repeats. No result is inferred from transport.

DISPOSITION / EVIDENCE:
  Restore the exact s64 leaf and direct call sites. Do not retain a checker for
  rejected code, open another arithmetic family, or claim a speedup from host
  proof/disassembly. Packed lighting already emits MUL + two MLA in current
  ITCM and was not reopened.
  artifacts/performance/2026-07-19_task28-armv5te-matrix.md
KEEP / REWORK / REVERT: REVERT MATRIX LEAF / NO RETAINED CODE / TASK 29 NEXT
```

## 2026-07-19 - Task 29 exact GX command/owner census

```text
IDEA ID: TASK29-EXACT-GX-CENSUS-20260719
BOUND / IDENTITY:
  Source 214051254a80eb9d18406d765d79f25f519a7dae. Canonical mode 163,
  fast mode 9, live Fox, static AOT, incremental wallpaper, and retained
  Task-26 segment 0. The census is profile-1/real-GX-only and performs no
  suppression. Census ROM / ELF:
    8CD572E096F33CABDB43EB720A204882C20EA6C248A9AC84C1DD14EC01BD4684
    DCB10C9C8D56056EF1F5F0A851EF976DD13FD898583692EEBE3424FE25FE9D39
  Compiled-out control ROM / ELF:
    C04071BDE55DF038705B29A950917C1E8F5F943EF5116C5151CEC7DFD2C4404F
    770672436F882B1EF3C4CEE405E5A4EB96E1DB140C92E297C365C6C5F1061685

PHASE A — KEEP DIAGNOSTICS:
  Every control, texture, matrix, geometry, and flush class is counted by
  exact logical command and parameter words, with owner partitions, equal-
  consecutive counts limited by owner/flush reset boundaries, dual stream
  hashes, boundary hashes, and a fail-closed never-suppress mask. Early
  frames 438..445 conserve 121 runs, 828 actual triangles, 202/320/306 fast
  ownership, 7,227 commands, 14,415 words, 1,976..1,991 equal observations,
  13 boundaries, and zero faults. Whispy conserves 844..856 actual triangles;
  KO conserves 524, including the dynamic stage/effect residuals instead of
  hiding them inside the fixed-owner total.

PHASE B — NOT ATTEMPTED:
  Control has 330/412 equal values and alpha test 28/36 in the early window,
  but GX/MMIO performance promotion requires a repeatable retail result and
  the user declined repeats. Texture parameter and polygon format are already
  protected by the retained exact Task-8 shadow and show zero safe repeats;
  matrix mode shows zero repeats; previously measured color/texcoord
  suppression regressed and remains reverted. No new state shadow exists.

PHASE C — STOP AT ENTRY GATE:
  Task-26 segment 0 is a retained generated representation rather than one
  immutable Task-29 GX span. Its five same-control phase pairs save only
  3,424..3,616 stage P50 ticks, below the 5,000 actual-run entry gate. The
  user's single retail observation saves 21,568 draw ticks but changes active
  +4,288 and is explicitly non-repeatable. No immutable command template,
  packet, copy, scanner, DMA feed, or new runtime footprint is admitted.

EXACTNESS / SAFETY:
  Instrumented versus compiled-out frame 445 is 0/49,152 changed pixels,
  mean delta 0.00, overlap 100%. The independent profile-2 oracle passes all
  828 semantic events and exact 202/320/306 provenance with zero overflow.
  All census windows retain zero fallback/fence faults and one flush; lab
  reserve is 174,864 bytes. Final profile-0 Boundary ROM / ELF are
  21D789F3439FB2223C7F0F4F097B5A2ABD9652F2BDE4A6648B1A6808C404EEC1 /
  89C83C403E59365BC938A2DF5745C506EE66F63DAB8AF772C93440EC5CF1C355;
  the ELF contains zero Task-29 symbols. Keep the owner-conservation verifier,
  runner-slot GDB-port isolation repair, and forensic timeout forwarding.

EVIDENCE:
  artifacts/performance/2026-07-19_task29-gx-census.md
  artifacts/performance/2026-07-19_task29-gx-census-early.json
  artifacts/performance/2026-07-19_task29-gx-census-whispy.json
  artifacts/performance/2026-07-19_task29-gx-census-ko.json
  artifacts/performance/2026-07-19_task29-control-early.json
KEEP / REWORK / REVERT: KEEP PHASE-A DIAGNOSTICS / NO PHASE-B SHADOW / NO PHASE-C STREAM / TASK 24R NEXT
```

## 2026-07-19 - Task 24R evidence-safe quiet-slot cleanup

```text
IDEA ID: TASK24R-QUIET-SLOT-DIET-20260719
SOURCE / RECOVERY:
  HEAD f2534ccaafb1abe3ae522bf7e88b006e3212feda.
  Pre-delete Lean snapshot:
    <user profile>\Desktop\Snapshots\Smash64DS_Port_Lean_20260719_053053.zip

DELETED BATCH:
  7,929 files / 3,746,285,595 bytes from 17 exact immediate children of
  builds/ or artifacts/.
  - 6,581 files / 699,406,817 bytes: 14 lab roots owned by CLOSED/REVERT
    Tasks 13, 20R, 21R, 22R, 28, and 29.
  - 1,348 files / 3,046,878,778 bytes: artifacts/verifier-cost,
    artifacts/verifier-temp, and artifacts/emulator-logs.
  No worktree, branch, ref, Git object, logs/ entry, permanent evidence,
  current generated-program A/B, hardware pair, canonical build, or user file
  was removed. No git gc was run.

PERMANENT EVIDENCE:
  Immediately before and after deletion:
    performance 329 files / 91,169,794 bytes / aggregate
      25B1BD9A9D17824E7C0FD35A6C032C7A14F0209084DE948B2AE4FE39B306DDAD
    visibility 1,577 files / 209,195,688 bytes / aggregate
      586ADF5B846D789678598DD1551461551C0BAB537B68DFFBC023554827B1122D
  The post-delete Boundary gate then added two normal dated captures. A new
  fail-closed checker uses each migration record's deduplicated destination,
  rejects path/size/hash drift and disagreement, and excludes only rolling
  latest.png/previous.png. It passes 1,814 records / 1,745 immutable
  destinations / two aliases / zero failures.

GIT SURFACES:
  16 registered worktrees remain: main plus all 15 prior holds. Three temp
  Task-16 worktrees are dirty; seven named worktree branches retain unique
  commits; four detached clean tips remain non-ancestors. All 24 branches are
  preserved. The five-effects attack/Fox/hit branches remain specifically for
  the required pre-Task-30 A/V reconciliation.

POST-DIET STATE / GATES:
  builds/: 38,255 files / 4,397,115,650 bytes.
  artifacts/: 1,950 files / 391,137,559 bytes after the dated capture pair.
  Parser, GBI/source fixtures, melonDS policy, migration-manifest replay, and
  final profile-0 Boundary all pass. Battle ROM/ELF remain
    21D789F3439FB2223C7F0F4F097B5A2ABD9652F2BDE4A6648B1A6808C404EEC1 /
    89C83C403E59365BC938A2DF5745C506EE66F63DAB8AF772C93440EC5CF1C355
  and public ROM remains D06323485C866D74BA5D82F87B58182C82A3D7FBE5E9AAC08B83807583171A9E.

EVIDENCE:
  artifacts/performance/2026-07-19_task24r-quiet-slot-cleanup.md
KEEP / REWORK / REVERT: KEEP CLEANUP + MANIFEST CHECKER / TASK 24R COMPLETE / A/V AUDIT NEXT
```

## 2026-07-19 - Task 30 stable-30 entry gate

```text
IDEA ID: TASK30-FINAL-STABLE30-PRECONDITION-20260719
SOURCE / CANDIDATE:
  Decision HEAD 45ae233a502. Current verifier-covered battle ROM:
    C344CA8B89903A35678A6DC4A849F217B690CBCE16841FAC47974626899EB9DF
  This ROM is not designated a Task-30 qualification artifact because no
  same-HEAD profile-1 sibling or three-run retail packet exists.

ENTRY PRECONDITION:
  FAIL. Task 25R's immutable same-artifact matrix has six of seven phase
  P95/maximum loop rows above the 1,120,380-tick two-VBlank deadline. Profile 0
  completes exact 4,084/2,042 update/presentation accounting and one teardown,
  but reaches only 18.6 presentations/s / 37.3 updates/s. Histogram 2/3/4/5+
  is 61/1547/396/38: 1,981 intervals are 3+ VBlanks and slips are 2,457.

POST-MATRIX ACCOUNTING:
  Retained Task 26 fixes historical KO-audio/texture-fence debts and saves
  3,424..3,616 stage P50 ticks. Its single device sample saves draw 21,568,
  changes active +4,288, and leaves loop near 2.24M ticks. Task 23R's ~12K
  exact cache left its histogram unchanged and was reverted. Tasks 20R/21R/
  22R/27/28/29 add no retained profile-0 speed mechanism beyond diagnostics or
  inert certificates.

STRICT CHECKER:
  stable30=False intervals3plus=1981 slips=2457 reserve=166672.
  The old KO-FGM/fence false flags belong to the immutable Task 25R packet and
  were repaired later; pacing independently fails.

TASK30 DISPOSITION:
  Run 0/3 retail lifecycles. Do not build a ceremonial final pair, average
  failed phases, weaken a checker, or claim stable-30. Freeze retained exact
  mechanisms and reverted selectors. Publishing may proceed only with honest
  observed retail performance of approximately 13.5-15 FPS in heavy combat.

EVIDENCE:
  artifacts/performance/2026-07-19_task30-stable30-precondition.md
KEEP / REWORK / REVERT: TASK 30 CLOSED AT PRECONDITION / STABLE-30 RED / PUBLISH P1 NEXT
```

## 2026-07-19 - BG-0 retained hardware-affine Dream Land wallpaper

```text
IDEA ID: BG0-FAST-WALLPAPER-AFFINE-20260719
SCOPE / IDENTITY:
  Dream Land VSBattle wallpaper only. Stage/fighter/GX/texture/audio/gameplay,
  BG3 foreground, and src/nds/nds_renderer.c are unchanged. Control and
  candidate are profile 1, mode 163, live Fox, renderer mode 9, generated stage
  segment 1, static texture mode 1, scene-mip lab 0, and identical synchronized
  windows. Candidate alone sets NDS_FAST_WALLPAPER_AFFINE=1.

MECHANISM:
  At first eligible presentation, save camera/wallpaper source state, derive
  the proven 14000.0 neutral wallpaper transform, restore source state exactly,
  prefill BG2 with opaque Dream Land sky, and invoke the existing final
  wallpaper compositor once. Retain that 256x256 B16 surface. Every later
  wallpaper SObj computes a clamped 8.8 BG2 affine tuple with 64-bit
  intermediates and quarter-pixel Q8 scroll quantization; VBlank commits only
  changed register tuples. Invalid live state retains the prior tuple. A seed
  failure admits STATIC_DEGRADED and never reopens recurring software work.

SYNCHRONIZED MELONDS A/B:
  Window                 wallpaper P50/P95/max A -> B       draw P50/P95 A -> B
  early 600..607         340672/363072/363072 -> 2016/2048/2048
                                                       1057184/1079808 -> 715744/715904
  countdown 438..445     286208/351424/351424 -> 2048/2048/2048
                                                       1144896/1213248 -> 857504/860736
  Early wallpaper saves 338,656 P50 / 361,024 P95 ticks; countdown saves
  284,160 P50. Candidate warning-rate sampling rises 21.4 -> 26.3 FPS in the
  early window. Whispy frames 1398..1405 retain about 1,984-2,048 wallpaper
  ticks. No hardware speed claim is made from these emulator rows.

ONE-TIME SEED / HARD FAIL-CLOSED CONTRACT:
  Representative profile-1 marker:
    state 2; attempts/success/failure/degraded 1/1/0/0;
    seed 4,184,640 ticks; affine last 1,600 ticks;
    post-ready software draw/pixel writes 0/0;
    seed hash 5F10E8BB; source-opaque 42,834; restore mismatch 0.
  The 4.18M seed is pre-gameplay one-time work and excluded from ready-state
  timing. The verifier requires exactly one successful seed, >=75% source
  opacity, nonzero hash, no restore mismatch, and zero recurring draws/writes.

VISUAL / LIFECYCLE:
  Candidate captures are recognizable Dream Land with full opaque coverage,
  stable movement, no repeating seam, and unchanged countdown/HUD/foreground/
  stage/fighter layering. Pixel equality is intentionally not a BG-0 gate.
  The strict profile-0 lifecycle passes 4,084 updates / 2,042 presentations,
  exactly two source updates per presentation, KO/rebirth, Time Up, Results,
  one teardown, exact KO FGM, zero post-GO texture-fence failures, and reserve
  166,672. It remains stable30=False: 19.6 presentations/s, 39.2 updates/s,
  2,137 slips, and VBlank histogram 2/3/4/5+ = 92/1780/153/17.
  Final-source strict-wrapper ROM F6132D93DAE9CD80914550EC952D7D523B8C453BD2737D98F569E25C2C46E92F
  repeats the complete lifecycle pass in 68 seconds on a clean runner slot.
  Its first launch timed out only because slot 6 still owned an earlier BG-0
  control process; exact executable/ROM inspection identified and stopped that
  orphan before the clean no-build rerun. Do not classify that transport event
  as a game hang, and verify a selected runner slot is idle before a long gate.

PUBLISHED / HARDWARE:
  smash64ds-battle-playable-hwtri.nds
    14,692,352 bytes
    BC236C610581A6361DE84677ED05878B05FF01A259F00736BE5D2D155171DE7D
  Published melonDS smoke: 28.1 presentations/s, 56.0 updates/s. Use the
  project planning rule of 0.75x throughput (about 21.1 presentations/s here),
  while treating retail DS as the performance referee.
  Hardware pair:
    control 849D5CD93E6C8F5D5F3F78C812DC622620449714D6653D9A70B73D0ECF0E6E47
    affine  A9F6C66169498AFC8B55B00994899A98E8A7D33A16148E7845D94587E2E54FC1

EVIDENCE:
  artifacts/performance/2026-07-19-bg0-{control,candidate}-early600-607.json
  artifacts/performance/2026-07-19-bg0-{control,candidate}-countdown438-445.json
  artifacts/performance/2026-07-19-bg0-profile0-lifecycle.json
  artifacts/visibility/2026-07-19-bg0-{control,candidate}-early600.png
  artifacts/visibility/2026-07-19-bg0-candidate-{countdown438,whispy1398}.png
  artifacts/visibility/2026-07-19-bg0-profile0-results.png
  builds/task-bg0-hardware-pair/README.md
KEEP / REWORK / REVERT: KEEP BG-0 PRODUCTION / RETAIL A/B PENDING / STABLE-30 STILL RED
```

## TICK-HUD BASELINE / FORK EMULATOR (2026-07-22)

First tick-HUD bucket capture taken over GDB instead of photographed off the
HUD, and the first measurement of any kind on the cache-modelling melonDS fork.
This row is the new emulator baseline for the shipping profile-0 configuration.

```text
IDENTITY:
  target                  smash64ds-battle-playable-tickhud-hwtri
  git                     093690b
  ROM                     11,430,912 bytes
                          20D25EE075A4B8B9C0CF0541A0842BCDCEF284647C0E17FBCBC2E9604407E022
  melonDS                 emulators/melonds/melonDS.exe (owner fork, cache-modelling)
                          DE80E46BDCF1FD986162DE6AFFD9EE1148F8C40565DBAF667B9C2B3EF5475715
  config                  profile 0, TICK_HUD=1, SHIP_TELEMETRY=0, HW compose 2,
                          Task 44 on, fast-run 9, static textures on
  command                 scripts/sample-tick-hud-buckets.ps1 -Samples 256
                            -StartFrame 438 -Build build-task41-tickhud-current
  sampling                256 presented iterations, frames 438..693, CPU Fox
  reproducibility         two independent runs byte-identical in every field
                          including min/max: this scene has NO run-to-run noise

BUCKETS (guest cpuGetTiming ticks per presented iteration).
P50/P95 are the decision basis per docs/VERIFYING.md; spread = p95/p50 names the
buckets whose mean is not usable. %ALLp50 is p50-relative and does not sum to 100
because percentiles do not add - the additive identity is on the means.

  bucket        p50        p95  spread       mean        min        max  %ALLp50
  ALL     1,680,448  2,800,512    1.67  1,879,473  1,679,424  3,361,280    100.0
  FTR       590,144    598,848    1.01    557,730    324,672  1,046,080     35.1
  STG       597,632    605,440    1.01    598,236    589,760    615,744     35.6
  BG          4,032      4,096    1.02      4,036      3,904      4,224      0.2
  AUD         1,344     65,216   48.52      6,542      1,152     68,544      0.1
  HUD         1,024    198,272  193.62     25,732        832    200,256      0.1
  SRC       324,096    955,904    2.95    378,464    162,688  1,293,568     19.3
  MISC       82,432    129,152    1.57     77,537     45,888    198,592      4.9
  OTHR      134,080    546,624    4.08    231,195     24,512    577,024      8.0

  Read the spread column first. FTR/STG/BG at 1.01-1.02 are steady enough that
  p50, p95 and mean agree; those three are the trustworthy numbers and the only
  ones comparable against a device running mean. SRC 2.95 and OTHR 4.08 are
  moderately bursty. AUD 48.52 and HUD 193.62 are pure burst - audio refill and
  text redraw fire on a minority of frames - so their means are an artefact of
  how many bursts a window happens to contain and must not be compared at all.

  mean named sum 1,648,277 (87.7% of mean ALL); ALL is measured wall ticks and
  OTHR is the remainder, so named + OTHR == ALL exactly on the means.
  ALL p50 1,680,448 == exactly 3 VBlank intervals at 33.51 MHz / 59.83 Hz.
  VBI 2:0 3:516 4:147 5+:30 max:19 over 693 presents; cadence slips 0.

FORK VS RETAIL (owner device measurement, same nine buckets).
The retail and ownerFork columns are HUD running means, so only buckets with
spread near 1.00 can be compared at all - there, mean and p50 coincide and the
comparison is meaningful. The rest are listed for completeness and marked.

  bucket    retail   ownerFork    p50 here   p50 vs R   mean vs R  comparable
  FTR      617,246     606,840     590,144      -4.4%       -9.6%  YES
  STG      616,721     597,698     597,632      -3.1%       -3.0%  YES
  BG         4,164       4,032       4,032      -3.2%       -3.1%  YES
  MISC      71,789      72,203      82,432     +14.8%       +8.0%  marginal
  ALL    2,020,898   1,841,744   1,680,448     -16.8%       -7.0%  no (mix)
  SRC      443,284     363,493     324,096     -26.9%      -14.6%  no (burst)
  OTHR     252,720     181,582     134,080     -46.9%       -8.5%  no (burst)
  AUD       10,172       7,110       1,344     -86.8%      -35.7%  no (burst)
  HUD        4,904       8,693       1,024     -79.1%     +424.7%  no (burst)

  The headline: on the three steady buckets the fork lands within 3.1-4.4% of
  retail. Stock melonDS for comparison was ALL 1,322,741, -34.5% vs retail.
  That is the result that makes the fork usable as a stand-in for the device.

  FTR is the correction the P50 view bought: its mean reads -9.6% only because
  a fighter is dead/respawning for part of this window (min 324,672 against a
  p50 of 590,144). Its p50 is -4.4% and its spread is 1.01, so per-frame fighter
  cost tracks retail far better than the mean suggested.

  ALL cannot be compared directly. It is a mix of 3-VBlank and 4-VBlank frames
  (516/147/30), so its p50 snaps to exactly 3 VBlanks while the device mean
  reflects a different 3/4/5+ ratio from a played match. Compare the VBI
  histogram for whole-loop questions, never ALL alone.

  AUD/HUD/SRC/OTHR differences are workload and burst-count, not emulator
  accuracy: this is a scripted CPU-Fox window, the retail column came from a
  continuously played match.

CONSEQUENCE FOR TASK 10 CALIBRATION:
  The Task 10 multipliers (whole draw x1.51, update x1.73, cache-resident x0.88,
  main-RAM streaming x1.50, ...) exist to correct stock melonDS for unmodelled
  cache behaviour. On this emulator STG is already within 3% of retail, so
  applying x1.51 on top would overstate a stage saving by roughly 50%. Treat
  those multipliers as stale for any measurement taken on the fork. They have
  not been re-derived; that is a decision for the owner, not an assumption to
  make silently.

EVIDENCE:
  artifacts/performance/tick-hud-buckets-fork-20260722-run1.json  (n=32)
  artifacts/performance/tick-hud-buckets-fork-20260722-run2.json  (n=256)
  artifacts/performance/tick-hud-buckets-fork-20260722-run3.json  (n=256, repeat)
KEEP / REWORK / REVERT: BASELINE RECORDED / TASK 10 MULTIPLIERS STALE
```
## TASK 37 HOT-CODE PLACEMENT REPACK (2026-07-22)

```
IDENTITY:
  branch codex/task37-itcm-repack, git bfb72d9
  melonDS DE80E46BDCF1FD98 (repo fork, cache-accurate), JIT off, DS console mode
  target smash64ds-battle-playable-tickhud-hwtri, profile 0
  control  ROM 8919BE714709A8C9   candidate ROM 84A34592D3676E0B
  matched window frames 438..637, 200 samples each, slips=0 both

WHAT WAS MEASURED, AND WHY IT IS A NEW KIND OF MEASUREMENT:
  The fork's ARM9 per-PC profiler was armed over 128 settled battle frames by
  CP15 markers the ROM emits at frame 438 and 566 (NDS_TASK37_PROFILE=1).
  500,810,896 cycles, 168,894,530 instructions, 60,709 PCs, 0.00% unattributed.

  Every PC was then classified by opcode as memory or non-memory. Placement
  changes what an instruction FETCH costs and does nothing for what a LOAD
  costs, so only non-memory stall -- cycles beyond one per instruction on
  instructions that touch no data -- is recoverable by moving code. Ranking by
  raw cycles would have picked memset, memcpy and memcmp (38.9M cycles, 7.8%)
  as the top targets; ranking by recoverable stall shows 34.8M of that is data
  traffic no placement can touch.

  armWaitForIrq is excluded from tier statistics. It is 8 bytes of deliberate
  VBlank spinning worth 9.21% of all cycles, and including it makes the
  zero-waitstate tier look like the worst-stalling one.

TIER RESULTS (the finding that outranks the repack itself):
  tier              cycles      insns  cyc/insn  non-mem stall %   mem stall %
  .itcm         88,250,502  52,480,774    1.68            14.7          25.8
  .text.hot     15,851,755   4,805,435    3.30            30.0          39.7
  .text.hot.draw 65,601,182 24,985,828    2.63            22.4          39.5
  .main        284,956,329  86,619,628    3.29            29.5          40.1

  ITCM works: half the non-memory stall rate of ordinary code.
  .text.hot.draw works: 22.4% vs 29.5%. Task 32 earned its retail KEEP.
  .text.hot DOES NOT WORK: 30.0% against .main's 29.5%, and a marginally worse
  cycles-per-instruction. The Task 17 update-path tier is buying nothing
  measurable, and its 3,716 free bytes are free because they are not worth much.

  Probable discriminator: re-entry frequency, not address grouping.
  .text.hot.draw holds functions called thousands of times inside one frame,
  where grouping compounds; .text.hot holds update functions called once per
  frame, cold on arrival regardless of their neighbours. Any future plan resting
  on "group the hot functions together" should be checked against this first.

  Also corrected: .itcm is NOT ~12.6 KB unenumerated. 64 residents cover 31,628
  of 31,676 bytes; 48 bytes are unnamed. And 5,040 bytes of it never executed
  once in 128 frames -- an eviction budget worth ~26.3M non-memory stall cycles,
  deliberately left unspent here (dead in this scene's steady state is not dead,
  and several are pinned by the renderer and Task 9/16 float checkers).

WHAT SHIPPED: NDS_TASK37_ITCM_LEAVES=1
  Seven measured toppers moved from .main into ITCM's free space. Placement
  only: library members are byte-identical objects from SHA-pinned archives with
  the section renamed, port functions carry a section attribute with no ISA or
  optimization change. No eviction, no verifier contract edited.
  906 bytes carrying 7,387,317 non-memory stall cycles.
  .itcm 31,676 -> 32,596 of a 32,736 hard cap; .main -800 B.

RESULT (P50/P95, never means -- HUD spread is 310x):
  bucket   ctl P50     cand P50      d P50       %      d P95
  ALL    1,680,192   1,680,192          0    0.00   -559,680
  FTR      591,936     576,640    -15,296   -2.58    -26,624
  STG      610,560     570,240    -40,320   -6.60    -40,512
  SRC      326,080     322,432     -3,648   -1.12    -10,496
  MISC      47,808      47,680       -128   -0.27     -2,688
  AUD        2,240       2,176        -64   -2.86       -192
  BG         4,096       4,224       +128   +3.12        +64
  HUD        1,024       1,024          0    0.00    -11,200
  OTHR     126,784     165,696    +38,912  +30.69    -83,776

  Named work P50 -59,328 ticks. OTHR rising is the correct signature: ALL is
  VBlank-quantized and did not move, so work removed from named buckets
  reappears as pacing slack inside the same 3-VBlank envelope.

  VBI share, normalized by sample count (n=637 each), never min-FPS:
                3-VBI  4-VBI  5+-VBI
    control      71.7   23.1    5.2
    candidate    76.0   20.9    3.1

  +4.3 points into the 3-VBlank bucket; 5+ frames cut from 5.2% to 3.1%.
  ALL P95 fell 559,680 -- one whole VBlank interval (560,190), so the 95th
  percentile frame moved from a 5-VBlank frame to a 4-VBlank one.

GATE STATUS, STATED HONESTLY:
  The plan's success gate was "FTR + SRC P50 down >= 40,000 combined". They fell
  18,944. THE GATE AS WRITTEN IS NOT MET. It named the wrong buckets: the census
  showed the recoverable stall sits in the renderer/stage path, and that is
  where the win landed. STG -- which the Task 37 plan itself called "exhausted,
  no variance left, hard median floor" -- gave up 40,320 ticks to pure code
  placement. The gate predates the measurement that replaced it.

EVIDENCE:
  artifacts/task37-census/census.txt        (full census, three ranked tables)
  artifacts/task37-census/census.json
  artifacts/task37-census/arm9-profile.csv  (per-PC, 60,709 rows)
  artifacts/task37-census/ab-control.json
  artifacts/task37-census/ab-candidate.json
  artifacts/task37-census/ab-report.txt
EXACTNESS GATE: FAILED -- THIS IS NOT A KEEP
  verify-task37-itcm-state-hash-ab.ps1 requires every per-update game-state
  record to match across the placement change. 692 of 3,892 differ (17.8%),
  first at update 1412, in three bursts separated by runs of identical records
  (the last run is 1,357 consecutive identical updates).

  Evidence that this is probably the instrument and not a real divergence:
  the hash covers syUtilsRandSeed() and a divergent RNG stream cannot
  re-converge, yet it re-converges three times; heap offset (57,168) and GObj
  count (646) are identical in all 3,892 records; and two fighters at different
  positions do not return to bit-identical whole-state. The likely source is a
  GObj field touched by the draw path rather than by logic -- and Task 44, also
  a perf change, deliberately proved exactness with the Task 36 replay word
  stream instead of this gate.

  Probably is not the standard for changing a published ROM, and moving the gate
  after seeing its result would make it worthless. NDS_TASK37_ITCM_LEAVES stays
  0 in every target; the published ROM is unchanged at 9E27BD3D...; nothing
  merged. The enabled build would have been 1818AA77..., 11,428,864 bytes.

  TO SETTLE IT: the instrument already tags inputs by region
  (SCENE/BATTLE/CAMERA/GROUND/CONTROLLERS/COLLISION/GObjs). Export a per-region
  hash instead of one combined pair and the diverging region names itself in one
  run. If it is GObj draw state, exclude the draw-touched fields and this gate
  becomes usable for every future perf task. If it is a gameplay region, Task 37
  is a real bug and stays reverted.
INVESTIGATION (all runs on the fast-logic match-lifecycle gate, mask 0 vs 7):
  same ROM twice                            IDENTICAL  gate is deterministic
  mask 1 / 2 / 4 individually               FAIL 692   byte-identical signature
  BGM falsified                             FAIL 692   not audio
  gSYControllerDevices excluded             FAIL 692   not ARM7 input phase
  realtime pacing                           BLOCKED    hits the pre-existing
                                                       locked-30 pacing red
  +800B dead padding in .main               PASS 3892  layout is invisible
  +800B dead padding in .itcm               PASS 3892  section growth invisible

  Padding changes layout but not speed; relocation changes speed. Every failing
  arm changes speed, every passing arm does not -- and three disjoint symbol
  groups produce ONE byte-identical failure signature, which no per-symbol fault
  explains.

REGION BISECT (NDS_TASK9_STATE_HASH_REGION_MASK, added for this):
  core: scalars/RNG, scene, battle, camera, ground,
        controllers, collision                        PASS 3892 identical
  object tree                                         FAIL 692
    gameplay objects                                  FAIL 692
      fighter/item/weapon/effect                      FAIL 692
        fighter FTStruct alone                        FAIL 692

  The core PASS is the load-bearing result. syUtilsRandSeed() is in it: both
  builds draw the same random numbers in the same order for all 3,892 updates,
  and agree on battle state, camera, ground and collision throughout. The
  simulation is not running a different match. The camera in particular tracks
  fighter positions and is identical on every update, which is hard to reconcile
  with fighter positions differing.

STOPPED at the standing time-box (~twelve full match lifecycles). FTStruct is
hashed as one blob so region masks cannot resolve further. Next step is
mechanical, not another guess: split FTStruct across the free mask bits 23..31
and binary-search the differing offset in three runs. If it lands on a code
pointer, note that ndsTask9StateCanonicalWord collapses main-RAM addresses to
0x20000000 and ITCM to 0x30000000 -- a pointer whose target moved to ITCM
changes class with no behavioural change, which would explain the identical
signature across disjoint groups. If it lands on a gameplay member, Task 37 is a
real defect.
KEEP / REWORK / REVERT: WIP -- measured win real, exactness unresolved, NOT shipped
```

## TASK 45 — the Task 37 exactness gate was measuring relocation (2026-07-22)

Method: dump raw `FTStruct` bytes for both fighters at updates 1411 (last
identical record) and 1412 (first differing), baseline `LEAVES=0` vs candidate,
and diff. Two runs instead of the planned ~6-run region-mask bisect, and it
yields exact offsets rather than a 376-byte window.

```
215 differing words, both fighters, both updates
  ALL are main-RAM heap pointers
  ALL differ by exactly +0x180 (384 bytes)
  ZERO non-pointer differences
delta histogram: one bucket, 384 x215
```

Members hit: next, fighter_gobj, coll_data, input, damage_colls[0..10],
joints[0..25], motion_scripts, computer, attack_colls. Every one a pointer. The
image shrinks when the leaves leave `.main`, so every heap object below it
relocates. The fighters are in bit-identical logical state.

Exports corroborate: on all 692 differing records `bytes` (57,168), `records`
(646) and `overflow` (0) are identical between builds. Hash-only differs, so the
traversal walked identical structure.

FALSIFIED, both mine:
  ITCM boundary crossing        no ITCM-range values exist in the diff
  .ptr vs .end class flip       changed nothing at all -- same 692, same 1412
                                (reverted, 60ce8eb)

STILL OPEN: why exactly 692 of 3,892 in bursts that heal, when the delta is one
constant present at both sampled updates. Next step is a measurement, not a
guess: dump `gSYTaskmanGeneralHeap.start/.ptr/.end` alongside the snapshot so
canonical values compute offline. Key question: does `start` shift by the same
+0x180 as the pointers?

SHIPPED ANYWAY on the owner's decision, gate still red. Published ROM
`9E27BD3D..37CE369` -> `1818AA77..FDF54207`; `.itcm` 31,676 -> 32,596 of 32,736.

Two silent Makefile defects surfaced shipping it: `LIBC/LIBM/PORT` derived with
`:=` before the per-target overrides (first "shipped" build was byte-identical;
the device A/B pair would have built control == candidate), and the device pair
literal `1` left over from when `LEAVES` was a boolean rather than a bitmask,
silently narrowing seven leaves to three. Both fixed.

## Task 49 — `NDS_BATTLE_PROFILE` axis + GX equivalence differ (2026-07-23)

```text
IDEA ID: TASK49-GX-DIFFER-20260723
DECISION: KEEP CANDIDATE; DIFFER PROVEN ON BOTH CONTROLS; READY TO JUDGE PROFILE-0 (TASKS 51/52)
TARGET: smash64ds-battle-playable-task49-differ-hwtri (dedicated lab block)
OWNER CAPTURED: STAGE (owner 0), one owner per run
WINDOW: 438..445 (8 frames); dump at the last presented frame, per-frame reset
STREAM/FRAME (STAGE): 2,229 entries; 2,996 words; 8 bindings (MATRIX_LOAD4X4)
OVERFLOW/FAULT: 0 / 0 in every capture
PUBLISHED ROM (default off): 1818AA775DCFFD52C82B35ED3D4FA6C6D02FCE232E9EE70D9B3F1DA3FDF54207
```

This task ships no rendering change. Part 1 is the `NDS_BATTLE_PROFILE` axis
(additive; `=0` fails the build closed until Task 51 lands; `=1` is today's
shipping path / correctness oracle). Part 2 is the GX equivalence differ:
default-off capture instrument + host analyzer reporting Tier 1 (non-matrix,
bit-exact, zero tolerance) and Tier 2 (matrix effective transform, screen-space
pixels) separately.

**Positive control:** profile-1 vs profile-1, same ROM, two runs — Tier 1
2860/2860 words matched, 0 divergences; Tier 2 8 bindings, max 0.0 px. The
capture is deterministic (the control Task 45 flagged as the one that should
have run first and did not).

**Negative control:** one VERTEX16 word perturbed → Tier 1 FAIL, named (cls19,
entry 9, word_index 0). One LOAD4X4 matrix word +1 LSB → Tier 2
max_screen_px = 0.0312 (binding 0; non-zero). A differ that has never reported
a divergence is not known to work; this one has reported both.

**Tier 2 threshold: 1.0 screen-space pixel per binding** (half a DS pixel on
the 256x192 framebuffer; below it is sub-pixel/imperceptible; the 1-LSB
perturbation measures 0.03 px = 32x under, so a real defect clears it).

**No-op:** published ROM byte-identical `1818AA77...` (clean builds, master and
this branch). The `60C68AFF` tick-HUD reference is unreproducible from clean
master today (47 bytes of header relocation, same class Task 45 documented);
the honest no-op test is master-vs-mine in matched fresh dirs, both
`C24867BA...`, byte-identical.

Full certificate: `artifacts/performance/2026-07-23_task49-gx-differ.md`.
Spec + results: `docs/optimization/ClaudeFable5_Task49_BattleProfileAxisAndGxDiffer_20260723.md`.

## Task 51 — Dream Land native stage (2026-07-23)

```text
IDEA ID: TASK51-STAGE-NATIVE
DECISION: KILL — STG budget not met; the targeted bindings don't draw
ROM (published, default-off): 1818AA77..FDF54207 (byte-identical to master)
ROM (tickhud A native off):    B07E384F7BC42C70..
ROM (tickhud B native on):     24B7A6E98E237988..
WINDOW: profile 0 tickhud, mode 9, frames 438+64 samples, NDS_TASK51_STAGE_NATIVE 0 vs 1
STG P50 A (off): 569,216  P95: 574,208
STG P50 B (on):  587,968  P95: 595,008   (target <=120,000; kill >200,000)
DIFFER STAGE owner 438-445: Tier 1 = 0/2860 words, Tier 2 = 0.0 px (ZERO_DEVIATION)
```

STG P50 587,968 ≫ the 200,000 kill line; the native path is ~18,752 ticks
*worse*. Root cause: the differ (frames 438–445, 600–607, 1200–1207) shows only
8 bindings draw (indices 0–7, all `layer0`, all rigid) and MATRIX_MULT4x3 = 0 —
the Task 51 path never fired. The 16 non-rigid bindings Task 51 targets (20–29,
33–38 — `map0–3`, `layer1`) submit no GX commands; they are economy-skipped stage
elements (`gNdsRendererEconomySkippedRunCount`, nds_renderer.c:21159). Their
world matrices are constant (Task 48) but constant-and-undrawn costs zero either
way, so there is no STG cost for Task 51 to recover. The campaign's STG 597,632
is owned by the 8 rigid `layer0` bindings that Task 36 already targets.

The differ proves the path is mechanically correct (ZERO_DEVIATION) and the
matrix-math foundation is bit-exact — but the prize the campaign assumed (the 16
non-rigid bindings) is not a real cost in this scene. Default-off byte-identity
holds (`1818AA77…`). Branch `codex/task51-dreamland-native` is the checkpoint;
nothing merges. Full evidence:
`artifacts/performance/2026-07-23_task51-stage-native.md`.

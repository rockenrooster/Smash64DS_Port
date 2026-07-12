# Smash64DS Performance Ledger

Do not delete reverted or inconclusive rows. Measurements use synchronized
canonical mode 163 unless a row explicitly says otherwise. Nested renderer
timers are diagnostic subdivisions and are never added to the whole-loop sum.

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
  GBI fixtures passed. ITCM placement passed under Windows PowerShell 5.1.
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

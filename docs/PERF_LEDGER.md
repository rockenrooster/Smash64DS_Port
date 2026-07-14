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
  difference was approved by Tyler.
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

# Task 30 final stable-30 qualification — precondition decision

Date: 2026-07-19

Decision: **STOP / STABLE-30 NOT QUALIFIED.** Task 30's entry precondition is
false, so no final profile-0/profile-1 qualification pair is designated and no
three-cold-boot retail claim is made. This closes the current Task 20R–30 queue
honestly; it does not declare the stable-30 release contract complete.

## Identity and evidence boundary

The decision was made at source HEAD
`45ae233a502` (`audio: qualify attack and hit assets`). The current integrated
user-facing ROM is 14,688,256 bytes, SHA-256
`C344CA8B89903A35678A6DC4A849F217B690CBCE16841FAC47974626899EB9DF`.
That ROM remains a verifier-covered candidate, but it is not called a Task-30
qualification ROM: there is no same-HEAD diagnostic sibling and no three-run
retail packet.

The authoritative Task 25R same-artifact pair remains:

```text
source HEAD     f088db98de272e9788405c2181029ad4a4c353ba
profile-1 ROM   6E90D4140E6332E8F37BB05CB8B35ED192AAB448B26E110916992F2C15701921
profile-0 ROM   E685C034D301D3C6881D398B14820D0D60A112FA259657A6B05408C90683C5CE
deadline        1,120,380 ARM9 ticks / two-VBlank presentation
```

Later retained work is accounted for separately. Task 26's generated stage
segment 0 saves 3,424–3,616 stage P50 ticks and 3,104–3,680 draw ticks across
its same-control emulator phases. Its one user-supplied retail observation
saves 21,568 draw ticks, changes active +4,288, and leaves loop in the same
roughly 2.24-million-tick VBlank bucket. Task 23R's approximately 12K active/
draw cache did not change the interval histogram and was reverted for missing
retail/working-set proof. Tasks 20R, 21R runtime cuts, 22R writer, 27 executor,
28 leaf, and 29 state/template add no retained profile-0 speed mechanism.

## Failed entry precondition

Task 30 requires the current matrix to predict sufficient P95 and maximum
margin in every phase before asking for retail qualification. Task 25R reports:

| Phase | Loop P50 / P95 / maximum | Two-VBlank result |
|---|---:|---|
| Countdown / GO | 1,680,448 / 1,680,512 / 1,680,512 | fail |
| Early combat | 1,680,448 / 1,680,448 / 1,680,448 | fail |
| Whispy | 1,680,448 / 2,240,640 / 2,240,640 | fail |
| Late combat | 1,120,256 / 1,120,320 / 1,120,320 | pass by only 60 ticks |
| Natural KO | 1,120,256 / 1,680,512 / 1,680,512 | fail |
| Natural rebirth | 1,680,448 / 2,240,640 / 2,240,640 | fail |
| Time Up / Results | 1,120,256 / 1,680,448 / 1,680,448 | fail |

Six of seven phases miss the deadline at P95/maximum. The complete profile-0
lifecycle is:

```text
source updates / presentations / teardown  4,084 / 2,042 / 1
presentations/s / source updates/s          18.6 / 37.3
interval histogram 2 / 3 / 4 / 5+          61 / 1,547 / 396 / 38
intervals >=3 VBlanks / slip events         1,981 / 2,457
reserve / floor                              166,672 / 131,072 PASS
synchronized pixels                          0 / 49,152 changed
```

The existing strict checker was re-run against its immutable Task 25R packet
and produced:

```text
Task 25R matrix report produced: phases=7 profile1=6E90D4140E6332E8F37BB05CB8B35ED192AAB448B26E110916992F2C15701921 profile0=E685C034D301D3C6881D398B14820D0D60A112FA259657A6B05408C90683C5CE pixels=0/49152 exact=False stable30=False intervals3plus=1981 slips=2457 reserve=166672 recommendation=M3-first
Task 25R stable-30 gate failed: 3+ intervals=1981 slips=2457 profile1-exact=False reserve=True KO-FGM=False fence=False.
```

The final two flags in that historical packet were repaired by retained Task
26 work: the natural `439/292/154` KO audio route and all ten post-GO texture-
fence counters now pass. They are not rewritten into the immutable Task 25R
artifact. The pacing failure is independent and remains decisive.

## Task 30 hard gates

| Gate | Evidence | Decision |
|---|---|---|
| Exactly two unchanged updates/presentation | 4,084 / 2,042 | pass |
| No debt/catch-up/skipped/interpolated state | retained scheduler contract | pass |
| Zero intervals of 3+ VBlanks | 1,981 | **fail** |
| Zero pacing-slip events | 2,457 | **fail** |
| Approximately 30 presentations/s / 60 updates/s | 18.6 / 37.3 | **fail** |
| Every phase P50/P95/max within deadline | six of seven P95/max rows fail | **fail** |
| Current state/audio/lifecycle/geometry/material/texture/depth automation | Task 26 lifecycle and current Boundary green | pass as automation, not final retail sign-off |
| Synchronized native pixels | 0 / 49,152 in Task 25R and retained exact cuts | pass as existing A/B evidence |
| Reserve >=128 KiB | 166,672 | pass |
| Three cold retail lifecycles and device safety packet | 0 / 3; user unavailable and declined repeats | not run |

No failed phase is averaged away and no checker threshold is weakened.

## Retained and rejected mechanisms

Retain the exact Task 16 compare/i2f/add-sub leaves, Task 17's 11-function
update hot-text placement, Task 26 segment-0 generated stage program, exact
static texture residency, exact completed-channel audio-owner retirement,
Task 21/27 inert generated certificates, Task 20/23/29 fail-closed diagnostics,
current incremental wallpaper, existing GX shadows, and the exact attack/hit
A/V additions.

Keep reverted or absent: Task 20 DTCM placement/stack shrink, Task 21 runtime
clear/pointer cuts, Task 22 threshold writer/DMA phase, Task 23 residual cache,
Task 27 runtime fighter executor/expansion, Task 28 arithmetic leaf, Task 29
state suppression/immutable stream, whole-owner packets, generic scanners/VMs,
approximate lighting, affine wallpaper, visual skipping/LOD, and all stale lab
selectors.

## Freeze and next work

- The current ROM may be published only with honest observed performance: real
  hardware heavy combat is approximately 13.5–15 FPS in the available samples.
- It must not be described as stable-30 qualified. “Exactly two source updates
  per presentation” describes semantics, not achieved 30 FPS throughput.
- No lab/profile-1 ROM, oracle selector, cache, DTCM candidate, or reverted
  renderer executor may replace the canonical profile-0 configuration.
- Reopening stable-30 requires a materially different exact design, a refreshed
  same-artifact phase matrix, and three cold retail lifecycles. It is not part
  of the publish-input/provenance lane.

Disposition: **TASK 30 CLOSED AT PRECONDITION / RELEASE STABLE-30 GATE RED /
PUBLISH LANE MAY PROCEED WITH PERFORMANCE HONESTY.**

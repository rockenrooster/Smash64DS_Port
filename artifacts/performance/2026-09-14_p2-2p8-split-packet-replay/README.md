# P2-2p8 split-root fighter packet replay

Date: 2026-09-14

Verdict: **KEEP. Fighter renderer clears the 1.12M-tick sub-budget; overall
four-CPU performance remains RED.**

The production fighter packet already had a large retained replay path, but the
standard Donkey/Samus/Link/Kirby stress roster never hit it: CPU-composed native
roots deliberately use the split projection/modelview loader, and packet capture
faulted every time it met one. The control records 1,347 packet attempts and zero
hits; a focused census showed all 1,347 attempted recordings faulted before a
packet became valid.

The retained fix completes the existing packet representation for those native
split roots. `gx_valid` is already part of the packet shape key, so split roots
reuse the fixed root patch table without growing packet BSS:

- `local_index[0]` patches that root's `GL_PROJECTION` `LOAD4x4`;
- `seed_index` patches its `GL_MODELVIEW` `LOAD4x4`;
- the modelview patch applies the same BattleShip-world -> GX row-3 scaling as
  `ndsRendererLoadHardwareSplitMatrices`;
- the record frame still executes the proven direct split loader, while the
  recorded packet is self-contained and does not depend on pre-DMA matrix state.

The owner overlay already supplied the matching replay-side interpretation in
`ndsFighterPacketTryReplay`; this checkpoint adds the missing store/record tee and
routes production split roots through it instead of deliberately faulting the
packet.

## Synchronized frames 600..607

Standard four-CPU Dream Land stress roster, same collector and frame window as
the immediately preceding P2-2p8 control.

| Metric | Control | Candidate | Delta |
|---|---:|---:|---:|
| FTR P50 | 1,143,680 | **638,848** | **-504,832** |
| FTR P95 | 1,158,720 | **656,256** | **-502,464** |
| WORK-H P50 | 2,191,680 | **1,698,048** | **-493,632** |
| WORK-H P95 | 2,713,856 | **2,216,576** | **-497,280** |
| Packet hits | 0 | **1,326** | +1,326 |
| Packet records | 1,347 | **21** | -1,326 |
| Packet faults | 1,347 focused control census | **0** | -1,347 |
| Packet declines | 462 focused control census | **462** | unchanged |
| Packet max words | — | 2,813 / 8,840 | bounded |
| Native failures | 0 | 0 | pass |
| Native direct rejects | 0 | 0 | pass |

Control ROM SHA-256:
`880C3841486AAAD8C655B5CB2A62EAF5F104E34A888BC74FAC90952334A38840`.

Candidate ROM SHA-256:
`AB473BC8705657B8A4B43FF5D017C68A53F2DC15D880560E3CA5A4755B1BD504`.

## One-minute four-distinct-CPU stress

The configuration-exact `verify-p2-four-fighter-stress.ps1` child was run from
frames 2..1973 (1,972 timing samples), with source clock 60 -> 1: 59 of the
60-second match, Donkey/Samus/Link/Kirby, item overrides zero.

| Bucket | Previous UV-only checkpoint | Split-packet candidate | Delta |
|---|---:|---:|---:|
| FTR P50 | 1,144,768 | **640,000** | **-504,768** |
| FTR P95 | 1,218,880 | **781,056** | **-437,824** |
| WORK-H P50 | 2,381,312 | **1,907,200** | **-474,112** |
| WORK-H P95 | 3,488,576 | **3,013,056** | **-475,520** |

The fighter renderer is now comfortably below the 1.12M-tick sub-budget even at
P95. The match as a whole is still above the 30 FPS product budget, so P2-2p8
does not close here.

Same-run correctness/resource evidence remains healthy: native failures/direct
rejects 0/0; all four fighter slots emit hardware triangles (`0xF`); observed
roster is Donkey/Samus/Link/Kirby; general-heap low-water 118,752 B (93,152 B
above the 25,600 B floor); libc runtime high-water 32,960 B; weapon pool 2/10
with zero refusals; graphics-heap overflow/no-room 0/0; pose bind-full 0;
animation arena 5,216 B reserved / 4,224 B used with 620 fills, 14 hits and 240
recycles; animation stream failures 0; DamageSlash roots 0/1 both engage with no
submit/snapshot/texture failures.

The focused production-manifest and full native-owner geometry-closure checks
also pass before the stress run.

## Boundary umbrella note

`verify-all.ps1 -Profile Boundary` reaches the architecture preflight and then
fails on the pre-existing owner workspace condition `?? decomp/alt_assets/`.
That directory is read-only project input and was not deleted, staged or changed
to force a green umbrella. The configuration-exact four-CPU Boundary child above
was therefore run directly to qualify this battle-renderer change. The umbrella
remains locally red for that unrelated worktree audit, not for a renderer/native
runtime failure.

## Permanent files

- `control.json` / `control.csv`: synchronized pre-fix frames 600..607.
- `candidate.json` / `candidate.csv`: synchronized retained packet candidate.
- `candidate-stress.json` / `candidate-stress.csv`: full four-CPU timing run.
- `candidate-stress-coverage.json`: same-run source identity / match-clock proof.
- `candidate-stress-memory.json`: same-run resource/native-coverage ledger.

# Task 24R evidence-safe quiet-slot cleanup

Date: 2026-07-19
Source HEAD: `f2534ccaafb1abe3ae522bf7e88b006e3212feda`
Recovery snapshot:
`C:\Users\Tyler\Desktop\Snapshots\Smash64DS_Port_Lean_20260719_053053.zip`

## Scope decision

This was a quiet slot after Tasks 20R-23R and 25R-29 closed at their current
gates. Drive D had 646,630,379,520 free bytes, so cleanup was bounded by proof,
not urgency.

All 15 previously held worktrees remain held. Three temporary Task-16
worktrees are dirty. The other 12 are clean, but seven named branches still
have patch-unique commits and four detached tips are not ancestors of master.
No worktree, branch, ref, Git object, user file, or `logs/` entry was removed.

## Exact deletion batch

The batch removed 7,929 files / 3,746,285,595 bytes from 17 exact paths.
Every path was an immediate child of the resolved `builds/` or `artifacts/`
root and is recoverable from the pre-delete Lean snapshot.

Closed/reverted lab builds: 6,581 files / 699,406,817 bytes.

- `builds/build-task13-fighter-lod-lab`
- `builds/task13-lod`
- `builds/task13-lod2`
- `builds/task13-lod3`
- `builds/build-task20-dtcm-control`
- `builds/build-task20-reconcile`
- `builds/build-task20r-lifecycle`
- `builds/build-task21-phase0-lab`
- `builds/task21-renderer-census`
- `builds/task21r-21c-slot5-pair`
- `builds/build-task22-wallpaper-profile1-lab`
- `builds/build-task22-wallpaper-profile2-lab`
- `builds/build-task28-matrix-candidate`
- `builds/build-task29-gx-census-lab`

Rotated verifier/emulator telemetry: 1,348 files / 3,046,878,778 bytes.

- `artifacts/verifier-cost`
- `artifacts/verifier-temp`
- `artifacts/emulator-logs`

After the safety gate, `builds/` contains 38,255 files / 4,397,115,650 bytes.
`artifacts/` contains 1,950 files / 391,137,559 bytes. The verifier recreated
empty `verifier-temp` and `emulator-logs` directories; `verifier-cost` remains
absent.

## Permanent evidence and active keeps

Immediately before and after deletion, the protected roots were byte-identical:

| Root | Files | Bytes | Sorted path/length/SHA-256 digest |
|---|---:|---:|---|
| `artifacts/performance` | 329 | 91,169,794 | `25B1BD9A9D17824E7C0FD35A6C032C7A14F0209084DE948B2AE4FE39B306DDAD` |
| `artifacts/visibility` | 1,577 | 209,195,688 | `586ADF5B846D789678598DD1551461551C0BAB537B68DFFBC023554827B1122D` |

The post-delete Boundary gate then added two new dated visibility captures;
that expected growth is outside the deletion comparison. The new
`scripts/check-task24-evidence-manifest.ps1` replays the migration manifest
using each record's actual deduplicated destination, rejects path escape,
size/hash drift, and record disagreement, and excludes only the intentional
rolling `latest.png` / `previous.png` aliases. It passes 1,814 records, 1,745
immutable destinations, two mutable aliases, and zero failures.

The following active/current roots were explicitly checked and retained:

- `artifacts/performance` and `artifacts/visibility`;
- `builds/task19-hardware-hud-pair` and
  `builds/task26-hardware-hud-pair`;
- Task-26 control/candidate, segment-0 profile-1, and current generated A/B
  build roots;
- `builds/build-task27-mario-compile`;
- `builds/build-battle-playable-canonical-hwtri-harness`;
- both published ROMs;
- the user-owned untracked publish task and Task-21R disassembly log.

## Preserved Git surfaces

All 16 registered worktrees (main plus 15 held) remain. Held paths are:

- `.tura/control-task8-cut-e`;
- three `%TEMP%/smash64ds-task16-*` worktrees;
- `../Smash64DS_Port-worktrees/{attack,fox,hit}`;
- `../Smash64DS_Port-wt-{audio,gameplay,soak}`;
- `../Smash64DS_Port_task14_verifier`;
- `../Smash64DS_Port_task16_{fadd_fsub,i2f,i2f_c9}`;
- `../Smash64DS_Port_task17_census`.

All 24 branches remain:

`codex/countdown-reference-v2`, `codex/five-effects-attack`,
`codex/five-effects-audio-union`, `codex/five-effects-fox`,
`codex/five-effects-hit`, `codex/five-effects-integration`,
`codex/five-effects-mario`, `codex/five-effects-visual`,
`codex/m4-aot-generator`, `codex/p1-audio-ids`, `codex/p1-fireball`,
`codex/p1-five-minute-soak`, `codex/renderer-parity-contract`,
`codex/task10-hardware-calibration`, `codex/task11-renderer-economy`,
`codex/task14-verifier`, `codex/task16-fmul-audit`,
`codex/task9-phase2-proof`, `codex/wip-hw-visibility-gates`,
`codex/wip-natural-combat-source-start-collision`,
`codex/wip-renderer-fidelity-2026-07-09`,
`codex/wip-renderer-sampler-boundary-20260710`, `master`, and
`wip/ftmain-import`.

## Safety gates

- PowerShell parser: pass for the cleanup/evidence checker and affected
  verification wrappers.
- Task-24 migration manifest: 1,745 immutable destinations, zero failures.
- GBI/source fixtures: pass.
- melonDS policy: pass.
- final profile-0 Boundary: pass, including canonical rebuild/runtime,
  publication, Task-9/16 placement, renderer ITCM, Task-20 DTCM, registry, and
  visual capture.
- final battle ROM/ELF:
  `21D789F3439FB2223C7F0F4F097B5A2ABD9652F2BDE4A6648B1A6808C404EEC1` /
  `89C83C403E59365BC938A2DF5745C506EE66F63DAB8AF772C93440EC5CF1C355`.
- public ROM:
  `D06323485C866D74BA5D82F87B58182C82A3D7FBE5E9AAC08B83807583171A9E`.

Task 24R is complete. No further cleanup is justified before release work.

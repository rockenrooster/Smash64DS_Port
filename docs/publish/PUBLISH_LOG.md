# Smash64DS publication log — TASK P4

Date: 2026-07-19

State: **LIVE** — TASK P4 publication complete.

## Publication preflight

The P3 gates were rechecked at dev HEAD
`ad4dafd18bec6cb21c55845e9a351a687a8929a3` before any cleanup:

- staging HEAD `0199d8f665da1c319dddd7e66364b651d5f63f27`, branch
  `master`, one commit, clean status;
- 275 files / 8,391,073 bytes outside `.git`;
- zero forbidden filenames, trees, embedded-content files, large binaries, ROM
  magic hits, or forbidden string hits;
- clean-room clone at the same staging HEAD with clean status and no temporary
  decomp junction; and
- clean-room ROM 14,688,256 bytes, SHA-256
  `C344CA8B89903A35678A6DC4A849F217B690CBCE16841FAC47974626899EB9DF`.

Result: `LEAK_AUDIT=PASS`; clean-room identity gate PASS.

## Full recovery bundle

The bundle was created and verified before any worktree or branch deletion.

| Property | Value |
|---|---|
| Path | `D:\Stuff\DevFolder\_backups\smash64ds-full-20260718.bundle` |
| Bytes | 45,485,731 |
| SHA-256 | `435000D2503DE30457BA2A35DA1C9AA230C7BA372835B936E1413B8630B96593` |
| Dev HEAD recorded | `ad4dafd18bec6cb21c55845e9a351a687a8929a3` |
| Refs verified | 42, including all 24 local branches and every registered worktree HEAD |
| `git bundle verify` | PASS: complete history, SHA-1 object format |

This bundle recovers committed source history and the clean detached/branch tips.
Ignored build outputs are not Git objects. Before worktree removal, all permanent
artifact evidence was independently checked against the Task-24 migration
manifest as described below.

## Worktree audit before removal

Sixteen worktrees were registered: the main worktree plus 15 auxiliaries. Each
was checked with `git status --porcelain`. The existing Task-24 evidence checker
then passed 1,814 records, 1,745 immutable destinations, two rolling aliases,
and zero failures. A second source-path audit found every one of the 50
`artifacts/performance` or `artifacts/visibility` files in the clean auxiliary
worktrees represented in that manifest; uncovered evidence files: zero.

The following clean worktrees were evidence-cleared and removed. Their
ignored payload consists of rebuildable emulator/build/ROM outputs plus the
already hash-migrated evidence. Counts were captured before removal.

| Worktree | HEAD | Ignored files | Ignored bytes | Migrated evidence files |
|---|---|---:|---:|---:|
| `Smash64DS_Port-worktrees/attack` | `e56003c2adabcaae2a5cf95a05843403a91c1fdb` | 1,192 | 779,708,942 | 13 |
| `Smash64DS_Port-worktrees/fox` | `636737c555fb0a0920f63845434cf705444c5cec` | 1,178 | 691,839,336 | 3 |
| `Smash64DS_Port-worktrees/hit` | `85d0ac30202b5220fdbb5aa41e236e640fc614b7` | 1,189 | 813,144,654 | 7 |
| `Smash64DS_Port-wt-audio` | `4891bacb0083a84ab9e7ae66cb2e65c4ac10a217` | 2,258 | 442,752,533 | 0 |
| `Smash64DS_Port-wt-gameplay` | `534f032bc19bb422342ac05567f96e5b2bf09d65` | 1,680 | 157,319,612 | 0 |
| `Smash64DS_Port-wt-soak` | `54379201b245ca3d17fdc013dc742d571a54401c` | 1,685 | 169,765,698 | 10 |
| `Smash64DS_Port_task14_verifier` | `dbf5898286799ccc9dda7a3863c18e4784bd5883` | 0 | 0 | 0 |
| `Smash64DS_Port_task16_fadd_fsub` | `18668b7d6ccf766c4a294c560cc61ad01c19c2ce` | 2,994 | 300,921,735 | 4 |
| `Smash64DS_Port_task16_i2f` | `d079a18e8a87ca801784026e5abde83810526774` | 1,248 | 159,498,586 | 2 |
| `Smash64DS_Port_task16_i2f_c9` | `4be5752d93e6798a7e5f6141163f208ec1d8ea41` | 1,223 | 680,719,869 | 2 |
| `Smash64DS_Port_task17_census` | `910662d28cfc99c38c4e35c199410a3b4811cccd` | 1,815 | 375,100,414 | 9 |
| **Total** |  | **16,462** | **4,570,771,379** | **50** |

These four dirty auxiliary worktrees are kept. No file in them is reset, cleaned,
or overwritten:

- `.tura/control-task8-cut-e` — special Tyler-tool worktree; untracked
  `.tura-control-runtime.log`;
- `%TEMP%/smash64ds-task16-bisect-51fc-5e0a55200438495f9604859939edd894`
  — one modified verifier script;
- `%TEMP%/smash64ds-task16-statehash-b175cffb1c29460db0b919896223da55`
  — three modified files plus one untracked assembly leaf; and
- `%TEMP%/smash64ds-task16-statehash-c9a-a73b9b169db34003914e4a329b69c7bc`
  — two modified files plus one untracked assembly leaf.

The main worktree's pre-existing user changes and untracked files are also kept.

After removal and `git worktree prune`, five worktrees remain registered:
`master`, `.tura/control-task8-cut-e`, and the three dirty `%TEMP%` Task-16
worktrees listed above.

## Local branches

The verified bundle contains all 24 local branch tips. The five worktrees left
registered are `master` plus four detached dirty/special worktrees, so no
non-master branch is pinned by a kept worktree. `master` remains at
`ad4dafd18bec6cb21c55845e9a351a687a8929a3`; these 23 branches are cleared for
deletion only after the bundle verification above:

| Branch | Tip |
|---|---|
| `codex/countdown-reference-v2` | `f9f0deda5a4d40908ea116e469c27a5338edb97b` |
| `codex/five-effects-attack` | `e56003c2adabcaae2a5cf95a05843403a91c1fdb` |
| `codex/five-effects-audio-union` | `cb7b70c89579cb2abb4f109a6f965983faaf36aa` |
| `codex/five-effects-fox` | `636737c555fb0a0920f63845434cf705444c5cec` |
| `codex/five-effects-hit` | `85d0ac30202b5220fdbb5aa41e236e640fc614b7` |
| `codex/five-effects-integration` | `7cd139af32f2fe14999890f3c18b05264f0ecdfc` |
| `codex/five-effects-mario` | `c76d0d1022f23f4cd835b796d436460b1ce33f18` |
| `codex/five-effects-visual` | `4d23dedf6ca339756a20d87fe52bda52176b4d29` |
| `codex/m4-aot-generator` | `601f2be4c378d51305c2ed69ced7f5fec72dbae4` |
| `codex/p1-audio-ids` | `4891bacb0083a84ab9e7ae66cb2e65c4ac10a217` |
| `codex/p1-fireball` | `534f032bc19bb422342ac05567f96e5b2bf09d65` |
| `codex/p1-five-minute-soak` | `1edb09dc27842e486225a1ec71ab95d769e67415` |
| `codex/renderer-parity-contract` | `54379201b245ca3d17fdc013dc742d571a54401c` |
| `codex/task10-hardware-calibration` | `db9d25a13760102f74ac6821c62b7ea67d412f6a` |
| `codex/task11-renderer-economy` | `d8671f6d1b93c67b32c13f315b559cfc752ae37b` |
| `codex/task14-verifier` | `dbf5898286799ccc9dda7a3863c18e4784bd5883` |
| `codex/task16-fmul-audit` | `445f5fc556530a720c90cf10292910323351b90c` |
| `codex/task9-phase2-proof` | `7c7e38e42772a3c6bf9e2ea4fc445c927d188e74` |
| `codex/wip-hw-visibility-gates` | `cc340aa5305b13e59e357652a841549db0b5011e` |
| `codex/wip-natural-combat-source-start-collision` | `6595add431cd48d33340281c592e98ae75800f25` |
| `codex/wip-renderer-fidelity-2026-07-09` | `24b95d43f35eb0984de6f4d2cc8d15eabe528d65` |
| `codex/wip-renderer-sampler-boundary-20260710` | `98b4851bf3c6459dcbb3abe99038d7195746917a` |
| `wip/ftmain-import` | `8273c5c9dd87967971a0fcddabfcccdaf078c00e` |

All 23 listed branches were then deleted. Local branch inventory is exactly one
branch: `master` at `ad4dafd18bec6cb21c55845e9a351a687a8929a3`.

One optional GC was run as
`git -c gc.reflogExpire=never -c gc.reflogExpireUnreachable=never gc
--prune=now`; no reflog expiration was permitted. `.git` changed from
3,611,206,095 bytes / 9,243 files to 46,442,900 bytes / 91 files, saving
3,564,763,195 bytes. Object storage is one 43.50 MiB pack with zero loose or
garbage objects; stdout and stderr are empty.

## Existing public README archive

`gh` 2.95.0 is authenticated as `rockenrooster`. Immediately before push, the
public repository was verified as:

- URL `https://github.com/rockenrooster/Smash64DS_Port`;
- visibility `PUBLIC` (not changed by this task);
- description `Source-faithful Nintendo DS port of Super Smash Bros. 64.`;
- default branch and only branch `main`, tip
  `0d76ca9b761b91470979c3f2cae997b943bed595`; and
- sole root entry `README.md`, 1,480 bytes, Git blob
  `b887da7226343f63176eafdab482d1c3189d3547`.

The exact pre-publication README text is archived below before `main` is
removed:

````markdown
# Smash 64 DS Port

Nintendo DS source port of the BattleShip Super Smash Bros. 64 decompilation:

```text
Original BattleShip game code + Nintendo DS backend = playable port
```

`decomp/` is read-only. Original runtime imports live under `src/import`; DS
backends and compatibility seams live under `src/nds`, `src/port`, and `include`.

## P1

The active target is a verifier-covered Mario-versus-level-3-Fox match on Dream
Land, one-minute Time mode, items off, due **2026-07-19 23:59 America/Chicago**.
Canonical `battle_playable_realtime`, mode `163`, is the natural-runtime boundary.

Current artifact identity, blockers, gates, and ownership live only in
[`docs/P1_EXECUTION_BOARD.md`](docs/P1_EXECUTION_BOARD.md). Profile-1 and forensic
ROMs are laboratory evidence and never replace the two published root ROMs.

Presentation targets roughly 90% overall likeness under DS limits. After one
measured cosmetic attempt, keep the cheapest recognizable source-derived result;
gameplay semantics stay source-faithful. Screenshots belong under
`artifacts/visibility`.

## Work

Run repository commands with PowerShell 7 (`pwsh`). Windows PowerShell 5.1 is
not supported.

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Use [`docs/HANDOFF.md`](docs/HANDOFF.md) for the exact restart command and
[`docs/VERIFYING.md`](docs/VERIFYING.md) for the focused A/B verification loop.
The documentation index is [`docs/README.md`](docs/README.md).
````

## Publication result

The repository is live at
`https://github.com/rockenrooster/Smash64DS_Port`.

| Property | Final value |
|---|---|
| Visibility | `PUBLIC`, unchanged |
| Default branch | `master` |
| Remote branches | one: `master` |
| Public commit | `0199d8f665da1c319dddd7e66364b651d5f63f27` |
| Commit count | one |
| Git tree | `dfbd7c1d06a92a5bb243fea28b8acca596d5fdc4` |
| Public blobs | 275 |
| Git blob bytes | 8,388,412 |
| Forbidden public paths | 0 |
| Description | `Nintendo DS source port of Super Smash Bros. 64 via the BattleShip decomp.` |

The remote tree SHA exactly matches the local staging tree. The recursive
GitHub tree response is complete rather than truncated. The public branch's
path scan finds no ROM/archive/output filename, `BattleShip_o2r`, `relocData`,
or derived audio/renderer tree. The pre-push byte-level audit remains the
authoritative content scan.

The archived README-only `main` branch was deleted only after `master` was
pushed and made default. The staging repository is clean and tracks
`origin/master`. The development repository still has no remote; it was never
pushed. Repository visibility was not changed.

## Tyler review checklist

- Open the [GitHub file listing](https://github.com/rockenrooster/Smash64DS_Port)
  and eyeball it against the leak-audit exclusions: no ROM, `.nds`, O2R,
  extracted assets, decomp tree, logs, artifacts, prompts, or internal agent
  files.
- Read [README.md](https://github.com/rockenrooster/Smash64DS_Port/blob/master/README.md)
  and [NOTICE.md](https://github.com/rockenrooster/Smash64DS_Port/blob/master/NOTICE.md)
  as a first-time builder, including the honest 13.5–15 FPS device statement,
  exact ROM requirement, and no-assets policy.
- Decide the port's own license before adding a `LICENSE` file. V1 intentionally
  publishes no project-wide license; upstream notices and the decomp-source
  caveat are in `NOTICE.md`.

Recovery surface: `D:\Stuff\DevFolder\_backups\smash64ds-full-20260718.bundle`
(`435000D2503DE30457BA2A35DA1C9AA230C7BA372835B936E1413B8630B96593`).

# GitHub Publish Lane — TASK P1–P4 (2026-07-18 22:00)

Goal: publish this port to GitHub as a MINIMAL public-ready repo that a stranger can build
into `smash64ds-battle-playable-hwtri.nds` with ONE PowerShell script, providing their own
legally-obtained ROM. Locally, reduce to master-only. Clean-room verification happens in
`D:\Stuff\DevFolder\Smash64DS_Port_MinimalGithub\`.

Relay order: **P1 → P2 → P3 → P4, strictly serial, exclusive lane** (pause the R-lane; one-writer
rule). P1 is read/trace only. P2 is the heavy one (asset regeneration mechanics). P3+P4 close it.

## Hard invariants (every task, non-negotiable)

- NEVER commit, stage, export, or push: N64 ROMs (`*.z64/*.n64/*.v64`, `baserom*`), built `*.nds`,
  `BattleShip_o2r/` or any `.o2r`, `relocData`, extracted textures/audio, save/state files, or any
  file whose BYTES derive from ROM content. When in doubt, exclude and note it.
- The public repo gets a **FRESH git history** (single initial commit from an allowlist export).
  The dev repo's 2.6GB history is never pushed anywhere.
- The target repo **already exists and is PUBLIC** (`rockenrooster/Smash64DS_Port`) — the moment
  P4 pushes, content is live. Therefore the P3 leak audit + clean-room gate are the publication
  gates and MUST be green before any push. Codex never changes repo visibility; if the owner wants a
  staged review he flips it private himself before relaying P4.
- Export is an explicit **ALLOWLIST** (manifest-driven copy). Never "everything minus ignores" —
  the dev repo tracks 533 `logs/` and 275 `artifacts/` files that must not ride along.
- `decomp/` stays read-only for SOURCE files. Writing torch/extraction OUTPUTS into the decomp
  checkout (as upstream's own build does) is permitted.
- The two root ROMs (`smash64ds.nds`, `smash64ds-battle-playable-hwtri.nds`) remain local-only
  publish artifacts per AGENTS.md; they are already gitignored — keep it that way in the public
  `.gitignore` too.

## Decisions for the owner (defaults so codex is not blocked)

1. **Repo (CONFIRMED by the owner 2026-07-18)**: `https://github.com/rockenrooster/Smash64DS_Port`.
   It exists, is PUBLIC, and contains only a 2-commit README stub on branch `main`
   (verified via gh 2026-07-18: single file README.md 1,480 B; commits e8db67a45 "Initial
   commit" + 0d76ca9b7 README expansion). the owner's requirement: the GitHub repo contains ONLY
   `master` — so `main` gets replaced per P4 step 4.
2. **Visibility**: already public — pushing IS publishing. P3's gates are the review. the owner may
   optionally flip the repo private before relaying P4 if he wants to inspect the pushed tree
   first; codex never touches visibility either way.
3. **License**: v1 ships NO LICENSE file; `NOTICE.md` states provenance (BattleShip decomp,
   sm64-nds architecture reference, libultraship/torch, devkitPro) and the "you must own the
   game; no Nintendo assets distributed" policy. the owner decides a license at flip-public review.
   Codex must read `decomp/BattleShip-main/LICENSE` and mirror any attribution it requires.
4. **Decomp delivery**: default = the build script FETCHES BattleShip upstream at a pinned commit
   (public repo stays tiny). Fallback if upstream is unreachable or diverged: vendor a filtered
   source-only subtree. P1 produces the evidence; if divergence from upstream is found, STOP and
   report before choosing.

## Known facts (verified 2026-07-18, cite-don't-rediscover)

- Tracked files: 1,295 total. By top dir: logs 533, artifacts 275, src 170, scripts 149,
  include 97, docs 28, assets 11, prompts 10, .agents 8, emulators 3, linker 2, plus root files
  (Makefile, AGENTS.md, README.md, architect.md, worktree_report.md, .github, .zcode,
  .gitattributes, .gitignore).
- `decomp/` is fully gitignored (`.gitignore:17`) — BattleShip-main and sm64-nds are NOT tracked.
- ROM-derived data currently tracked = exactly the 11 `assets/` files: 4 BGM pcm16 `.raw`+`.json`
  pairs, `fgm_phase_pack_ima.bin/.json`, `renderer/battle_playable_static_textures.rgb5a1.bin`
  (~5.8MB total). Generators exist: `scripts/render-audio-bgm-pupupu.py` (+ siblings),
  `scripts/generate_battle_playable_static_textures.py`, with checkers
  `check-audio-bgm-derived-assets.ps1` / `check-battle-playable-static-textures.ps1`.
- Build consumes from the untracked decomp side: `BATTLESHIP_O2R :=
  decomp/BattleShip-main/BattleShip_o2r` (extracted dir, 2,159 files / 22MB; Makefile:225,
  nitrofs rule :1048) and `BATTLESHIP_RELOCDATA :=
  decomp/BattleShip-main/decomp/assets/us/relocData` (Makefile:226, rule :1052).
- BattleShip upstream flow (its BUILDING.md): `baserom.us.z64` at ITS repo root; CMake build
  auto-extracts assets from the ROM via the `torch` submodule; there is an `ExtractAssets`
  target and a standalone torch sidecar; needs CMake + VS Build Tools on Windows. LICENSE and
  README.md exist at its root.
- Shipping target: `TARGET=smash64ds-battle-playable-hwtri` (Makefile:29 published targets, :90
  block, output basename :134). Reference identity: 14,669,824 bytes, SHA-256 `DADB7C96…` (full
  hash on `docs/P1_EXECUTION_BOARD.md`).
- GitHub target: `rockenrooster/Smash64DS_Port` exists, PUBLIC, README-only stub on `main`;
  gh CLI is authenticated as rockenrooster with repo scope (verified 2026-07-18).
- Git state: NO remote, NO tags, 24 local branches (master + 22 `codex/*` + `wip/ftmain-import`),
  16 extra worktrees (3 in `%TEMP%` task16 scratch, one INSIDE the repo at
  `.tura/control-task8-cut-e`, `Smash64DS_Port-worktrees/{attack,fox,hit}`,
  `-wt-{audio,gameplay,soak}`, `_task14_verifier`, `_task16_*` ×3, `_task17_census`).

---

## TASK P1 — Publish audit: build-input closure + provenance manifest

```
/task Publish audit for the GitHub release: determine EXACTLY what is needed to build
smash64ds-battle-playable-hwtri.nds, and classify every file's provenance. Read/trace
only — no source changes; new files go under docs/publish/ only.

1. Record the exact shipping build invocation for TARGET=smash64ds-battle-playable-hwtri
   (Makefile:29/:90/:134 area) and the reference identity: re-hash the root
   smash64ds-battle-playable-hwtri.nds, record FULL SHA-256 + size, cross-check against
   docs/P1_EXECUTION_BOARD.md (expect 14,669,824 B, DADB7C96…).

2. Compute the build-input closure: run one clean detached build of the shipping target
   (Start-Process → log → poll completion stamp; never foreground a long build), then union:
   (a) every source/header from the compiler .d dependency files in the build dir (add -MMD in
   a scratch config if not already emitted); (b) every Makefile prerequisite (nitrofs reloc/
   relocdata/audio/texture rules :849-:877, :997, :1048-:1064); (c) every script the build
   invokes, plus every file THOSE scripts read (audit each script's inputs); (d) linker
   scripts, config.yml-style inputs, generated-header inputs.

3. Classify EVERY closure file into exactly one bucket, with evidence:
   PORT-CODE (the owner's tracked src/include/linker/Makefile/scripts — ships),
   DECOMP-SOURCE (under decomp/BattleShip-main — arrives via pinned fetch, does not ship),
   ROM-DERIVED (bytes derive from ROM content — never ships; must be regenerated),
   GENERATED-METADATA (generated from decomp/port ANALYSIS, values are offsets/flags/indices,
   not content — ships; name its generator),
   TOOL (emulators, runners — excluded, stranger does not need them).
   Expected ROM-DERIVED set: the 11 tracked assets/ files + BattleShip_o2r dir + relocData
   dir; flag anything beyond that loudly.

4. Embedded-data audit: scan every tracked source in the closure for large literal data
   (>64KiB of numeric initializers or byte arrays) — at minimum
   src/nds/nds_native_stage_owner.generated.inc, src/nds/battle_playable_static_textures.c,
   and the lit-shade LUT in nds_renderer.c. For each, classify CONTENT (vertex coords,
   texels, samples — ROM-derived, must move to build-time generation in P2) vs METADATA
   (offsets, ownership bits, liveness flags — ships). Cite the generator script for each.

5. Regeneration coverage: for each ROM-DERIVED file, name the existing script/tool that
   regenerates it from a user ROM (render-audio-*.py family,
   generate_battle_playable_static_textures.py, BattleShip torch extraction for
   BattleShip_o2r + relocData) and TRACE ITS INPUTS (does it read the extracted O2R, the
   decomp tree, an emulator capture?). Produce a GAP LIST of files with no scripted
   regeneration path — these become P2 work items. Determine specifically: (a) the exact
   torch/CMake command sequence that produces BattleShip.o2r from baserom.us.z64 and how
   BattleShip_o2r/ (the extracted dir) is derived from it; (b) whether relocData is emitted
   by that same extraction or another step; (c) the canonical expected baserom hash (cite
   BattleShip's config/torch yamls — never invent it).

6. Upstream identification: from decomp/BattleShip-main/README.md, record the upstream
   GitHub URL; determine the best-matching upstream commit for the vendored copy and diff
   (source files only — ignore build outputs, BattleShip_o2r, baserom). Report IDENTICAL @
   <sha> or the divergence list. Also verify sm64-nds contributes ZERO files to the closure
   (reference-only) and confirm no closure file needs melonDS/emulators.

7. Deliverables: docs/publish/PUBLISH_MANIFEST.md (human summary: closure counts per bucket,
   gap list, upstream verdict, embedded-data verdicts, reference identity with full hash,
   exact build invocation, prereq list with versions used) + docs/publish/publish_manifest.json
   (machine-readable allowlist: every shipping file path + bucket — P3 consumes this).
   Commit those two files only. Snapshot as final command.

Constraints: decomp/ read-only (writing extraction outputs into it is allowed, editing
source is not). No changes to src/, Makefile, or scripts/ in this task. Timebox any single
rat-hole to ~1h, then record it as a P2 work item and move on. Subagents stay OFF.
```

## TASK P2 — One-script stranger build: `build.ps1` (ROM in → verified .nds out)

```
/task Implement the single-script stranger build: a fresh clone + prereqs + a legally
obtained ROM must produce smash64ds-battle-playable-hwtri.nds with one command. Consume
docs/publish/PUBLISH_MANIFEST.md from TASK P1 as ground truth.

1. Author build.ps1 at repo root. UX contract:
     .\build.ps1 -Rom "C:\path\to\baserom.us.z64" [-Jobs N] [-Clean] [-DecompPath <dir>]
   Steps it performs, in order, each with a clear progress line and actionable failure text:
   (a) Prereq check: DEVKITPRO/DEVKITARM env + arm-none-eabi-gcc, make, python3, git, and
       (only if torch must be built) cmake + MSVC Build Tools. Print found versions; on a
       miss, print the install pointer (devkitPro pacman nds-dev, etc.) and stop.
   (b) ROM validation: size + hash against the canonical expected baserom hash cited in the
       P1 manifest (cite BattleShip config — never invent). Wrong hash = refuse, naming the
       expected dump. Detect byteswapped .v64/.n64 and either auto-normalize or refuse with
       an explanatory message.
   (c) Decomp acquisition: if decomp/BattleShip-main is absent, git-clone upstream at the
       PINNED commit from the P1 manifest (record the pin in a checked-in file, e.g.
       DECOMP_PIN.txt) including required submodules (libultraship/torch per its
       .gitmodules); -DecompPath reuses an existing checkout instead (used by the owner and the
       clean-room rehearsals to skip re-downloads). If P1 found divergence, STOP — the owner
       decision pending.
   (d) Asset extraction: stage the ROM where BattleShip expects it, run the torch/
       ExtractAssets sequence from the P1 manifest to produce BattleShip.o2r + relocData,
       and materialize the extracted BattleShip_o2r/ directory layout the port Makefile
       consumes (Makefile:225/:1048).
   (e) Port-asset regeneration: regenerate all 11 assets/ files via their scripts
       (render-audio-* family, generate_battle_playable_static_textures.py), implementing
       any P1 gap-list regenerators first. Generated outputs land at the paths the Makefile
       already expects.
   (f) Port build: make TARGET=smash64ds-battle-playable-hwtri -j<N>.
   (g) Report: output path, size, SHA-256, and comparison against the reference identity
       from the P1 manifest. Toolchain-version mismatch note if hashes differ but the build
       succeeded. Non-zero exit only on build/validation failure.

2. Byte-identity gates (all must pass before commit):
   G1 REGENERATION EXACTNESS: in the dev repo, move aside the current BattleShip_o2r dir,
      relocData, and the 11 tracked assets/ files; run build.ps1 with the owner's existing
      baserom; diff every regenerated file byte-for-byte against the moved-aside originals.
      Any mismatch = investigate, do not ship a "close enough" generator. Restore/commit.
   G2 ROM IDENTITY: the script-built smash64ds-battle-playable-hwtri.nds SHA-256 equals the
      reference identity exactly (proves the Makefile/script edits changed nothing).
   G3 NEGATIVE TESTS: missing prereq, missing ROM, wrong-hash ROM each fail with the
      designed message and non-zero exit; re-running after a success is idempotent.

3. Keep changes additive: build.ps1 + any new generator scripts + DECOMP_PIN.txt + minimal
   Makefile glue. Do not reorganize existing scripts. Any Makefile edit must keep G2 green.
   Run .\scripts\verify-dev-fast.ps1 -Build once as sanity (no gameplay TUs change here, so
   no Regression sweep). Update docs/publish/PUBLISH_MANIFEST.md with the final script
   inventory. Separate commits: (1) generators/gap-fills, (2) build.ps1 + pin, (3) manifest
   update. Snapshot as final command.

Constraints: decomp/ source read-only (extraction outputs into it are fine). Long
builds/extractions run detached with logs, never foreground. Never commit the baserom,
any .o2r, or regenerated assets beyond the 11 already-tracked paths. Timebox torch
bring-up debugging to ~1h per rat-hole, checkpoint findings to the manifest, continue.
```

## TASK P3 — Minimal export, fresh-history repo, clean-room verification

```
/task Assemble the publishable repo from an allowlist and prove the stranger flow end-to-end
in D:\Stuff\DevFolder\Smash64DS_Port_MinimalGithub\.

1. Author scripts/publish/export_minimal.ps1: manifest-driven ALLOWLIST copy from
   docs/publish/publish_manifest.json into a staging repo at
   D:\Stuff\DevFolder\smash64ds-publish\. Contents = PORT-CODE + GENERATED-METADATA closure
   files, build.ps1 + DECOMP_PIN.txt + required scripts subset, .gitattributes, plus the
   new public files below. EXCLUDED always: logs/, artifacts/, docs/ (dev docs), prompts/,
   .agents/, .zcode/, .github, emulators/, assets/ (regenerated at build), AGENTS.md,
   architect.md, worktree_report.md, all ROMs/o2r/decomp. Re-runs must be idempotent
   (clean-and-recopy), so future releases are re-export + new commit in staging.

2. Author the public-facing files (they live in the dev repo under scripts/publish/templates/
   or similar and are copied by the exporter):
   (a) README.md: first fetch the EXISTING README from
       https://github.com/rockenrooster/Smash64DS_Port (main branch) and fold the owner's
       authored content/intent into the new one — do not silently discard it. Then cover:
       what this is (DS port of Smash 64 via the BattleShip decomp, Mario vs Fox on Dream
       Land vertical slice), status/perf honesty (locked-30 pacing, device ~13.5-15fps
       heavy combat), prereq table WITH the exact versions the reference build used, the
       three-line build quickstart (clone → install prereqs → .\build.ps1 -Rom …),
       expected output identity, credits (BattleShip upstream URL, sm64-nds, libultraship/
       torch, devkitPro), and the legal stance (bring your own legally-obtained ROM; no
       Nintendo assets or ROM-derived data in this repo).
       READABILITY REQUIREMENT: write for a human stranger, NOT in this repo's internal
       style. Plain English; zero internal jargon (no harness/mode numbers, no verifier/
       ledger/board/slice terms); short sections under clear headings; the quickstart within
       the first screenful; any perf term explained in plain words (e.g. "runs at a steady
       30 fps pacing; heavy fights dip lower on real hardware"). Before committing, re-read
       it as a first-time visitor who has never heard of this project and fix anything that
       needs prior context. No game screenshots in v1 — they are ROM-derived imagery; adding
       any is the owner's explicit call.
   (b) NOTICE.md: provenance + no-assets policy + BattleShip LICENSE attribution as P1
       found it requires. No LICENSE file in v1 (the owner decides at flip-public).
   (c) Public .gitignore: baserom*/z64/n64/v64/nds/o2r, decomp/, build dirs, BattleShip_o2r,
       regenerated assets/ paths, logs — so a builder's clone can never accidentally commit
       Nintendo-derived bytes back in a fork.

3. Initialize the staging repo: git init, branch master, ONE initial commit of the export
   (fresh history — nothing from the dev repo's history). Nothing else.

4. LEAK AUDIT on the staged tree (hard gate, all must be clean):
   zero files matching \.z64|\.n64|\.v64|\.nds|\.o2r|baserom; zero BattleShip_o2r/relocData/
   assets-audio/assets-renderer content; no binary file >256KiB; byte-scan every file for
   N64 ROM magic (80 37 12 40 / 37 80 40 12 / 40 12 37 80) and for "SMASH BROTHERS"; no
   embedded-CONTENT arrays per the P1 embedded-data audit; grep for "D:\\Stuff" and "the owner"
   (zero hits outside README credits if the owner wants credit). Record total repo size
   (expect single-digit MB).

5. CLEAN-ROOM PROOF in D:\Stuff\DevFolder\Smash64DS_Port_MinimalGithub\ (the owner-designated
   scratch): list then wipe its contents; git clone the staging repo into it; from a fresh
   pwsh -NoProfile session, run ONLY:
     .\build.ps1 -Rom <absolute path to the owner's existing baserom, OUTSIDE the clone>
   Gate: produced smash64ds-battle-playable-hwtri.nds SHA-256 == reference identity.
   Exercise the real network decomp fetch at least once here (the -DecompPath shortcut may
   be used for repeat runs). Then re-run the two negative tests (no ROM / wrong ROM) in the
   clean room. Never copy the baserom into the clone.

6. Deliverables: docs/publish/PUBLISH_VERIFICATION.md (leak-audit results, clean-room build
   log excerpt, final hash, repo size, file count) committed to the dev repo; exporter +
   templates committed; staging repo left ready at D:\Stuff\DevFolder\smash64ds-publish\.
   Snapshot as final command.

Constraints: allowlist only — if a build in the clean room fails on a missing file, the fix
is adding that file to the manifest CONSCIOUSLY (re-classify its provenance first), never
"copy the directory". If any file cannot be cleanly classified, STOP and report rather than
export it. Long operations detached with logs.
```

## TASK P4 — Local master-only cleanup + GitHub push

```
/task Reduce the local repo to master-only (with a full safety bundle first) and push the
staging repo to https://github.com/rockenrooster/Smash64DS_Port. That repo is PUBLIC, so
pushing IS publishing — preflight: TASK P3's leak audit AND clean-room gate must both be
green, re-checked at the top of this task.

1. SAFETY BUNDLE (before anything is deleted): git bundle create
   D:\Stuff\DevFolder\_backups\smash64ds-full-20260718.bundle --all, then git bundle verify
   on it; record its size + SHA-256 in docs/publish/PUBLISH_LOG.md. This is the recovery
   path for every branch deleted below.

2. Worktree cleanup (16 registered). For EACH worktree: git -C <path> status --porcelain.
   CLEAN → check PERF_LEDGER/board for evidence citations pointing INTO that worktree path
   (hash-migrate anything cited, per AGENTS.md Task-24 rule) → git worktree remove it.
   DIRTY or ambiguous → leave it, list it in PUBLISH_LOG for the owner. Special cases: the three
   %TEMP% task16 worktrees are closed-lab scratch (remove if clean); leave
   .tura/control-task8-cut-e ALONE (the owner's tool manages it) and just note it. Finish with
   git worktree prune.

3. Branch cleanup (after bundle verify only): delete every local branch except master —
   the 22 codex/* branches and wip/ftmain-import — recording each name + tip SHA in
   PUBLISH_LOG. Branches pinned by a kept (dirty) worktree stay, listed for the owner. Then an
   optional single git gc --prune=now (no reflog expiry); report .git size before/after.
   Do NOT rewrite master history; deep repo diet remains the deferred R-lane TASK 24.

4. GitHub push (staging repo only — the dev repo never gets a remote in this task).
   Target: https://github.com/rockenrooster/Smash64DS_Port (exists, PUBLIC, README-only
   stub on main; gh already authenticated as rockenrooster). Steps:
   (a) gh auth status; on failure STOP and report.
   (b) Verify the remote is still only the known stub (branch list = main; contents =
       README.md). If ANYTHING else appeared since 2026-07-18, STOP and report.
   (c) Archive the existing remote README.md text into PUBLISH_LOG.md (nothing is lost
       when main goes away; P3's README already folded its content).
   (d) From D:\Stuff\DevFolder\smash64ds-publish\:
         git remote add origin https://github.com/rockenrooster/Smash64DS_Port.git
         git push -u origin master
   (e) gh repo edit rockenrooster/Smash64DS_Port --default-branch master
   (f) git push origin --delete main   (the owner's requirement: repo contains ONLY master)
   (g) Set a one-line repo description via gh repo edit; verify final state with
       gh repo view (default branch master, only branch master) and record it in
       PUBLISH_LOG. Do NOT touch visibility.

5. Hand-off: finish PUBLISH_LOG.md with a "repo is LIVE" note and the owner's prompt-review
   checklist (open the GitHub file listing and eyeball it against the leak-audit items,
   read README/NOTICE as a stranger would, decide license), plus the kept-worktree/branch
   list, the archived old README text, and the bundle location. Commit the log. Snapshot
   as final command.

Constraints: no git reset --hard / checkout -- anywhere; deletions only after the bundle
verifies; dirty state is reported, never discarded. If ANY leak-audit item from P3 is in
doubt at push time, push nothing and report.
```

---

## Sequencing and effort

- P1 ≈ one focused session (mostly tracing + one detached clean build).
- P2 is the long pole: torch/extraction bring-up + byte-exactness on all regenerated assets.
  If a generator can't reach byte-identity inside the timebox, checkpoint and report — do not
  weaken G1/G2 to "close".
- P3 + P4 fit one session once P2 is green.
- This lane runs EXCLUSIVE (R-lane paused) — the owner prioritized publish. After P4, resume the
  R-plan queue (25R first).

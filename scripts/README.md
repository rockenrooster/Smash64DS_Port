# scripts/ layout

Reorganised 2026-07-30 to match the Runtime 2 generic/specific generator idea
(`docs/Smash64DS_Runtime2_SwitchPlan.md` §2): build tooling is generic, content
is specialised, and the directory now says which is which.

## The rule

**The pipeline spine stays flat. Content moves into area folders.**

- **Flat (spine):** everything typed by name or enumerated by the harness
  registry — `verify-*`, `check-*`, `capture-*`, `compare-*`, `census-*`,
  `benchmark-*`, `debug-*`, `sample-*`, `soak-*`, `clean-generated.ps1`,
  `New-Smash64DSSnapshot.ps1`, `suggest-verification.ps1` — plus `lib/`
  (dot-sourced PowerShell helpers), `fixtures/` (checker inputs), and
  `generated/` (script outputs). These are infrastructure, not content; they
  stay at the top level permanently.
- **Area folders (content):** generators, compilers, oracles, and their host
  fixtures for one area of the game. Generic machinery sits at the area root;
  content-specific machinery sits one level down:

```text
fighters/            generic fighter compiler (generate_nds_native_owners.py
                     covers Mario and Fox) + its host checkers
fighters/mario/      Mario-only tooling, when it exists
fighters/fox/        Fox-only tooling, when it exists
stages/              generic stage compiler + host checkers + shared math
stages/dreamland/    the Dream Land family: world mesh, camera oracle,
                     simplifier/quantizer/primitive compiler, DS mesh
                     generator, Pupupu water (water = Dream Land)
sfx/                 audio pack generators (FGM phase pack)
sfx/bgm/             BGM renderers
sfx/items/           item sounds, when they exist
2d_vfx/              sprite-effect generators (Task 39 hit sparks + census)
3d_vfx/              3D effect generators, when they exist
menus/               menu tooling, when it exists
publish/             minimal-export audit tooling
```

**A new script lands in its area folder from day one.** Only genuinely
cross-area infrastructure earns a flat spot.

## Python path convention

The generators cross-import across folders. Every script inside an area
folder carries the standard prelude (walk up to `scripts/`, `import _paths`),
and `_paths.py` puts the root plus every area folder on `sys.path`, so bare
`import generate_nds_native_stage` keeps working from anywhere. Moved or new
scripts must not compute the repo root from `__file__` — use
`_paths.REPO_ROOT` / `_paths.SCRIPTS_ROOT`. Adding a new area folder means
adding it to `_AREA_DIRS` in `_paths.py`.

## Enforcement and couplings

- `check-melonds-policy.ps1` scans `scripts/` **recursively** (fixed
  2026-07-30 — before that, a `.ps1` moved into a subfolder silently escaped
  the Start-Process/hidden-window policy). Keep any future scanner recursive.
- Callers that name a script by path: the Makefile (`2d_vfx` hit sparks),
  `build.ps1` (audio + native-owner generators), and the `check-*` wrappers.
  Moving a file means fixing those in the same commit; `rg` for the basename
  before and after.
- `docs/publish/publish_manifest.json` + `PUBLISH_MANIFEST.md` are **dated
  audit records** pinned to their audit commit — they still name pre-reorg
  paths on purpose. The next publish audit regenerates them against this
  layout; do not hand-edit the old ones.

## Still flat, deliberately

The `task*`-prefixed probes, one-shot censuses, and closed-lab `.c`/`.S`
fixtures at the top level are deletion candidates, not filing candidates.
They get a deletion census in a Task 24 quiet slot (cleanup never combines
with active implementation); whatever survives gets an area home then.

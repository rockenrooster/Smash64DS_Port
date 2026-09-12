# P2-2 — Semantic Pack Estimator and Capacity Verdict

A read-only host estimator supplies the go/no-go answer for resident-set migration. Reuse the existing estimator and manifests. This contract must not resurrect old fixed allowances or turn an optimistic bound into a measured fit.

## Inputs and precise scope

Use `scripts/fighters/estimate_fighter_pack.py`, the generated fighter production manifest, native image census, source typed declarations/relocation edges, and the current linked build's capacity witness. The manifest already owns file IDs, paths, hashes, core/dependency closure, motions and status contracts. Do not rediscover that file inventory.

The estimator adds semantic object/consumer reachability and the DS representation cost. Include source-derived non-pointer edges: hierarchy/part/detail selectors, material-script texture IDs and palette binding, copy selections, late reset/constructor readers, script commands and all aliases. Unrecognized declaration forms, integer-indexed reachability or ambiguous readers are STOP—not dead data.

## Capacity equation

Use a common, explicit scene/life stage:

`usable pack allowance = measured removable-data credit + measured free bytes - required reserve - unaccounted future/other/binder costs`

All terms identify the source revision, generated inputs, build configuration, allocator and measurement stop. Deduct transient overlap/setup state as well as final residency. Some native-image/binder bytes may already be in the measured baseline; explain their accounting exactly once. The same byte cannot be both credited as removed and silently retained.

The September 10 evidence in `artifacts/performance/2026-09-10_pack-skeleton-ceiling/CEILING.md` is a **relaxed upper bound** measured before complete battle setup. Its optimistic assumptions can prove RED if even the minimum required pack exceeds it. They cannot prove GREEN. The old `175,604`/`227,380` allowances and provisional green bands are historical, not current thresholds. Current numeric observations stay in that evidence and the board; do not maintain another mutable number table here.

When a complete pack-disabled skeleton cannot yet run, retain the useful bound, name the stop and unmeasured deductions, and work on the missing capacity inputs or sufficient recovery. Do not require a fictitious exact answer before recording a proven negative result.

## Semantic disposition

| Source class | Required accounting / representation |
|---|---|
| Raw Gfx/Vtx already replaced by native programs | Remove only after all consumers are translated; charge resident native images/IDs/geometry |
| Required textures/palettes | Charge the chosen VRAM/handle plan; no “uploaded therefore free everywhere” accounting |
| Materials and animated selection | Retain compact records/tables for every reachable state/frame |
| Joint/detail/model-part data | Retain effective reachable selections, including Low→High common-part fallback and normal High states |
| Attributes/hurtbox defaults | Retain required defaults and reset readers; current mutable copies do not replace reset sources |
| Setup-only scaffolding | Remove only when no gameplay reconstruction, respawn, weapon/effect or state-reset consumer needs it |
| CSS/Results-only data | Charge the owning scene, plus any overlap at transitions; do not make battle pay by default |
| Shared atoms and aliases | Canonical object identity; immutable sharing once, mutable instance copies separately |

A native owner is additional real storage, not a blanket credit. Copy hats/powers and donor articles depend on demonstrated source reachability; absence from the initial roster is not sufficient if another legal source path can introduce the capability.

## Domain and algorithm

Enumerate all unique sets of one through four playable kinds: `12 + 66 + 220 + 495 = 793`. This is a host capacity proof, not a requirement to ship 793 monolithic ROM packs. For each set, union immutable atoms and price legal four-instance multiplicities, selected costumes/team shades, detail selections and capabilities. Source bounds can collapse equivalent profiles only with an explicit reason. Stage/items/audio and campaign variants/waves need their own declared profile contributions; “any four” alone does not establish those.

Algorithm: canonicalize typed objects and references; add semantic edges; start from actual runtime roots; walk to fixed point; apply only justified DS dispositions; price resident/temporary/native/VRAM/audio costs; compare each legal profile against the appropriate capacity. Keep provenance from an expensive atom to its retaining source consumer. This makes a failed total actionable.

## Outputs and tests

Required report fields: baseline identity, configuration/input hashes, exact versus bounded allowance, unresolved costs, domain tested, per-class totals, worst sets/profiles, remaining deficit/reserve and verdict. Cost ranking must not double-count mutually exclusive variants or shared atoms.

Positive controls reproduce known source inventories and existing native-image costs. Negative controls deliberately remove a required edge/root, use an unsupported type/selector, break an alias/range or omit a Low fallback; the estimator must fail rather than underestimate. Validate ordinary numeric behavior without reading current pointer addresses as file offsets. Compare representative output counts to the existing actual build/runtime census before trusting a new parser rule.

## Verdicts and next work

**STOP:** Any unclassified required atom/consumer or invalid input. Report its owner and location; no size verdict.

**RED:** A justified lower demand exceeds even the justified upper capacity. Record the minimum deficit; choose sufficient measured recovery, not another hopeful loader experiment.

**CONDITIONAL:** Demand might fit, but deductions, profiles or transient peaks remain unknown. Name what settles it; do not label the runtime accepted.

**GREEN for migration:** All declared profiles and lifetime peaks fit an actually justified capacity with reserve, with no unknowns. Authorizes the P2-2 runtime integration/proof package, not final gameplay acceptance. The natural runtime still must demonstrate the same bounds and semantics.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `scripts/fighters/estimate_fighter_pack.py`.
- `scripts/fighters/fighter_production_manifest.json`.
- `include/nds/generated/nds_native_fighter_image.generated.h`.
- `docs/reviews/Review_Deriving_Fighter_Live_After_Setup_Set.md`.
- `artifacts/performance/2026-09-10_pack-skeleton-ceiling/CEILING.md`.
- `decomp/BattleShip-main/decomp/src/ft/ftparam.c: model-part and damage-collision reset readers`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-2-pack-estimator.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-2-pack-estimator.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.

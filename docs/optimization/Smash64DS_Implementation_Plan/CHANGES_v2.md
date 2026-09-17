# Changes from the original implementation package

## Binding requirement

Every source-legal four-fighter lineup must work on every selectable VS stage,
including repeated fighters and legal slot assignments. A known legal failure
blocks universal closure. Rare-overrun allowance and P2/P3 scope are unchanged.
A safe rejected legal match is containment, not passing support.

## Implementation changes

The master, shared contracts, every subsystem scope and affected existing task
cards now carry the universal requirement. Current enabled content is distinct
from full required content; unfinished fighters/stages produce explicit blocked
obligations, not smaller test denominators. Shared immutable banks are separated
from per-instance writable state. No combinatorial expansion of full ROM banks.

Six new cards bring the task register from 75 to 81:
N00.06 catalogue/enumeration; N02.07 exhaustive resource feasibility;
N03.11 duplicate/slot proof; N10.07 resumable runner; N10.08 exact-set release
reconciliation; N10.09 adverse interactions and separate cost leaders.
Dependencies, CSV/JSON, graph and completion conditions are updated together.

## Qualification changes

DEV_FAST and SCREEN retain efficient local and pairwise/targeted testing.
RELEASE_EXHAUSTIVE requires a full scored source-normal battle for every base
roster-stage case: 12,285 for 12 fighters and 9 VS stages. The 186,624 ordered
cases are tracked separately; unproved timing permutations remain executable
obligations. Resource symmetry does not prove mechanical or timing symmetry.
Costumes, rules, teams, CPU/control modes and dynamic interactions add separately
declared obligations; base counts do not represent every possible game state.

All product gates are per case/run, then logical AND. Pooled P95 cannot hide one
bad lineup. Missing/blocked/stale/invalid cases are explicit; failures and seeds
cannot disappear on retry. Case identities attest actual guest configuration.
Resume/invalidation rules preserve compatible proofs without re-running a full
matrix after every edit. Shared layout changes conservatively stale timing.

New source-clock/window requirements prevent treating historical presented frame
1973 as a universal one-minute endpoint after the runtime speeds up. Existing
complete-population cadence and rare-overrun policy stay in force.

## Files and evidence

New 16_ALL_ROSTERS_ALL_STAGES.md specifies the algorithms, proof scopes and
COV01–COV18 negative fixtures. New machine-readable coverage contract and four
unmeasured evidence templates accompany the revision. Static planning tests and
a count helper are included. These do not implement or run game qualification.

GitHub master moved to e67e5871ba8c4ae972f4826bfeb89757d3686401 during this revision.
Old benchmark figures remain historical at their original source/ROM pins; no
new runtime measurements are claimed. Existing research in docs/optimization is
preserved. Nothing was pushed to GitHub.

INSTALL_ADDITIVE.patch is for a first install; UPGRADE_FROM_V1.patch is only for
the exact original package. Check the appropriate patch before applying it and
do not overwrite active or hand-edited work.

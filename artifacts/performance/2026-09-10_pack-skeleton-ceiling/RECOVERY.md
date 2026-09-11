# P2-2 source-complete recovery bound — 2026-09-10

Outcome: **RED.** The current approved three-lever recovery package cannot make
the source-complete four-kind pack fit even under an impossible zero-cost
replacement assumption.

Immutable code baseline for this cycle: `907c46daffbec55477459cc56e83dfc9a417dabb`.
The working tree was already dirty; this result changes only the pack estimator,
its focused tests, this evidence, and the owning execution-board row.

## Source contract

BattleShip starts a fighter from `desc->detail`
(`decomp/BattleShip-main/decomp/src/ft/ftmanager.c:704`), but Low is not a
four-player lifetime guarantee. The normal top-KO path explicitly requests High
(`decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondead.c:529`) and battle
pause does the same (`decomp/BattleShip-main/decomp/src/if/ifcommon.c:2955`).
Therefore the capacity verdict must retain the reachable High+Low native-owner
union. The Low-only numbers remain diagnostic only and cannot drive acceptance
without a separately measured and owner-accepted presentation delta.

## Capacity and recovery result

Command:

```powershell
python scripts/fighters/estimate_fighter_pack.py --ledger --strict --json builds/p2-2-capacity-source-complete.json
```

The strict 793-set enumeration reports:

- source-complete worst raw set: Captain + Link + Pikachu + Kirby = **361,362 B**;
- source-complete worst optimistic VRAM-bound set: Donkey + Captain + Link + Kirby = **351,776 B**;
- current relaxed ceiling from `CEILING.md`: **291,268 B**;
- current source-complete VRAM-bound shortfall: **60,508 B**.

For the actual worst VRAM-bound set, source atoms were deduplicated by
`(file_id, symbol)`. Lever 7.1 is already fully credited by the 351,776 B
VRAM-bound figure, including **4,716 B / 27** still-unresolved texture/palette
bank objects, so those bytes cannot be counted again. The only remaining legacy
recovery pools that can reduce that endpoint are:

- lever 7.3 unresolved `u32`: **41,700 B / 642 objects**;
- lever 7.2 unowned weapon geometry: **18,048 B / 95 objects**;
- combined maximum raw recovery: **59,748 B**.

Even pretending all 59,748 B disappear with **zero replacement bytes** leaves
`60,508 - 59,748 = 760 B` unfunded. Real compact event/semantic records and
native weapon replacements cost nonzero bytes, so **760 B is only a lower bound
on the additional source-equivalent recovery required**. The exact current
32 KiB-floor ceiling is still unknown because the pack-disabled skeleton halted
before full battle; that uncertainty cannot invalidate this RED result because
291,268 B is already the relaxed upper bound.

This closes the question of whether finishing only recovery levers 7.1–7.3 can
make P2-2 fit: **it cannot**. P2-2 now requires an additional non-overlapping,
source-equivalent memory reduction (including its own replacement cost), or new
direct evidence that raises the real allowable pack ceiling. Low-only residency
is not such a source-equivalent reduction.

## Stable inputs and verification

Inputs used by the strict run:

| Input | SHA-256 |
|---|---|
| `scripts/fighters/fighter_production_manifest.json` | `0EF7DF32519E324E39A21A0D609DCE87EDD760C97FADEEE2991B345E76627344` |
| `include/nds/generated/nds_native_fighter_image.generated.h` | `8BD031BDD9C2FA99E04952FB5EFCB8FB05165183C62B50E2C5F7ACC8605635CA` |
| `artifacts/performance/2026-09-10_pack-skeleton-ceiling/CEILING.md` | `ED1D1248E7C7069AC451A6212FF8B490ED159F5BC83BEE60890168C59B1A99E0` |
| `scripts/fighters/estimate_fighter_pack.py` | `04BE8962F64200648FB891244F9B19847F4AF52D395C5949EDB9C7E75692F851` |

Focused/widest relevant host verification after the estimator correction:

```text
python -m unittest scripts.fighters.test_estimate_fighter_pack
Ran 72 tests in 6.969s
OK
```

No ROM/runtime implementation changed in this outcome, so no build or emulator
run was added; this follows the current code-first verification deferral in
`docs/VERIFYING.md` and avoids consuming a Boundary run for a host-only capacity
proof.

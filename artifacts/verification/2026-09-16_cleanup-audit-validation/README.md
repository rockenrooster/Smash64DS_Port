# The 2026-09-16 cleanup audit, validated item by item

The owner's rule for these lists is "do the safe ones and update the fixtures for
the rest". Safe and worthwhile are not the same thing, so each item was checked
at HEAD rather than taken from the audit's prose — a previous list in this
session contained claims that were already done, wrong, or would have gone RED.

## Executed

| # | claim | measured | outcome |
|---|---|---|---|
| **1** | 113 wrappers, ~1,900 dead lines in `reloc_backend_compat_shims.c` | **171 candidates, 166 admissible, 3,779 lines** | **DONE** (`63cf6704c64`). 17,745 -> 13,966. Proven codegen-neutral: the linked ELF differs by 5 bytes, all embedded git stamp (`…/2026-09-16_dead-tail-trim/`) |
| **10** | 20 duplicated wave-1 Makefile recipes; possibly already done | **NOT already done** — 20 recipes + 20 `_PREREQ` lines, all uniform | **DONE** (`5216e67dbfd`). Makefile 7,958 -> 7,862 |

Item 1's four hand-exclusions are the value of the tooling, not a shortfall:
`ftCommonSpecialHiCheckInterruptCommon`, `efManagerQuakeMakeEffect`,
`ndsBattlePlayableRuntimeEnabled` and `ftParamCheckSetFighterColAnimID` all have
a "dead tail" that is the `#else` arm of an `#if` and is fully reachable. A fifth,
`ftCommonDamageUpdateDamageColAnim`, was refused as ambiguous: its forward
declaration's first line does not end in a semicolon, so the definition could not
be identified uniquely.

## Validated SAFE but NOT worth executing

Both were confirmed unpinned — nothing under `scripts/` asserts on them — and
both fail on value once the copies are actually diffed.

### Item 8: Python `words_at` x17, `check_text_pins` x9

`words_at` is **two lines**:

```python
def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]
```

and the 17 copies are **three distinct bodies** (14 / 2 / 1), not one.
`check_text_pins` is six lines across **three distinct bodies** (4 / 4 / 1).

The drift argument that justified collapsing `census()` earlier does not
transfer. `census()` is a 20-line corpus scanner whose staleness is **silent** —
a stale copy reports a smaller corpus rather than failing. A `struct.unpack_from`
wrapper has no such mode: if it drifts, the decode is wrong, the generated output
changes, and the pinned hashes catch it immediately.

Net saving is roughly 50 lines, against adding an import dependency to 23 files
that are each a **producer of pinned generated output**. Declined.

### Item 7: PowerShell `Assert-Condition` x17, `Get-ElfSymbolAddress` x14, `Get-Ints` x9

The audit's own caveat is the right one — *"move only the identical, stable
helpers"* — and measurement says almost none are identical.
`Get-ElfSymbolAddress` is 10-11 lines per copy and the 12 files carry **10
distinct bodies**. Centralising means reconciling ten variants, each consumed by
a verifier that can go RED, for about 130 lines. Declined.

## Blocked

| # | measured | blocker |
|---|---|---|
| **2** | `check-gbi-decode-fixtures.ps1` is 2,954 lines with 61 `Get-Content` — **zero are decode fixtures**; 60 are source-text pins, 1 (`:1460 $titleBackend`) is assigned and never used | This is the keystone: it pins items 3, 5 and 6. The real decode fixtures are two `ReadAllBytes` at `:2918` and `:2944` |
| **3** | 7,337 lines, Task 9/16/20/25/29/32/34/36/44 modes | **FIXTURE-BLOCKED hard** — 340 `.Contains()` on its exact text from `check-gbi-decode-fixtures.ps1` (`:1866`, `:2427`, `:2572`, `:2715`), and it runs in Boundary |
| **5** | 539 `override NDS_*`, 27 TARGET blocks | **FIXTURE-BLOCKED** — `:2443` matches an exact `override` chain |
| **6** | `nds_startup.h` 8,926 lines / 7,300 `extern volatile` | **FIXTURE-BLOCKED (partial)** — `:2525` plus six `.Contains()` |

## Mixed

**Item 4** — three of the five flags are hardwireable (`NDS_R2_FTR_DRAW_MEMO`,
`NDS_R2_FIXED_SQRT`, `NDS_R2_CAMERA_FIXED`; the last two A/B through runtime
`-SetGlobals` pokes rather than build arms). Two are **load-bearing and must not
be deleted**: `NDS_R2_FIGHTER_PACKET`, because `nds_renderer_native_common.c:3386`
`#error`s and `NDS_LAB_NO_CULL` cannot build without `=0`; and
`NDS_R2_RESULTS_AFFINE`, because `check-gbi-decode-fixtures.ps1:2466` asserts the
literal `NDS_R2_RESULTS_AFFINE ?= 1` Makefile line.

This matters beyond item 4: several of this repo's flags are the only mechanism
for a **same-ROM A/B**, which is the sole comparison form that survives layout
noise. Deleting an "unused" off arm removes the ability to price the on arm.

**Item 9** — `.tmp-sample-tick-hud-nobreak.ps1` is 1,627 lines, identical to
`sample-tick-hud-buckets.ps1` except line 426's `-BreakOnStartup`, **untracked**,
with zero references. Safe to delete, but it is an untracked file in the owner's
working tree and may be an in-flight experiment. **Left for the owner** rather
than deleted.

## Recommended order, revised

The audit proposed 1 -> 2 -> 3 -> 4 -> 5 -> 6. Item 1 is done and item 10 is
done. **Item 2 is the only remaining one that unblocks anything**, and it turns
out to protect nothing: zero of its 61 `Get-Content` reads is a decode fixture.
Replacing its 60 source-text pins with generated-config, symbol or runtime-result
checks is what makes 3, 5 and 6 possible at all.

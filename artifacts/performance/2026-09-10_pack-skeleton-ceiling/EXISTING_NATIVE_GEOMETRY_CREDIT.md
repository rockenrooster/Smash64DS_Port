# P2-2 existing-native geometry credit

Date: 2026-09-10

## Outcome

The semantic pack estimator was still charging raw special/donor `Vtx`/`Gfx`
bytes even when the exact immutable geometry is already translated by a
source-pinned DS-native owner linked into the ARM9 baseline. That double count
is now removed only for fully covered, geometry-reader-closed source rows.

This is a capacity-accounting correction, not a new renderer or a content
drop. Source DObj, AnimJoint, MatAnimJoint, gameplay and material semantics stay
resident/translated under their existing classes. The already-linked native
packet/sidecar continues to provide the pixels.

## Fail-closed credit rule

`scripts/fighters/estimate_fighter_pack.py::linked_native_geometry_rows` starts
from the exact source roots already compiled by the linked native producers.
It walks the pinned O2R Fast3D source graph with the same local-pointer and
segment-E material-branch rules as the entry/effect compiler, and records:

- every executed eight-byte `Gfx` command;
- every byte of every `Vtx` block loaded by `G_VTX`;
- fixed source spans for the separately linked Fox gun sidecar.

A source object receives `NATIVE_BASELINE_GEOMETRY` only when its complete
declared byte extent is covered. It then additionally requires every `Vtx` or
`Gfx` reader of that object to be covered too. Partial rows and rows shared by
any still-raw geometry remain `NATIVE_REPLACE_WEAPON` and continue to be
charged at raw bytes.

The compact semantic pointer itself is still charged by its owning semantic
record. The immutable target packet is already resident in the measured ARM9
baseline, so the covered source geometry contributes **0 additional W**.

## Producer locks

The estimator tests pin the root table to
`scripts/3d_vfx/generate_nds_entry_effects.py` and pin the Fox fixed extents to
`scripts/fox_gun_bake.py`. The producers independently revalidated their
checked-in outputs:

```text
python scripts/3d_vfx/generate_nds_entry_effects.py --check
verified src\nds\nds_entry_effects.generated.inc: groups=91 triangles=498 textures=52 ...

python scripts/fox_gun_bake.py --check src/nds/nds_fox_gun.c
FOX_GUN_BAKE=PASS  every decoded row is present in nds_fox_gun.c
```

The linked entry/effect packet is included directly by
`src/nds/nds_renderer_assets.c`; `nds_fox_gun.c` is in the normal C file list
and `NDS_R2_FOX_GUN_OVERLAY` defaults to 1. These are therefore baseline
representations, not future pack allocations.

## Exact old-worst-set closure

The previous VRAM-bound argmax was `Donkey+Captain+Link+Kirby`. Under the new
full-row + reader-closure rule it contains 56 already-native source rows /
**12,376 B**:

| source asset | rows | raw bytes already represented natively |
| --- | ---: | ---: |
| 350 CaptainSpecial2 | 34 | 8,272 |
| 353 LinkSpecial2 | 6 | 1,360 |
| 315 FoxUnknown gun sidecar | 2 | 1,008 |
| 355 DonkeySpecial2 | 3 | 872 |
| 325 LinkSpecial3 | 11 | 864 |
| **total** | **56** | **12,376** |

That exact four-kind set falls from 330,028 B to **317,652 B**. The global
argmax consequently changes, so subtracting 12,376 from the old envelope would
be wrong.

One important negative control remains charged: LinkSpecial3's 224-byte
`dLinkSpecial3_Vtx_0x02A0_Vtx` is only partially loaded by the linked boomerang
roots, so the whole row remains unresolved. CaptainSpecial3 Falcon Punch,
KirbySpecial2, LinkBoomerangModel and other geometry without a proven linked
owner likewise remain charged.

## Recomputed source-complete envelope

The estimator was run over all 12 currently closable fighters. A control that
forces `NATIVE_BASELINE_GEOMETRY` atoms back to their raw size reproduces the
previous result exactly:

```text
RAW_BASELINE (330028, ('Donkey', 'Captain', 'Link', 'Kirby'))
```

With the source-proven baseline credit enabled:

```text
CREDITED (318181, ('Donkey', 'Samus', 'Link', 'Kirby'))
```

The new VRAM-bound worst set contains 28 unique credited rows / **5,648 B**:

| source asset | rows | credited bytes |
| --- | ---: | ---: |
| 349 SamusSpecial2 | 6 | 1,544 |
| 353 LinkSpecial2 | 6 | 1,360 |
| 315 FoxUnknown gun sidecar | 2 | 1,008 |
| 355 DonkeySpecial2 | 3 | 872 |
| 325 LinkSpecial3 | 11 | 864 |
| **total** | **28** | **5,648** |

Because the argmax changes, the global lower-envelope reduction is **11,847
B**, from 330,028 B to **318,181 B**. Against the still-relaxed current-shell
upper bound of 291,268 B, P2-2 remains RED but its proven shortfall falls from
38,760 B to **26,913 B**.

Remaining recovery in that new worst set is still conservative:

```text
unresolved u32 raw bytes: 40156
unowned weapon raw bytes: 5616
maximum zero-cost recovery: 45772
current relaxed shortfall: 26913
```

Those are opportunity bounds only. Real replacement storage must be measured;
no remaining unresolved byte is assumed free.

## Verification

```text
python -m unittest scripts.fighters.test_estimate_fighter_pack -v
Ran 77 tests in 14.311s
OK
```

The test suite includes producer-root synchronization, Fox-gun source extent
synchronization, current-worst-set capacity pins, full-row credit checks and a
partial-row negative control. Source-index reconciliation remains exact:
retained + removable + STOP == indexed for every ledger.

## Next discriminator

P2-2 is still capacity-blocked by **26,913 B** against even the relaxed ceiling.
The new worst set's largest unresolved pool is the AObjEvent32-backed `u32`
animation data. In the old worst-set audit, four ShieldPose files alone account
for 35,520 B; discrete eight-angle substitution is not source-safe because CPU
logic can generate intermediate stick vectors. The next useful measurement is
therefore a **lossless command-aware AObjEvent32 representation**, not a content
or interpolation reduction.

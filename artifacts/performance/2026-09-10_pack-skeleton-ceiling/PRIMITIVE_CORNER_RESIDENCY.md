# P2-2 native-owner primitive/corner residency cut — 2026-09-10

This is the permanent measurement for the first additional source-equivalent
recovery lever after `RECOVERY.md` proved the original 7.1/7.2/7.3 package
insufficient. Baseline is commit `0520870a7b3` plus the already-present dirty
integration state; unrelated work was preserved.

## Change

Shipping P2 uses `NDS_TASK56_FIGHTER_PRIMITIVES=2` and
`NDS_R2_STRIP_ROUTE=0`. The generated Task56 stream already contains every
vertex reference production submits. Cross-matrix groups are deliberately one
`GL_TRIANGLES` group in source order and retain the complete packed 11-bit
dense-id + 5-bit matrix-slot value. Raw groups contain the equivalent native
primitive stream.

Native-owner image ABI v4 therefore:

- derives `run_first_corner` as `run.first_triangle * 3`; generation fails if
  that equality ever stops holding;
- omits `packed_corners` from the shipping image and makes cross-matrix emit use
  the existing Task56 primitive vertices;
- keeps `packed_corners` when primitives are disabled, when
  `NDS_R2_STRIP_ROUTE` enables the raw A/B route, or when
  `NDS_RENDERER_SCREEN_SPACE_CENSUS` needs source-order triangles;
- retains `triangles[]`: its high bit still validates source `TRI2` pairing and
  source-command progression, so this cut does not weaken that contract.

The source-order corner copy was not the executed production representation in
the shipping configuration. Removing it changes residency, not source
geometry, material choice, detail selection, animation, collision or gameplay.

## Mechanical proof

`python scripts/fighters/check_native_owner_geometry_closure.py` passed across
every generated owner and detail. For mode 2 it expands the generated primitive
groups and compares them against the source triangles, including vertex source
data, matrix routing, facing and winding. Final result:

```text
NATIVE_OWNER_GEOMETRY_CLOSURE_OK every source triangle reaches the emitted primitive stream exactly once, carrying its own source vertex, routed to its own joint's GX slot, facing outward, with source winding, for every owner and detail
```

The image loader ABI test also passed all three cases against all 68 current
owner/copy-hat image structs, including wrong-tag and short-read negative
controls. `generate_nds_native_owner_images.py --check` and
`generate_nds_native_owners.py --check` both pass. The pack estimator suite is
72/72 green.

No ROM/emulator run was performed: `docs/VERIFYING.md` currently keeps P2 in
code-first mode until the consolidated final verification pass. This cut adds
no new runtime feature or cadence path; the required later build/runtime gate
remains outstanding with the rest of that pass.

## Capacity result

Same 12-fighter source-complete ledger, same relaxed current-shell ceiling
`291,268 B`, same favorable unresolved-bank VRAM assumption:

```text
                                      before v4       after v4       delta
raw worst set                         361,362 B        341,454 B    -19,908 B
VRAM-bound worst set                  351,776 B        330,028 B    -21,748 B
minimum relaxed-bound shortfall        60,508 B         38,760 B    -21,748 B
```

The VRAM-bound argmax remains `Donkey + Captain + Link + Kirby`.

A same-tree diagnostic census with only packed-corner residency re-enabled
prices that set at `350,900 B`, versus `330,028 B` shipping. Thus `20,872 B`
of the new saving is specifically the source-order packed-corner copy. The
remaining `876 B` is the derived `run_first_corner` table plus resulting image
alignment changes.

The old unresolved 7.2 + 7.3 pools for this set are unchanged at `59,748 B`
raw. They now exceed the `38,760 B` minimum shortfall by `20,988 B` under the
deliberately impossible assumption of zero-byte replacements. That is not a
fit verdict: real native/semantic replacements cost bytes and the exact shell
ceiling remains unknown. It does prove that the previous structural
insufficiency is gone; the next P2-2 recovery work should resolve enough actual
7.2/7.3 representation cost to cover 38,760 B plus replacement overhead.

P2-2 remains **RED** until that real representation fits and a complete
four-kind natural lifetime measures the exact dynamic reserve.

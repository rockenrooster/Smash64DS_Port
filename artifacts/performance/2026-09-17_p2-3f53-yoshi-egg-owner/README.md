# Yoshi's egg: two owner-reported invisibilities, one unowned display list

Status: **derivation complete, implementation not yet written.** Everything
below is resolved from source; nothing here is baked yet.

## The two reports

From `docs/BUGS.md`, both the owner's words:

- *"character intro is invisible (egg hatching)"*
- Yoshi's shield — not separately reported, but named in board row `P2-3f53`

I had previously annotated the intro as *"NOT the hidden-part class ... Cause is
elsewhere"*. Ruling out the anim-desc mechanism was right; stopping there was
not.

## The cause, and why a motion-command sweep could not find it

`ftParamHideModelPartAll` hides a fighter's entire drawable tree. I swept it
across `*MainMotion.c` and found seven sites, all carried. **That sweep was
scoped wrong** — the function has three more callers in plain C:

| call site | state |
|---|---|
| `ftcommonguard2.c:23` | Yoshi shield |
| `ftcommonguard1.c:391` | Yoshi shield, other entry |
| `efmanager.c:5441` | `efManagerYoshiEggEscapeMakeEffect`, the hatching intro |

All three sit behind `fp->fkind == nFTKindYoshi`, and all three hide Yoshi's
body **on purpose** so an egg can draw in its place. The egg is an `EFDesc`, the
`EFDesc` has no native owner, so nothing draws and Yoshi is simply gone.

`ftcommonguard1.c:387-397` and `ftcommonguard2.c:20-26` are an if/else on
`fkind`: Yoshi takes `efManagerYoshiShieldMakeEffect`, everyone else takes
`efManagerShieldMakeEffect`. So the common shield owner already in the generator
(`SHIELD_ROOTS = (0x0248,)`, FTManagerCommon, asset 163) **never fires for
Yoshi**.

**The general lesson:** anything reachable as an `ftMotionCommand` is normally
also a plain C function, and the C callers are where the gameplay-conditional
uses live. Sweep the function, not the command table.

## One root, not two

`dEFManagerYoshiShieldEffectDesc` (`efmanager.c:490`) and
`dEFManagerYoshiEggEscapeEffectDesc` (`:1375`) name the **same**
`&llYoshiModelShieldDObjDesc`. They differ only in flags and in their secondary
matrix field (`0x2C` vs `0x4A`). Both carry `MObjSub`, `AnimJoint` and
`MatAnimJoint` all `0x0`.

So the shield and the hatching intro share one display list, and one owner fixes
both reports.

## It needs no DObjDesc resolution — it is already a Gfx root

The general expectation for these effects is
`DObjDesc -> DObjDLLink -> Gfx root`. Not here. `llYoshiModelShieldDObjDesc` is
`0xa860` (`src/import/battleship_efmanager_symbols.h:160`), and decoding that
offset as a `DObjDesc` yields garbage — depths in the billions, absurd floats.
The bytes are a display list:

```
0x0a860: e7000000 00000000   RDPPIPESYNC
0x0a868: e3001001 00008000   SETOTHERMODE_H
0x0a870: e2001e01 00000001   SETOTHERMODE_L
0x0a878: e200001c 00553048   SETOTHERMODE_L
0x0a880: fc40fe81 55fff3f9   SETCOMBINE
...      f5 x3, fd, f0, d7, f2, f3   tile/texture setup
0x0a900: d9ddfbff 00000000   GEOMETRYMODE
0x0a908: 01004008 2aa82a08   VTX
0x0a910: 06060402 00000602   TRI2
0x0a940: ...                 ENDDL
```

**29 commands, 232 bytes, 1 VTX, 1 TRI, self-contained.** Structurally the same
shape as Yoshi's canonical root `0x2800` (34 commands, 272 bytes, 1 VTX, 1 TRI).

The EFDesc field is labelled `// DObj Setup attributes offset (?)` in the decomp
— with the question mark — and for a descriptor whose MObjSub/AnimJoint/
MatAnimJoint are all zero it holds the immutable `Gfx` directly. That is the
precedent `generate_nds_entry_effects.py:175-182` already records for the Fox
reflector: *"The drawable child holds its immutable Gfx directly at 0x01B8."*

The microcode is **F3DEX2**, so `G_ENDDL` is `0xDF` and `G_DL` is `0xDE`. A walk
looking for F3D's `0xB8` runs off the end and reports no terminator — it did,
before the opcode was corrected.

## Everything the InputSpec needs, resolved

| field | value |
|---|---|
| path | `decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/YoshiModel` |
| sha256 | `e2654cbdc969a473de1e78fa392211a4f465657a7c16ffc93e6bb2b073d4b04c` |
| asset id | **338** (read from the O2R header, independently of the census) |
| root | `0xa860` |

## What remains

1. `YOSHI_MODEL` InputSpec + `YOSHI_EGG_ROOTS = (0xa860,)`, compiled at the
   tuple tail per the stable-ordinal rule
   (`generate_nds_entry_effects.py:263-266`), with the `FALCON_KICK_ROOTS`
   tail-append at `:146` as the precedent.
2. One `FIRST`/`COUNT` macro pair.
3. An asset-338 branch in `ndsRendererEntryEffectRoot`
   (`src/nds/nds_renderer_native_common.c:4898-4974`).
4. An admission arm, modelled on the one-root common shield at
   `src/port/renderer_adapter_stage.c:5213-5219`.
5. Checker tokens.

**Keep runtime-owned:** `efManagerYoshiShieldProcDisplay`
(`efmanager.c:4147-4166`) derives the env colour from `fp->shield_health` every
frame, exactly as the common shield's per-player env is. Nothing about that
fade may be baked.

**Not proven:** that either state draws correctly once owned. Both are visual
claims and both need a capture — the same debt the Yoshi root programs carry.

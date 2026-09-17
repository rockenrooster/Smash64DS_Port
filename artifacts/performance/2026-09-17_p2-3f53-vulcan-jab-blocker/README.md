# Kirby's Vulcan Jab is blocked on a texture format the DS does not have

**Attempted, blocked, reverted.** P2-3f53's *most self-contained* remaining item
turns out not to be self-contained, and the reason is specific.

## Why it looked easy

The evidence pass ranked Vulcan Jab first of the three remaining `P2-3f53`
effects because the runtime half already exists: `KirbySpecial2` is an
`InputSpec` in `generate_nds_entry_effects.py`, **asset 348 is already routed**
in `ndsRendererEntryEffectRoot` and **already admitted** in
`renderer_adapter_stage.c`. The stated gap was one `case`.

Unlike the Yoshi egg — which needed a whole new `InputSpec`, a new file and a
new texel array, and cost an arena page for it — this adds roots to a file the
build already carries. It should have been nearly free.

## Resolving it, which the egg did not need

`dEFManagerVulcanJabEffectDesc` (`efmanager.c:700`) has `MObjSub`, `AnimJoint`
and `MatAnimJoint` all `0x0`, the same shape as the egg. But its render proc is
`gcDrawDObjTreeDLLinksForGObj`, not `gcDrawDObjTreeForGObj`, so
`&llKirbySpecial2VulcanJabDObjDesc` (`0x0B20`) really **is** a DObjDesc and does
need the full chain the egg skipped:

```
0x0B20  DObjDesc   desc[0] depth 0 -> DObjDLLink 0x0b00
                   desc[1] depth 1 -> DObjDLLink 0x0b10
                   desc[2] depth 18 -> terminator
0x0b00  DLLink     { count 1, gfx 0x09b0, stride 4, 0 }
0x0b10  DLLink     { count 1, gfx 0x0a78, stride 4, 0 }
```

Two Gfx roots, both well-formed and terminated:

| root | commands | bytes | VTX | TRI |
|---|---:|---:|---:|---:|
| `0x09b0` | 8 (then branches) | 64 | 0 | 0 |
| `0x0a78` | 17 | 136 | 1 | 1 |

## The blocker

Wiring both roots in and regenerating fails on the generator's own guard:

```
entry texture format escaped CI4/IA8/IA16/RGBA16/I4: 0/3
```

`0/3` is `FMT_RGBA` / `SIZ_32B` — **RGBA32**.

A shallow walk does not show it, and a first read of `0x09b0` said "8 commands,
no geometry, state only". That was wrong: `0x09b0` ends in a **`G_DL` branch to
`0x09f0`**, and the texture is down that branch:

```
0x09e0 DL -> 0x09f0
  0x0a00 SETTILE fmt=0(RGBA) siz=3(32b)   <== unsupported
  0x0a08 SETTILE fmt=0(RGBA) siz=3(32b)   <== unsupported
  0x0a18 SETTIMG fmt=0(RGBA) siz=3(32b)   <== unsupported
```

The geometry root `0x0a78` is fine on its own — `I16`/`I4`, both supported. It
is the state root's branch target that escapes.

## Why this is not a generator limitation to relax

**The DS has no 32-bit texture format.** Its formats are A3I5, palette 4/16/256,
4x4 compressed, A5I3 and direct 16-bit A1BGR5. There is nothing to widen the
allowlist *to*.

So supporting Vulcan Jab means converting RGBA32 to a DS format, and the
conversion carries a fidelity decision rather than a mechanical one: 8-bit alpha
has to become **1 bit** (A1BGR5) or **5 bits** (A5I3), or the texel has to become
paletted. `atlas-carries-shape-not-colour` and the render-fidelity doctrine both
bear on which, and it is not a choice to make silently inside a wiring change.

## State

Reverted; `generate_nds_entry_effects.py --check` is clean and the generated
packet is byte-identical to `f025e9be8e4`. Nothing was committed to the
generator or the runtime.

`P2-3f53`'s three remaining items now read:

| item | state |
|---|---|
| Yoshi egg | derived and implemented, **reverted** on the arena page boundary (`…_p2-3f53-yoshi-egg-owner/`) |
| Kirby Vulcan Jab | **blocked on RGBA32 → DS conversion**, this artifact |
| Pikachu Thunder (down-B) | unstarted; the row's name also conflates it with Thunder Jolt, which is already natively owned |

None of the three is now a wiring change. That is worth stating plainly, because
the row's summary still implies they are.

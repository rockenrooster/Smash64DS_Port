**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

## Hit-effect presentation (owner, 2026-08-05, with N64/RetroArch reference shots)

**1. Some Crowd audio cues get cut off after big hits.**
    Crowd audio cues should never be cut off, why is this happening


**2. Fire burn effect missing.** Not fixed. It is the victim burn
(`efManagerDamageFireMakeEffect`), confirmed by the owner as "the flame VFX that
come off the victim" — **not** the hit spark (`efManagerFireSparkMakeEffect`).
Different makers; do not conflate them. It is already wired, but to the
procedural 3D template `nNDSVisualEffectHitFire`
(`src/port/reloc_backend_compat_shims.c:7998`, scale
`ndsVisualDamageScale(size, 0.60F, 0.03F)`) — **not** the sprite path, which
only handles normal-element hits under `NDS_TASK39_FX_SPRITES`. The source asset
is present and mapped (`dEFManagerFireSparkEffectDesc`;
`battleship_efmanager_symbols.h:54-57` in EFCommonEffects2). Per `386fb8e2`, in
P1 the **only** fire-element source is Mario's Special N fireball, a WEAPON
taking its element from `attr->element` (`wpmanager.c:197`) and reaching the
fire maker through the weapon switch at `ftmain.c:2770`, never the fighter
switch at `:2715`. So it can only appear when the fireball connects. First
question is existence, not appearance: does the maker get called at all?

**3. The billboard observation — owner, 2026-08-06. Read this before designing
any effect work.** Owner reports that on N64 *every* VFX except the platform is
a camera-facing billboard, at the fighter's own Z depth, always drawn on top,
with alpha on some parts. If that holds, effects need no per-effect 3D geometry
and no depth reasoning at all — which is the same conclusion cycles 88-91
reached from the opposite direction when the G3 packet path was refuted
(effect geometry is per-instance, and static per-layer world Z could not
reproduce the painter order). A billboard model sidesteps both: order is
submission order, and "on top" is a draw-last property rather than a Z one.
This is a design input, not a filed bug, and it is the owner's observation of
the N64 — confirm against BattleShip before building on it.

Item retired 2026-08-06: "go back to the blue spark effect" — owner withdrew it
("Forget I said anything") before any change was made. The colour is baked by
`scripts/generate_task39_hit_sparks.py`, not chosen at runtime, if it returns.

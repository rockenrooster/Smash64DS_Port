**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

## Hit-effect presentation (owner, 2026-08-05, with N64/RetroArch reference shots)

Owner's words, kept verbatim:

> so the a attack VFX is a little too big.
> I have a normal a attack and a strong a attack, they both look like 2d
> animations like the coin effect.
> also we are missing the fire burn effect, 2nd screenshot.

> I'm talking more of the flame VFX that come off the victim

> I noticed that when playing on the N64 version that ALL the VFX are
> billboards (minus the platform) that always face the camera and have the same
> z depth as the fighter, but always draw on top of them and some/parts of the
> VFX have some alpha transparency.

Reference: N64/RetroArch captures. (1) normal-A hit, large blue/white spark;
(2) Mario fully engulfed in flame — the missing burn.

**1. A-attack spark too big.** Not fixed. TWO multipliers, neither
source-derived. `spark->scale` ramps with damage in the LIGHT branch only
(`src/nds/nds_ifcommon_oam.c:1820`): `0.5` below damage 10, `(size-10)*0.13+1.0`
above it, clamped to `NDS_TASK39_HIT_SPARK_SCALE_MAX 2.2F` by `386fb8e2`. HEAVY
is a **flat 1.0 and does not ramp at all**, which is why a big light hit can
out-size the heavy one. That product is then multiplied by
`NDS_TASK39_HIT_SPARK_SCREEN_SCALE 1.6F` (`:69`), so on a 16x16 cell the light
spark spans 12.8px to **56px** on a 256px screen and the heavy one a flat
25.6px.

`size_double` at `:2453` is **NOT** a third multiplier — corrected 2026-08-06
after it was reported as one. It is libnds's affine bounding-box flag ("double
the sprite size for rotation", `sprite.h:396`): it stops a scaled sprite being
clipped at its cell edge, and the visible size comes from the affine matrix.
Enabling it above 1.0 is correct.

`386fb8e2` already recorded that no source number exists here (the N64 draws
particles, not sprites) and that **the ceiling is a port choice with the owner
as oracle**. Owner proposed `SCREEN_SCALE` 0.8; note that is global and halves
the heavy spark to 12.8px as well, whereas `SCALE_MAX` reins in only the light
ramp's top end. Owner picks.

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

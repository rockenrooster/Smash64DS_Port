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

**1. A-attack spark too big. FIXED 2026-08-06, owner-approved by play.**

**Everything written here about `NDS_TASK39_HIT_SPARK_*` on 2026-08-05 was about
DEAD CODE, and the note saying so was already in the tree.**
`src/nds/nds_ifcommon_oam.c:1732` records that the whole sprite hit-spark path is
unreachable in the shipping ROM: `battleship_efmanager.c` defines STRONG
`efManagerDamageNormal{Light,Heavy}MakeEffect` under
`NDS_R2_SOURCE_EFFECTS_PARTICLE` (Makefile default 1, no override), so the
linker never takes the weak shims that reach `ndsTask39HitSparkSpawn`. That note
post-dates `386fb8e2` and corrects it; `386fb8e2`'s 2.2 clamp changed nothing on
screen. Do not tune `SCREEN_SCALE` or `SCALE_MAX` — nothing reads them.

The live behaviour is SSB64's own, in
`decomp/BattleShip-main/decomp/src/ef/efmanager.c`: LIGHT ramps `xf->scale` with
damage (0.5 below 10, `(size-10)*0.13+1.0` above, **unclamped**, so 4.9x at the
40-damage ceiling) and HEAVY never sets `xf->scale` at all, leaving it flat 1.0 —
so a big light spark out-sizes the heavy flash it decays into. The port was
reproducing source exactly; 4.9x simply does not survive 640x480 -> 256x192.

Fixed **port-side** in `src/import/battleship_efmanager.c`: the two wrappers call
the source maker unchanged and set `pc->xf->scale` on the way out to
`ramp * NDS_DAMAGE_SPARK_SCALE` (0.5F), giving HEAVY the same ramp. `decomp/`
is untouched — it is the specification, and a reader opening `efmanager.c` must
find SSB64 there, not a port preference. Engagement proof
`gNdsDamageSparkScaleCount`; retune via `-DNDS_DAMAGE_SPARK_SCALE`.

**A first attempt did this as a `decomp/` patch and was reverted in full** on the
owner's objection (2026-08-06). The eight existing decomp patches are all things
that physically cannot work on DS — framebuffer addresses, taskman, objman. A
cosmetic size preference is not that category. The port-side seam existed the
whole time (`pc->xf`, used by `ndsParticleTransformForDraw`); it just was not
checked first.

`size_double` at `nds_ifcommon_oam.c:2453` is **not** a size multiplier — it is
libnds's affine bounding-box flag ("double the sprite size for rotation",
`sprite.h:396`). Recorded because it was reported as one.

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

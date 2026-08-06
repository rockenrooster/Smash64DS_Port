**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

## Hit-effect presentation (owner, 2026-08-05, with N64/RetroArch reference shots)

Owner's words, kept verbatim:

> so the a attack VFX is a little too big.
> I have a normal a attack and a strong a attack, they both look like 2d
> animations like the coin effect.
> also we are missing the fire burn effect, 2nd screenshot.
> For the normal a attacks we can go back to the blue spark effect that we had
> a few days ago for now.

Reference: three N64 captures. (1) normal-A hit, large blue/white spark;
(2) Mario fully engulfed in flame — the missing burn; (3) a small tight
orange-and-blue hit spark on contact.

**1. A-attack spark too big.** Not fixed. Three multipliers stack, none of them
source-derived: `NDS_TASK39_HIT_SPARK_SCREEN_SCALE 1.6F`
(`src/nds/nds_ifcommon_oam.c:69`), the LIGHT ramp `(size - 10) * 0.13 + 1.0`
clamped to 2.2 by `386fb8e2`, and `size_double` at `:2453`, which redraws the
16x16 cell at 32x32 whenever the product exceeds 1.0 — with SCREEN_SCALE at 1.6
that is every spark. Big normal hit lands near 3.5x, doubled. `386fb8e2` already
recorded that no source number exists here (the N64 draws particles, not
sprites) and that **the ceiling is a port choice with the owner as oracle**;
this is that call being made. HEAVY is a flat 1.0 and is not implicated.

**2. Normal-A back to the blue spark.** Not fixed. The colour is baked into the
sheet, not chosen at runtime: `scripts/generate_task39_hit_sparks.py` tints via
`HEAVY_ENV` (player one red, which is the orange the owner filed at `386fb8e2`).
The runtime only picks a frame index. Confirm against the generator's history
which build was blue before changing it.

**3. Fire burn effect missing.** Not fixed, and it is the victim burn
(`efManagerDamageFireMakeEffect`), not the hit spark
(`efManagerFireSparkMakeEffect`) — different makers, do not conflate them. The
spark route is wired: `nEFKindFireSpark` -> `nNDSVisualEffectHitFire` at
`src/port/reloc_backend_compat_shims.c:7779`, plus `:7933` and `:7999`. The
source asset is present and mapped (`dEFManagerFireSparkEffectDesc`;
`battleship_efmanager_symbols.h:54-57` in EFCommonEffects2). Per `386fb8e2`, in
P1 the **only** fire-element source is Mario's Special N fireball, which is a
WEAPON taking its element from `attr->element` (`wpmanager.c:197`) and reaching
the fire maker through the weapon switch at `ftmain.c:2770` — never the fighter
switch at `:2715`. So the burn can only appear when the fireball connects, and
the first question is existence, not appearance: does the maker get called at
all?

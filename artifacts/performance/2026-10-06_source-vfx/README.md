# Source effect makers for the stand-in kinds; GO lamp glyph; intro stills (2026-10-06)

Three owner rows in one ROM (`build-lab-clean1006q47`, against q46):

- "Generic yellow/green/green ring VFX still played for some effects. Need to
  audit against source": `ftParamMakeEffect` answered FlashSmall, FlashLarge
  and Ripple with the generic Wave ring (nNDSVisualEffectImpactWave), and
  DamageNormal, Psionic, KirbyStar, SparkleWhiteMulti, HealSparkles and
  EggBreak with other stand-ins (or nothing). They now take their source makers
  (ftparam.c:1892-2112); every one starts an efcommon bank script the
  generator has packed since 2026-10-04.
- '"GO" unlit and lit is still blurry and hard to read': the GO lamp's
  lettering is a hand-set 12x9 pixel font in the source lettering's colour
  (owner's pick, candidate E of
  `artifacts/visibility/2026-10-06_go-candidates`, local only).
- "Rendered stills ... need to be closer to the screen side edges": the 1P
  intro stills move 8 px outward (draw-only).

## Replay digest: re-baselined, cause identified

| config | digest | first diverging frame | paired median before it |
|---|---|---|---|
| gate (Dream Land) | changed | 1,347 | -2,560 (1,287 frames) |
| Castle | identical | - | -1,664 (1,902 frames) |
| Sector Z | changed | 1,683 | -3,392 (1,623 frames) |

`ftParamMakeEffect` census on the q47 gate (scratchpad `vfxhits.ps1`): the
first request of any newly routed kind is HealSparkles (kind 74) at presented
frame 1,344, three frames before the digest moves. Particle scripts draw from
the game's one random stream (`syUtilsRandFloat` in lbparticle.c, as on N64),
so the real maker shifts every later roll -- the N64's own coupling, which the
stand-in did not reproduce. The new trajectory is the baseline from q47 on.

WORK on the new trajectories (not comparable to q46 past the divergence):
gate P50/P95 762,304 / 1,071,232 (over 1.12M: 68), Castle 799,936 / 1,094,976
(77), Sector Z 851,840 / 1,184,448 (140). Before the divergence the build is
1.6-3.4K a frame cheaper (paired medians above).

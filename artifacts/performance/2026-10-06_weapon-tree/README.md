# DObjDesc weapons draw their tree; Master Hand's bullet owner; dust bands after GO (2026-10-06)

`build-lab-clean1006q49` against q48:

- `wpManagerMakeWeapon` gives a DObjDesc weapon without DL links
  `wpDisplayMain(weapon, gcDrawDObjTreeForGObj)` instead of
  `func_ovl3_80167618`, whose `lbCommonDObjScaleXProcDisplay` is a port no-op.
  Blastoise's Hydro Pump, Onix's rocks, Meowth's coins, Beedrill's swarm and
  Master Hand's bullets now reach their baked owners (the bullets' owner,
  BossBullet, is new: BossModel 0x2BA0, 18 vertices, 22 triangles).
- The flying-dust bands that did not fit at battle start (Dream Land: 9 of 25)
  are prepared again once the entry-only textures are retired after GO.

| config | digest | P50 | P95 | over 1.12M | paired median |
|---|---|---|---|---|---|
| gate | identical | 763,584 -> 763,392 | 1,070,912 -> 1,074,496 | 67 -> 66 | -832 |
| Castle | identical | 800,384 -> 800,000 | 1,094,464 -> 1,096,384 | 78 -> 77 | -768 |
| Sector Z | identical | 852,096 -> 852,544 | 1,186,112 -> 1,182,144 | 137 -> 140 | -1,024 |

Forced-Blastoise lab match (Poke Balls only, scratchpad `kamexcap*.ps1`): the
water draws in both shots and the native failure count stays 0 (q48: 27, all
DamageFlyMDust refusals). Gate dust counters (scratchpad `mdustprobe.ps1`):
q48 9 prepared / 1 refused for the whole match; q49 25 prepared by frame 300.

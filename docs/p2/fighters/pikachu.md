# Pikachu — P2-3 fighter 6

Status: source-derived production inventory + native-model census staged; behavior/article runtime next · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftpikachu/`

## Role

The terrain-following projectile and the double-teleport recovery — two
mechanics nothing earlier in the order exercises. Small hurtbox extreme.

## Moveset uniques

- **Thunder Jolt (B)**: hops along the ground, follows terrain contours,
  crawls down walls and around ledges, dissipates on time/impact; air
  version arcs then crawls on landing. Terrain-following = per-step collision
  queries — precompute per-stage crawl paths if profiling demands (allowed by
  the optimization doctrine; behavior must stay equivalent).
- **Thunder (Down-B)**: cloud spawns at top, bolt descends to Pikachu, hits
  along its length; self-hit interplay (bolt striking Pikachu has its own
  hit) per source; screen-length article.
- **Quick Attack (Up-B)**: two chained teleport segments with distinct angle
  choice, brief vulnerability rules, no hitbox (in 64 — verify).
- Small, fast, strong edge game; famous u-smash/b-throw KO power.

## Assets & audio

Small model (cheapest draw in roster), 4 costumes (party-hat variants),
voice = actual "Pika" samples (identity-critical), announcer clip.

## DS notes / risks

- Thunder's tall bolt: fill-rate + effect pool; consider 2D-composited bolt
  (billboard doctrine) — visual doctrine allows it if telegraphs stay exact.
- Quick Attack across platforms/walls — teleport collision resolution
  equivalence.
- Thunder Jolt on moving/irregular stages (Congo barrel area, Zebes acid
  slopes) — per-stage crawl verification rows when those stages land.

## Source-derived inventory — 2026-09-01

The production generator derives Pikachu from the same BattleShip tables and
O2R inputs as the landed fighters; no runtime-completion claim is implied yet.

- `dFTPikachuData` pins the source `FTAttributes` block at **0x41c**
  (`243_PikachuMain.c`); `llPikachuMainFileID` is **0xf3**.
- Core closure is PikachuMain/MainMotion/Model/ShieldPose/Special1/2/3 with no
  external dependency file.
- **141** local animation files resolve from **0x7a5..0x831**; the complete
  fighter closure is **150 unique NitroFS files**, including **19**
  item-motion files and **2** Event32 animations (Appear1/Appear2).
- `dFTPikachuSpecialStatusDescs` has **18** entries: AppearR/L, Thunder Jolt
  ground/air, Thunder start/loop/hit/end ×2, and Quick Attack start/zip/end ×2.
  The source table's own comment records that its SpecialHi/SpecialLw attack
  IDs are swapped; that is the game's data and is imported as-is.
- The exact source `PikachuModel` O2R is SHA-256
  `12c543dc39b62b7669cc5453d97af142a1af987c4f5a8098814da214e14da9f1`, file
  id 0x155. `dPikachuMain_setup_parts = {0xFFFFFFC0,0}` is a plain 26-bit
  prefix over 27 raw descriptors (JointTrees High `0x2650` / Low `0x5490`),
  the same shape as Fox: **27 live joints including synthetic TopN** and
  **16 drawable bindings** in both details, GX seed/push/pop **1/8/8**.
- Pikachu is the first owner whose source welds adjacent parts in *both*
  details: eleven logical bindings (0,1,2,3,4,7,8,9,10,12,13) are read across
  six root pairs, so the owner needs **11 cross-matrix stores** (palette slots
  16..26). High closes at **317 triangles** (25 cross runs, **130 restores**);
  Low at **197 triangles** (16 cross runs, **106 restores**).
- Pikachu's head root `0x1c40` is the first owner display list to set the
  combiner `(0xfc121605, 0xff17ffff)` — TEXEL0×SHADE colour with
  TEXEL0α×SHADEα alpha. It is pixel-identical to the textured-lit family
  because SHADEα is the vertex alpha, which is 0xff or the raw 0 the port's
  vertex decode already maps to 0xff (the opaque-surface render mode ignores
  pixel alpha, and family 3 has shipped on that identity since Mario). The
  generator canonicalises the pair through `DIRECT_POLICY_COMBINE_ALIASES`
  and proves the identity per aliased triangle, so the runtime's per-run
  combine validation and two-bit family index are untouched.
- Attribute audio (`243_PikachuMain.c`): dead voice 550 / DeadSlam FGM 0x126,
  DeadUp 542, Damage 544, Smash 537/538/539, HeavyGet 548, item-throw scales
  0x64/0x64 — asserted by the FTAttributes normalizer when admitted.

## Acceptance

- [ ] Move inventory sweep vs `ftpikachu` data.
- [ ] Thunder Jolt crawl paths equivalent on Dream Land + each landed stage.
- [ ] Thunder bolt/self-hit semantics equivalent.
- [ ] Quick Attack segment/angle rules equivalent.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.

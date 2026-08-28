# Link — P2-3 fighter 5

Status: source-derived production inventory staged; behavior/article runtime next · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftlink/`

## Role

Two live articles at once (boomerang out + bomb in hand) — the fighter that
forces article ownership to be right. Scheduled adjacent to P2-5 because his
bomb IS an item.

## Moveset uniques

- **Boomerang (B)**: angleable throw, returns to Link along a homing path,
  catch on return, one airborne at a time; hits on both legs of flight.
- **Bombs (Down-B)**: pulls a held bomb *item* (timer fuse, explodes on
  impact/timeout, can be thrown/dropped/caught, hurts Link too). Implement
  through the item system's held-item seam — this is the bridge unit between
  P2-3 and P2-5; if items aren't started yet, land the bomb as the first
  item-system client rather than a bespoke fork.
- **Spin Attack (Up-B)**: multi-hit ground version, weaker air recovery —
  famously poor recovery overall.
- Sword ranged normals with tip semantics; d-air strong spike; wall-jump? no
  (verify — 64 movement quirks belong to source).

## Assets & audio

Sword+shield model (shield is cosmetic passive block on idle? verify the 64
passive shield behavior), 4 costumes, sword swing/chime SFX, voice grunts,
announcer clip.

## DS notes / risks

- Boomerang return-path steering must be equivalent — it's a gameplay tool,
  not VFX.
- Bomb self-damage/ownership rules; bombs surviving Link's KO (verify).
- Two articles + sword trails = draw/effect budget watch.

## Source-derived inventory — 2026-08-28

The production generator now derives Link from the same BattleShip tables and
O2R inputs as the landed fighters; no runtime-completion claim is implied yet.

- `dFTLinkData` pins the source `FTAttributes` block at **0x708**.
- Core closure is LinkMain/MainMotion/Model/ShieldPose/Special1/2/3 plus the
  source external dependency `0x146` (`MiscData326`).
- **144** local animation files resolve from **0x45b..0x4ea**; the complete
  fighter closure is **154 unique NitroFS files**, including **19** item-motion
  files and **2** Event32 animations.
- `dFTLinkSpecialStatusDescs` has **17** entries. The source table owns Jab3 and
  Attack100 start/loop/end, AppearR/L, Spin Attack ground/end/air, boomerang
  ground/get/empty/air/return/empty, and bomb ground/air.
- The exact source `LinkModel` O2R is SHA-256
  `93c9ee108c0e8f1680c35d8d11ec980891850cadcac5eed5bd731c43e85f163e`.
  Its non-prefix `dLinkMain_setup_parts = {0xFFF9FFFE,0}` produces **30 live
  joints including synthetic TopN** and **19 drawable bindings** in both detail
  levels. High is **338 source triangles**, GX seed/push/pop **1/8/8**, six
  cross-matrix stores and 44 restores; Low is **217 triangles** and genuinely
  needs only the source-surviving 11/12 cross pair (**2 stores / 6 restores**).
  The native-owner generator was generalized so Low does not store four unused
  High-detail matrices.
- Neutral-B is a real `WPStruct` owner (`wpLinkBoomerangMakeWeapon`) retained in
  `fp->passive_vars.link.boomerang_gobj` through outbound/return/catch lifecycle.
  Down-B is intentionally different: BattleShip calls `itLinkBombMakeItem`, and
  a held `nITKindLinkBomb` flows through the common light-item throw statuses.
  The DS port must therefore graduate the shared item seam for LinkBomb rather
  than implement a fighter-local bomb object.

## Acceptance

- [ ] Move inventory sweep vs `ftlink` data.
- [ ] Boomerang out/return/catch matrix equivalent.
- [ ] Bomb pull/throw/catch/fuse/self-damage equivalent via item seam.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.

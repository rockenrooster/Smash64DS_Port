# Task 39 Phase A visual-effects census

Mechanical inventory: 97 efManager entry points, 7 efParticle entry points, 4 lbParticle constructors, plus shield and hurt-color seams.

Runtime snapshot: `3ADCF123EF7FE30F873C13596B0966DFA136DFA723A20390F0528C0D635A4A39` with Fox AI off, fast natural-input update 600 / phase 20. Unlisted rows observed zero calls.

| Entry point | Classification | Called | Original | Substitute | Skipped | Ownership evidence |
|---|---|---:|---:|---:|---:|---|
| `efManagerMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:19; src/import/battleship_efmanager.c:168 |
| `efManagerDamageNormalLightMakeEffect` | DS substitute | 1 | 0 | 1 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:25; src/port/reloc_backend_compat_shims.c:7713 |
| `efManagerImpactShockMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:27; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerDamageFireMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:29; src/port/reloc_backend_compat_shims.c:7750 |
| `efManagerDamageElectricMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:30; src/port/reloc_backend_compat_shims.c:7760 |
| `efManagerDamageSlashMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:31; src/port/reloc_backend_compat_shims.c:7776 |
| `efManagerFlameLRMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:32; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerFlameRandomMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:33; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerFlameStaticMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:34; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerDustCollideMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:35; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerShockSmallMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:36; src/import/battleship_efmanager.c:168 |
| `efManagerDustLightMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:38; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerDustHeavyMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:39; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerDustHeavyDoubleMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:41; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerDustExpandLargeMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:42; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerDustExpandSmallMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:43; src/port/reloc_backend_compat_shims.c:7691 |
| `efManagerDustDashMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:44; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerDamageSpawnOrbsMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:47; src/import/battleship_efmanager.c:168 |
| `efManagerDamageSpawnOrbsRandomMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:48; src/port/reloc_backend_compat_shims.c:7790 |
| `efManagerImpactWaveMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:51; src/port/reloc_backend_compat_shims.c:12923 |
| `efManagerImpactAirWaveMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:52; src/import/battleship_efmanager.c:168 |
| `efManagerStarRodSparkMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:54; src/import/battleship_efmanager.c:168 |
| `efManagerDamageSpawnSparksMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:57; src/import/battleship_efmanager.c:168 |
| `efManagerDamageSpawnSparksRandomMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:58; src/port/reloc_backend_compat_shims.c:7797 |
| `efManagerDamageSpawnMDustMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:60; src/import/battleship_efmanager.c:168 |
| `efManagerDamageSpawnMDustRandomMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:61; src/port/reloc_backend_compat_shims.c:7804 |
| `efManagerSparkleWhiteMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:62; src/port/reloc_backend_compat_shims.c:7705 |
| `efManagerSparkleWhiteMultiMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:63; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerSparkleWhiteMultiExplodeMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:64; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerSparkleWhiteScaleMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:65; src/port/reloc_backend_compat_shims.c:7677 |
| `efManagerSparkleWhiteDeadMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:66; src/port/battle_playable_compat_stubs.c:130 |
| `efManagerQuakeMakeEffect` | no-op / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:69; src/port/reloc_backend_compat_shims.c:7633 |
| `efManagerDamageCoinMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:71; src/port/reloc_backend_compat_shims.c:7768 |
| `efManagerSetOffMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:72; src/port/reloc_backend_compat_shims.c:7810 |
| `efManagerFireSparkMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:73; src/import/battleship_efmanager.c:168 |
| `efManagerFoxReflectorMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:76; src/import/battleship_efmanager.c:166 |
| `efManagerYoshiShieldMakeEffect` | no-op / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:81; src/port/reloc_backend_compat_shims.c:1693 |
| `efManagerThunderAmpMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:82; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerRippleMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:83; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerCatchSwirlMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:84; src/port/reloc_backend_compat_shims.c:6036 |
| `efManagerReflectBreakMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:85; src/import/battleship_efmanager.c:168 |
| `efManagerFuraSparkleMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:86; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerPsionicMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:87; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerFlashSmallMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:88; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerFlashMiddleMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:89; src/port/reloc_backend_compat_shims.c:7657 |
| `efManagerFlashLargeMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:90; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerShieldBreakMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:91; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerPikachuThunderShockMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:94; src/import/battleship_efmanager.c:168 |
| `efManagerPikachuThunderTrailMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:97; src/import/battleship_efmanager.c:168 |
| `efManagerPikachuThunderJoltMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:98; src/import/battleship_efmanager.c:168 |
| `efManagerKirbyVulcanJabMakeEffect` | no-op / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:100; src/port/reloc_backend_compat_shims.c:2438 |
| `efManagerSamusGrappleBeamGlowMakeEffect` | no-op / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:101; src/port/reloc_backend_compat_shims.c:2450 |
| `efManagerCaptainFalconKickMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:102; src/import/battleship_efmanager.c:168 |
| `efManagerCaptainFalconPunchMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:103; src/import/battleship_efmanager.c:168 |
| `efManagerKirbyStarMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:104; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerStarSplashMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:105; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerPurinSingMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:106; src/import/battleship_efmanager.c:168 |
| `efManagerDeadExplodeMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:107; src/port/battle_playable_compat_stubs.c:118 |
| `efManagerKirbyCutterUpMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:109; src/import/battleship_efmanager.c:168 |
| `efManagerKirbyCutterDownMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:110; src/import/battleship_efmanager.c:168 |
| `efManagerKirbyCutterDrawMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:111; src/import/battleship_efmanager.c:168 |
| `efManagerKirbyCutterTrailMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:112; src/import/battleship_efmanager.c:168 |
| `efManagerNessPsychicMagnetMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:113; src/import/battleship_efmanager.c:168 |
| `efManagerNessPKThunderTrailMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:116; src/import/battleship_efmanager.c:168 |
| `efManagerNessPKReflectTrailMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:118; src/import/battleship_efmanager.c:168 |
| `efManagerNessPKThunderWaveMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:119; src/import/battleship_efmanager.c:168 |
| `efManagerNessPKFlashMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:120; src/import/battleship_efmanager.c:168 |
| `efManagerLinkEntryWaveMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:121; src/import/battleship_efmanager.c:168 |
| `efManagerLinkEntryBeamMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:122; src/import/battleship_efmanager.c:168 |
| `efManagerKirbyEntryStarMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:123; src/import/battleship_efmanager.c:168 |
| `efManagerMBallRaysMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:124; src/import/battleship_efmanager.c:168 |
| `efManagerMBallThrownMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:126; src/import/battleship_efmanager.c:168 |
| `efManagerFireGrindMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:127; src/port/reloc_backend_compat_shims.c:7698 |
| `efManagerHealSparklesMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:128; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerYoshiEntryEggMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:129; src/import/battleship_efmanager.c:168 |
| `efManagerYoshiEggLayMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:132; src/import/battleship_efmanager.c:168 |
| `efManagerYoshiEggEscapeMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:133; src/import/battleship_efmanager.c:168 |
| `efManagerFoxBlasterGlowMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:136; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerLinkSpinAttackMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:137; src/import/battleship_efmanager.c:168 |
| `efManagerDonkeyEntryTaruMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:138; src/import/battleship_efmanager.c:168 |
| `efManagerSamusEntryPointMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:139; src/import/battleship_efmanager.c:168 |
| `efManagerCaptainEntryCarMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:141; src/import/battleship_efmanager.c:168 |
| `efManagerMarioEntryDokanMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:142; src/import/battleship_efmanager.c:168 |
| `efManagerFoxEntryArwingMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:144; src/import/battleship_efmanager.c:168 |
| `efManagerStockSnapMakeEffect` | no-op / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:146; src/port/battle_playable_compat_stubs.c:221 |
| `efManagerStockStealStartMakeEffect` | no-op / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:147; src/port/battle_playable_compat_stubs.c:230 |
| `efManagerStockStealEndMakeEffect` | no-op / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:148; src/port/battle_playable_compat_stubs.c:239 |
| `efManagerMusicNoteMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:149; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerYoshiEggExplodeMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:150; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerCaptureKirbyStarMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:152; src/import/battleship_efmanager.c:168 |
| `efManagerLoseKirbyStarMakeEffect` | original imported | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:154; src/import/battleship_efmanager.c:168 |
| `efManagerRebirthHaloMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:155; src/port/battle_playable_compat_stubs.c:141 |
| `efManagerBattleScoreMakeEffect` | no-op / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:156; src/port/battle_playable_compat_stubs.c:248 |
| `efManagerEggBreakMakeEffect` | no-op / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:157; src/port/reloc_backend_compat_shims.c:1699 |
| `efManagerKirbyInhaleWindMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:159; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerItemSpawnSwirlMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:161; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efManagerConfettiMakeEffect` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.h:162; src/import/battleship_efmanager.c:168; src/port/reloc_backend_compat_shims.c:12870 |
| `efParticleInitAll` | particle control shim | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efparticle.h:11; src/port/reloc_backend_compat_shims.c:12730 |
| `efParticleGObjSetSkipID` | particle control shim | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efparticle.h:12; no port definition (unreached) |
| `efParticleGObjSetSkipAll` | particle control shim | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efparticle.h:13; src/port/battle_playable_compat_stubs.c:261 |
| `efParticleGObjClearSkipID` | particle control shim | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efparticle.h:14; src/port/battle_playable_compat_stubs.c:271 |
| `efParticleGObjClearSkipAll` | particle control shim | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efparticle.h:15; no port definition (unreached) |
| `efParticleGetBankID` | particle control shim | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efparticle.h:16; src/port/reloc_backend_compat_shims.c:12754 |
| `efParticleGetLoadBankID` | particle control shim | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efparticle.h:17; src/port/reloc_backend_compat_shims.c:12735 |
| `lbParticleMakeScriptID` | particle-shimmed / skipped | 2 | 0 | 0 | 2 | include/ef/effect.h:117; src/port/reloc_backend_compat_shims.c:12870 |
| `lbParticleMakeCommon` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | include/ef/effect.h:118; src/port/reloc_backend_compat_shims.c:12955 |
| `lbParticleMakePosVel` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | include/ef/effect.h:119; src/port/reloc_backend_compat_shims.c:12966 |
| `lbParticleMakeGenerator` | particle-shimmed / skipped | 0 | 0 | 0 | 0 | include/ef/effect.h:122; src/port/reloc_backend_compat_shims.c:12944 |
| `efManagerDamageNormalHeavyMakeEffect` | DS substitute | 0 | 0 | 0 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.c:2197; src/port/reloc_backend_compat_shims.c:7733 |
| `efManagerShieldMakeEffect` | DS substitute | 1 | 0 | 1 | 0 | decomp/BattleShip-main/decomp/src/ef/efmanager.c:4119; src/port/reloc_backend_compat_shims.c:1664 |
| `ftParamCheckSetFighterColAnimID` | source color state; DS draw adapter | 1 | 1 | 0 | 0 | decomp/BattleShip-main/decomp/src/gm/gmcolscripts.c:48; src/port/reloc_backend_compat_shims.c:852 |

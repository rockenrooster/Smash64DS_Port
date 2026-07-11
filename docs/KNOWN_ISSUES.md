# Known Issues

## Local Tooling Issues

- `P1Gate` is an additive shadow checkpoint, not a P1-completion claim. Its
  lifecycle leg uses one minute on the original expiry/Results path; a dated
  canonical five-minute soak is still required. Boundary, Regression, and Full
  profile memberships remain unchanged, and the historical harness fleet
  remains diagnostic while unique assertions are migrated.
- [coverage-reduced] `FastIteration` lowers only the moving left-shrub and pond
  variation floors from `50%/35%` to `40%/30%`. The default realtime gate stays
  strict, both flat-run caps remain `16px/60px`, and GDB still hard-proves both
  source-selected fighter display contracts before capture.
- [coverage-reduced] Realtime capture no longer requires Mario/Fox to remain in
  historical fixed color crops during live combat. The preceding GDB pass still
  requires both selected/submitted display contracts, in-bounds geometry, and
  fighter triangles; scene/object/texture regions remain visual gates.
- [coverage-reduced] P1Gate's compact opening leg omits the legacy exact symbol/
  event stack and terminal `<279` tick bound. Current normal runs reach Title
  with relocation symbol count `45` versus the untouched verifier's `43`, and
  sometimes after the first asset event; use that exact verifier only to
  localize the historical mismatch. Scene/taskman/reloc-failure/Title outcomes
  remain hard gates.
- Three user-facing persistent ROM filenames represent two unique configurations:
  normal `smash64ds.nds`, and one canonical battle configuration copied to both
  canonical and shipped names and checked by exact length/SHA-256 parity.
  Realtime, supplemental scripted battle, and lifecycle mode-163 scenarios are
  still compile-distinct builds; runtime scenario selection remains tooling
  debt.
- The Full verifier profile is large enough to time out under short command
  limits. This is a workflow/runtime-cost issue, not a source-boundary failure.
  Use `scripts/verify-dev-fast.ps1`, `scripts/verify-boundary.ps1`,
  `RegressionCore`, and detached `scripts/build-verify-profile.ps1` prebuilds
  with `-VerifyStamp` for normal iteration. Run or resume
  `scripts/verify-all.ps1 -Profile Full` when change risk requires it, and
  report timeouts honestly instead of claiming Full green.
- RegressionCore's two shared-slot mode switches still took `477.31s` and
  `474.68s` after their first slot builds, rather than rebuilding only the
  harness-aware objects. Investigate that invalidation in the next tooling
  slice; it is build-cost debt, not a runtime correctness blocker.
- Snapshots created without `scripts/New-Smash64DSSnapshot.ps1 -Mode Lean` can
  include hundreds of MB of generated build directories, root ROM/ELF outputs,
  artifacts, emulator payloads, and GDB scratch files.
- Local emulator binaries/configs now live under `emulators/`. melonDS remains
  the automated GDB/runtime verifier. no$gba is available through
  `scripts/debug-nogba.ps1` for interactive DS hardware, VRAM, OAM, palette,
  and timing inspection, and through `scripts/verify-nogba-smoke.ps1` for a
  launch/window-capture smoke check. It has no automated runtime-global or
  renderer-correctness verifier yet. The debugger build's window layout is a
  local setting: the current machine is configured for one combined debug
  window, while other settings can expose separate debugger and emulator
  windows. `scripts/capture-nogba.ps1 -AllWindows` handles both layouts.
- melonDS 1.1 previously started in this Codex desktop session as a live process
  with a hidden, untitled top-level window and no ARM9 GDB listener on `3333`.
  That reproduced with `smash64ds.nds`, the local `sm64-nds` ROM, and the
  devkitPro `Simple_Tri.nds` sample, so if it recurs, treat it as an
  emulator/session launch issue before changing ROM code. The verifier scripts
  now write melonDS's `Enable = true` compatibility key in addition to
  `Enabled = true`, without duplicating either key in `[Instance0.Gdb]`. A
  duplicate `Enable` / `Enabled` key in that TOML section can prevent the ARM9
  listener from opening and produce a misleading GDB connection timeout. The
  capture script temporarily disables true GDB keys for visible capture and
  restores the original config afterward. The current material-branch run
  verified GDB and window capture successfully.
- The current natural opening movie-to-Title visual appears after the default
  startup/Opening Room capture window. Use
  `scripts/capture-melonds.ps1 -Build -DelaySeconds 145` when the goal is to
  capture the Title preview after the paced action bridge. Shorter captures may
  correctly show only startup, Opening Room, portrait cards, name-card scenes,
  or one of the action-scene preview windows.
- Visual-debugging polish is now lower priority than moving the original source
  boundary forward. Keep captures and HUD counters as regression evidence, but
  do not spend the next milestone on visual fidelity unless it blocks a
  source-driven scene/menu import.

## Active Stub Boundaries

- Full `gm/gmcollision.c` is imported through
  `src/import/battleship_gmcollision.c`, replacing the local
  `gmCollisionGetFighterPartsWorldPosition`, `func_ovl2_800EDA0C`, and
  `gmCollisionGetWorldPosition` copies. `FTStruct` now has a BattleShip-layout
  source region through `display_mode` (`coll_data=120`,
  `motion_attack_id=648`, `attack_colls=660`, `joints=2280`, callbacks at
  `2516+`, source-region size `2896`) and the port-only DS/proof extension
  begins at offset `2896`. Full `ft/ftmain.c` is imported by default through
  `src/import/battleship_ftmain.c` and passes the init/wait/dash-run ladder,
  boundary, continuous live-hit verifier, and four-way sharded Regression after
  routing the public `ftMain*` entry points through imported BattleShip code
  plus bounded diagnostics.
- The imported `ftMainSetStatus` path still contains one deliberate
  duplicate-behavior seam: stage compat replay in the imported post-hook.
  The old cliffmotion restore hook in `src/import/battleship_ftmain.c` was
  deleted after direct and menu-chain cliff-family Regression stayed green.
  Natural combat now covers Wait, Walk, Dash, Run, RunBrake, Turn/TurnRun,
  Attack11, common damage, GuardOn, Guard, and GuardOff without the compat
  replay early-return path, but older stage proofs still depend on scoped
  hooks. Delete the remaining hook status-by-status only after matching natural
  proofs are green.
- `ft/ftdata.c` is imported, but its particle ROM banks are link stubs in
  `src/port/reloc_backend_ftdata_stubs.c` because the current DS O2R manifest
  has no particle bank assets. The generated
  `src/port/reloc_backend_ftdata_symbols.c` also lists every upstream-stubbed
  fighter submotion `ll*` token that BattleShip's US reloc symbol table leaves
  at zero. Mario/Fox manager payloads load for the fenced `ftmanager.c` proof;
  non-Mario/Fox broader fighter payloads may still fail-soft until future
  slices import their active runtime.
- Full `ftmanager.c`, original common/Mario/Fox status descriptor tables, and
  live `ftanim.c`/`ftanimend.c`/`ftkey.c` are now default runtime. Mario/Fox
  manager payloads load through `lbRelocGetStatusBufferFile`, fighters are
  created through original `ftmanager.c`, and modes `39/40`, `53/54`, and
  `161/162` now prove natural movement, Attack11 hitbox spawn, live
  hit/damage/recover, and guard through imported animation/key/status/main/
  collision runtime.
  Inactive statuses whose TUs pull HUD, items/weapons, stage hazards, other
  fighters, or Mario/Fox special weapon/effect chains still use documented weak
  no-op callbacks in `src/import/battleship_ftstatus_inactive_stubs.c`; delete
  those stubs status-by-status as the owning original TUs and assets are
  imported.
- Full `ft/ftcomputer.c` is imported by default. Canonical mode `163` configures
  Mario human versus Fox level-3 CPU and the fast natural gate proves target
  acquisition, movement, attack/live hitboxes, defense, and damage through the
  source AI. The fixed proof did not naturally select the Recover objective, so
  offstage recovery remains coverage debt rather than a synthetic claim.
  `grHyruleTwisterCheckGetPosition` and the two `grJungleTaruCann*` helpers are
  weak fail-soft boundaries until those stage runtimes are imported; they are
  unreachable in the current items-off Dream Land P1 match.
- Full `ft/ftparam.c` remains deferred. Its two fighter-part transform cache
  invalidators are now source-shaped in the narrow compatibility layer
  (`ftparam.c:2161-2349`), replacing no-op stubs; mode `163` proves the
  resulting joint world position with a natural fireball/reflector cycle.
- Default `NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL=1`,
  `NDS_IMPORT_BATTLESHIP_FOX_BLASTER=1`, original `efmanager.c`, and Fox
  `ftfoxspeciallw.c` import the natural projectile/effect/reflector path for
  mode `163`. The proof drives B input through original status tables and
  motion commands, loads `EFCommonEffects1/2/3`, then proves a live Mario
  fireball reflected by Fox's reflector through imported `ftmain`/`wpmain`
  code. Heavy map adjustment, display-scale, common particle/glow visuals,
  broader projectile victim-damage, shield, rebound, and free-flight proofs
  remain follow-up.
- Mario Super Jump Punch, Mario Tornado, and Fox Fire Fox are default through
  original `ftmariospecialhi.c`, `ftmariospeciallw.c`, `ftfoxspecialhi.c`, and
  `ftcommonfallspecial.c`. The fall-special public reaction call remains a
  weak no-op until `ftpublic.c`/audio reaction behavior is imported
  (`ftcommonfallspecial.c:96`, `ftpublic.c:261`).
- Common particle script/texture banks remain non-resident for now; particle
  calls stay on diagnostic/no-op shims until a dedicated particle asset gate
  budgets or streams the 326 KiB bank pair.
- `battle_playable` is default for original `gm/gmcamera.c`,
  `ftcommondead.c`, `ftcommonrebirth.c`, battle-critical `if/ifcommon.c` HUD
  paths, and original `if/ifscreenflash.c`. Mode `163` now proves natural
  attack/damage -> KO -> stock decrement -> Rebirth -> Wait, rendered percent
  digits, stock icon decrement, and a hardware stage/fighter frame. The
  live-input path now also runs the original five-minute timer, Time Up/end
  interface, taskman return, scoring check, and `VSBattle -> VSResults` scene.
  The automated lifecycle gate uses a one-minute harness limit for iteration;
  canonical/manual match length remains independently configurable.
  Original `mnvsresults.c`, `lbtransition.c`, and its subsystem fighter/data
  support now run by default with all eight source files and source Win/Lose
  statuses. The DS compositor preserves source 2D layers around the fighter
  camera, but exact per-SObj RDP/camera interleave remains follow-up.
- Imported `gr/grwallpaper.c` owns the live Pupupu wallpaper. Its 1P Training
  and Boss wallpaper-loader calls remain weak no-ops, and the Bonus3 fill DL
  initializer remains an unreachable zero placeholder, until those separate
  scenes and assets are imported.
- The imported `scsubsysdata.c` opening-fighter callback is redirected to a
  narrow no-op because Opening Room actor behavior is outside the Results
  slice. Results data tables and fighter status dispatch remain original.
- [coverage-reduced] Deleted legacy standalone modes `57/58`
  (`battle_mariofox_gcdrawall_loop` /
  `menu_chain_mariofox_gcdrawall_loop`). Active stage gcDrawAll, natural-combat
  Boundary, and `battle_playable` retain live scene coverage; the old
  standalone moving-preview marker stack is not reproduced.
- [coverage-reduced] Deleted legacy selected Fox Jab2 live-hit modes `159/160`.
  Modes `161/162` and `163` prove natural Attack11 hitbox, hit/damage/recover,
  guard, KO, and HUD stock updates through the original manager/runtime; the
  older synthetic source-order/private-hitlog marker stack is not reproduced.
- [coverage-reduced] Stage gcDrawAll/collision/floor/MP smoke modes now assert
  the stage-side original-manager/animation/ground mask `0x24f` plus stage and
  fighter hardware submission. The 300-frame Wait, Walk, live-hit, and full
  combat ownership stays with modes `39/40`, `161/162`, and `163`.
- Default ftmain verifier coverage is reduced in these follow-up areas until the
  imported-original path exposes direct observations for every marker bit:
  `ftMainProcParams` masks skip shield-damage, shield-break, and
  damage-status-setup bits; damage setup masks skip status, expire, and
  sleep-status bits; GuardOn/Guard/GuardOff state masks skip diagnostic bits
  `0x000cc000`; colanim-update requires only restore; common damage callbacks
  skip `NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE` and
  `NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE_ORIGINAL`; catch-resist skips the
  original mirror bit; damage-kind skips the Twister procparams mirror; sleep
  skips the motion mirror. Modes `161/162` now prove a natural Attack11 hit and
  damage/recover cycle, but the deleted selected live-hit private hitlog mirrors
  from modes `159/160` are no longer reproduced from BattleShip's private
  static storage.
- Renderer stage 3b proves opt-in DS hardware triangles, source-shaped
  billboard/recalc DObj matrix seed coverage, first bounded Opening Room
  RGBA16/I16 texture upload, and stage-inclusive Pupupu hardware draw, but it is
  not a full renderer cutover. Stage-inclusive `gcDrawAll` hardware remains the
  current default-manager gate and uses the BattleShip battle-camera `0x4C`
  matrix seed.
  BattleShip
  `gSPMvpRecalc` / `G_MW_MATRIX` display-list
  streams are now decoded when emitted, fighter-parts matrix kind `0x4B` has a
  source-backed seed path, cached fighter-parts `Mtx44f` seeds use fixed-W
  conversion semantics matching `syMatrixF2LFixedW`, and selected DObj parent
  chains compose root to child. The hardware texture upload path now handles
  `LOADTLUT`, CI4/CI8 palette conversion, IA4/IA8/IA16, I4/I8/I16, and
  RGBA32 conversion for source material records; the opt-in hardware path applies
  recorded primitive/environment material color and alpha from the current
  combine state, forces opaque DS poly alpha for source opaque render modes,
  maps recorded F3DEX2 front/back cull geometry mode to DS polygon cull bits,
  applies the sm64-nds decal-combine rule to DS `POLY_DECAL`, and applies its
  texture-filter coordinate-bias rule. After original-manager graduation, the
  strict direct Mario/Fox all-DL hardware verifier is green on live-manager
  fighters by preserving source-equivalent segment `0xE` material state plus
  RSP vertex/render state across selected DObjs, attaching original
  fighter-part MObjs, and seeding missing direct-list CI TLUT state from the
  current material palette. The same opt-in stage `gcDrawAll` hardware replay
  now submits both selected manager-created fighters with the Pupupu stage on
  direct and menu-chain routes; adjacent stage-collision scenes reuse that
  stage replay. Remaining renderer work is broader combiner/material/depth/
  texture source-scene coverage and cutover policy.
- Live-hit status lifecycle modes `161/162` now prove a natural original-
  manager cycle: Fox reaches Attack11 from controller A input, imported
  motion-command runtime spawns the hitbox, imported `ftmain.c`/`gmcollision.c`
  search hits Mario, both fighters enter hitlag, Mario installs a common damage
  status through the original tables, takes damage/knockback, recovers to Wait,
  and then Fox runs GuardOn -> Guard -> GuardOff. This replaces the former
  selected Fox Jab2 status-loop verifier for the Boundary/Latest pair. The old
  selected Fox Jab2 modes `159/160` were deleted as coverage-reduced legacy
  proof. Continuous multi-hitbox runtime, arbitrary damage-state duration,
  items/weapons, audio, and unbounded gameplay scheduling remain deferred.
  The current menu-chain verifier needs `-DelaySeconds 3` so post-loop
  finalizer markers are captured after the longer VS Mode -> PlayersVS -> Maps
  -> VSBattle path.
- PassiveStand/Passive recover-loop modes `155/156` prove guarded stable
  update/physics/map frames for PassiveStandF/Ground, PassiveStandB/Ground,
  and Passive/Ground, followed by the original `ftAnimEndSetWait` handoff into
  Wait/Ground. They also prove the bounded original
  PassiveStandF/PassiveStandB/Passive input gates and the neutral/expired-Z
  no-transition cases, plus a bounded imported `ftCommonDamageFallProcMap`
  path through source-order MP floor collision that selects PassiveStandF and
  Passive. This is still a bounded recovery proof. Fully live collision-driven
  recovery selection from
  unseeded collision frames, arbitrary recovery durations beyond the guarded window,
  hitlag/damage interaction, full player-driven recovery runtime, and broader
  fighter gameplay remain deferred. The current menu-chain verifier needs
  `-DelaySeconds 3` so post-loop finalizer markers are captured after the
  longer VS Mode -> PlayersVS -> Maps -> VSBattle path.
- CliffAttack/Common2 and CliffEscape/Common2 are now folded into current
  modes `155/156` with delayed aggregate reseeding and isolated shared
  Common2 bridge diagnostics. Continuous natural ledge attack/escape runtime
  remains deferred.
- The `dashRun=0x3fffffff hitboxPos=0x1ffff damageStatus=0x1f damageSetup=0xffffffff` marker now attached to modes
  `155/156` is an aggregate regression guard for older bounded Dash-Run
  attack/guard status proofs plus the selected Fox Jab2 hitbox
  position/range/rectangle/collide/record/hit-log/SFX/stats/proc-params and
  damage-status selector/setup/proc-passive invincible and electric-status dispatch/sleep-element imported FuraSleep setup/update handoff/physics/flyroll/knockback-invincible/lag-update/hitlag-lifecycle/air-map/interrupt/expiry/fall-physics/fastfall/map-floor-cliff/hammer-interrupt/setup-tail proofs, plus the first source-shaped `ftCommonDamageUpdateMain` catch-resist/keep-hold branch, sibling catch-resist release/lose-grip branch, catch-side and capture-side non-resist keep-hold stats branches, catch-side non-resist zero-knockback damage-release branch, first capture keep-hold branch, capture-side keep-hold false lose-grip branch, capture-side keep-hold false zero-knockback no-damage release branch, zero-knockback catch branch, capture zero-knockback branch, no-grab/no-capture tail colanim and damage-status branches, and DK-family heavy-item catch-resist/drop branches. The selected source-order shield-contact gate summarized by `shc=0x7fffff/3142` is retired with modes `159/160`. It keeps Run -> TurnRun -> Run,
  Attack11/Attack12/Mario Attack13/Fox Attack100, AttackDash,
  GuardOn/Guard/GuardOff/GuardSetOff, and EscapeF/EscapeB callback/update
  slices covered by the current boundary, but it does not enable full attack
  hitboxes, natural multi-slot hurtbox runtime beyond the bounded source-order
  scan, continuous rehit gameplay beyond
  the selected Link down-air timer window, full damage
  damage/hitlag lifecycle, continuous `ftCommonDamageUpdateMain` runtime, continuous multi-frame FuraSleep breakout scheduling and real color-animation runtime, real DS audio, positional audio balance, continuous
  TurnRun/Attack100/Guard/SetOff runtime, continuous shield collision beyond
  the selected contact/set-off proof, or player-driven attack/shield gameplay.
  This aggregate is historical; Regression no longer consumes modes `159/160`
  or older selected diagnostics.
- The standalone Jump-loop `JUMP_ATTACKAIR` refresh proof now carries and
  clears the original four-slot fighter attack records for the bounded
  AttackAirLw rehit branch and proves the installed original AttackAir map
  callback's smooth LandingAirN, missing-animation LandingAirNull,
  skip-landing Wait, and plain LandingLight handoffs through proof-local
  restored seeds (`map=0x3ff`). It also proves the original AttackAirF/B/Hi/Lw
  directional selector and Link down-air callback setup with `dir=0x1f`.
  The Dash proof now also has a selected
  `MakeAttackColl` `New -> Transfer -> Interpolate` position/matrix proof plus
  broad-phase fighter-range, selected rectangle, and selected attack/damage
  collision-decision proofs on `FTStruct.attack_colls[1]`, selected
  damage-record insertion, selected normal-hit front-half damage/hit-log
  bookkeeping, selected hit-collision FGM table/SFX seam, and selected
  source-shaped `ftMainProcParams` damage/hitlag scheduling, including
  selected attacker `attack_damage` / `proc_hit` and `attack_shield_push` /
  `proc_shield` branches plus selected victim `shield_damage` -> GuardSetOff,
  shield-break -> ShieldBreakFly, reflect-damage break, Fox reflector hit, Ness
  reflector sound, Ness absorb branches, a bounded original-compatible
  `ftcommondamage.c` damage-status selector, and a guarded damage-status
  setup/update tick, but full continuous multi-hitbox runtime is still not
  enabled. Natural rehit timers, full damage
  status lifecycle, effects, real DS audio, real stale-queue behavior,
  exact Fox/Ness special runtimes, and full fighter/item/weapon attack-record
  interaction remain deferred.
- The direct/menu-chain Mario/Fox init proof now has a bounded
  `FTDamageColl[11]` state shell with every valid source descriptor slot copied
  from the real loaded `attr->damage_coll_descs[]`: Mario `10`, Fox `11`, plus
  a bounded `FTParts` transform shell for the selected slot-0 joint. This only
  proves storage, hit-status propagation, source descriptor half-size copy
  shape, matrix/world-position consistency, and scale availability. Modes
  `159/160` now add a bounded source-order hurtbox scan that proves the global
  hitstatus and damage-detect gates, skips slots `0` and `1`, misses slot `2`,
  hits slot `3`, observes
  Mario's slot-`10` `None` sentinel, and feeds that selected slot into bounded
  damage bookkeeping. They also prove the selected attack-record detect gate
  allows an empty record and rejects hurt/shield/nondefault-group records
  before range/hurtbox work. Natural continuous multi-slot
  victim runtime remains deferred.
- The wall-hit floor proof now attached to modes `155/156` is an isolated
  Hyrule scout proof. It validates the source-order MP wall-line/floor-edge
  relationship for the selected wall-hit probe. The current wall-copy proof
  adds one bounded source-order copyback pass for that same probe, and the
  WallDamage proof adds one bounded original status/update handoff
  `56/49 -> 57/50`, but it does not yet make natural live wall collision,
  arbitrary wall copyback, wall teching, or full collision response part of
  unbounded gameplay.
- The pass-through proof now attached to modes `155/156` is a bounded collision
  route plus natural input proof. It proves `ignore_line_id` same-line
  rejection, different-line pass-callback acceptance, and one down-input
  Wait -> Squat -> Pass route on Dream Land line `0`, but does not yet make
  moving platform pass-through or continuous platform gameplay live.
- The platform-floor proof now attached to modes `155/156` is a bounded active
  yakumono predicate proof. It checks BattleShip's platform predicate for the
  selected pass-through line, installs the bounded active DObj, and now proves
  one guarded update tic keeps the predicate active plus one
  `mpCollisionSetYakumonoPosID` position/speed primitive. It also proves the
  bounded `mpCollisionGetSpeedLineID` reader and selected dynamic
  floor/ceil/wall consumers for that speed. Continuous platform movement,
  unbounded live speed transfer, and continuous platform gameplay remain
  deferred.
- Inishie/Mushroom Kingdom scale source setup is now required by current modes
  `155/156` and remains standalone regression coverage in modes `153/154`,
  while normal boot and older harnesses still default to the non-source path.
  The current-boundary builds stage read-only
  `StageInishieFile3` file `155.vpk0.bin` into NitroFS, validate the raw size
  plus DObjDesc/map-head-DL/ScaleRetract offsets, convert DObjDesc scalar
  fields, and run original `grInishieMakeScale` through bounded scale-anchor
  map-object stubs. The narrow scale DL/Vtx slices are converted to native word
  order and patched into the DObj/map-head pointers. The four source-created
  scale DObjs prove expected display callbacks and clean renderer scans
  (`sourceDL=0xff`, `91` commands, `20` triangles), expose texture/material
  command state (`tex=0x3f`), and commit a bounded software source-DL preview
  (`preview=0x3f`, `432` pixels). The latest bounded threshold probe also
  forces original `grInishieScaleProcUpdate` through
  `Wait -> Fall -> Sleep -> Retract -> Wait` (`fall=1->2/0`,
  `step=3->0/0`) with two sparkle calls, four fall-phase yakumono position
  writes, four retract-phase position writes, two platform re-enable calls,
  final Fall platform Y `1460000/-741000`, Fall speed `-3000/-3000`,
  retracted altitude `0`, and restored platform Y `363000/362000`. Sleep
  happens immediately because the proof-owned ground/deadzone state trips the
  original Fall branch's lower-bound check. The remaining issue is full texture
  upload, material application, continuous scale runtime, and hardware-backed
  Mushroom Kingdom rendering, not the bounded source-DL visibility or threshold
  proof.
- The NDS dev/test scene harness is a build-time diagnostic entry point, not a
  replacement scene system. `NDS_DEV_SCENE_HARNESS=title` starts from
  `nSCKindTitle` by pre-seeding `dSCManagerDefaultSceneData` before imported
  `scManagerRunLoop` copies it, then runs the bounded imported Title path.
  `NDS_DEV_SCENE_HARNESS=vs_setup` now starts from `nSCKindVSMode` with
  `scene_prev = nSCKindTitle`, runs bounded imported `mnvsmode.c` setup,
  loads original `MNCommon`/`MNVSMode`, creates the original VS setup
  GObj/camera/SObj graph, and parks before `mnVSModeMain` input/update
  transitions and continuous drawing. `NDS_DEV_SCENE_HARNESS=vs_start_transition`
  runs the same setup, proves the original VS Start -> PlayersVS state change,
  and parks at the bounded imported PlayersVS setup boundary.
  `NDS_DEV_SCENE_HARNESS=players_setup` imports bounded original
  `mnplayersvs.c` setup but does not run full interactive character-select
  navigation, fighter actor runtime, fighter rendering, audio, or continuous
  drawing. The PlayersVS ready/start transition proof uses deterministic
  two-player ready-state injection to prove original proceed behavior.
  `NDS_DEV_SCENE_HARNESS=maps_setup` imports bounded original `mnmaps.c` setup,
  now un-defers the Pupupu/Dream Land preview path, loads the real Pupupu stage
  O2R dependency chain, and proves external fixups plus preview diagnostics.
  The Maps A-select proof is still bounded to original stage-data saving and
  the scene request to VSBattle.
  `NDS_DEV_SCENE_HARNESS=menu_chain_vsbattle` proves VS Mode -> PlayersVS ->
  Maps -> imported bounded VSBattle setup with Pupupu stage-data adoption.
  `NDS_DEV_SCENE_HARNESS=battle_fd` starts directly at the same bounded
  VSBattle setup with one seeded Mario and the current Final Destination
  sentinel. `NDS_DEV_SCENE_HARNESS=battle_pupupu_stage` starts directly at
  bounded VSBattle with two seeded players and real Pupupu `MPGroundData`.
  `NDS_DEV_SCENE_HARNESS=battle_pupupu_update` starts from that same direct
  Pupupu setup and proves two guarded original `grPupupuProcUpdate` ticks.
  `NDS_DEV_SCENE_HARNESS=menu_chain_pupupu_update` proves the same update after
  the guarded VS Mode -> PlayersVS -> Maps -> VSBattle path.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_model` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_model` prove the direct and
  menu-chain Mario/Fox model boundary by loading `FTManagerCommon`, Mario, Fox,
  and required external fighter O2R dependencies and creating asset-backed
  fighter model GObjs. `NDS_DEV_SCENE_HARNESS=battle_mariofox_struct` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_struct` extend those paths by
  attaching persistent project-owned `FTStruct` shells to the real Mario/Fox
  GObjs and proving `ftGetStruct` returns those pool objects.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_init` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_init` extend the struct-backed
  paths with a bounded source-order Mario/Fox init-state proof for damage,
  shield, velocities, collision contracts, floor projection, passive vars, and
  guarded compatibility-call diagnostics while fighter status/process/display/
  gameplay stays parked.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_wait` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait` import original
  `ftcommonwait.c` and prove original `ftCommonWaitSetStatus` through a
  Wait-only project-owned `ftMainSetStatus` seam for initialized Mario/Fox
  structs. They install status `10`, motion `4`, animation frame `0`, speed
  `1.0`, player tag wait `120`, special interrupt `TRUE`, and callback
  pointers, but still do not execute fighter process/update/physics/gameplay or
  full display traversal. `NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_tick`
  and `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_tick` add one bounded
  original `ftCommonWaitProcInterrupt` callback tick plus guarded physics/map
  seam calls for both fighters, but still keep real fighter update loops,
  unbounded physics/map mutation, hit/catch/search runtime, shadows, full
  display traversal, and gameplay parked.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_ground` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_ground` add a second
  controlled pass that seeds ground velocity, mirrors original ground friction
  and safe air-transfer order, and proves the safe floor branch of the map
  seam without Fall/Ottotto or process/display/gameplay escape. This is not
  continuous fighter physics or collision runtime.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_display_probe` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_display_probe` add direct and
  menu-chain metadata-only display callback probes. They record Mario/Fox DObj
  counts, current zero MObj/AObj counts, and display-list candidate counts, but
  do not render fighters or enter full fighter display traversal.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_scan` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_scan` add parser-only scans of
  the first selected Mario/Fox fighter display lists through
  `ndsRendererScanDisplayList()`. Those DLs are currently taskman-arena copied
  original data (`0xfffffffe` ownership sentinel), not registered loaded-file
  pointers. The current scans complete with blocker `0/0` and zero unsupported
  opcodes after the renderer adapter records known benign state/culling/image/
  TLUT commands instead of treating them as fatal blockers.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_execute` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_execute` add decode-only
  `ndsRendererExecuteDisplayList()` proofs for the same selected DLs. They
  decode real vertices and triangles, but this is still not fighter rendering.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_draw` add bounded software
  preview drawing for those same selected first DLs. They prove nonzero
  retained `96x72` preview pixels for Mario and Fox, but still do not implement
  full fighter rendering.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_walk_input` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_walk_input` import original
  `ftcommonwalk.c` and prove one bounded original Wait -> Walk input/status
  transition, one Walk interrupt callback, one velocity-generation pass, and
  one safe map pass. They do not make fighters playable yet. Continuous fighter
  process scheduling, root-position integration outside bounded proof helpers,
  jump/attack/special/guard/catch paths, hit/catch/search, items, HUD, audio,
  full collision, and full imported `ftmain.c` remain deferred.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_walk_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_walk_loop` add only a bounded
  synthetic-input movement proof on top of the Walk-input boundary. They run
  four held Walk frames, integrate root X through `physics.vel_air`, release to
  Wait through original Walk interrupt logic, and run one Wait friction/map
  settle frame. They do not make fighters fully playable: continuous fighter
  process scheduling and real DS controller input remain deferred.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_dash_run` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dash_run` add only a bounded
  synthetic-input original Dash -> Run -> RunBrake movement proof on top of the
  Walk-loop boundary. They import `ftcommondash.c`, `ftcommonrun.c`, and
  `ftcommonrunbrake.c`, run bounded original attack status/callback proofs
  from Wait/Run, and now run a bounded original `ftcommonguard1.c` /
  `ftcommonguard2.c` GuardOn status/callback/update proof from Wait before
  restoring the movement path and now prove the bounded animation-end handoff
  into Guard status `153`, one Guard hold update tick, and one release tick
  into GuardOff status/motion `154/135`, and one GuardOff completion back to
  Wait/Ground. They also prove a bounded original GuardSetOff branch where
  held Z returns to Guard and released Z returns to GuardOff, then import
  `ftcommonescape.c` for a bounded original Guard -> EscapeF/EscapeB
  status/update/Wait-handoff proof. They still do not make
  fighters fully playable:
  continuous fighter process scheduling, real DS controller input,
  turn/jump/squat/special/catch transitions, continuous Guard hold,
  continuous SetOff/Escape shield runtime, attack hitboxes/item branches/
  animation runtime, full
  collision/ledge logic, jostle, hit/catch/search, items, HUD, audio, and full
  imported `ftmain.c` remain deferred.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_jump_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_jump_loop` add a bounded
  synthetic-input original RunBrake -> Wait closeout and Wait -> KneeBend ->
  JumpF airborne ascent proof only. They import `ftcommonkneebend.c` and
  `ftcommonjump.c`.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_landing_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_landing_loop` add only a bounded
  synthetic-input original JumpF -> Fall -> LandingLight -> Wait proof on top
  of that boundary. They import `ftcommonfall.c` and `ftcommonlanding.c`, but
  still do not make fighters fully playable: full collision line tracing,
  platform pass-through, ledges, ceiling bonks, FallAerial, LandingHeavy,
  JumpAerial/double jump, aerial attacks/specials, continuous fighter process
  scheduling, real DS controller input, hit/catch/search, items, HUD, audio,
  and full imported `ftmain.c` remain deferred.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_process_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_process_loop` add a bounded
  scripted source-order fighter frame loop on top of that boundary. They prove
  Wait -> Walk -> Wait, Wait -> Dash -> Run -> RunBrake -> Wait, and Wait ->
  KneeBend -> JumpF -> Fall -> LandingLight -> Wait for both Mario and Fox
  using deterministic scripted input, but they still do not make fighters fully
  playable: continuous object-manager process scheduling, real DS controller
  input, squat/attack/special/guard/catch transitions, full collision,
  hit/catch/search, items, HUD, audio, and full imported `ftmain.c` remain
  deferred.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_scheduler_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_scheduler_loop` attach selected
  Mario/Fox `GObjProcess` callbacks with original `gcAddGObjProcess`, invoke
  them through `gcRunGObjProcess`, and run the same bounded scripted movement
  contract from wrapped `scVSBattleFuncUpdate` under a capped VSBattle taskman
  update loop. This proves the current scheduler-facing path only; it still
  does not enable full `gcRunAll` process scheduling, arbitrary fighter
  processes, real DS controller input, full gameplay, or full imported
  `ftmain.c`. The `battle_mariofox_gcrunall_loop` and
  `menu_chain_mariofox_gcrunall_loop` harnesses now call original `gcRunAll()`
  only after pausing previous proof-owned and non-target object processes, so
  the maintained proof is still a selected Mario/Fox runtime boundary.
  Unpaused full-scene `gcRunAll`, continuous unbounded taskman scheduling,
  arbitrary live input, attacks, specials, guard, catch, items, hit/search,
  full collision/platform/ledge logic, hardware fighter rendering,
  camera-correct battle projection, HUD, audio, and full imported `ftmain.c`
  remain deferred.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_live_preview` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_live_preview` now route the
  selected Mario/Fox `gcRunAll` moving-preview path through live DS controller
  reads and original controller global-update code, but the maintained
  automated proof intentionally verifies only 60 neutral idle frames with no
  direct FTStruct script writes. The `NDS_DEV_LIVE_INPUT_PREVIEW=1` build path
  is for manual longer P0 movement checks and is not a full playable battle
  loop. Attacks, specials, guard, catch, items, full collision/ledge/platform
  behavior, real battle HUD, audio, full `ftmain.c`, and unbounded taskman
  scheduling remain deferred.
  Retired standalone gcDrawAll modes `battle_mariofox_gcdrawall_loop` and
  `menu_chain_mariofox_gcdrawall_loop` were deleted as coverage-reduced legacy
  proof. Use the active stage gcDrawAll wrappers and `battle_playable` for live
  scene coverage.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_gcdrawall_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_gcdrawall_loop` add selected
  Pupupu display-layer and map GObjs to that bounded original `gcDrawAll`
  traversal proof. They verify camera capture plus DS-owned DObj draw bridge
  entry and DObj/DL-ready masks for the selected stage GObjs, but they still do
  not unmask arbitrary full-scene display callbacks, prove hardware polygon
  rendering, import camera-correct battle matrices, run Whispy wind/yakumono
  stage runtime, or enable full collision lines, items, HUD, audio, or gameplay.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_collision_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_collision_loop` add an
  opt-in real Pupupu floor projection seam for the selected proof-owned
  Mario/Fox fighters. They now decode `MPLineInfo` using the original
  yakumono/floor/ceil/rwall/lwall halfword layout, prove final line IDs are
  inside the decoded floor range, and guard the bounded `MPYakumonoDObj`
  one-entry shim instead of indexing unsafe yakumono IDs. The proof re-centers
  the selected proof-owned roots from decoded floor endpoints before the final
  projection sample, so it does not yet prove continuous floor following for
  the whole movement slice. It still does not import the full BattleShip
  map-collision processor, platform/ledge logic, arbitrary slopes/ceilings/
  walls, cliffcatch, stage hazards, items, HUD, audio, or gameplay.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_floor_follow_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_floor_follow_loop` build on
  that geometry path and prove continuous selected-fighter floor following for
  the bounded moving slice by projecting Mario/Fox roots against decoded Pupupu
  floor lines during the update path and clamping root Y to the projected floor.
  This removes the final re-center/adopt shortcut for modes `63/64`, but it is
  still not full BattleShip map collision: platform pass-through, ledges,
  ceilings, walls, arbitrary slopes beyond the selected decoded floor,
  cliffcatch, stage hazards, items, HUD, audio, and gameplay remain deferred.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_floor_edge_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_floor_edge_loop` build on
  the continuous floor-follow path and prove the first real Pupupu floor-edge
  / original MP floor-query boundary for selected proof-owned fighters. They
  select the widest decoded floor line, seed Mario/Fox near opposite floor
  edges, prove inside floor-query hits and outside misses through
  `mpCollisionGetFCCommonFloor`, and record line-type/vertex-position helper
  use. Edge-under helpers are intentionally deferred `-1` stubs, so this still
  does not prove wall/ledge/platform resolution, cliffcatch, arbitrary map
  collision, items, HUD, audio, or gameplay.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpprocess_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpprocess_floor_loop`
  build on that floor-edge path and prove the first source-order
  `mpprocess.c` floor slice through a project-owned original-layout
  `MPCollData` adapter. The proof covers selected-floor projection, signed
  below-floor distance handling, inside/outside/below local probes, and live
  Mario/Fox floor-field copyback for the bounded moving slice only. It still
  does not prove platform pass-through, ledges, edge-under wall resolution,
  ceilings, walls, cliffcatch, full fighter map callbacks, arbitrary live
  gameplay, items, HUD, audio, or full `ftmain.c`.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpupdate_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpupdate_floor_loop`
  build on the MP floor-process path and route selected Mario/Fox map
  callbacks through bounded source-order `mpProcessUpdateMain` with a
  floor-only `mpCommonRunFighterAllCollisions` callback. The proof covers
  nonzero moving-loop `pos_diff`, update-main stepping/split probes,
  selected-floor callback hits, decoded floor line `3`, final
  `MAP_FLAG_FLOOR`, and Wait/Ground/Floor safety. It is still not full map
  collision.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpsweep_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpsweep_floor_loop`
  replace the previous second-floor-test deferral with bounded same-line and
  different-line floor sweep helpers plus
  `mpProcessCheckTestFloorCollision`. The selected Mario/Fox callback path
  calls the second-floor branch and rejects same-line/no-new-floor cases while
  standalone probes prove same-line reject, different-line accept from line
  `3` to line `0`, and no-hit miss behavior.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcross_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpcross_floor_loop` then
  prove the accepted second-floor path through the live selected-fighter
  callback route by priming P0 with source line `-1` and accepting decoded
  Pupupu line `3`.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpadjust_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpadjust_floor_loop`
  replace the old floor-edge-adjust deferral with bounded source-order
  `mpProcessRunFloorEdgeAdjust` left/right floor-edge checks and wall-line
  sweep calls. The current floor-only slice records 17 live adjust calls,
  17/17 left/right checks, 17/17 wall misses, 17/17 edge-under deferrals, and
  old floor-edge-adjust deferred count `0`. The newer
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpedge_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpedge_floor_loop` modes
  replace the edge-under deferral with bounded decoded Pupupu adjacency and
  resolve wall lines `6/5` with wall kinds `3/2`, keeping MPAdjust
  edge-under deferred count `0` for modes `77/78`. This is still not full map
  collision: arbitrary live wall-hit adjustment, live selected-callback
  stale-valid-floor crossing, ceiling tests, platform pass-through,
  ledges/cliffcatch, Fall/Ottotto, moving yakumono collision, items, HUD,
  audio, and full `ftmain.c` remain deferred.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpwall_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpwall_floor_loop` modes
  prove the current selected Dream Land main-floor slice cannot produce a real
  wall-hit adjustment: both left/right wall candidates are the same edge-under
  wall lines rejected by original `mpProcessCheckFloorEdgeCollisionL/R`. The
  proof records zero wall hits, nonzero blocker miss evidence, zero adjust
  calls, zero position deltas, and stable Wait/Ground/Floor state. The same
  verifiers now run a geometry-wide source-order scout over the currently
  loaded Dream Land map and report `floors=4`, `walls=8`, `candidates=6`, and
  zero hits. The same verifiers also stage an isolated Hyrule Castle scout
  (`GRHyruleMap`, `StageCastle`, and `ExternDataBank113`) and expose the
  concrete non-edge source-order wall-hit candidate promoted by modes
  `135/136`: floor `5`, wall `13`, edge-under `12`, side `0`, delta
  `-1600/-388`. Modes `137/138` now consume that proof and copy the adjusted
  Hyrule result back into the selected P0 root/collision shell while keeping
  P1 unchanged. Modes `139/140` now consume that wall-copy proof and prove the
  bounded pass-through floor contract on Dream Land line `0` with
  `MAP_VERTEX_COLL_PASS` flags `0x4000`, `MAP_PROC_TYPE_PASS` route count `2`,
  same-line rejection `1`, different-line acceptance `1`, pass callback
  `1/1/0`, and unchanged P1 root state. Modes `141/142` now remain regression
  coverage for the inactive platform blocker: Dream Land line `0`, yakumono
  id/count `1/1`, DObj present but status off, predicate `0`, blocker `0x40`.
  Modes `143/144` add the active predicate regression by installing a bounded
  original-compatible yakumono DObj for the same line, recording DObj present,
  status on, predicate `1`, and blocker `0`. Modes `145/146` add the
  bounded status/update-tic proof by routing that active DObj through
  `mpCollisionSetYakumonoOnID` and one guarded original-compatible
  `mpCollisionAdvanceUpdateTic`, recording status `1 -> 1`, predicate `1`,
  tic `0 -> 1`, and no unsafe escape. Modes `147/148` consume that proof and
  prove the bounded original `ftcommonpass.c` / `ftcommonsquat.c`
  drop-through input gate from Wait into Squat and Pass on Dream Land line
  `0`, with status `10 -> 28 -> 33`, tap Y `0 -> 254`, ignored line `0`, and
  pass wait `3 -> 0`. Modes `149/150` now consume the pass-input proof and run
  source-order `mpCollisionSetYakumonoPosID`, recording Dream Land line `0`,
  yakumono `1`, status `1 -> 1`, predicate `1`, and speed delta
  `12000/-4000/2000`. Modes `151/152` consume that proof and read the same
  vector through the original-compatible `mpCollisionGetSpeedLineID` line API,
  then exercise the bounded dynamic floor/ceil/wall, process-wall, yakumono
  animation, bounds, and Inishie map-header dependency markers. Modes
  `153/154` now consume those proofs and run two bounded original
  `grInishieScaleProcUpdate` ticks through a narrow Inishie scale-platform proof
  shell, recording line groups `1/2`, map object kinds `5/6`, altitude
  `80000 -> 64000`, Y `363000/362000 -> 427000/298000`, and second-tick speed
  `-8000/8000`. The same modes now force the threshold path through
  `Wait -> Fall -> Sleep -> Retract -> Wait` (`fall=1->2/0`,
  `step=3->0/0`) under proof-owned ground/deadzone state. Full Hyrule stage
  setup, live broad wall collision, full
  `grInishieMakeScale` model/data setup, Pakkun/Power Block/item runtime,
  natural platform-speed collision consumption, full player-driven
  drop-through runtime, and arbitrary map runtime remain deferred.

  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpstale_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpstale_floor_loop` modes
  prove a bounded valid-stale second-floor path with a local source-order
  `MPCollData` probe from Dream Land floor line `1` to `0`. This keeps the
  live selected-callback wall/edge roots intact and remains as finalizer-local
  regression evidence.
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivestale_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mplivestale_floor_loop`
  now run the same valid-stale pair from the selected P0 callback path through
  a contained local `MPCollData` source-order pass. This proves the
  selected-callback trigger without mutating the real Mario/Fox movement loop;
  the newer `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpmotionstale_floor_loop`
  and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpmotionstale_floor_loop`
  modes seed that same valid-stale pair into the selected P0 root/collision
  shell, run the selected callback through source-order `mpProcessUpdateMain`,
  accept Dream Land floor line `1 -> 0`, and copy the target floor back to live
  P0 state while P1 stays grounded on line `3`. The newer
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffstatus_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpcliffstatus_floor_loop`
  modes import original `ftcommonottotto.c` and prove the bounded
  source-order `mpCommonProcFighterOnCliffEdge` branch into Ottotto when
  `MAP_FLAG_FLOOREDGE` is set and Fall when it is clear. The current
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpclifftick_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpclifftick_floor_loop`
  modes consume those created states with one guarded original Ottotto
  update/interrupt/map tick and one guarded original Fall interrupt tick. The
  newer
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpfallmap_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpfallmap_floor_loop`
  modes consume that P1 Fall state, run the selected original Fall physics
  callback, integrate one bounded airborne step, and reach the selected Fall
  map callback through a guarded no-collision branch. The current
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpfallland_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpfallland_floor_loop`
  modes consume that Fall-map proof, cross decoded Pupupu floor line `3`,
  call landing-floor setup, reach original LandingLight/Ground, and clamp
  vertical velocity. The newer
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpceil_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpceil_floor_loop` modes
  choose real Pupupu ceiling line `4` and prove a bounded source-order ceiling
  test/adjust path. The newer
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpceilstatus_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpceilstatus_floor_loop`
  modes route the selected original map callback through the ceiling-heavy
  path and original `ftCommonStopCeilSetStatus`, but arbitrary natural-motion
  ceiling hits remain deferred. The previous
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffwait_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpcliffwait_floor_loop`
  modes import original `ftcommoncliffcatchwait.c` and prove the selected
  original map callback reaches the right-cliff test, CliffCatch status, the
  original CliffCatch update into `ftCommonCliffWaitSetStatus`, and one safe
  original `ftCommonCliffWaitProcInterrupt` tick on real Pupupu line `3`. The
  previous
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffattack_floor_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpcliffattack_floor_loop`
  modes import original cliff attack/climb/escape helpers and prove an
  A-button CliffWait interrupt reaches CliffQuick/AttackQuick setup on the
  same Pupupu ledge. The previous
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffattack_action_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpcliffattack_action_loop`
  modes advance original CliffQuick through CliffAttackQuick1 and
  CliffAttackQuick2. They reach the original common2 collision-data helper
  through a narrow project-owned `MPCollData` bridge, copy the ledge floor
  result back to the live `FTCollisionData` shell, and now prove
  `floor_line_id=3` while retaining `cliff_id=3`. The previous
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffcommon2_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpcliffcommon2_loop` modes
  consume that created CliffAttackQuick2/Ground state through one bounded
  original common2 update/physics/map tick and preserve `93/81/0`,
  `cliff_id=3`, and `floor_line_id=3`. The current
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffescape_action_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpcliffescape_action_loop`
  modes start from CliffWait, inject a Z-button tap, prove original escape
  selection, and advance CliffQuick -> CliffEscapeQuick1 -> CliffEscapeQuick2
  while retaining `cliff_id=3` and copying `floor_line_id=3`. The previous
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffescape_common2_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpcliffescape_common2_loop`
  modes consume the created CliffEscapeQuick2/Ground state through one bounded
  original common2 update/physics/map tick and preserve `97/85/0`,
  `cliff_id=3`, and `floor_line_id=3`. The current
  `NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpdownwait_loop` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_mpdownwait_loop` modes
  consume the verified Passive-loop proof, then prove the bounded original
  DownWait interrupt source order into DownAttackU, DownForwardU, DownBackU,
  and DownStandU/Ground, plus eight guarded stable callback frames and
  original animation-end handoff back to Wait/Ground for the DownAttack and
  roll branches, with bounded roll root movement `+10000/-10000` milli and
  DownAttackU attack IDs `53/33/33` verified. The DownStand branch also proves
  the guarded callback/handoff path. Modes `155/156` now consume the verified
  Passive-loop proof, run guarded stable update/physics/map frames for
  PassiveStandF/Ground, PassiveStandB/Ground, and Passive/Ground, then prove
  the same original animation-end handoff into Wait/Ground. They also call the
  imported original DamageFall map callback through the bounded source-order
  MP floor collision path for PassiveStandF and Passive. The previous
  Passive-loop modes
  remain regression coverage for the two-frame version. The previous CliffWait
  damage modes
  still prove the
  CliffWait/Ground fall-wait timeout into DamageFall/Air, one no-collision
  DamageFall tick, the right-cliff branch into CliffCatch/Air, the
  PassiveStand/Passive setup branches, and the DownBounce/DownWait/DownStand
  recovery branch while clearing stale `proc_damage`.
  Modes `155/156` also prove one bounded original Appeal/Taunt branch from
  Wait into Appeal/Ground `189/164`, one installed original Appeal interrupt
  callback tick with no catch/guard branch, the installed update handoff back
  to Wait/Ground, and one isolated source-order Appeal `flag1` branch through
  catch-fails-then-GuardOn `152/134`; full Appeal and shield runtime remains
  deferred.
  The same modes now also import bounded original `ftcommoncatch1.c` enough to
  prove Wait/Ground Z-hold plus A-tap reaches Catch/Ground `166/146` through
  original `ftCommonCatchCheckInterruptCommon` / `ftCommonCatchSetStatus`,
  run one installed original Catch map callback, and force one installed
  Catch update callback back to Wait/Ground `10/4`. They now also import
  bounded original `ftcommoncatch2.c` enough to prove CatchPull/Ground
  `167/147` reaches CatchWait/Ground `168/-2`, with `throw_wait 60->59` after
  one CatchWait interrupt tick. They now also import bounded original
  `ftcommoncapturepulled.c` and `ftcommoncapturewait.c` enough to prove the
  seeded victim reaches CapturePulled/Ground `171/150`, then CaptureWait/Ground
  `172/-2` through the original CapturePulled physics callback. They now also
  import bounded original `ftcommonthrow.c` and `ftcommonthrown1.c` enough to
  prove CatchWait -> ThrowF/Ground `169/148` plus stick-left ThrowB/Ground
  `170/149` on the catcher and ThrownCommon/Air `186/161` on the seeded
  victim, then tick one installed
  original ThrownCommon update/physics/map callback slice with
  `throwCb=1/1/1 floor=3` and one animation-end update branch into immediate
  ThrownCommon setup with `end=1/186/161`. They also tick the installed
  original ThrowF update callback with `flag2` set, proving the real throw
  release branch through `throwUpdate=169/148 dmg=50->58 script=0`.
  They now also import
  bounded original `ftcommonthrown2.c` enough to prove
  `ftCommonThrownReleaseThrownUpdateStats` updates victim damage `10->18`,
  clears capture state, forces Air, installs the thrown proc-status callback,
  and reaches the damage/stat/stale/rumble compatibility seams. They also tick
  the installed original proc-status callback once with
  `throwProc=param=1 script=123`, and prove bounded original damage-release,
  update-damage-stats, and no-damage-release helpers with
  `throwReleaseStatus=dmg=20->26 upd=30->36 noDmg=40->40`. They now also
  prove bounded original dead-result cleanup with
  `throwDead=call=1 coll=1 air=1 waitFall=2 status=10/26`. Item throw
  branches, continuous ThrownCommon animation/release scheduling,
  continuous grab/capture/throw runtime, full damage runtime, and player-driven
  grab gameplay remain deferred.
  Modes `155/156` now also enable the existing bounded original DownWait proof
  after the current recovered Wait/Ground boundary, covering DownWaitU,
  DownStandU, DownAttackU, DownForwardU, and DownBackU guarded branch/handoff
  paths.
  Modes `155/156` now also enable the existing bounded original Turn proof,
  covering Wait/Ground -> Turn/Ground `18/12`, the installed update callback
  facing/ground-velocity flip, and the Wait/Ground handoff.
  Modes `155/156` now also enable the existing bounded original face-down
  DownRecover proof, covering DownWaitD/Ground, DownStandD, DownAttackD,
  DownForwardD, DownBackD, and their Wait/Ground handoffs.
  Modes `155/156` now also enable the existing bounded CliffLedge aggregation
  proof, covering same-cliff occupancy block, drop into Fall/Air, recatch after
  release, and CliffClimbQuick2 finish into Wait/Ground.
  Modes `155/156` now also enable the existing bounded CliffLive proof,
  covering a proof-owned selected P0 process through CliffCatch -> CliffWait ->
  CliffQuick -> CliffClimbQuick1 -> CliffClimbQuick2, one guarded common2
  update/physics/map tick, Wait/Ground finish, and reseeded CliffWait drop.
  The bounded same-cliff occupancy blocker is now
  proven for the current Pupupu line-`3` CliffCatch slice, but natural-motion
  cliffcatch, broader natural ledge occupancy/release/drop/climb,
  hitboxes/damage, continuous DownAttack/DownForward/DownBack/DownStand/
  PassiveStand/Passive action runtime beyond the current bounded stable-frame
  proofs, continuous Appeal/Taunt runtime beyond the isolated
  callback/update/GuardOn branch handoff, continuous face-down DownRecover
  runtime beyond the isolated branch
  handoffs, natural ledge occupancy/release/drop/climb beyond the bounded
  CliffLedge aggregation and selected-process proofs, continuous player-driven
  Turn movement beyond the isolated callback/update handoff,
  platform pass-through,
  continuous Fall/Ottotto/Cliff runtime, and full map collision remain
  deferred.
  The VSBattle proof imports
  `scvsbattle.c` / `scvsbattlefiles.c` only through setup and one interface
  update tick; setup-only harnesses still create stub fighter GObjs from
  original descriptors, while the Mario/Fox model/struct/init harnesses replace
  those stubs with bounded asset-backed model GObjs, persistent FTStruct
  shells, and bounded init-state diagnostics. For Pupupu/Dream Land, it now
  enters a Pupupu-only imported original
  ground setup path and creates original display-layer, Whispy eyes, Whispy
  mouth, back flowers, and front flowers GObjs. The update probe covers only a
  deterministic safe Sleep -> Wait / wait-countdown path and asserts zero wind
  FGM, fighter push, quake, particle script, and GObj count changes. The
  FTStruct proof uses deterministic preorder DObj traversal for common joints;
  exact original `lbCommonSetupFighterPartsDObjs` joint-ID mapping is deferred.
  It still does not run continuous stage update/draw, Whispy wind/fighter push,
  particle banks, full collision line processing, yakumono/stage object runtime,
  item/weapon runtime, full fighter status/process/physics/gameplay, real
  fighter input/update loops, full fighter display rendering, audio backend,
  HUD rendering, or gameplay.
- `syTaskmanRunTask` runs one bounded startup draw pass at update `17`, then
  55 bounded original startup updates through the Opening Room request and
  original load-scene break/eject path, mirrors the taskman cleanup tail, and
  returns to the scene manager. The subsequent Opening Room taskman seam runs
  560 bounded Opening Room updates and executes one bounded Opening Room
  `scene_draw`/`gcDrawAll` probe. It still returns before BattleShip's
  continuous draw/render path.
- `mvopeningportraits.c`, `mvopeningmario.c`, and the Donkey/Link/Samus/Yoshi/
  Kirby/Fox/Pikachu name-card scenes are imported enough to run bounded updates,
  draw original SObj previews, and hand off through the natural scene sequence.
  The fighter/stage-heavy action scenes from `OpeningRun` through
  `OpeningNewcomers` are still bounded bridge stubs in original order. They now
  show a paced original-action-asset SObj preview for each scene boundary, but
  this is not imported gameplay/action rendering and does not instantiate those
  scenes' fighter/stage object graphs.
- `mnTitleStartScene` now dispatches through imported `mntitle.c` /
  `mntitlefiles.c` for a bounded original Title setup slice: it loads
  `MNTitle` and `MNTitleFireAnim`, creates the original actor pair, four
  cameras, the original logo-fire GObj/display link, the original fire
  GObj/SObj/process/display boundary, and initial Title vars, normalizes the 30
  original `MNTitleFireAnim` frame sprites, then runs one guarded original
  `mnTitleFuncUpdate -> gcRunAll` tick on the natural
  `OpeningNewcomers -> Title` path before rendering ten selected original
  Sprite/Bitmap records through the bounded DS SObj preview path. Full title
  input, fire background presentation, animated logo, labels/Press Start,
  slash, logo-fire particles, audio, and continuous title draw remain deferred.
- `mnVSModeStartScene` now dispatches through imported `mnvsmode.c` for a
  bounded original VS setup slice, and the VS Start -> PlayersVS transition is
  proven through original `mnVSModeMain`. Full VS Mode navigation, rule/value
  editing, `VSOptions`, audio, and continuous menu drawing remain deferred.
- `mnPlayersVSStartScene` now dispatches through imported `mnplayersvs.c` for a
  bounded original PlayersVS setup slice, and the ready/start transition to
  Maps is proven through original `mnPlayersVSFuncRun` using deterministic
  two-player selected state. Full interactive cursor/puck character selection,
  fighter object runtime/rendering, character-select audio, and continuous draw
  remain deferred.
- `mnMapsStartScene` now dispatches through imported `mnmaps.c` for a bounded
  original Maps setup slice, and the A-select transition to VSBattle is proven
  through original `mnMapsFuncRun`. The Pupupu/Dream Land preview path now
  resolves `GRPupupuMap`, `StageDreamLand`, `ExternDataBank104`,
  `ExternDataBank103`, and `MiscDataBank152`, applies external fixups with
  zero failures, runs bounded original preview setup, and records real object
  and pointer diagnostics. Continuous Maps drawing and preview renderer
  fidelity remain deferred.
- `scVSBattleStartScene` now dispatches through imported `scvsbattle.c` /
  `scvsbattlefiles.c` for a bounded original VSBattle setup slice. It loads the
  original/common battle file list, reaches camera/manager/interface/audio
  compatibility stubs, builds active fighter descriptors from
  `SCBattleState`, creates stub fighter GObjs for setup-only harnesses, creates
  asset-backed Mario/Fox model GObjs for the model/struct/init harnesses,
  attaches persistent project-owned `FTStruct` shells for the struct/init/wait
  harnesses, runs a bounded source-order Mario/Fox init-state helper for the
  init/wait harnesses, runs original `ftCommonWaitSetStatus` for the Wait
  harnesses, proves one Wait callback tick and one bounded source-order
  ground-friction/safe floor-map pass for the Wait ground harnesses, proves a
  direct and menu-chain Mario/Fox fighter display metadata callback probes with
  DObj counts `25/27` and display-list candidates `14/18`, proves parser-only
  and decode-only first-DL Mario/Fox display-list boundaries with `59/69`
  commands, `28/23` decoded vertices, and `37/20` triangles, proves bounded
  first-DL software preview pixels `4274/5345` for Mario/Fox after corrected
  F3DEX2 command decoding, proves a bounded multi-DL software preview for the
  first four DL-ready DObjs per fighter with candidate counts `14/18`, drawn
  triangles `87/79`, and pixels `6190/7026`, and proves a guarded all-DL
  software preview through `ftDisplayMainProcDisplay` for all current DL-ready
  Mario/Fox DObjs with selected counts `14/18`, represented triangles
  `334/322`, pixels `14913/13432`, and strict all-clean counts
  (`clean=14/18`, `failed=0/0`). The first-failure diagnostics are required to
  stay fully clear with sentinel failed indices and no renderer blocker,
  unsupported opcode/command, or vertex-range reject. It proves one
  bounded `scVSBattleFuncUpdate` interface tick,
  adopts real Pupupu
  `MPGroundData` in the Pupupu harness/menu-chain path, and can run a guarded
  two-tick original Pupupu update probe in a safe substate, plus bounded
  Mario/Fox scripted process-loop proofs, scheduler-loop proofs,
  controller-loop proofs, and moving preview-loop proofs through Walk,
  Dash/Run/RunBrake, and Jump/Fall/Landing. The controller-loop proof feeds deterministic
  `OSContPad` playback through `osContGetReadData`, original
  `syControllerReadDeviceData`, original `syControllerUpdateGlobalData`, and a
  DS-owned `gSYControllerDevices` to `FTStruct` bridge. The preview-loop proof
  samples seven guarded `ftDisplayMainProcDisplay` frames into the bounded
  `96x72` software preview, but it is still diagnostic software rendering, not
  the final fighter renderer. It parks before real gameplay/update or draw.
  Full fighter
  logic/status execution outside the bounded imported slices, broad fighter
  status table/process/update/physics/gameplay loops, real
  arbitrary live fighter input execution, exact original fighter
  joint-ID mapping, real fighter display rendering beyond the bounded software
  preview, camera-correct battle projection, matrix prep,
  material/texture upload/sampling, full
  collision line
  processing, Whispy wind, yakumono/stage object runtime,
  broader Opening Room gameplay/rendering and exact scene presentation remain
  deferred. The battle path now reaches and runs the imported VS Results scene;
  sudden-death presentation remains separate follow-up.
- `mvopeningroom.c` is imported with an NDS entry slice. Original video/task
  setup, relocation setup/file-list resolution, actor/default-camera,
  Scene 1 camera, close-up overlay camera, wallpaper-camera, and logo-camera
  creation,
  NitroFS O2R payload copying, blanket `u32` word byte-swap, internal
  pointer-chain relocation, selected symbol-offset probes, first tick-280
  pencils asset-reference probes, logo-camera camanim probes, logo asset
  probes, boss-shadow asset probes, pencils descriptor/animation table shape
  probes, tick-280 deferred fighter diagnostics, original
  `mvOpeningRoomMakeScene1Cameras`, original
  `mvOpeningRoomMakeCloseUpOverlayCamera`, original
  `mvOpeningRoomMakeWallpaperCamera`, original
  `mvOpeningRoomMakeLogoCamera`, original `mvOpeningRoomMakeLogo`, original
  `mvOpeningRoomMakePencils` object creation from inside the update callback,
  original logo, logo-wallpaper overlay, and boss-shadow setup/ejection,
  wrapper-created Outside, Haze, sunlight, and Desk setup plus sunlight
  ejection, tick-380 deferred
  fighter-status/rotation diagnostics, original tick-450
  `mvOpeningRoomMakeCloseUpOverlay` object creation, original tick-500
  `mvOpeningRoomMakeSpotlight` object
  creation, tick-500 deferred pulled-fighter display-link diagnostics, original
  tick-560 Scene 1 camera ejection plus Scene 2 camera creation, tick-560
  deferred Boss fighter status diagnostics, fighter-kind selection, ticks
  1-560, and one bounded Opening Room draw probe execute.
  Renderer/game-usable display-list and texture data, fighter models, effects,
  audio, rendering, remaining room objects, later Opening Room events, and
  `mvOpeningRoomMakePulledFighter`
  remain guarded.
- `lbCommonMakeSObjForGObj`, `lbCommonDrawSObjAttr`, and `lbCommonDrawSprite`
  are narrow startup `N64Logo` shims. They are sufficient to traverse the
  startup camera/display-link path, skip hidden SObjs, validate the one RGBA16
  Sprite/Bitmap asset, and copy it into a DS preview buffer. Full
  `lb/lbcommon.c` currently pulls in fighter part data and N64 display-list
  rendering definitions that are not ready.
- Object-display and object-script dependencies are still no-op or parked
  stubs. The bounded Opening Room draw probe now executes enough original
  object-display structure to reach `func_80017EC0`,
  `gcCaptureCameraGObj`, and `gcDrawDObjDLLinksForGObj`, then records the
  first exact blocker as DObj display-list translation (`ORDW`, blocker `3`,
  active Scene 2 camera CObj flags `0x4`, XObjs `2/3/8`, viewport
  `600,440,640,480`, callback `0x444C4E4B`, nonzero DObj display-list
  pointer, DObj meta `0x11`). A narrow diagnostic preview records that first
  linked DObj as fallback evidence, then prefers the first material-bearing
  link-27 `gcDrawDObjDLHead1` candidate once bounded branch expansion succeeds.
  The active `ORDP` path now parses the 42-command branch-expanded stream
  through `src/nds/nds_renderer.c`, classifies renderer state/skip/render
  commands, applies a best non-degenerate fallback plane because camera
  projection still has no projected triangles, samples the inline
  `G_IM_FMT_I`/16-bit texture state as a CPU I16 preview, and writes nonzero
  pixels to the retained top-screen preview. The renderer adapter now also
  decodes that bounded texture/tile/load state itself and the verifier asserts
  it, but it does not upload textures to DS VRAM, submit hardware polygons, or
  provide general camera/material/combiner behavior. `ORTX=0` still means the
  source-side scan found no texture-bearing original `MObj`; it does not mean
  the selected display list lacks inline texture commands.
  The DObj draw callbacks (`gcDrawDObjTreeForGObj`,
  `gcDrawDObjTreeDLLinksForGObj`, `gcDrawDObjDLLinksForGObj`, and
  `gcDrawDObjDLHead1`) still do not provide general N64 GBI/RDP rendering.
  The Opening Room logo-wallpaper overlay display callback is linked and its
  alpha state is initialized, `gcDrawDObjTreeDLLinksForGObj` is stored on the
  real logo GObj, `gcDrawDObjDLLinksForGObj` is stored on the real Outside,
  Haze, sunlight, and Desk GObjs, `gcDrawDObjDLHead1` is stored on the real
  boss-shadow GObj, `lbCommonDrawSprite` is stored on the real close-up overlay
  camera and wallpaper-camera GObjs, `mvOpeningRoomCloseUpOverlayProcDisplay`
  is stored on the real tick-450 close-up overlay GObj,
  `gcDrawDObjDLHead1` is stored on the real tick-500 spotlight GObj, and
  `func_80017EC0` is stored on the real Scene 1, Scene 2, and logo-camera
  GObjs, but those N64 GBI/RDP drawing bodies remain parked until
  display-list translation exists.
  `gcPlayCamAnim` is attached to the logo-camera GObj, but the current boundary
  verifies attachment/setup only. `gcParseGObjScript` is reached by the
  default camera GObj on each bounded update, but startup has no active GObj
  scripts yet. Original `sys/objanim.c` and `sys/interp.c` are imported for
  DObj setup and animation playback.
- Directly importing all of `sys/objdisplay.c` is currently too broad. It
  exposes missing `LookAt` matrix ABI, GBI texture/render-mode macros, sprite
  helpers, camera capture, and framebuffer/depth display-list commands before a
  DS display-list translator exists. Keep the current display stubs until a
  smaller original-backed draw slice is selected and verified.
- Relocation symbols and `lbReloc*` functions are still partial. The DS
  manifest resolves current Startup/Opening Room file IDs and loads current
  Opening Room O2R payload bytes from NitroFS. Blanket `u32` word byte-swap,
  internal pointer-chain relocation, and selected `ll...` symbol-offset
  resolution are implemented for the staged Opening Room files. The current
  `MVOpeningRoomScene1` Scene 1/logo-camera symbol, the `MVOpeningRoomScene2`
  camera symbol, plus the `MVCommon`
  pencils, logo, boss-shadow, Outside, Haze, sunlight, Desk, and spotlight symbols needed by the first
  tick-280 asset event, setup objects, tick-500 spotlight object, and tick-560
  Scene 2 camera setup resolve
  through the same backend, and the immediate pencils
  `DObjDesc`/animation table shape is validated. Original
  `mvOpeningRoomMakeScene1Cameras`,
  `mvOpeningRoomMakeCloseUpOverlayCamera`, `mvOpeningRoomMakeWallpaperCamera`,
  `mvOpeningRoomMakeLogoCamera`,
  `mvOpeningRoomMakeLogo`,
  `mvOpeningRoomMakePencils`, and `mvOpeningRoomMakeBossShadow` can consume
  those narrow slices. `mvOpeningRoomMakeSpotlight` also consumes the spotlight
  display-list/MObj/material-animation slice, and the project-owned wrapper
  consumes the Outside, Haze, sunlight, and Desk object slices. The startup
  `N64Logo` asset is the only renderable relocation slice: it has a
  asset-specific Sprite/Bitmap halfword normalizer after the blanket `u32`
  endian pass. The MVCommon material slice also has a narrow mixed-width
  normalizer for 18 background/logo/close-up/desk/spotlight `MObjSub` records
  used by the current material probe. The ORMT verifier proves no texture
  flags, no zero-flag fallback, 18 prim-color records, one light-bearing
  record, and no first texture offset for that selected source set. General mixed-width
  struct fixups, external dependency recursion, texture/display-list fixups,
  fighter data, and full symbol coverage are not implemented yet.
- Minimal BGM playback is a default compatibility backend for exactly one
  track, Dream Land/Pupupu (`nSYAudioBGMPupupu`). `syAudioPlayBGM`,
  `syAudioStopBGMAll`, `syAudioCheckBGMPlaying`, and `syAudioSetBGMVolume` are
  the permanent seam, but the current DS streamer behind them is interim and is
  superseded by the future original sequence-player import. It currently loops
  by wrapping the whole rendered track; original CSEQ loop-point extraction is
  still follow-up. FGM/voice playback, positional audio, broader BGM IDs, mixer
  behavior, and original sequence envelopes remain unimplemented.
- Realtime battle presentation is paced to DS vblank. The original N64
  scheduler drives gameplay from retrace; the DS hardware vblank is 59.8261 Hz,
  about 0.3% slower than N64 60 Hz. This is an inherent platform difference,
  not a gameplay rewrite.
- The canonical realtime + live-input + HW-tri battle-playable ROM polls live
  DS input, submits stage/fighter triangles, keeps BGM timer-paced, and is
  pixel-gated, but the frame is not yet demo-fidelity. Latest gate: GX RAM
  `729/2209`, source depth for stage/Mario/Fox, zero oracle mismatches, and a
  HUD-off capture with `21.179%` meaningful 100ms frame change. Dream Land
  and its original wallpaper are recognizable. The
  original fighter-display preamble and part-selection contract are now live;
  fighter attachment restores mixed-width O2R `MObjSub` lanes before the
  original object manager copies each record. The evidenced one-cycle
  `PRIMITIVE * SHADE` LERP now multiplies source primitive RGB by computed
  shade. Imported DObj/MObj/CObj AObj32 attachments now normalize complete
  source command graphs once per reloc generation, and packed RGBA output is
  corrected without replacing original animation timing. Fighter AObj16 data
  remains on its separate original parser. The source VSBattle pre-render
  light callback and modelview-transformed direction are live. Fighter playback
  seeds initial diffuse/ambient colors from the first selected source `MObjSub`
  (`0xffffff00/0x4c4c4c00`) because `ftDisplayLightsDrawReflect` writes only
  direction into its dynamic heap `Light`; later material light commands remain
  source-ordered. Canonical fallback use is zero. Both fighters are broadly
  recognizable on DS; Mario's pant-leg asymmetry remains unclassified until a
  fixed-light opposite-facing A/B. Anim-lock, fog/color-animation coverage,
  and exact cross-GObj/camera-wide RSP state ownership remain active debt.
  Dream Land's source
  player map objects now decode through an aligned O2R
  halfword accessor, and the original manager grounds Mario/Fox at separated
  source starts. The VSBattle wrapper's forced `is_skip_entry` remains a
  separate compatibility slice; normal BattleShip VSBattle does not set it.
  The unflagged palette seed is an intentional compatibility path until
  command-order proof removes it. Tile-origin math now scales signed 10.5
  vertex coordinates before subtracting the independently converted 10.2
  origin; its fixed-camera probe changed only `18/49152` pixels. Broad ribbons
  first came from suppressing masked repeat/mirror under source `CLAMP`.
  Decorative stars then remained triangles because the DS path clamped each
  vertex before interpolation. Varyings now stay linear, and masked-clamp
  logical axes through 128 texels are materialized through the source address
  function before DS clamp. Dream Land's 192-wide island axis remains above
  that bound. Nonzero `LOADBLOCK` DXT now reconstructs 64-bit words per source
  row from BattleShip `gbi.h:3291,3309-3317`, independent of render-tile width;
  this fixes Fox's 8x8 CI4 tail reading every other zero-padding half-row.
  The large pond now recognizes its exact TEXEL0/TEXEL1 mux and runs a DS
  RGBA5551/A1 precomposition approximation. Whispy material lanes now normalize
  on the live path; Tyler accepts the corrected water. Final face-strip
  comparison remains visual work. Ground flowers and the foreground fence are
  no longer active issues: per-traversal RSP cache/ST carry restores all five
  flower groups, while two-phase no-Z ordering restores the fence over the
  floor/path. Other texture debt
  includes fractional relative tile-origin phase, nonzero shifts,
  DXT-zero/pre-swizzled loads, other TEXEL1 formulas, unmasked POT padding, and
  camera-wide state ownership. Do not conflate mask, load, logical, or upload
  extents again.
- Projected HW submission takes X/Y/Z from one composed clip vertex. Early no-Z
  background draws count down from far signed 20.12 NDC; the first submitted
  source-Z triangle switches later no-Z painter draws to the near foreground
  range without consuming a synthetic slot. This restores source layer-3 over
  layer-1 ordering. Exact raw-GX/no-Z behavior remains deferred. Full source-selected
  fighter submission plus the current CPU-scaled 300x220 wallpaper and water
  precomposition now presents at about `1.6fps`; renderer work remains P1 debt.
- A source-shaped `gcAddMObjAll` attachment wrapper normalizes mixed-width O2R
  fields in a validated local copy before unchanged `gcAddMObjForDObj` owns it.
  Loaded-file plus asset/generation provenance separates raw and already-native
  records; invalid conversion fails closed. Canonical observes at least four
  active Dream Land water/Whispy swaps, zero native/failure cases, and first
  flags `0x0200 -> 0x006b`. The global seam's other callers are not yet claimed.
  The rejected five-address load-time mutation remains reverted.
- Stage submission still validates but discards `DObjDLLink::list_id` for
  stages that use secondary callbacks. Dream Land is not such a case:
  `255_GRPupupuMap.c:25-35` sets `layer_mask=0`, selecting four primary head-0
  callbacks. A camera-wide `42/0` head-0/head-1 queue changed `0/49152`
  canonical pixels and was reverted. Preserve global heads when a live stage
  actually supplies them; do not attribute Pupupu's current ribbons to head 1.
- Fighter events now retain source cycle/render mode, including the normal
  `0x00100000 / 0xc4112078` preamble. Fog color, alpha behavior, non-white ENV
  second-cycle multiplication, and the `is_use_animlocks` inverse-scale branch
  from `lbcommon.c:1369-1441` remain open. The port retains the high-bit descriptor
  branch from `lbcommon.c:1067-1071`, but active Mario/Fox measured `0/0` such
  descriptors, rejecting it as their current fragment cause. Fixed-pose plus
  anim-lock visual gates remain required.
- Dream Land wallpaper runs through imported `grwallpaper.c` and the source
  Sprite/SObj path on the DS 2D back layer. The exact 300x220 RGBA16 asset is
  decoded once for the immutable loaded asset and rebuilt on platform clear or
  provenance mismatch; every frame consumes the live source transform. The cache
  proves one build, 44 hits, 45 exact opaque inverse draws, zero fallback, and
  all 66,000 pixels opaque. It never retains the composed stage frame, water,
  Whispy, flowers, fences, or fighters. Canonical present cost fell from
  `34,839,424` to `24,764,160` ticks (`-28.9%`). Adjacent same-state triangle
  batching proves `103/725/103` begin/reuse/end for all `828` triangles and
  reduces present again to `24,238,464` (`-2.1%`). An exact 256-entry CI4
  palette-pair table preserves source addressing/coverage while reducing
  present to `20,285,888` (`-16.3%`) and conversion to `5,035,776` (`-42.8%`);
  command-hoisted exact light normalization reduces present again to
  `19,725,696` (`-2.8%`) and DL work `4.3%`. Pacing is still only
  `16/16 x0.1`, so profile/no-oracle, raw-GX, and software-2D work remain.
  Audible BGM resyncs remain possible while frames exceed the half-buffer
  deadline. Lane totals remain aggregate conversion observations covered by
  host byte/halfword fixtures.
- The live diagnostic HUD and startup banner are behind `NDS_DEBUG_HUD`; the
  canonical/shipped target forces it off while verifier markers remain active.
- Save/backup functions are stubs. No persistent SRAM/flash behavior exists.
- RSP/RDP graphics tasks are acknowledged but display lists are not generally
  translated to DS rendering. The visible startup `N64Logo` is a bounded Sprite
  preview conversion that preserves the original bitmap overlap rows and
  presents a native `128x108` retained copy for melonDS debugging. The visible Opening Room DObj
  preview is now a narrow bounded display-list interpreter for the first
  material-bearing candidate: it uses active Scene 2 camera state, original
  DObj/XObj transform state, the emitted segment-`E` prim-color branch stream,
  42 parsed commands, four vertices, four triangles, five `G_DL` calls, two
  segment resolves, five color-state commands, prim color `0xFFFFFFFF`, and
  nonzero pixels. The original default RDP viewport math is implemented and
  verified for camera creation. The older link-6 preview remains a fallback,
  and the `ORMB`/`ORME`/`ORMP`/`ORMD`/`ORMX` diagnostics prove only this one
  selected material-candidate path. The new source-side texture-material scan
  currently reports `ORTX=0` with no texture-capable `MObj` candidates, so the
  selected material path is still prim-color only even though the fallback
  link-6 display-list preview can decode one inline `G_SETTIMG` texture.
  The `ORMX` branch-expanded command walk and visible `ORDP` command traversal
  now go through `src/nds/nds_renderer.c` with scene-provided
  validation/segment callbacks, which is the intended DS backend boundary, but
  it still only scans/proves and software-previews the current command family.
  General material/combiner mapping, texture source fixup and upload,
  z-buffering, broad display-list execution, full RDP reset state, and
  continuous draw are still not implemented.
- Overlay loading is a no-op. Overlay linker symbols exist only as compatibility
  placeholders.
- `osGetTime`/`osGetCount` now use libnds CPU timers 0/1. Other DS code must not
  claim those timers without replacing the shared monotonic timing backend.

## Source Compatibility Caveats

- `decomp/` contains independent upstream repositories and is read-only for
  this port. Use `docs/DECOMP_MAP.md` to decide which reference folders are
  useful, and keep local hooks in project-owned wrappers and compatibility
  layers.
- The Makefile intentionally does not include BattleShip's `include` directory
  globally because its N64 libc headers can conflict with devkitARM/libnds
  headers.
- DS shadow headers in `include/` are incomplete by design. Add only the ABI
  required by imported source and document broad stubs.
- `include/ft/fighter.h` and other shadow headers expose only the ABI needed by
  imported slices. They must keep enum values/layouts aligned with BattleShip
  when expanded.

## Current Build Warnings

Known nonfatal warnings come from original decomp code and temporary ABI
differences:

- signed/unsigned comparison in `sys/main.c`
- unused parameters or matching placeholders in original code
- scheduler pointer type mismatches from decomp task structs
- maybe-uninitialized warnings in `sys/vector.c`
- maybe-uninitialized warnings in imported `sys/objman.c`
- strict-aliasing/maybe-uninitialized warnings in imported `sys/objanim.c`
- maybe-uninitialized warning in imported `sys/interp.c`
- array-bounds warning in imported `sys/taskman.c` matching decomp-era globals
- unused parameter in `mnStartupActorFuncRun`
- imported `mnplayersvs.c` / `mnmaps.c` pointer-to-int reloc-symbol warnings
  from original local `intptr_t` offset tables crossing the project-owned
  `lbRelocGetFileData` shim
- imported menu matching-placeholder warnings such as unused variables,
  unused parameters, maybe-uninitialized locals, and control reaching end of
  non-void helper functions in decomp source
- imported `gmcommon.c` / `scvsbattle.c` pointer-to-int reloc-symbol warnings
  from original local file-ID tables crossing the project-owned relocation shim
- imported `grpupupu.c` pointer-to-int reloc-symbol warnings from original
  offset tables crossing the project-owned relocation shim

Do not silence warnings globally unless they block real signal. Prefer fixing or
isolating the compatibility type that causes the warning.

## Runtime Risks

- A broad stub can make verification pass while hiding missing behavior. When
  adding a stub, add a diagnostic or document the boundary.
- The DS has much tighter memory constraints than the N64. Do not optimize
  memory layout before proving the original code path, but expect overlays,
  assets, and display lists to need a deliberate memory plan.
- The DS taskman arena targets `0x150000` when the imported fighter manager is
  enabled, because original Mario/Fox manager creation plus the inherited stage
  proof chain exceeded the old 1 MiB diagnostic arena. Smaller non-battle
  harnesses may tier down if that full allocation is unavailable, but mode
  `163` still asserts the full battle arena and 128 KiB reserve. This is not a
  final overlay/memory strategy.
- The N64 framebuffer dimensions and fixed memory addresses are unsafe on DS.
  Any original code that writes fixed `0x80xxxxxx` ranges must be inspected and
  guarded.
- Display-list translation is a major correctness boundary. Placeholder frames
  prove bootability, not rendering fidelity.
- The live melonDS HUD is diagnostic output. It now includes the bounded
  original `N64Logo` Sprite preview, the material-candidate Opening Room DObj
  preview, `dlp=4f524450 t=4 p>0 x=3f`, and
  `draw=4f524457 b=3 c=3 d=5`, but it does not prove that general original
  display lists or material branch lists are being rendered. The old DS-native
  moving probe render path is intentionally disabled so it does not flash over
  the original asset previews. The startup logo is still a software diagnostic
  preview; it is now presented as a native `128x108` retained copy and applies the
  original `SP_TEXSHUF` odd-row TMEM line unswizzle. The Opening Room preview
  is smaller and moved to the lower-left corner to avoid overlap in
  post-boundary captures. The moving top progress/orange markers and top status
  rectangles are disabled to avoid reading diagnostics as screen flashing. The
  startup logo can still look coarse because the visible diagnostic copy is the
  native `128x108` asset magnified by melonDS, not final renderer quality. The
  bottom HUD FPS row is ROM/VBlank-relative, so it can report normal pacing
  even when the host emulator window is visibly slow; `cv/ch` report retained
  original-preview content cadence, which is closer to visible movie progress.
  `scripts/sample-runtime-speed.ps1` reports one-shot hidden melonDS host speed;
  current 8-second evidence is `hostfps=54.28` with Opening Room at tick `420`.
  A 45-second one-shot sample after limiting retained Opening Room DObj work to
  two original draw probes plus `24` retained-preview reuse presentations still
  reached Opening Room tick `1320`, Portraits tick `150`, Mario tick `60`, and
  action bridge progress `4/128` at `hostfps=48.74`. That effectively rules out
  repeated Opening Room draw probes as the dominant slowdown.
  `scripts/verify-opening-movie-speed.ps1` now provides the maintained
  full-opening speed gate and passes with `hostfps=40.47`, `room=1320`,
  `rdraw=2/24`, `portraits=150`, `mario=60`, `action=9/324`, and
  `title=0x54494457`. The next performance target remains the opening
  movie/title bridge and later original scene imports, but performance work
  should follow the source-code import plan rather than more visual polish.
  Lowering `NDS_OPENING_MOVIE_DRAW_INTERVAL` below the verified `30` is not a
  safe fix yet: interval `10` missed the Title/action verifier window, and
  interval `20` reached Title but made the strict Opening Room DObj blocker
  sample unreliable.
  Use that value, emulator host-speed indicators, and capture timing for
  wall-clock performance symptoms. Do not repeatedly poll GDB for progress;
  repeated attach/detach has produced melonDS packet errors.
- `scripts/capture-melonds.ps1` captures pixels from the visible desktop window.
  Keep melonDS unobstructed; the script foregrounds it and temporarily disables
  GDB for visible capture, but Windows focus policy and hidden-window launches
  can still affect captures in remote or locked sessions. The host OpenGL
  renderer can produce a white frame for a known-good ROM on this setup, so
  automated pixel gates pin software 3D and normalize the scaled top screen.
- The current ledge action proofs clear `is_cliff_hold` and
  `is_jostle_ignore` through the bounded `ftMainSetStatus` seam, and the
  CliffClimb finish proof now verifies the broader current common-reset mask
  `0x7ffff` after the Wait handoff, including `damage_mul=1.0`. The common2 bridge now guards queued
  cliff-motion status IDs plus verifies root snap before/after/expected values
  for the current CliffClimb/CliffAttack/CliffEscape Quick2 action handoffs.
  The `ftCommonCliffCommon2UpdateCollData` fallback also routes through that
  bridge now, so it no longer directly indexes the original cliff-status table
  without the port-side guard.
  The current Pupupu line-`3` same-cliff occupancy blocker and bounded
  release-then-recatch handoff are proven in guarded CliffCatch/CliffClimb
  probes, but broader natural ledge runtime still needs continuous
  release/drop/climb/occupancy coverage before the path is opened beyond
  harnesses.

## Documentation Gaps To Keep Closed

When importing a new subsystem, update:

- `docs/PORTING.md` with reused, replaced, stubbed, and verified behavior.
- `docs/STATUS.md` with the short current boundary and latest proof.
- `docs/HANDOFF.md` with the new runtime boundary.
- `docs/ROADMAP.md` status if a milestone changes.
- `docs/KNOWN_ISSUES.md` if a stub is added or removed.
- `docs/GOAL_DEBUGGING.md` when verifier workflow changes.
- `docs/DIAGNOSTIC_REFERENCE.md` when diagnostic globals or marker meanings
  change.

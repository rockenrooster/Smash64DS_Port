# Fighter Animation/Status Scout

Runtime slice 2 cannot honestly graduate from only `ftmanager.c`,
`ftanim.c`, `ftanimend.c`, and `ftkey.c`. The animation parser side is
bounded, but the manager/status side is tied to original fighter data and
relocation ownership.

## Source Dependencies

- `ft/ftanim.c` is the smallest piece. `ftAnimParseDObjFigatree` parses
  BattleShip AObj/figatree commands and then uses the object-animation APIs
  already provided by the imported `sys/objanim.c`
  (`decomp/BattleShip-main/decomp/src/ft/ftanim.c:65-404`).
- `ft/ftanimend.c` only dispatches finished animations to Wait/Fall
  (`decomp/BattleShip-main/decomp/src/ft/ftanimend.c:16-28`).
- `ft/ftkey.c` is the original key-event/input-command processor reached by
  imported `ftmain.c`
  (`decomp/BattleShip-main/decomp/src/ft/ftmain.c:1274`).
- `ft/ftmanager.c` depends on full original fighter metadata:
  `dFTManagerDataFiles`, `gSCManagerFighterFileSizes`, and full `FTData`
  records (`decomp/BattleShip-main/decomp/src/ft/ftmanager.c:13,81-82`).
  It computes animation heap sizes from every fighter motion descriptor
  (`ftmanager.c:87-124`), loads manager/common moveset files
  (`ftmanager.c:166-168`), loads mainmotion/submotion/model/shield/special
  payloads through `lbRelocGetStatusBufferFile` (`ftmanager.c:300-332`), and
  seeds fighters from `dFTManagerDataFiles` during creation
  (`ftmanager.c:671-695`).
- BattleShip defines the needed full `FTData` layout in `fttypes.h`
  (`decomp/BattleShip-main/decomp/src/ft/fttypes.h:86-118`). The port header
  currently exposes only a trimmed seed layout with `mainmotion`, `submotion`,
  `p_file_shieldpose`, and main/submotion file slots
  (`include/ft/fighter.h:1266-1272`).
- The source data table that satisfies `dFTManagerDataFiles` lives in
  `ft/ftdata.c` (`decomp/BattleShip-main/decomp/src/ft/ftdata.c:127-1395`).
  It also pulls in all fighter motion descriptor arrays, submotion descriptor
  externs, particle ROM symbols, and `motiondesc_offsets.h`.
- The requested "full original status descriptor tables" are not just the
  common table. Original common and character status headers refer to original
  callback symbols across common statuses and fighter specials
  (`decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonstatus.h:7-131`,
  `decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariostatus.h:23`,
  `decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxstatus.h:63,283`).

## Current Port Seams

- The current manager path is a DS-side Mario/Fox loader and fighter creator,
  not the original manager: `ftManagerAllocFighter`,
  `ftManagerSetupFilesAllKind`, `ftManagerSetupFilesPlayablesAll`,
  `ftManagerAllocFigatreeHeapKind`, and `ftManagerMakeFighter` live in
  `src/port/reloc_backend_compat_shims.c:350-439`.
- The current figatree parser and animation-end callbacks are also local seams:
  `ftAnimParseDObjFigatree` and `ftAnimEndSetWait/Fall/CheckSetStatus` live in
  `src/port/reloc_backend_compat_shims.c:1041,2334,2443,2496`.
- Mario/Fox main-motion payloads are present in NitroFS and the manifest
  (`src/nds/nds_reloc_assets.c:74,81`) and the current loader can load their
  main, model, shieldpose, and special payloads
  (`src/port/reloc_backend_fighter_model.c:1617-1676`), but this is not the
  same ownership model as BattleShip `ftmanager.c`'s status-buffer path.
- The motion-script extraction seam still seeds selected decoded main-motion
  commands directly into `fp->motion_scripts`
  (`src/port/reloc_backend_diagnostic_recorders.c:17278-17283`).

## Result

The fighter-data asset slice is now unblocked. `FTData` matches BattleShip
`fttypes.h:86-118`, `ft/ftdata.c` is imported whole, and the tiny
`ftchar/*/*.c` storage TUs plus `sc/scsubsys/scsubsysdata*.c` descriptor data
provide `dFTManagerDataFiles` and the Mario/Fox submotion descriptors.
`lbRelocGetStatusBufferFile` now loads Mario/Fox manager/status payloads through
the O2R/NitroFS path, and fenced `ftmanager.c` proves original fighter creation
for Mario/Fox under `NDS_IMPORT_BATTLESHIP_FTMANAGER=1`.

The fenced proof source-correctly reaches Entry rather than Wait for the normal
VSBattle descriptors because `ftdata.c:75-96` leaves `is_skip_entry` false and
`ftmanager.c:867-899` installs Entry unless that flag is set. Runtime slice 2
should graduate the original manager/status/animation path, then prove natural
Entry -> Wait and Wait -> Walk motion before deleting the current DS manager and
motion-extract seams.

Runtime slice 2 started by swapping the fenced common/Mario/Fox status headers
to BattleShip's original descriptor tables
(`ftcommonstatus.h:7-131`, `ftmariostatus.h:23`,
`ftfoxstatus.h:63,283`). The first fenced gate links by bridging already
imported bounded callbacks back to their original names and by keeping inactive
status callbacks as documented weak stubs until their owning HUD/item/stage/
special dependencies are imported.

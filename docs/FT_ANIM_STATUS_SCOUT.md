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

Importing `ftanim.c`, `ftanimend.c`, and `ftkey.c` can be staged after symbol
collision cleanup. Importing and graduating `ftmanager.c` plus full original
status tables is blocked until the port first expands the fighter-data asset
slice: full `FTData` layout, source `ftdata.c` or an equivalent original-data
bridge, and `lbRelocGetStatusBufferFile` semantics for the Mario/Fox payloads.

Do not replace the current status table or delete the manager/motion seams
until that data/reloc slice exists; otherwise the build would either fail to
link or silently run a hand-authored manager under original status names.

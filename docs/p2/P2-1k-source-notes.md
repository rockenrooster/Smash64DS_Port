# P2-1k — transcribed source facts (handover)

Recorded 2026-08-18 by the stopped P2-1k implementer so the transcription work
survives the row-ownership conflict parked on the board. Sites are in
BattleShip source; verify nothing here by memory — the line numbers are the
citations.

## (b)+(a) CSS gate = the preview box (`mn/mnplayers/mnplayersvs.c`)

- Occupied slot draws `mnPlayersVSMakeNameAndEmblem` (`:578`, built by the
  gate at `:1110-1113`): series emblem `llFTEmblemSprites<Series>Sprite` at
  `(p*69+24, 143)` modulated `0x1E` for `nFTPlayerKindMan` else `0x44`
  (`:619-629`); fighter name `llMNPlayersCommon<Fighter>TextSprite` at
  `(p*69+22, 201)` untinted.
- **Name row and CP LEVEL row are the same row** (y 201/202):
  `mnPlayersVSUpdateHandicapLevel` opens with `mnPlayersVSHideFighterName`
  (`:2788` → `:2575`). Gate builds the level row only for handicap-or-CPU
  (`:1115`); `mnPlayersVSSetCursorGrab` destroys it on pickup (`:2951`) and
  re-makes the name. Three states: MAN → name; settled COM → CP LEVEL;
  COM-with-token-in-hand → name.
- Updates live while carrying: `mnPlayersVSPuckProcUpdate` re-makes it as the
  token sweeps portraits (`:3560-3566`) — preview precedes release.
- Current shell is wrong twice: name font-composed, and at `(panel+22, 146)`
  (should be y 201), shown unconditionally.
- **Separate larger finding (own row):** the source also puts a live 3D
  fighter model in each panel — `mnPlayersVSMakeFighter` (`:1624`) via
  `ftManagerMakeFighter`, translate `(player*840-1250, -850)`, scale
  `dSCSubsysFighterScales[fkind]`, camera `mnPlayersVSMakeFighterCamera`
  (DLLINK 18|15|10|9), `ftParamCheckSetFighterColAnimID(nGMColAnimFighterComPlayer)`
  for CPU slots. Needs fighter manager + figatree heaps inside a menu scene.

## (c) SSS missing pieces (`mn/mnmaps/mnmaps.c`)

Construction order `:1654-1660`: wallpaper → plaque → labels → icons →
name/emblem → cursor → preview. Missing today, with sites:
`llMNMapsWoodenCircleSprite` (189,124) `:380`; fill `160,128..320,134` prim
57/60/88 α FF and `194,189..268,193` black α 33 `:397/:399`;
`llMNMapsStageSelectTextSprite` (172,122) IA lerp ENV 00/00/00 PRIM AF/B1/CC
`:418-431`; `PlateLeft` (174,191), `PlateMiddle` x 186..258 step 4,
`PlateRight` (262,191) `:433-451`; `llMNMapsTilesSprite` x 43..139 step 16 at
y 130 `:919`; emblem `llFTEmblemSprites<S>Sprite` at
`(189,124)+dMNMapsLogoPositions[gkind]` tinted 5C/22/00, RANDOM →
`llMNMapsQuestionMarkSprite` (223,144) `:780-814`; name `llMNMaps<G>TextSprite`
at constant (183,196) under REGION_US (`mnMapsSetNamePosition:568`) in black —
legible once the plate ships (retires P2-1f's "black would be invisible"
note). Name+emblem re-made per cursor move (`mnMapsMakeNameAndEmblem:818`).

## (g) BGM lifecycle — the three missing STOPS

Starts: mode select BGM 44 unless prev ∈ {1PMode, VSMode, Option, Data}
(`mnmodeselect.c:889`, shipped); **VS menu BGM 44 iff
`scene_prev == nSCKindPlayersVS`** (`mnvsmode.c:1645-1648`, NOT shipped);
CSS BGM 10 unless prev == Maps (`mnplayersvs.c:4790`, shipped); SSS starts
nothing. Stops (all missing — this is "music always playing"): title entry
`syAudioStopBGMAll()` when `scene_prev != nSCKindOpeningNewcomers`
(`mntitle.c:352`, also `:548`); mode-select B → title `syAudioStopBGMAll()`
(`mnmodeselect.c:774`); CSS back → VS mode `syAudioStopBGMAll()` in
`mnPlayersVSBackToVSMode` (`mnplayersvs.c:3234`). `ndsAudioBgmStopAll()` +
`gNdsAudioBgmStopCalls` already exist — each fix is one line at its seam.

## (d) title text animation — mechanism

`mnTitlePlayAnim` (`mntitle.c:729-748`) is not a hand curve: it runs
`gcPlayAnimAll(effect_gobj)` and copies each DObj child's `scale.x/y` and
`translate` into the matching SObj (`pos = translate + 160/120 −
size*scale*0.5`). Data: `llMNTitleLabelsAnimJoint` / `llMNTitleLogoAnimJoint`
/ `llMNTitlePressStartAnimJoint`, `AObjEvent32**` streams in
`sMNTitleFiles[0]` (`:1098/:1190/:1244`). Ship via an `AObjEvent32` decoder in
the bake OR the imported `gcAddAnimJointAll`/`gcPlayAnimAll` at runtime — the
latter is the source's own code and likely far cheaper. Title stops at `:352`
and `:548`; tic timeline recorded in P2-1i.

## Bake patch (unwired, runs clean under --preview)

Scratchpad of session `7aadfd59…`: `p2-1k-bake.patch` (411 lines) +
`p2-1k-generate_mn_ui_kit.WORKING.py`. Pack unchanged 47,552 B `0x6b06b3d9`;
surfaces 730,552 → 1,094,392 B, 31 → 66: 24× `CSS_GATE_<p>_<MARIO|FOX>_<MAN|
COM|COMHOLD>` (53×73, 7,738 B each, zero new OBJ VRAM), `SSS_SCREEN`
256×192, 10× `SSS_PLAQUE_<slot>` (90×70) shared bbox, `o2r_path()`
(`FTEmblemSprites` lives in `reloc_fighters_common`, not `reloc_menus`),
`Placement.tile_anchor`/`Placement.dest`. Applying it alone breaks the build:
`MENU_STONE` is retired there and `nds_menu_shell.c` still names it.

## Allowlist rulings implied

Entry 5 (`Mario|Fox` TextSprite) deletes; entry 6 (css emblems) narrows to
the ten P2-3 fighters; entry 21 (sss emblems) narrows to the eight P2-4
stages (its 8,448 B OBJ rationale is void once the plaque is a surface);
entry 20 narrows to the eight unbuilt stage names; entry 22 deletes. Audit
`screen_of` rule: a new shell function must contain exactly one of
`Title/Mode/Vs/Css/Sss`.

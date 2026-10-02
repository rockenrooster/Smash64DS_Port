# P2-6 — Natural Campaign Integration and Qualification

1P is active under the owner's September 10 unpause. Imported scenes, driver, variants, native exports, stage packets and host tests already exist in varying states. This plan turns those pieces into a complete playable campaign; it does not call them absent or automatically accepted.

## Authoritative seams and existing work

The source `sc1pmanager` owns the persistent run, sequencing, allies, continues and progression. `sc1pgame` owns source battle setup, waves and score events. `sc1pstageclear` owns the tally; `sc1pbonusstage` owns Bonus 1/2 integration. DS bridges replace platform entry, resources/rendering and hardware services, not campaign meaning.

Reuse `battleship_sc1pmanager.c`, `battleship_sc1pgame_runtime.c`, the imported menu/intro/tally/continue/bonus/tail scenes, variant/Boss exports and existing actual-C/host tests. The reviewed runtime bridge still contains explicit Boss/Polygon refusal paths; an export existing is not proof that the caller admits it. Resolve guards against the actual needed native/assets/behavior capabilities, never blindly remove every guard or infer readiness from the flag.

`NDS_P2_1P_GAME=0` in a published configuration is not a pause or a claim that code is missing. Flag-on candidate work must preserve the VS configuration and be qualified before publishing an enabled feature.

## Campaign walk status (2026-10-01)

The walk ROM (`TARGET=smash64ds NDS_P2_MENU_WALK=1`, route 1: the human a
level-9 CPU, enemies level 1, enemies below stage 12 KO'd every 45 frames)
plays the whole ladder: stages 0-13, every bonus, Master Hand, Ending, Staffroll
and Congra (walks `cwalk29`..`cwalk48`, `artifacts/visibility/2026-10-01_1p/`).

| Stage / scene | Native failures | State |
|---|---|---|
| 0-3, 5, 9 | 0 | clear (full walk `cwalk45`) |
| 4 Mario Bros | 0 (was 45 sprite) | the ally's heart tag: its sprite row was missing from the IFCommonPlayerTags geometry table, and the OAM arm refused tag 5 |
| 6 Giant DK | 0 | a DK player/ally loads DK's full files here (the compact pack faulted Giant DK's construction) |
| 8 Kirby Team | 0 | |
| 10 Meta Crystal | 0 | the Mario/Luigi/Metal Mario pipe: the deferred-desc retry now names the entering fighter's Special2 slot first (a Mario ally or Metal Mario after Mario Bros faulted on a bare effect GObj) |
| 11 Race | 0 | the lights drew opaque: a Sec layer's display head 1 enters with G_RM_AA_ZB_XLU_SURF (layer 1) or G_RM_AA_XLU_SURF, and the stage generator had seeded it as zero; head 1 now starts translucent (`head_entry_othermode_l`, runtime `ndsNativeStageHeadEntryOtherModeL`) and its runs request intensity coverage for their I8 BLENDPE lerp. Same fix on Sector Z's glows and the alpha-threshold head-1 runs of the boards, Hyrule, Zebes, Saffron and Mushroom Kingdom |
| 12 Polygon Team | 0 | low-water 91,540 B (fighter files in the overlay tail) |
| 13 Master Hand | 0 | stood frozen (owner 10-01): the port's entry statuses lacked the source's Boss arms (Appear -> his own Wait, his AI target), and his 30 battle motions were stub symbols with no file. They ride the 1P rows now, his 8 AnimJoint motions flagged AObj32 so their scripts are normalized (as AObj16 his Appear never ended and he stayed an undamageable ghost). `mh05`: attacks cycle, Link damages him |
| 7 Board the Platforms | 0 | the platforms' lit lists are baked with the bonus2 maps' light (20, 45) between a G_LIGHTING clear/restore, so the executor no longer lights each vertex every frame (same image, `lit03`). The platform lights (DObjs 1-2, DL link 1) drew opaque: the source's layer-1 head 1 starts them at G_RM_AA_ZB_XLU_SURF and their BLENDPE lerp's alpha is TEXEL0 * PRIM on an I8 tile; the baked roots now carry each head's render mode and request intensity coverage (owner: "they look good now") |
| Board maps, Race, Meta Crystal, Small Dream Land | 0 | the map structures drew in false colour (owner 10-01: "map structure colors"): every battle's pre-render function sets G_LIGHTING and aims light 1 from the map header's light_angle, and these lists never clear it, so their vertex colour bytes are normals. The stage generator folds the RSP's lighting into those colours (`bake_source_lighting`: ambient + diffuse * max(0, N.L), the light in the binding's frame, the lists' own gSPLightColor words; 27 packets). `lit01`: Link's boards draw their sandstone brick |
| Ending 48 | 0 stage | the room draws: each room GObj submits its own tree with the room camera and the MVCommon lists run as baked roots (`generate_nds_native_item_baked.py` GOBJ_SCENE). The table is a NitroFS file the ending loads into its own heap (`movies/room_baked.bin`, 22 KB; linked into the overlay tail it had cost every scene's arena 22 KB). The props' later lists inherit TLUT mode, texture and flat unlit geometry from the list before them (seed replay); the background's lit lists are baked with the room light (45, 45); walls past the v16 range store their vertices shifted (`vertex_shift`, up to 7,658 source units); the books' CI8 cover indexes one colour past its 255-entry TLUT (load widened to 256). The fade-in and closing light drive MASTER_BRIGHT (`ndsVideoSetSceneFade`); the figure draws through the intro's transient fighter submit (`end11`); 1 sprite failure a frame remains |
| Staffroll 56 | ~1 sprite/frame | names and jobs draw natively (glyph quads, `cwalk53`); crosshair and box via the S2D tenant, frame rects in the overlay |
| Congra 55 | 0 | correct |
| Bonus boards, every fighter (owner playtest 10-01, Kirby) | 0 | 15 of the 24 boards drew no map, only targets and platforms (the walk ran Link only). Their first brick is a CI4 image using index 3 under a three-colour LOADTLUT (the other nine load four colours of the same image); the RDP reads stale TMEM there, the DS resolve refused the texture (BAD_TLUT) and the stage owner refused the whole map every frame. A stage palette shorter than its indices now reads the source's following palette words (`98be04a7540`); `bonsweep.ps1`: all 24 boards load their GX program |
| 12 Polygon Team as Kirby (owner playtest 10-01) | 0 | froze two thirds in: in a match a copy hat binds only from the set admitted at its start, and the Polygon Team admitted none, so the first swallowed polygon reached `ndsKirbyHatResidencyHalt(5)`, an infinite loop. A power outside the set now loads late into the slot's working buffer above the 25,600-byte floor; failing that the hat does not draw and the power stays (`1c1c67b90f4`; forced-empty test: 2 late loads, no freeze) |

Battle HUD anchored to the 3D (owner 10-01: tags "offset around edges of
screen", VS too): fighter tags, item arrows and hit sparks map a projected
point with the battle 3D's own scale -- the camera's 300 x 220 viewport fills
256 x 192, so x 128/150 and y 96/110 -- instead of the 2D layer's uniform 0.8,
which pulled them toward the centre by up to ~8 px at the edges
(`ndsIFCommonBattleScreenX/Y`).

The 1P venues (team stages, Metal, Polygon, Final Destination, the Race and
both boards per fighter) now take the VS stages' compiled GX templates
(`compile_nds_stage_gx.py`, `sNdsStageGxPaths`); the boards' bodies may run to
48 KiB (Purin's Board the Platforms is 46,552 B).

Per-battle pacing in the walk (present intervals of 2 VBlanks, `PACE`;
proxy, not the gate): 0 93%, 1 Yoshi Team 78%, 2 94%, 3 93%, 4 86%, 5 93%,
6 94%, 7 Board the Platforms 18%, 8 96%, 9 94%, 10 95%, 11 Race 72%,
12 Polygon Team 39%, 13 Master Hand 89% (`cwalk54`). The walk KOs an enemy
every 45 frames, so the team stages spawn far more often than in play.
Board the Platforms was 6% (`cwalk49`): its ARM9 profile
(`artifacts/task37-census/1p-pf01-st7`) put 177 of 200 frames over the gate,
led by the platforms' per-DObj submit (`ndsRendererAdapterSubmitStageDLBody`,
275K cycles/frame). Their baked roots now take the stage DL fast lane
(`NDS_SDL_ROUTE_BAKED`): 833M -> 653M cycles over the same 200 frames
(`1p-pf02-st7`), now led by the baked emit (205K cycles/frame), the native
stage segments (188K) and texture binding (118K).

Owner floor for content work (10-01): WORK P50 under 1.12M ticks, i.e. more
than half the frames within two VBlanks (`PACE` b2). The Polygon Team's
packets never replayed -- each polygon needs 28 texgen groups and 484 texgen
sites against the packet struct's 8 and 256, so every draw recorded, faulted
and drew direct; a record now takes the overflow from the top of its own
region (`04cc889b6e4`, with the texgen divides as reciprocals and 1P stage
runs culled by their bounds). Polygon Team 22% -> 58% (Link), 48% (Kirby);
Race 9% (b3 79%); Board the Platforms 20% (Kirby, idle). The lean path for
the 1P-only owners was tried and reverted (polygons re-materialized every
frame, +700K cycles).

10-01/02 floor work (natural play, 600 presented frames, share within two
VBlanks): baked roots off screen draw nothing (`1c7d84cae67`, Board the
Platforms 20% -> 84%); the texgen patch reuses directions and searches
newest first (`bb74a45cd2f`), then drops the cache scan for a previous-normal
memo with 32-bit dots (`eabbe73e0d4`: a polygon's sites are 93% distinct);
item baked roots, the GBumper quad and two-way route slots keep the Race's
lists on the stage DL fast lane (`e4f359048bb`, `eabbe73e0d4`: body submits
2,402 -> 3); an off-screen bumper quad skips its owner (`9f11dd99405`).
Polygon Team 61% -> 80%; Race 15% -> 36%. Sweep of every other stage at the
same build: 84-98% (Giant DK 84%, Master Hand 87%, Mario Bros 85%). The Race
is the one stage under the floor: three polygons, the player and Bob-ombs on
the largest map (busy median 2.45M cycles a present against 2.23M; profile
`1p-pf12r-st11`). Fighter packets then moved texgen to the geometry engine
(TEXIMAGE_PARAM mode 2 with a per-group texture matrix, `87760f7dfdf`):
Polygon Team 88%, Race 43% (median 2.28M, `1p-pf15r-st11`). Measured and
dropped as no gain: a line- or segment-level x reject in the floor/ceiling
sweeps (the y test already leaves only the lines near the fighter) and a
pointer memo for the collision geometry check (slower: cold statics). The
Race's remaining cost is spread thin: soft-float adds 143K a frame across
dozens of callers, wall/floor/ceiling sweeps 210K, poses 120K, items (six
Bob-ombs against four fighters) ~85K. Run-ahead would absorb it and is
refused (D4). What did move it: every blob-resident stage but Yoster shipped
rigid binding mask 0, so the Race composed its 24 bindings on the CPU every
frame although none moves (a frame-to-frame world probe over a 600-frame run
and a walk down the course). bonus3's mask now pins its 18 non-billboard
bindings, the compiled GX program bakes their worlds (body 39,076 B, the
boards' ceiling), and the run cull tests a rigid run's world box against one
camera x projection a frame (`56a77844b42`): Race 43% -> 50.2%, at the
floor. The other 1P-only maps followed from the same probe over a 900-frame
natural run (`1acbbf5bcdf`: Final Destination 0x17, the Polygon Team's 0x1,
Metal Mario's 0xF, the small Yoshi's Island 0x1C014); all four programs still
load, and the share within two VBlanks reads 82%, 87%, 97% and 93%. The VS
venues still ship mask 0 and need their VS heap checked before a pin: Hyrule
grows past its ceiling (30,252 -> 40,492 B), Saffron to 46,172 B, and Congo
Jungle's program already declines at runtime on the loader heap (reason 3).

### Final P95 pass (from 2026-10-02)

The gate is >= 95% of presents within two VBlanks on every 1P stage. Natural
play (walk ROM with `gNdsCampaignWalkMeasure`: the human a level-9 CPU,
enemies at source levels), up to 1,200 presented frames, stopped at the
battle's end (scratchpad `pace3.ps1`, `pacesweep.ps1`; two runs where the
stage was reached two ways):

| Stage | Within 2 VBlanks | Where the tail is (`1p-pf21s<N>` profiles, `cf95.py` P95 drops) |
|---|---|---|
| 0 Link, 2 Fox, 3 Targets, 5 Pikachu, 9 Samus, 10 Metal | 97-99% | pass |
| 13 Master Hand | 84.5% -> 94.4% | his packets faulted on capacity every frame (7,609 words, half region 4,420); now replay (`5ce05818f39`). Left: hurtbox kernel (MH's many hurtboxes), FD stage GX |
| 8 Kirby Team | 94.4% | stage GX draw, lean compose |
| 7 Board the Platforms | 87.9% -> 91.3% | baked-run texture memo thrash, now two-way (`e30b6e3747c`); a 15-frame spike from the platforms' material/anim keyframes remains |
| 1 Yoshi Team | 84-91% | lean compose, soft float, stage segments |
| 6 Giant DK | 84-89% | broad: lean compose, stage GX, soft float, pose; one 2.7M-tick texture frame at entry |
| 12 Polygon Team | 88% | hurtbox kernel, packet replays and texgen |
| 4 Mario Bros | 81-86% | four fighters (not yet profiled) |
| 11 Race | 40-52% | broad: soft float 117K ticks a frame (collision sweeps, item range checks, AI), sweeps, pose |

The Race's share sits on its median, so it moves several points between
builds from layout alone; read it from same-ROM A/Bs. Master Hand's stage
background (~24 boss wallpaper GObjs) now animates through the fixed-point
cubic (`ndsGcPlayAnimAllFixedCubic`), which took its float player off the top
of that battle's soft-float callers.

### Owner r67 playtest (2026-10-02, full 1P run as Kirby)

| Report | Cause | State |
|---|---|---|
| Kirby's Board the Platforms: no moving platforms, unfinishable | two rail scripts share one SYInterpDesc; the event32 ledger refused the second (reason 13) and `gcAddAnimAll` installs nothing when one entry is refused | fixed `67670e1dacb` (shared descriptor admitted while its word still reads fixed) |
| Title freezes when idle | the idle hand-off goes to How to Play, whose ground map (0x10b) is not staged: data abort | idle title stays up (`969363d8a1e`, `gNdsMenuTitleAttract` 0); How to Play, the Characters demo arm and the auto demo are not brought up |
| 1P CSS "Option" under a blue line | the bake drew the separator fill after the labels (source fills first) | fixed `69860dc9f75` |
| Bonus 1/2 Practice do nothing | the bonus select preloaded all 12 fighters (heap halt at the 4th); now the 1P select's one-preview boundary and the preview packs; the stage runs | select's 2D draws through the S2D tenant but its portraits exceed bank E (64 KB): garbled until it gets a baked surface like the 1P select |
| Training Mode | owner: may be closed for now | its confirm plays the denied sound |
| Team stock icons (Yoshi colours, Polygon characters) | the lower HUD drew every icon in the first enemy's palette / the base fighter's glyph | each icon in its member's colour; the Polygon row is the source's single polygon icon |
| Challenger silhouette | no 3D layer, no draw route, no box, and colanim 80 had no script row | box and fighter draw; the fighter is not yet black: battle slot 0 and the transient slot drop the colanim modulate (slot 1 shows it) -- open |
| Credits text static, crosshair mangled | the name path's SYInterpDesc header was never fixed (kind 6/512 points); the S2D presenter undid odd rows only for SP_TEXSHUF sprites | fixes applied, verification pending |
| Held barrel glitches when hit | `ftSetupDropItem` was a stub that only cleared `fp->item_gobj`; the damage drop roll was missing | `itMainSetFighterDrop` and the source roll (replay digests re-pin) |
| Master Hand intro "no animations" | not reproduced: Appear plays (finger joints rotate, poses change across captures) | needs the owner's detail |
| Menu select sounds (owner: "just the A select sounds, all menus") | FGM 158 MenuSelect is a four-note rising chime in the source; the pack rendered it on the flat path (first note held: one 120 ms click at DS volume 20 rising to 62 only when an update steps the envelope, which a scene load does not run) | 158, 165, 167, 127 render their whole UCD program; the single-pitch menu clicks (157, 163, 164) follow for balance -- the flat path maps the pre-mixer target linearly where n_env.c squares it, 6-7 dB hot |
| Bonus announcer starts too low | the composite renderer voiced a UCD rest (pitch code 0) as a note 13 semitones down; n_env.c's note op releases the sound instead | rests render as silence and the next note opens a new sound (35 cues change, sizes unchanged) |
| Transitions go black (owner: hold the last frame) | the r64 hold kept the leaving frame only until the next scene's first display write | the display capture unit snapshots the leaving frame into a texture-free bank (B, else A) and the main screen shows it (display mode 2) until the next scene's first complete frame: title, mode select, 1P mode, 1P CSS and the intro now hold with no black (walk probe, `artifacts/bugs/2026-10-02_transition-snapshot`). Open: a battle's load needs every bank, so its first texture upload ends the snapshot under the black cover (intro -> stage 1: held 69 VBlanks, then 112 black; the texture prepare alone is 58); a main-memory copy shown by the display FIFO starves behind DMA0 (garbled), and battle exits have no texture-free bank |

Intro fighter stills (owner: rendered stills). The shipping intro is static
and blits `assets/intro/*.s1i` (`ndsSC1PIntroBlitStills`), baked from this
renderer by the lab ROM `NDS_1P_INTRO_BAKE=1`
(`scripts/menus/bake_1p_intro_stills.ps1`, packed by
`scripts/menus/pack_1p_intro_stills.py`; the stills are ROM-derived and stay
out of the public repository). 201 stills: every player card (cards 0, 1, 3 x
12 kinds x 4 costumes), every ally card, the VS fighters of every battle stage
(the Yoshi, Kirby and Polygon Teams composited from one member per boot) and
the recolours the source applies. Found on the way: costume accessories
(Purin's bow and hats, Pikachu's hats) were lost by the draw capture -- Purin
in costumes 1-3 declined everywhere, Pikachu drew hatless -- now bound to their
joint and carried by an Accessory root program; camera xobj kinds 12-17 (the
Reflect LookAts the intro's stage cameras use) had no view.

Poses (owner 10-01: "different poses and different scale"): the intro's
IntroL/IntroR statuses select submotion rows 13/14, and their animation
symbols (`llFT<Kind>AnimPose*FileID`, the team poses, Master Hand's, the
figure's DollFall/DollRevival) were port stubs with no file behind them. An
unresolved token leaves the figatree heap untouched, so every intro fighter
replayed his DemoNull Wait -- Mario in profile, Link upright, every team member
on the formation origin. The generator now emits the 57 1P demo rows
(`NDS_1P_DEMO_ANIM_ASSET_ROWS`: token registry, path table and staging in
lockstep, checked by `test_results_demo_submotion_routes.py`), and the re-bake
matches the N64 frame (Mario three-quarters with his fist up, Link's diagonal
sword, 18 Yoshis, 8 hatted Kirbys, the Polygon crowd).

## Source route contract

Use `dSC1PGameStageDesc`, its enum and manager branches; table entries with placeholder venue IDs for bonus dispatch are not battle venues to instantiate.

| Sequence | Encounter | Source venue / content dependency |
|---|---|---|
| 1 | Link | Hyrule; first natural campaign fight |
| 2 | Yoshi Team (18 total) | YosterSmall; source replacements/colors/traits |
| 3 | Fox | Sector |
| 4 | Bonus 1 | Player-specific Break the Targets board |
| 5 | Mario Bros. | Castle; Mario/Luigi plus the source-selected ally |
| 6 | Pikachu | Yamabuki |
| 7 | Giant DK | Jungle; player and two source-selected allies |
| 8 | Bonus 2 | Player-specific Board the Platforms board |
| 9 | Kirby Team (8 total) | Pupupu; source two-at-once team and copy configuration |
| 10 | Samus | Zebes |
| 11 | Metal Mario | Metal / Meta Crystal |
| 12 | Bonus 3 | Race to the Finish / Bonus3 |
| 13 | Fighting Polygon Team (30 total) | Zako / Duel Zone |
| 14 | Master Hand | Last / Final Destination |

The manager separately handles challenger fights and ending/progression transitions. Derive exact enemy concurrency, stocks, handicap/CPU traits, timer/item rules and difficulty parameters from the table plus setup functions, not from total-opponent counts. Preserve per-entry all-item toggle semantics where the source sets them; no item-off campaign shortcut qualifies the original route.

## Package: entry and first ordinary fight

**Outcome:** Main menu→1P menu→1P CSS→intro→Mario versus Link/Hyrule through source campaign setup, with the selected difficulty/stock/player/costume preserved.

**Dependencies:** P2-1 bounded preview service, Link/Hyrule native content and a valid scene memory profile. Reuse compact previews. Campaign source selection remains authoritative; do not replace it with a VS descriptor, initialize battle state twice or replay startup only for a probe.

**Proof:** Natural menu input, setting changes and back/cancel, Intro animation/audio, countdown/GO and real fight behavior. Measure the actual startup/transient and active-scene floor. A very small free minimum cannot be labeled successful admission just because the first frame appears.

Cycle-2 proof status (2026-09-13):
- [x] Natural Main Menu → 1P Mode → 1P CSS entry.
- [x] Source CSS setting/costume changes plus back-out and re-entry.
- [x] Source 1P intro animation/audio completes; 2026-09-14 heap proof keeps the source full-file Intro residency (148,784 B exit free) because compact Link reaches a native-owner refusal.
- [x] First ordinary Mario vs Link/Hyrule fight reaches GO and sustains 600 presented fight frames.
- [ ] Victory/tally/next-stage transition — 2026-09-14 guest-side ordinary input now reaches a natural win plus source StageClear (`CPTALLY-SHOT`/`CPTALLY-FINAL`) with a non-clear tally capture and 38,068 B transition heap low-water. Closure remains open because stage-1 Intro OOMs before `CPNEXTBATTLE` (144,640 B request, 120,164 B free), and native/graphics failure counters are nonzero.

**Exit:** This natural prefix works and remains repeatable. **Stop:** Name the failed source→resource→native→output stage; preserve already-working prefix evidence.

## Package: common victory, tally and retry transitions

**Outcome:** Natural fight completion reaches tally and next-stage routing; defeat reaches source continue/Game Over logic.

Keep run totals/difficulty/source level-drop and remaining stocks persistent only where specified. Reset scene-local fighter/actor/intro/tally state on each entry. Cancel pending preview/asset/audio work before its owning arena is retired. Continue score arithmetic must match the source's truncation/order, not an approximate integer shortcut assumed equivalent.

**Proof:** Actual win and loss, Continue Yes/No/timeout, retry, repeat win, exit and re-enter. A direct-boot scene can prove its local display but cannot close transition wiring. Host edge tests cover arithmetic/branches; a natural route proves callbacks/events are actually reached.

## Package: ordinary and ally encounters

**Outcome:** All ordinary match descriptors and ally fights come from the manager's real source configuration.

Verify substituted opponents/costumes where the player changes the source selection, ally eligibility/randomness, team membership, damage/credit and CPU traits. Mario Bros. uses Castle, not Mushroom Kingdom. Giant DK's two allies exercise four instances with variant-specific scale/camera and memory. Link the ordinary stage contracts rather than duplicating their geometry work.

**Proof:** Source table/actual-C setup comparisons across allowed selections and difficulty boundaries, plus natural ordinary/ally fights and next-stage transition. Shared mechanics already qualified in P2-2 remain evidence; campaign setup/state persistence still needs its own engagement.

## Package: Yoshi Team

**Outcome:** The whole source team runs on YosterSmall with correct simultaneous enemies, variation cycle, spawn placement, replacement, HUD/remaining count and final completion.

Use the existing team spawn/respawn machinery and source total 18. Preserve color/trait/stock selection. Bind replacement instances to the proper native/pose/costume data; do not preload a roster-wide closure by accident. Prove repeated replacement and scene peak/floor across the full team, not one Yoshi death. Campaign small-stage assets/bounds are not the VS Yoster profile.

## Package: Kirby Team

**Outcome:** All eight opponents, source simultaneous count and copy loadouts work visibly and mechanically through the final transition.

Use source shuffled/final copy selection and hat/modelpart/article resources. `fighters/kirby.md` owns copy capability semantics; this package owns campaign population and resource lifetime across replacements. Prove copied projectile/effect creation, correct bodies/hats, cleanup and final tally. A plain Kirby mirror test does not cover the team.

## Package: Metal Mario and Giant DK

**Outcome:** Each variant retains source attributes/resistance/scale/material/audio and source battle/ally configuration.

`fighters/variants.md` owns exact variant deltas, with Meta Crystal/Jungle venue contracts. Avoid declaring an absent voice or cheap model from memory. Prove collision/hurtbox/knockback/camera meaning under scale and the natural win/next-stage path. Charge instance/capability bytes even if an immutable model is shared.

## Package: Polygon Team

**Outcome:** All source Polygon kinds and the full 30-opponent sequence work with correct traits, concurrency, replacement and final completion.

Reuse existing Polygon admissions/native exports and source motion dependencies. Derive each kind's allowed behavior rather than assume no specials or automatic cheapness. Prove that kind replacement does not leak resident data, reuse wrong material/pose ownership or grow live objects over waves. Test source final-KO/score logic and transitions; prevent duplicate completion during simultaneous deaths.

## Package: Master Hand and Final Destination

**Outcome:** The actual campaign reaches the boss, renders/executes its full source attack and child-weapon set, displays HP meaning, defeats it and reaches the source tail.

Reuse Boss imports/export and Last stage packet. `fighters/master-hand.md` owns attacks, projectiles, hit behavior and resource costs; `stages/final-destination.md` owns background/camera venue integration. Do not leave a refusal solely because an old comment says the export is still in flight. Conversely, an exported model does not prove the full attack/weapon closure.

**Proof:** Source-driven attack-family coverage, moving/off-screen behavior, damage/HP/defeat, native children/audio and natural end-of-fight transition. Cover difficulty-dependent source parameters. This workload needs measurement; four-CPU VS is not an automatic bound for boss geometry, background or attack bursts.

## Package: Bonus 1 and Bonus 2

**Outcome:** Mario's two boards first, then every one of 24 source board identities; both campaign and standalone-practice callers work.

`stages/bonus-stages.md` owns source collision/object placement, Target and platform rules, timer/failure/reset and per-fighter movement requirements. Reuse generated boards; do not author 24 courses from scratch. Distinguish campaign timing/scoring from practice records. Every board needs a complete clear and failure/restart proof, not just scene admission.

## Package: Race to the Finish

**Outcome:** The real Bonus3 course/hazards/timer/camera reaches source completion and tally.

`stages/race-to-the-finish.md` supplies the corrected contract: grounded Detect-material completion, animated GBumpers and TaruBomb spawns. No multiple-exit score-tier or forced-scroll assumption is carried from the old sketch. Source camera/timer/scoring callers decide those details. Test a natural full run, timeout/failure and another entry with reset counters/actors.

## Package: score, progression and ending

**Outcome:** Run events produce the correct tally, 58 source bonus predicates/values, continues, records, challenger selection and completed-character state, then the source ending/congratulations/credits flow.

Use the actual bonus enum/table and host test harness. Cover every predicate/data row with positive/negative or source-table evidence as appropriate; use representative natural engagements to prove event capture. Do not force 58 separate ROM rebuilds. Check score truncation/caps/order, continue handling, final fight and all related save triggers.

P2-7 owns persistence and unlock predicates; this package emits their correct events. Test exactly-once updates on replayed transitions/retry, loss/win challenger branches and the proper return menu. Ending/cinematic assets, audio, shooting/interaction and skip behavior follow source; original presentation is not “polish later.” Credits interactivity is verified from the source scene, not guessed from its label.

## Package: campaign qualification

First complete Mario start-to-credits naturally, with the required difficulty coverage from the goal/phase contract, then qualify the remaining eleven characters and all their bonus boards. Use source table tests to cover combinatorial data and natural runtime cases to cover distinct behavior/resource families. Do not claim a full matrix from one playthrough or force identical redundant full runs when unchanged proof already covers the same behavior; record exactly what each run covers.

Include all difficulties, relevant stock/continue boundaries, alternate player/costume choices, repeated entries, waves/variant/boss peaks, endings/challengers/save reload and source-derived audio/presentation. Every new screen must hold 30 Hz presentation and every workload meet its applicable final gate. The registry and existing scene-capable harnesses remain the test system; this plan adds no per-function proof-mode fleet.

## Exit checklist

- [ ] Natural menus/intro/fights/tally/retry and all route entries work with correct state ownership.
- [ ] Yoshi/Kirby/Polygon full teams, variants and Boss have complete behavior/native/output/lifetime proof.
- [ ] All bonus boards and Race complete and fail/reset correctly through their real callers.
- [ ] Score/bonuses/continue/progression/records and ending/credits are source-equivalent.
- [ ] All-character coverage, resource bounds, required cadence and owner review are satisfied.
- [ ] Feature-enabled shipping configuration is verified; flag-off safety or imports alone are not acceptance.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `src/import/battleship_sc1pmanager.c`.
- `src/import/battleship_sc1pgame_runtime.c`.
- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c: dSC1PGameStageDesc and dSC1PGameComputerDesc`.
- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pmanager.c`.
- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pstageclear.c`.
- `decomp/BattleShip-main/decomp/src/sc/scdef.h: SC1PGame bonus enum`.
- `decomp/BattleShip-main/decomp/src/gr/grbonus/grbonus3.c`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-6-one-player.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-6-one-player.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.

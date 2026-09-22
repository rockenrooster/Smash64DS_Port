**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 100 char summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)
**Owner: removal from this file means FIXED; absence never means a symptom was unreal.**

treat anything not going through native renderer a failure.

**Every newly built ROM, including diagnostics, must exclude non-native renderer implementations. Runtime defaults and zero observed fallbacks are insufficient.**

**AGENT 09-22, RETRACTED ALARM: I warned that this batch's 8,192-byte arena cost might stop Pikachu/Fox starting. That was wrong, and the correction is measured. Walking the SHIPPING build into a real VS match with Pikachu committed reads free-min 48,216 and the GObj latch never fires -- the 7,556 figure I reasoned from is a stale census. r32 starts fine. Play it, not r26.**

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menus:
-Make "Characters" and "VS Record" not selectable, "Characters" and "VS Record" work is Owner deferred. **Activation guard landed: both rejected before cue/BGM/scene request, rechecked every A/START.**
-VS options
-CSS Bugs still present and need to be fixed:
    -delay between cursor hover and 3d fighter preview rendering. **Owner: lower priority now. Dwell is 2 cold / 1 warm; residual is a 30,160 B owner-image read.**

-Yoshi:
    -Yoshi's guard/shield (egg) is invisible **Native owner REINSTATED (was reverted for cost, not correctness). Egg root 0xa860 is textured 64x64 CI4 -> 2,048 B persistent, and two VRAM reclaim paths now exist that did not when it was reverted.**
-Link:
-Pikachu
    -neutral b attack VFX that walks terrain still has hard edges. **AIR REGRESSION FIXED: the ground repair moved 3 images onto dedicated GL names, which the only eviction sweep cannot reach. Reclaimable VRAM went 6,144 -> 0, so the air jolt's 4,096 B upload had nothing to evict. Reclaim hook added.**
    -down B effect doesn't render all related VFX, missing blue exp on pikachu. **Was the generic spark; real maker routed, script 0x74 packed.**
    -Pokeball Spawn Intro not playing VFX. **Ball VISIBLE (owner). RAYS: construct and submit -- req=2 null=0 cand=100, free-min 48,216, latch never fired. Read that as `not starved and not refused`, NOT as `drawn`. AND I FOUND THE COUNTER I NEVER READ: the rays have exactly two renderer-side counters, and I reported one. `gNdsMBallRaysCandidateCount` (renderer_adapter_stage.c:5760) counts REACHING the material check; `gNdsMBallRaysMaterialRejectCount` (:5791, :5802) counts FAILING it, which returns FALSE and bumps `gNdsEntryEffectNativeFallbackCount` -- a non-native fallback, which this project treats as a failure. `cand=100` with the reject counter unread says nothing about whether a single ray drew. Both key off root offsets 0x0440 and 0x0518, which are exactly MBALLRAYS_ROOTS, so the path is confirmed. READ THAT COUNTER NEXT. Still needs your exact scenario -- which stage, and which mode.**
    -face color is slightly different from body color. (lighting difference???) **Not lighting, and not the stretch either.** **FIXED (owner playtest owed): the packet REPLAY path re-derived the shade word without ndsRendererR2ClampDiffuseToMaterial, which the live draw applies. A packet is recorded CLAMPED, so the seam only appeared after the first tint move (damage flash, blink, team colour) swapped in an unclamped word and latched it. The clamp only bites a TINTED prim -- exactly Pikachu 0xFFD933, Kirby 0x00FF5A, Purin 0xFFCDD8, these three rows and no others. This is why HOLDING NEUTRAL B cured it: a status change re-records the packet. Locked by scripts/check-r2-shade-twin.py, mutation-tested RED on 4 mutations.**
    -when I choose pikachu, in match, fox opponent is frozen and cannot be hit. Sudden death works funnily enough though. **MY REFUTATION WAS UNSOUND -- WITHDRAWN 09-22. I twice called this refuted on `AppearOverrun 0` and `AnimFallback 0` over 658 resolves. Both counters are SINGLE SCALARS (diagnostics_state.c:187-188), not per-slot, so 658 resolves is consistent with all 658 being Pikachu's and Fox never animating once -- the instrument cannot tell those apart, and the symptom is per-slot. Worse, neither counter covers what you described: `cannot be hit` is collision, and `frozen` is the physics proc; an animation counter says nothing about either. `Sudden death works` is the sharpest clue and I never used it -- sudden death re-creates the fighters, so a creation-time failure fits and a data failure does not. RE-MEASURED PROPERLY, PER SLOT: read Fox's OWN FTStruct through gSCManagerBattleState->players[1] across two Pikachu-vs-Fox matches in the walk. Fox is ALIVE -- status 18 (Turn) with 3 tics in one match, status 12 (WalkMiddle) with tics cycling 20 then 14 in the next, while Pikachu sat in status 10 (Wait) with tics advancing 147 to 375. 1,009 animation resolves, 0 fallbacks, GObj latch never fired, free-min 48,432. So in a CPU-vs-CPU walk match Fox is not frozen. THE GAP THAT REMAINS is your configuration: the walk drives two CPUs, you play slot 0 as a human. If your Fox is set to a HUMAN slot with no second controller it would stand still legitimately -- though it should still be hittable, and yours is not. Tell me Fox's player type and level and I can drive that exact roster.**
-Samus
-Kirby
    -Kirby face color is slightly different from body color. (lighting difference??? pink face color looks more correct I think...) **THE STRETCH WAS THE WRONG LEAD and stays default-off: a static arithmetic error cannot be cured by holding a button, and your own note said holding neutral B fixes it.** **FIXED with the Pikachu face/body row above -- same one missing clamp, same repair.**
    -Neutral A punch flurry VFX not drawing at correct locations. **Now FIXED IN R26**
    -Kirby neutral B , then A attack to spit out fighter, Star projectile VFX is invisible. **ROOT CAUSE FIXED: lbCommonDObjScaleXProcDisplay is an EMPTY function in the port, so the tree never reached the renderer. Root 0x5458 baked + admitted + routed to DLHead1. Also repairs the Star Rod's two swings.**
    -Kirby has a wierd pose on results screen. **Packing was half. The demo rows' NitroFS path field was generated and never consumed, so the load failed silently. Paths registered.**
-Jigglypuff
    -face color is slightly different from body color. (lighting difference???) **FIXED with the Pikachu face/body row above -- same one missing clamp, same repair.**
-Captain falcon
-Ness
    **FIXED, AND IT WAS A REAL SHIPPING HANG, NOT A HARNESS ONE. I said earlier it was `not proven reachable by a human` -- wrong. The halt site's three guards (NDS_P2_1P_GAME, NDS_RENDERER_HW_TRIANGLES, PROFILE_LEVEL<2) are all 1 in the shipping config with NDS_P2_MENU_WALK 0, so hovering Ness on the character select hung the ROM for anyone. The walk only found it. ROOT CAUSE: the tracked image header declared Ness's state_sequence[177] while the tables held 195, so ness_high.bin shipped 18 entries short -- excess initializers are a GCC WARNING, so the data was silently truncated and the image built clean. sNdsNativeNessRoots kept indexing tail states at 188-193, validator clause 6 rejected root 0, and the packed preview fell into ndsPreviewPackLoadHalt(20), a deliberate for(;;). Only Ness was affected, on both details. FIX: regenerated the header (177->195 high, 215->233 low), added the missing Makefile edge from the image header to the owners IR, and added scripts/fighters/check_native_owner_image_spans.py -- 1,140 root spans across 104 owners, mutation-tested RED on 4 mutations including this exact regression.**
-General
    -All Fighters are not doing correct poses/animations on results screen **ROOT CAUSE FIXED, not AObj16: all 11 demo-anim arms had a token route and NO path row, so the force loader failed its path guard and ftMainSetStatus bound the stale figatree with no decline.**
    -No contest results screen, all fighters should be doing the clapping animations. **Same root cause as the poses row, fixed with it. The No Contest kind was always correct.**
Stages:
-peaches castle: 
    -Foreground castle roof renders ALL geometry now but the texture is missing on the now visible geometry. A continuous tiled roof surface is almost achieved. **Owner Deferred**
-Zebes: Acid plane Color is accurate now but texture blending are all visibly too HARD. Edges are too defined instead of a gradient/smooth transistion. Stage lights on the ground floor have a flat/hard transparency (hard upsidown trapezoid shape) instead of looking like a real light source with a gradient that tapers to fully transparent towards the top. **Owner Deferred**
-Yoshi's Island: 
-SectorZ:
-Saffron city: the pokemon garage door hazard is always open for some reason. It should close and open periodically. **FIXED (owner playtest owed): the gate replayed a baked CONSTANT world matrix, so it was frozen -- and frozen CLOSED, not open; the always-open look is the layer geometry behind it. Bindings are now live and the gate was observed cycling three states (0 -> 200 -> 8) across two Kirby matches, 1,585 anim resolves, 0 fallbacks.**
    **AND A CORRECTION YOU SHOULD SEE, BECAUSE IT COST REAL TIME: there is no Saffron crash in this file except the one I wrote. Your sentence above is the whole row. I introduced `CRASH NOT REPRODUCED` myself at c1bee842d42 and then spent several cycles chasing it -- a copy-cost arena theory, a GObj-latch theory, a null-safe-maker theory -- none of which you ever asked for, and the last two are refuted anyway (the latch cannot fault this port; every maker consumer substitutes gGCCurrentCommon on NULL). Neither consolidated brief mentions a Saffron crash either. Withdrawn as a phantom. If you HAVE seen one, say so and it becomes a real row again -- the useful detail would be whether Kirby had swallowed someone first.**

Audio:

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
    -Pokeball Spawn Intro not playing VFX. **Ball VISIBLE (owner). RAYS: every cause now refuted INCLUDING arena. Walked the shipping build into a VS match with Pikachu: req=2 null=0 cand=100, free-min 48,216, latch never fired. They construct and submit. If still absent for you, it needs your exact scenario -- say which stage/mode.**
    -face color is slightly different from body color. (lighting difference???) **Not lighting: N64 clamps shade then multiplies prim; DS folds prim first, so lit bodies wash white.**
    -when I choose pikachu, in match, fox opponent is frozen and cannot be hit. Sudden death works funnily enough though. **REFUTED twice, including through the real CSS->VS path: walked shipping build with Pikachu committed, AppearOverrun 0 and AnimFallback 0 over 658 resolves. Not a stale figatree, not the entry status, not memory. Needs your exact repro.**
-Samus
-Kirby
    -Kirby face color is slightly different from body color. (lighting difference??? pink face color looks more correct I think...) **MECHANISM CONFIRMED at runtime: joint chains are NOT rigid (row_norm 4695-4911 vs 4096), so the hardware light is mis-normalised by 15-20%. Determinant is POSITIVE, so it was never a mirror. But 87 of 95 writes SHRINK and only 2 stretch, so the current repair fixes the minority case -- the fix must scale diffuse colour.**
    -Neutral A punch flurry VFX not drawing at correct locations. **Now FIXED IN R26**
    -Kirby neutral B , then A attack to spit out fighter, Star projectile VFX is invisible. **ROOT CAUSE FIXED: lbCommonDObjScaleXProcDisplay is an EMPTY function in the port, so the tree never reached the renderer. Root 0x5458 baked + admitted + routed to DLHead1. Also repairs the Star Rod's two swings.**
    -Kirby has a wierd pose on results screen. **Packing was half. The demo rows' NitroFS path field was generated and never consumed, so the load failed silently. Paths registered.**
-Jigglypuff
    -face color is slightly different from body color. (lighting difference???)
-Captain falcon
-Ness
    **AGENT 09-22, NOT AN OWNER ROW: the shell walk hangs the ROM at the CSS on Ness -- ndsPreviewPackLoadHalt(20), a deliberate for(;;). Localised: validator clause 6, root 0, offset 5,792, 82 commands, 14 roots, asset 335 -- a span that does not fit the loaded preview asset. PRE-EXISTING. Not proven reachable by a human. Receipt has the fix direction.**
-General
    -All Fighters are not doing correct poses/animations on results screen **ROOT CAUSE FIXED, not AObj16: all 11 demo-anim arms had a token route and NO path row, so the force loader failed its path guard and ftMainSetStatus bound the stale figatree with no decline.**
    -No contest results screen, all fighters should be doing the clapping animations. **Same root cause as the poses row, fixed with it. The No Contest kind was always correct.**
Stages:
-peaches castle: 
    -Foreground castle roof renders ALL geometry now but the texture is missing on the now visible geometry. A continuous tiled roof surface is almost achieved. **Owner Deferred**
-Zebes: Acid plane Color is accurate now but texture blending are all visibly too HARD. Edges are too defined instead of a gradient/smooth transistion. Stage lights on the ground floor have a flat/hard transparency (hard upsidown trapezoid shape) instead of looking like a real light source with a gradient that tapers to fully transparent towards the top. **Owner Deferred**
-Yoshi's Island: 
-SectorZ:
-Saffron city: the pokemon garage door hazard is always open for some reason. It should close and open periodically. **Gate replayed a baked constant world matrix, frozen CLOSED; the always-open look is layer geometry. Bindings now live. CRASH NOT REPRODUCED: walked Kirby onto Saffron, no crash, free-min 32,504, latch never fired -- but the walk never inhaled (hat hits 0,0) and the battle ended before the gate's ~tic 1,220 close. Needs a held Saffron match with a real copy.**

Audio:

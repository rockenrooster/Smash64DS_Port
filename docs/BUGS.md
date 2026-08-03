**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

This board carries verdicts and the numbers that check them. The forensics for each
fix live next to the code that owns it -- the particle generator and its checker,
`battleship_lbparticle.c`, `battleship_efmanager.c`, `render-audio-fgm-phase-pack.py`
-- so a row here should not need to be an essay.

-Some Crowd noise audio cues get cut off (like for big hits that reach upper bound KO boundary).
    Owner: Ok if source cuts them off, then lets change that, I don't want the sound cues interrupted if possible.

-Respawn floating platform isn't visible when respawning after KO.
    Owner: is don't see the floating revival platform at all. the Halo is not the correct asset to use
    WRONG PROBLEM SOLVED: I fixed the halo's RESOLUTION (16x8 -> source 32x16). You are saying the halo
    is the wrong ASSET. The revival PLATFORM is a separate source object; routing is the open question.

-Fox down B VFX is not correct or using correct asset.
    Owner: you are still not using the correct asset for Fox's down B reflector.
    WRONG PROBLEM SOLVED: A3I5 fixed the PALETTE (8 -> 32 entries, so two blues fit). Still unproven
    that relocData/346 is the reflector the source draws. Verify the asset id before touching format again.

-Shield VFX not correct
    Owner: texture looks cut in half: `artifacts/visibility/2026-08-03_owner_shield-cut-in-half.png`
    LEAD: the cell is 16x32 (1:2) at source now, and the quad it draws on is square -- a 1:2 texture on a
    1:1 quad reads as cut in half. Check the quad's aspect against row->width/row->height, not the cell.

-Hard landing vfx not not using correct asset.
    Owner: incorrect asset for the impact wave is being used
    WRONG PROBLEM SOLVED: same as above -- I fixed the shockwave's resolution, not which texture routes.

-KO VFX not drawing correctly.
    Owner: Not fixed yet, the "blast pillar" VFX isn't drawing, and doesn't seen to draw on the same z axis as the fighters
    OPEN: the v16 rail fix moved the twinkle to the fighter's depth and that part holds. The blast PILLAR
    is a separate effect and has not been traced to a source maker yet.

-Results confetti doesn't look right.
    Owner: confetti falls behind fighter instead of infront and is not centered on the camera view: `artifacts/visibility/2026-08-03_owner_confetti-behind-fighter.png`
    **STRUCTURAL DIFFERENCE FOUND** (mnvsresults.c:3208 + efmanager.c:6206). The source makes TWO confetti
    emitters at DIFFERENT DEPTHS, on DIFFERENT GENERATOR LINKS:
      pos0 = (0, 1000, -1000) is_genlink_mask FALSE -> bankID | LBPARTICLE_MASK_GENLINK(3)  = link 3, BEHIND
      pos1 = (0, 1000,  -400) is_genlink_mask TRUE  -> bankID                                = link 0, IN FRONT
    You are seeing link 3 and not link 0, which is exactly "falls behind the fighter instead of in front".
    The port's draw loop gates each link on `gobj->camera_mask & (1 << link)`
    (battleship_lbparticle.c), so the next step is proving which of links 0/3 that mask admits at Results.
    Both emitters sit at x=0: "not centered on the camera view" is the Results camera, not the emitter.

-Some "hard hit" (side A attacks that hit) VFX look too big, please apply correct scaling to VFX.
    MEASURED: scale is source-exact (efmanager.c:2175/2197). Last cycle's clamp was in UNREACHABLE code.

-Shield freeze bug happened again. Screenshot: `artifacts/visibility/2026-08-03_owner_shield-freeze.png`
    (copied into the repo from your Pictures folder -- tracked files must not carry your name.)
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

-Fox down B VFX is not correct or using correct asset.
    Owner: you are still not using the correct asset for Fox's down B reflector.

-Shield VFX not correct
    Owner: texture looks cut in half: `artifacts/visibility/2026-08-03_owner_shield-cut-in-half.png`

-Hard landing vfx not not using correct asset.
    Owner: incorrect asset for the impact wave is being used

-KO VFX not drawing correctly.
    Owner: Not fixed yet, the "blast pillar" VFX isn't drawing, and doesn't seen to draw on the same z axis as the fighters

-Results confetti doesn't look right.
    Owner: confetti falls behind fighter instead of infront and is not centered on the camera view: `artifacts/visibility/2026-08-03_owner_confetti-behind-fighter.png`

-Some "hard hit" (side A attacks that hit) VFX look too big, please apply correct scaling to VFX.
    MEASURED: scale is source-exact (efmanager.c:2175/2197). Last cycle's clamp was in UNREACHABLE code.

-Shield freeze bug happened again. Screenshot: `artifacts/visibility/2026-08-03_owner_shield-freeze.png`
    (copied into the repo from your Pictures folder -- tracked files must not carry your name.)
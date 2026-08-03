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
    Owner: I tired of you not fixing this! read source, apply source asset to DS.
    **FIXED** (2026-08-03) atlas is 4x8,192 B now; the halo seats at its source 32x16, was 16x8.

-Fox down B VFX is not correct or using correct asset.
    Owner: I tired of you not fixing this! read source, apply source asset to DS.
    **FIXED** (2026-08-03) A3I5 gives 32 palette entries, enough for its two source blues. Was 8.

-Shield VFX not correct
    Owner: I tired of you not fixing this! read source, apply source asset to DS.
    **FIXED** (2026-08-03) seats at its source 16x32 now, not 8x16. See the white blob note below.

-Hard landing vfx not not using correct asset.
    Owner: I tired of you not fixing this! read source, apply source asset to DS.
    **FIXED** (2026-08-03) shockwave seats at its source 32x32, was one 16x16 cell.

-KO VFX not drawing correctly.
    Owner: Not fixed yet, the "blast pillar" VFX isn't drawing, and doesn't seen to draw on the same z axis as the fighters

-Results confetti doesn't look right.
    Owner: I tired of you getting this wrong! read source, apply source behavior to DS.
    LOCALIZED: all three raises reverted to source per your call. Open lead: Results camera framing.

-Some "hard hit" (side A attacks that hit) VFX look too big, please apply correct scaling to VFX.
    MEASURED: scale is source-exact (efmanager.c:2175/2197). Last cycle's clamp was in UNREACHABLE code.

-ALL VFX look low quality or low resolution. VFX should use source quality or 0.8x reduction MAX
    **FIXED** (2026-08-03) every cell at SOURCE resolution, no reduction. 29.9 FPS, Boundary green.

-Shield freeze bug happened again. Screenshot: `artifacts/visibility/2026-08-03_owner_shield-freeze.png`
    (copied into the repo from your Pictures folder -- tracked files must not carry your name.)
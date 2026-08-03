**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

This board carries verdicts and the numbers that check them. The forensics for each
fix live next to the code that owns it -- the particle generator and its checker,
`battleship_lbparticle.c`, `battleship_efmanager.c`, `render-audio-fgm-phase-pack.py`
-- so a row here should not need to be an essay.

-Some Crowd noise audio cues get cut off (like for big hits or upper bound KO).
   Owner: Still not fixed, not sure why they get cut off
    MEASURED (5-min both-CPU soak, NO-FREEZE): nothing steals and nothing retires early.
    PrematureRetire 0, PoolExhaust 0, GenerationMismatch 0, StaleStop 0, StopAll 0, MaxHandles 7 of 8.
    The cut-off was the release window closing at the NOTE end while the SAMPLE still had 129-309 ms
    left; nds_audio_fgm.c:1014 now releases at max(note, audible) for non-looping cues. Owner re-test.
    NEXT DIMENSION IF IT RECURS -- the MIX, which no row here ever measured. Nothing sets a DS master
    volume (only soundEnable), so there is zero headroom: 35 of 88 cues decode at the 32767 rail, and
    crowd 616 + hit 31 both sit there at ds_volume 127, summing to 200% with up to 7 handles live.
    Peaks coincide rather than sustain (crowd RMS 2k-10k), so this predicts crackle, not masking --
    stated as the untested lead it is, not swapped in as a second cause.

-Respawn floating platform isn't visible when respawning after KO.
    Owner: platform is invisible.
    MEASURED: 88 of 128 texels quantised to a near-black palette entry. Now white, alpha carries shape.

-Fox down B VFX is not correct or using correct asset.
    MEASURED: the asset is CI4 16x16 at 0x18 of relocData/346.vpk0.bin using only 2 palette indices --
    208 texels index 1, 48 index 2 in a 3-column band. Its LUT at 0x08 gives the deep blue (0,8,239)
    and cyan (0,231,247) this shipped INVERTED. Colours now taken from that LUT.
    The atlas route is refuted, not skipped: A5I3 carries shape, and this texture is colour with no
    shape -- its 81% body maps to alpha 0 and vanishes. The hexagon is geometry, not texture.

-Shield VFX not correct
    Owner: Looks Black in color for some reason
    MEASURED: colour passed as 0xRRGGBB where the API takes BGR555, so red packed to black.

-Hard landing vfx not correct or not using correct asset.
    MEASURED: dust was wired, the shockwave was not. nEFKindImpactWave fell through; now routed.

-KO VFX not drawing correctly.
    Owner: looks like its drawing too close to camera and is low quality
    MEASURED: the burst now builds in full -- KOBurstAttempt 3, Complete 3, DropMask 0, QuadMiss 0,
    over three KOs. It was 4 of 6 while the transform/generator pools were saturated; those are now
    24 and read 13/11, strictly below the cap, so the demand is measured rather than floored.
    "low quality" is the atlas cell (34 cells in 8,192 texels, ~16x16, one frame each) -- that bound
    was raised as a formal decision and ANSWERED 2026-08-02 on the board. Not reopened here.

-Results confetti doesn't look right.
    Owner: STILL doesn't cover the whole scene, when troublshooting, just play results screen.
    MEASURED: coverage is pieces x AREA, and three prior raises only ever bought pieces -- size sat at
    the source's 20.0 while the owner's own first wording was "pieces do not look like they are large
    enough". Now 32.0: sizepatch=4, maxsize 20->32 on both slots, 2.56x the area at the SAME 192+192
    pieces, 384 pool and 24 generators, and census-results-frame-cost reads 3.95 VBlanks/present --
    unchanged, so it is free. Fan-out also confirmed reaching the pieces: fan=6, x -1175 to +983.
 
-Star KO twinkle not playing in correct spot
    Owner: Still not playing at location of the fighter, VFX also looks low quality
    MEASURED: the caller is right. Over three KOs the spawn read (3451, 2399, -14999) -- y is
    camera_bound_top * 0.6 and z is the source's own DeadUpStar recession, both set in ftcommondead.c
    case 0 BEFORE the sparkle fires in case 1. It is at the fighter; the fighter is at the top blast
    zone and goes invisible on the next line, which is what makes it look detached. Not a position bug.
    "low quality" is the same answered atlas bound as the KO row: tex 29 resolves (QuadMiss 0) but
    into a ~16x16 single-frame cell, scaled up. Same closed decision, not reopened.

-Some "hard hit" (side A attacks that hit) effects look like they don't belong there's an orange ball visual effect that looks too big.
    MEASURED: light spark scale ramped unbounded to 4.9x at 40 damage; heavy is 1.0. Clamped 2.2.

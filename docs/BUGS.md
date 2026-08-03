**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

This board carries verdicts and the numbers that check them. The forensics for each
fix live next to the code that owns it -- the particle generator and its checker,
`battleship_lbparticle.c`, `battleship_efmanager.c`, `render-audio-fgm-phase-pack.py`
-- so a row here should not need to be an essay.

-Some Crowd noise audio cues get cut off (like for big hits or upper bound KO).
   Owner: Still not fixed, not sure why they get cut off
    LOCALIZED: nothing steals -- handle pool and sample cache both fail closed. Read gNdsAudioFgmPrematureRetireCount.

-Respawn floating platform isn't visible when respawning after KO.
    Owner: platform is invisible.
    MEASURED: 88 of 128 texels quantised to a near-black palette entry. Now white, alpha carries shape.

-Fox down B VFX is not correct or using correct asset.
    CONTRACT: source asset is llFoxSpecial2ReflectorDObjDesc, offset 0x2b0 of Fox's Special2 reloc file.

-Shield VFX not correct
    Owner: Looks Black in color for some reason
    MEASURED: colour passed as 0xRRGGBB where the API takes BGR555, so red packed to black.

-Hard landing vfx not correct or not using correct asset.
    LOCALIZED: no Landing-named function spawns an effect; they come from ftParamMakeEffect, ftparam.c:1966.

-KO VFX not drawing correctly.
    Owner: looks like its drawing too close to camera and is low quality
    LOCALIZED: "too close to camera" is the identity-matrix symptom; the burst draws outside the particle camera.

-Results confetti doesn't look right.
    Owner: STILL doesn't cover the whole scene, when troublshooting, just play results screen.
    MEASURED: source spawns both emitters at x=0 (mnvsresults.c:3212); spread comes from per-piece randomisation.
 
-Star KO twinkle not playing in correct spot
    Owner: Still not playing at location of the fighter, VFX also looks low quality
    LOCALIZED: source spawns at the fighter's TopN joint translate (ftcommondead.c:357). Check the port's caller.

-Some "hard hit" (side A attacks that hit) effects look like they don't belong there's an orange ball visual effect that looks too big.
    MEASURED: light spark scale ramped unbounded to 4.9x at 40 damage; heavy is 1.0. Clamped 2.2.

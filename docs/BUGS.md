**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

This board carries verdicts and the numbers that check them. The forensics for each
fix live next to the code that owns it -- the particle generator and its checker,
`battleship_lbparticle.c`, `battleship_efmanager.c`, `render-audio-fgm-phase-pack.py`
-- so a row here should not need to be an essay.

-Whispy blow VFX not correct and not at correct location.
    Owner: Emitter position source looks better. But emitted objects turn flat at end of lifetime???
    LOCALIZED: primcolor.a was never submitted, so every particle drew fully opaque its whole life.

-Some Crowd noise audio cues get cut off (like for big hits or upper bound KO).
   Owner: example: fgm360-nSYAudioVoiceFoxDeadUp-as-ds-plays-it.wav is the sound that cuts off a crowd sound that happens during a big hit.
    LOCALIZED: five cues outlive their own note, and the release ramp began before the waveform ended.

-Respawn floating platform isn't visible when respawning after KO.
    Owner: not using correct asset for the revival platform.
    CONTRACT: source is a four-node DObj chain reusing the MBallRays lists; the port draws a disc.

-The rolling dodge sound (escape roll?) sounds off, maybe too loud???
    Owner: sounds better, do one more volume down pass only for this asset.
    MEASURED: FGM 11 only, volume 68 -> 48. A third -3 dB, -8.4 dB total. Checker PASS.

-Correct VFX isn't played for various things (running foot dust VFX, fireball hit VFX, fox down B, shield, hard landing vfx, etc)
    Owner: better but looks extremely pixelated and low quality.
    MEASURED: every named effect 8x8 -> 16x16 inside the same 8,192-byte sheet, one frame not two.

-KO VFX wrong.
    Owner: doesn't look correct, like not all vfx are played or its too low quality
    MEASURED: 2 of 6 KO bursts drew nothing -- the transform pool saturated at 6. Now 24.

-Results confetti doesn't look right.
    Owner: I see its visible now but doesn't cover the whole scene, when troublshooting, just play results screen.
    MEASURED: pool 112 -> 384 and rate 0.42 -> 1.26 together. 62 -> 244 pieces visible.

-A new SFX FGM 153 AltitudeWarn plays at wrong trigger.
    LOCALIZED: not the trigger. alt_warning read +3500 for Dream Land's -2900 -- a word-swap pairing bug.

-Hitting Fox's shield freezes match sometimes.
    Owner: NOT FIXED
    MEASURED: root-caused. The realtime present never rewound the graphics heap: 16 bytes leaked per frame.

-Shield VFX is not correct.
    Owner: no progress made visually yet
    CONTRACT: the source shield is a textured quad, not a disc; the sheet now has room for its texture.
    
-Right side of stage looks like it compresses VFX (not sure if left side does it too...)
    MEASURED: particle vertices railed at 2047.9 world units, squashing x. Reach doubled; it did both sides.

-Star KO twinkle not playing in correct spot
    LOCALIZED: same rail. The twinkle fires near the top blast zone, well past the old reach.
    
-fgm12-nSYAudioFGMDeadUpStar-as-ds-plays-it.wav still sounds too harsh compared to original n64 cue
    MEASURED: the AOT render normalised to full scale. Volume 127 -> 90, the cue's own ucd_volume.

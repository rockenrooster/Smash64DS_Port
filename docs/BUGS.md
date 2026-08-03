**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

This board carries verdicts and the numbers that check them. The forensics for each
fix live next to the code that owns it -- the particle generator and its checker,
`battleship_lbparticle.c`, `battleship_efmanager.c`, `render-audio-fgm-phase-pack.py`
-- so a row here should not need to be an essay.

-Some Crowd noise audio cues get cut off (like for big hits that reach upper bound KO boundary).
    Owner: I tired of you getting this wrong! read source, apply source behavior to DS.
    MEASURED: source cuts them BY DESIGN -- ftpublic.c:165 stops the crowd when a call starts.

-Respawn floating platform isn't visible when respawning after KO.
    Owner: I tired of you not fixing this! read source, apply source asset to DS.
    BLOCKED(decision: atlas is 8,192 B by per-bank contiguity; source asset needs a cell it cannot hold).

-Fox down B VFX is not correct or using correct asset.
    Owner: I tired of you not fixing this! read source, apply source asset to DS.
        MEASURED: 8 shared palette entries could not hold its two blues. Atlas is A3I5, 32 entries now.

-Shield VFX not correct
    Owner: I tired of you not fixing this! read source, apply source asset to DS.
    BLOCKED(decision: same 8,192 B atlas bound -- IA8 16x32 source seats at 8x16).

-Hard landing vfx not not using correct asset.
    Owner: I tired of you not fixing this! read source, apply source asset to DS.
        BLOCKED(decision: same atlas bound -- shockwave routes, but at one 16x16 cell of a 32x32 source).

-KO VFX not drawing correctly.
    Owner: I tired of you getting this wrong! read source, apply source behavior to DS.
    **FIXED** (2026-08-03) v16 rail drew burst quads at z -2047, not the fighter's depth. Boundary green.

-Results confetti doesn't look right.
    Owner: I tired of you getting this wrong! read source, apply source behavior to DS.
    LOCALIZED: all three raises reverted to source per your call. Open lead: Results camera framing.

-Star KO twinkle not playing in correct spot
    Owner: I tired of you getting this wrong! read source, apply source behavior to DS. twinkle needs to play at the same location as the fighter location.
    **FIXED** (2026-08-03) it spawns at the fighter (z -14999); world->v16 railed it to -2047. Range fixed.

-Some "hard hit" (side A attacks that hit) VFX look too big, please apply correct scaling to VFX.
    MEASURED: scale is source-exact (efmanager.c:2175/2197). Last cycle's clamp was in UNREACHABLE code.

-Shield hit freeze is back please permanently fix it so it never happens in the future.
    **FIXED** (2026-08-03) source's overflow check ends in while(TRUE); the DS build records and continues.

-VFX get x flattened around stage edges, this needs to be expanded more. why is there a limit anyways???
    **FIXED** (2026-08-03) the limit was v16, +/-2047.9 world. Per-batch adaptive scale; clamp count 0.

-ALL VFX look low quality or low resolution. VFX should use source quality or 0.8x reduction MAX
    BLOCKED(decision: 32 KB atlas MEASURED at 8.6 FPS with untextured stage; bound is contiguity, not bytes).

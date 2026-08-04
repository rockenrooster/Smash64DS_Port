**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

- I see the floating platform, but its colors look off (too dark?), like it isn't rendering correctly.
  LOCALIZED: beam is dl_link list 1, a separate source RDP stream; port collapses all four heads into one.
  
- I see the death explode blast pillar thing, but the colors look off (too dark?) and it doesn't seem to play at the players death location off screen, check x,y,z coords. Also if a player hits the side boundaries, it should play horizontally im pretty sure.
  LOCALIZED: owner accepts current pillar; N64's whole-screen white KO flash is a deliberate omission, never re-add.

- Shield bubble is correct asset, looks off, (too dark?)
  LOCALIZED: now inherits the source's own translucent mode; unchanged by eye, cause not yet named.
  
- Impact wave, not showing the green impact effect looks gray/black instead.
  LOCALIZED: never captured; arming counters gNdsEffectImpactWave* now record spawn, index and NULL-pool at its maker.

- Fox reflector is green for some reason, should be blue

- I see missing textures/texture corruption. But ONLY AFTER dying. I don't know if the death explode blast pillar or the floating revival platform triggers it. once it triggers, some things lose their textures.

- Check scaling on the hit effects (for example, A attacks, Forward A, strong A), some look bigger than they should be.

- The results screen confetti, shouldn't the spawner for the emitter be just above, out of frame with the camera so we don't "see" them spawning?

- (found by instrumentation, not play) After the first KO the stage permanently drops its 24 pinned textures and runs slower for the rest of the match. 
    MEASURED - fix proven on the natural path; tick/VBlank A/B still owed before any speed claim.


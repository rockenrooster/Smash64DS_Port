**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

- I see the floating platform, but its colors look off (too dark?), like it isn't rendering correctly.
  LOCALIZED: beam list is source-white with texture coverage; drawn opaque because no render mode reaches it.
- I see the death explode blast pillar thing, but the colors look off (too dark?) and it doesn't seem to play at the players death location off screen, check x,y,z coords. Also if a player hits the side boundaries, it should play horizontally im pretty sure.
- Shield bubble is correct asset, looks off, (too dark?)
  LOCALIZED: its XLU blend comes from efDisplayXLUProcDisplay, whose gDPSetRenderMode still zeroes in gbi.h.
- Impact wave, not showing the green impact effect looks gray/black instead.
  LOCALIZED: same stubbed gDPSetRenderMode; its own proc emits G_RM_AA_ZB_XLU_SURF and it is discarded.
- Fox reflector is green for some reason, should be blue
- I see missing textures/texture corruption. But ONLY AFTER dying. I don't know if the death explode blast pillar or the floating revival platform triggers it. once it triggers, some things lose their textures.
- Check scaling on the hit effects (for example, A attacks, Forward A, strong A), some look bigger than they should be.
- The results screen confetti, shouldn't the spawner for the emitter be just above, out of frame with the camera so we don't "see" them spawning?
- (found by instrumentation, not play) After the first KO the stage permanently drops its 24 pinned textures and runs slower for the rest of the match. MEASURED - fix proven on the natural path; tick/VBlank A/B still owed before any speed claim.


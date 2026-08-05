**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

- I see the floating platform, but its colors look off (too dark?), like it isn't rendering correctly.
  OWNER-QUEUED: A5I3 dedicated texture restores all 16 source alpha levels; hard edge gone. See c74-a5i3 capture.
  
- I see the death explode blast pillar thing, but the colors look off (too dark?) and it doesn't seem to play at the players death location off screen, check x,y,z coords. Also if a player hits the side boundaries, it should play horizontally im pretty sure.
  CLOSED (2026-08-04): owner accepts current pillar; N64's whole-screen white KO flash is a deliberate omission, never re-add.

- **FIXED** (2026-08-04) Shield bubble is correct asset, looks off, (too dark?)
  This and the next three rows were one event: a texture the jammed cache refused (reason 0x400)
  draws untextured in flat material colour. Static slots now read their key from ROM, 48->69 entries at -64 bytes bss.
  
- **FIXED** (2026-08-04) Impact wave, not showing the green impact effect looks gray/black instead.

- **FIXED** (2026-08-04) Fox reflector is green for some reason, should be blue

- **FIXED** (2026-08-04) I see missing textures/texture corruption. But ONLY AFTER dying. I don't know if the death explode blast pillar or the floating revival platform triggers it. once it triggers, some things lose their textures.

- Check scaling on the hit effects (for example, A attacks, Forward A, strong A), some look bigger than they should be.
  CLOSED (2026-08-04): owner accepts current scaling.

- The results screen confetti, shouldn't the spawner for the emitter be just above, out of frame with the camera so we don't "see" them spawning?
  CLOSED (2026-08-04): owner accepts current spawner placement.

- **FIXED** (2026-08-04) (found by instrumentation, not play) After the first KO the stage permanently drops its 24 pinned textures and runs slower for the rest of the match. 
    Same jam as the rows above; paired A/B gave WORK P50 -11,776 and 20->17 over-gate frames of 128.


---
status: resolved
discovered: 2026-04-28
resolved: 2026-04-28
component: rendering / it
---

# NBumper renders with wrong colors

## Symptom

The bumper item (kind 16, `nITKindNBumper`) — the reflector circle that bounces
fighters away on contact — renders with incorrect colors:

- At rest: appears solid blue.
- On collision with a fighter: flashes pink.

Original game renders the bumper as a yellow/red disc with the smash logo at the
center. The disc texture has transparent pixels around the logo.

## Resolution

When the bumper lands and attaches to the ground, `itNBumperAttachedInitVars`
manually swaps its display list and MObjSub to the landed/wait material:

```c
gcRemoveMObjAll(dobj);
gcAddMObjForDObj(dobj, mobjsub);
```

Unlike the normal object setup paths, this direct replacement skipped
`portFixupMObjSub`, so the port consumed the landed bumper material in its raw
relocated layout. Calling `portFixupMObjSub(mobjsub)` before adding the MObj
fixes the material fields used by the landed state. The item already looked
correct while spawned and held because those states use the regular setup path.

User verification on 2026-04-28 confirmed the bumper is normal after landing and
after fighter collision.

## Reproduction

```
SSB64_FORCE_ITEM_KIND=16 ./ssb64
```

Forces every item spawn to be a bumper. One bumper spawns at a time.

## What is verified

1. **The bumper goes through the opaque display path.** A `port_log` placed in
   `itDisplayOPAProcDisplay` (`src/it/itdisplay.c`) fires for every bumper draw
   and reports `attr->is_display_xlu=0`, `attr->is_display_colanim=0`,
   `kind_dump=16`. Routing in `itManagerCreateItem` (`src/it/itmanager.c`) is:

   ```c
   if (attr->is_display_colanim)
       proc_display = (attr->is_display_xlu) ? itDisplayColAnimXLUProcDisplay : itDisplayColAnimOPAProcDisplay;
   else
       proc_display = (attr->is_display_xlu) ? itDisplayXLUProcDisplay : itDisplayOPAProcDisplay;
   ```

   So with both bits 0, the bumper is dispatched to `itDisplayOPAProcDisplay`.

2. **Forcing the bumper to the XLU path crashes the renderer.** Patching
   `itManagerCreateItem` to unconditionally route `nITKindNBumper` to
   `itDisplayXLUProcDisplay` produced an immediate SIGSEGV in
   `gcDrawDObjTreeDLLinks` during scene draw, with a single bumper spawned:

   ```
   SSB64: !!!! CRASH SIGSEGV fault_addr=0x18857c98
   pc=0x...gcDrawDObjTreeDLLinks + 772
   lr=0x...gcDrawDObjTreeDLLinks + 88   (recursion site)
   ... gcCaptureCameraGObj → gmCameraDefaultProcDisplay → gcDrawAll
   ```

   The XLU-force change was reverted; the file is back to its original routing.

3. **An ImportTexture-side diagnostic gated on a "drawing bumper" flag does
   not fire.** A flag set/cleared around the bumper proc in `itdisplay.c` was
   intended to gate logging inside `libultraship/src/fast/interpreter.cpp`, but
   the flag is always 0 by the time the queued DL actually runs. This is
   consistent with Fast3D's known async DL queue: C-side state set during the
   game-tick draw pass cannot be read from the renderer pass that consumes the
   queued DL.

## What is NOT verified

- Whether `attr->is_display_xlu` should be 0 or 1 for NBumper on N64.
- Whether the wrong color is caused by the OPA render mode itself, by a missing
  CC/blender setup in the bumper's DL, by texture-format misdecode, or by
  palette/CI-table corruption.
- Why forcing XLU crashes — could be a downstream issue in the XLU proc
  specific to bumper's DObj tree, or unrelated bumper-DL corruption that only
  surfaces under XLU's matrix/segment handling.

## Diagnostics cleanup

The temporary `gSsb64DrawingBumper` flag, `NBumper` display logs, and
`BUMPER TEX` import log were removed after verification.

## Next investigation steps (suggested, not done)

- Capture a GBI trace of one bumper draw (see `docs/debug_gbi_trace.md`) and
  diff against m64p plugin output for the same scene/item.
- Inspect the bumper's DL in ROM: what render mode / CC does it set up?
- Determine whether the texture is rendered via the bumper's own DL or via an
  MObj/material override — the OPA-vs-XLU distinction in `itmanager.c` only
  controls which proc walks the DObj tree, not the render-mode opcodes inside
  the DL itself.

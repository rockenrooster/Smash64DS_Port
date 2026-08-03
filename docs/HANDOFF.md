# Handoff

Updated: 2026-08-03 late. The owner rewrote `BUGS.md`: every prior fix was
rejected and four rows were added. **Boundary is GREEN** on
`smash64ds-battle-playable-hwtri.nds` (zero GDB timeouts, zero exceptions),
27.8 FPS in a live capture. Four rows are closed; four are blocked on ONE
measured bound; the rest are measured with no defect left in the port.

## THE BOUND THAT OWNS FOUR ROWS -- READ BEFORE TOUCHING THE ATLAS

The VFX atlas is 8,192 bytes and that caps every effect cell at 16x16 (8x8 for
long animations) against 32x32 sources. Four rows -- respawn pad, shield, hard
landing, and "ALL VFX low quality" -- are that one number.

**It is NOT capacity, and 2026-08-03 measured it rather than arguing it.** The
static battle corpus was repacked to DS paletted (22 of its 24 textures are
sixteen-colour CI4 sources that were stored at two bytes a texel), which freed
**74,496 bytes** of the 262,144 -- losslessly, same pixels, proven by the payload
SHA. The atlas was then grown to 128x256 / 32,768 with cells at the source's 32.
Result:

```text
27.8 FPS -> 8.6 FPS, logic 55.6 Hz -> 17.2 Hz, flat UNTEXTURED WHITE on the
platform edge and lower-left -- the stage texture resolve failing and dropping
every frame onto the generic renderer.
artifacts/visibility/2026-08-03_atlas32k-candidate.png  (broken)
artifacts/visibility/2026-08-03_atlas8k-reverted.png    (27.8 FPS, clean)
```

With 110,336 bytes free where 60,416 used to be. So the bound is a **contiguous
run inside libnds's per-bank texture splitting**, and more free space does not
buy one. The route to source-resolution cells is several bank-sized allocations,
not a bigger one. Do not spend another cycle on a larger single block.

The repack itself is CORRECT offline and is left in the generator, switched off
at `repack_paletted`: with it on, the runtime M4 residency prepare fails and the
renderer silently falls back to ordinary texture resolution -- the stage still
looks right and still runs at 27.8 FPS, which is exactly why it needs the
verifier's counter and not an eye. Re-enable it together with a fix for the
prepare, and grade it on the M4 residency assertion in
`verify-battle-mariofox-gcrunall-loop-harness.ps1`, never on the picture.

## What changed this cycle

- **The v16 rail is gone.** `ndsRendererSubmitParticleQuad` converted world to
  v16 at a fixed x16, so anything past +/-2047.9 saturated: quads straddling it
  collapsed along that axis ("VFX get x flattened around stage edges") and the
  Star KO sparkle, which spawns at the receding fighter's z = -14,999, stopped
  dead at -2,047 and hung near the camera. The factor is now chosen per BATCH
  and escalates only when a quad needs it, so ordinary frames keep full x16
  precision. `NDS_R2_PARTICLE_V16_HEADROOM` is deleted -- a fixed coarser factor
  charged every hit spark for the one effect a match that leaves the stage.
  Verify: `gNdsParticleWorldClampCount` must be 0; `gNdsParticleScaleShiftMax`
  says how much range a Star KO frame had to buy.
- **The freeze cannot recur as a freeze.** `syTaskmanCheckBufferLengths` ends
  both overflow branches in `while (TRUE);`. That is the mechanism behind every
  "the match froze" filed here. Under `SSB64_TARGET_NDS` it now records and
  returns (sixth hunk of `scripts/decomp-patches/battleship/src_sys_taskman.patch`,
  regenerated and verified to reproduce the tree byte-for-byte).
  `gNdsTaskmanDLOverflowCount` / `gNdsTaskmanGraphicsOverflowCount` must be 0;
  non-zero is still a real accounting defect, now diagnosable instead of dead.
- **The VFX atlas is A3I5, not A5I3.** 32 palette entries instead of 8, at the
  same 8,192 bytes and the same cell geometry -- alpha drops 32 levels to 8,
  which no effect in the sheet was using. Fox's reflector carries COLOUR, two
  flat blues, and a single shared 8-entry palette could only encode white plus
  coverage. The checkers derive palette size from the generator report now
  instead of pinning `+ 16`.
- **Confetti is back at the source's own numbers.** Three cycles raised pool
  (112 -> 384), rate (0.07 -> 1.26) and size (20 -> 32) against a fixed pool and
  the owner rejected every one. The overrides are 0 and the clamp-upward-only
  contract makes them inert, so `lbparticle` runs the source's values.

## Still open, and what each needs

- **Hard-hit VFX too big.** Last cycle's 2.2 clamp is in `ndsTask39HitSparkSpawn`,
  which is UNREACHABLE: `battleship_efmanager.c` provides a strong
  `efManagerDamageNormalLightMakeEffect` that overrides the weak shim that was
  its only external caller. The source ramp (`efmanager.c:2175`) is identical to
  the port's, so the size has to be measured on the particle quad path.
- **Crowd cues cut off.** The source cuts them ON PURPOSE: `ftPublicPlayCommon`
  (`ftpublic.c:132`) stops the previous common cue, and `ftPublicDecideCall`
  (`:165`) calls `ftPublicCommonStop()` when a character call starts at
  knockback >= 130. Check the port plays the replacing call.
- **Confetti.** Owner chose "find the structural difference". The tuning is
  reverted; the untouched lead is the Results camera framing, which no cycle has
  checked -- source-exact density can still read sparse through a wider frustum.

## What to look at, in BUGS.md order

Test `smash64ds-battle-playable-hwtri.nds` -- the only published P1 ROM and the
configuration every row was filed against. `smash64ds.nds` is P2 work; do not
build it for this queue (owner, BUG_FIXING_PROCESS.md).

- Star KO twinkle now follows the fighter out to the blast zone instead of
  hanging near the camera, and effects no longer squash at the stage edges.
- Fox's down B can carry its two source blues now the palette holds 32 entries.
- Results confetti is back to the source's own density and size.
- Shield, respawn pad and hard landing are unchanged -- they are the atlas bound
  above, not a routing bug, and nothing this cycle could move them.

## Standing hazards this cycle re-proved

- **A saturated counter is a floor, not a measurement.** The particle pools were
  graded twice from matches that never ran a KO burst, then trimmed to fit a
  reading that had hit its cap; two of six KO bursts drew nothing. Now proven
  below cap — if either ever pins at 24 again the demand is unmeasured.
- **Check that the all-clear counter covers the failure.**
  `gNdsParticleRejectCount` read 0 through both saturations — struct rejects only.
- **A symbol's guard must be the guard of the thing it belongs to**, not of
  whatever it was typed next to. `ndsRendererSetParticleCamera` lived inside
  `#if NDS_RENDERER_HW_TRIANGLES` with an unguarded caller, so the one ROM that
  ships was the only one that failed to link; the rebirth display proc repeated
  the shape the same day inside `#if NDS_TASK39_FX_SHIELD`.
- **Two equal counters are only saturation when the second one is the bound.**
  `probe-results-confetti.ps1` printed `gens_used=24 gens_max=24`, which reads
  exactly like the pool saturation above; `gens_max` is
  `gNdsParticleGeneratorsMax`, a HIGH-WATER MARK, and the Results cap is 48 from
  the override at `battleship_mnvsresults.c:236`. A pool bump was made and
  reverted on that misread — cost one build. The field is now printed as
  `gens_highwater`. `structs_used=384` against that scene's 384 **is** real
  saturation, and it is the one that matters: the confetti fan divides a fixed
  pool six ways.
- **Coverage is count x AREA, and three raises only ever bought count.** The
  confetti row went 112 -> 192 -> 384 pieces, each costing a VBlank of Results
  interval, while piece size sat at the source's 20.0 — even though the owner's
  own first wording was "pieces do not look like they are large enough". 32.0 is
  2.56x the area at the same 384 pieces and `census-results-frame-cost` reads
  3.95 VBlanks/present, unchanged. When a raise keeps hitting a resource bound,
  check whether the other factor in the product was ever moved.
- **Read the asset before assuming the atlas can hold it.** The Fox reflector
  row looked like the shield and rebirth rows and is not: those two carry SHAPE,
  which A5I3's one shared 8-entry palette can encode as white plus coverage.
  The reflector carries COLOUR — two flat tones, no shape — so the same
  treatment maps its 81% body to alpha 0 and deletes it. A twenty-line offline
  probe over `relocData/346.vpk0.bin` settled that without spending a ROM.

## Restart surface

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

`docs/P1_EXECUTION_BOARD.md` is the only dynamic queue. `docs/BUGS.md` carries
the owner's verdicts and one stage line per row; the owner edits it directly
during a session, so preserve their wording and ordering verbatim.

Uncommitted in the tree: the owner's own `AGENTS.md` edit and their in-progress
`.agents/skills` / `.claude/skills` rename to the `nds-*` set. Leave both alone.

Useful captures:

```powershell
# Five minutes ends MID-MATCH, which is why this one completed. A seven-minute
# run ends into the static post-match screen and trips the detector honestly.
# -PollSeconds also HALVES the threshold: it trips at IdenticalFramesToTrip x
# PollSeconds, so 8 x 5 = 40s lands on the ~30s NitroFS scene-load dead air.
.\scripts\soak-freeze-watch.ps1 -Build build-r2-bothcpu -MinutesToRun 5 -IdenticalFramesToTrip 16
.\scripts\probe-ko-vfx.ps1
.\scripts\capture-sudden-death-entry.ps1 -CaptureAnnounce 20   # TIME UP
```

A clean checkout must build through `build.ps1`, not bare `make`: four of six
generated `.inc` files are gitignored. `-j`/`MAKEFLAGS` rules are in `AGENTS.md`
`## Builds`. Preserve canonical mode 163, renderer mode 9, mip 0, static
textures, source countdown, Dream Land water at frame 0, Task 16 `1/1/1`. Do not
edit `decomp/`.

If the owner rejects a visual row: shield/halo cell resolution is capped by the
8,192-byte sheet bound, and the way to buy more is to name non-live common
textures to drop, **not** to grow the sheet — every larger allocation has been
measured to break stage texture resolves. The halo's source spin (node[2], rotY
0 -> 2*pi over 30 frames) is deliberately not reproduced; a camera-facing quad
has no meaningful rotY, so the honest route would be a second quad with a
rotating UV.

Run `New-Smash64DSSnapshot.ps1` last, and nothing after it.

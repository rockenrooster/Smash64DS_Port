# Handoff

Updated: 2026-08-03 evening. **Boundary is GREEN** on
`smash64ds-battle-playable-hwtri.nds` and a live capture reads **29.9 FPS /
59.8 Hz logic**, up from 27.8. The owner re-opened five rows the same afternoon
with sharper wording -- read `docs/BUGS.md` first, it is their board.

## What the atlas bound actually was, because it was not contiguity

The 2026-08-03 morning conclusion ("the bound is a contiguous run inside
libnds's per-bank splitting") is **WITHDRAWN**. That run had two changes in it
and the other one explains the picture on its own.

`ndsRendererHardwarePrepareBattleStaticTextures` asserted
`gNdsRendererBattleStaticTextureBankMask == 3` -- that the static corpus
STRADDLES texture banks A and B. That is a restatement of one corpus size, not
a property of a correct corpus. Repacking the corpus to DS paletted made it
small enough to fit in bank A alone, the mask read 1, residency failed closed
at zero keys, and the renderer fell back to ordinary texture resolution. A
stage whose static textures never became resident renders untextured -- the
exact symptom that got charged to a 32,768-byte particle sheet running beside
it.

The mask is derived from preparedBytes now, and so are the span end and the
residency byte count in the verifier, which restated the same size at **six**
sites. If a generated size ever appears as a literal in a gate again, that is
the defect, not the size.

## Where the atlas landed

Four sheets of 8,192 bytes -- the allocation size that has never been refused --
instead of one of 32,768. Same texels, bound per cell from the frame table; a
sheet change costs a rebind plus a new primitive group, which is what an
alpha-bucket change already cost. `NDS_PARTICLE_QUAD_ATLAS_SHEETS` is the free
variable; **the sheet SIZE is the invariant**. Growing coverage means more
sheets, never a bigger one.

Every admitted cell is at SOURCE resolution now (cell cap 64, no reduction):
shield 16x32 where it was 8x16, respawn halo 32x16 where it was 16x8, dust at
its full 64x64. The static repack that paid for it is lossless and freed 74,496
bytes; it also gained the oracle it never had, because nothing compared the
repacked bytes -- `output_sha256` and the slow oracle both compare the canonical
image upstream of the repack.

## Freezes are structurally closed now, and yesterday's fix was one of thirty-five

`rg 'while \(TRUE\);'` over the compiled decomp returns **35** sites. The
2026-08-03 fix converted **2**, both in `syTaskmanCheckBufferLengths`, and the
note it left read as though taskman owned the freeze. The owner filed "Shield
freeze bug happened again" that afternoon against a build containing it.

objman.c held nineteen more, and they are the ones a shield hits -- a shield
effect allocates a GObj, a DObj and an MObj on the frame it spawns, and every
exhausted pool ended in a spin. They record and fail the allocation now;
`gNdsObjmanPanicCount` must read 0 and `gNdsObjmanPanicMask` names the site in
source order. Left spinning on purpose: main.c's idle thread (a spin IS the
behaviour) and scheduler.c's PAL branch (unreachable).

## The confetti structural difference, found and not yet fixed

`mnvsresults.c:3208` makes **two** emitters at different depths on different
generator links: `(0,1000,-1000)` with `is_genlink_mask` FALSE, which
`efmanager.c:6206` turns into `bankID | LBPARTICLE_MASK_GENLINK(3)`, and
`(0,1000,-400)` with TRUE, which is `bankID` alone. `MASK_GENLINK(3)` is 32, so
they land in alloc slots 0 and 4 -- one behind the fighters, one in front. The
owner sees only the far one, which is the whole of "falls behind the fighter
instead of infront".

The port's per-link gate is **source-exact** -- `lbparticle.c:1500` uses the
same `gobj->camera_mask & (1 << j)` -- so the draw loop is not the defect, and a
GDB run confirms both calls execute (breakpoints hit at `mnvsresults.c:3216` and
`:3217`). What is unproven is whether the second emitter ALLOCATES
(`efManagerConfettiMakeEffect` returns NULL on a short pool) and whether the
Results GObj's `camera_mask` carries both slots. Blocked on the probe symbol
below. Both emitters sit at x=0, so "not centered on the camera view" is the
Results camera, not the emitter.

## THE FOUR ASSET ROWS ARE A LINK COVERAGE GAP -- NOT THE ATLAS, NOT THE CAMERA

Respawn platform, shield, Fox down B and hard-landing wave are source `EFDesc`
MODELS, not sprites, and every EFDesc names the display link it draws on:

| EFDesc | link | draws? |
|---|---|---|
| `dEFManagerShieldEffectDesc` (efmanager.c:460) | 15 | no |
| `dEFManagerFoxReflectorEffectDesc` (:420) | 15 | no |
| `dEFManagerRebirthHaloEffectDesc` (:1648) | 10 | no |
| `dEFManagerImpactWaveEffectDesc` (:201) | 10 | no |
| `dEFManagerDeadExplodeEffectDesc` (:850) — KO burst | **18** | **yes** |

`ndsStageGCDrawAllLoopIsEffectDisplay` required link 18, and the procedural
stand-ins are hard-wired onto 18 by `ndsEFManagerMakeVisualEffect`. The KO burst
is the control the repo was already carrying: same source-model route, works
unconditionally, differs only in that field. That is what "the battle hardware
path does not consume source effect DL links" meant — written twice, never
located, and looked for in the atlas, then the camera passes, then the tree walk.
The camera captures 10 (pass 3) and 15 (pass 4); it was never the gap.

A second gate sat behind it: `ndsEFManagerIsVisualEffectGObj` (efmanager.c:969)
returns TRUE only when `dobj->dl` matches a procedural TEMPLATE pointer, which a
ROM-asset list can never do. Both are open now, flag-gated, counted by
`gNdsEffectRendererSourceModelAdmitCount`.

A **fourth** gate sat behind those: the source models arrive as `DLHEAD0` and
nothing else (observed-kind mask `0x8`), while the submit accepted `DOBJ_TREE`
alone. `ndsRendererAdapterSubmitStageDObjNode` already handles DLHEAD0 in the
same switch arm, so only the guard changed. Beware: these kinds are FOURCCs
(`DOBJ_TREE` = `0x44545245`), so a `1u << kind` mask is undefined and reads 0 —
which would have been reported as "nothing arrives".

**Where it stands** (flag on, `probe-shield-vfx.ps1 -DrawCounter
gNdsEffectRendererSourceModelAdmitCount`): `admit=12 capture=6 dobjdraw=6
reject=6 tris=0 nodes=6 rejkinds=0x0`, `kindmask=0` throughout. Zero to twelve
admits, past the guard, **and the walk now runs**. They still do not appear, and
there are now two separate questions:

1. `tris=0`. **Two hypotheses tested, both dead — do not retry them.** "A DObj
   flag makes it undrawable" (DLHEAD0 uses the strict `flags == DOBJ_FLAG_NONE`
   rule): refuted, `dobjflags=0x0`. "Geometry is in `dl_link[]`, which only the
   `*_DLLINKS` kinds read": refuted, `fields=0x7` — `dl`, `dl_link` and `dv` are
   all non-null, and routing DLHEAD0→TREE_DLLINKS left `tris=0`; that routing
   was reverted. **Where it stops**: `ndsRendererAdapterSubmitStageDL` emits
   nothing from the list, with `texready=0` *and* `texreject=0` so it never
   reaches texture handling. Instrument there — everything upstream is proven.
2. `nodes=6` over 6 frames is **one node per frame**, but
   `llFTManagerCommonShieldDObjDesc` is a **three-node** desc. So the DObj has
   no child and no sibling chain — the tree was never built. That is a different
   defect from the tree not being walked, and a correct walk is what exposed it.

Do not spend a fourth cycle on cell sizes here.

The KO blast pillar is NOT in this family: `efManagerSparkleWhiteDeadMakeEffect`
(:3725) runs particle script 0x5C, whose texture 24 is admitted at source 32x32.
That one is genuinely a particle effect and the sheet is not its blocker.

**The pacing harness cannot judge these rows.** `sample-tick-hud-buckets.ps1`
reads `gNdsVisualEffectCreateCount=0` / `gNdsVisualEffectKindMask=0` over its
window — zero effects of any kind. Use `probe-shield-vfx.ps1`, which drives both
CPUs; it takes `-DrawCounter` now because it armed on the stand-in's counter and
so could not see the very configuration it exists to judge (that is the real
cause of the "380-second timeout" once misread as a performance result).

## A counter nothing reads is a counter the linker deletes

`probe-results-confetti.ps1` fails with "No symbol gNdsConfettiFanCount in
current context" even though `battleship_efmanager.c:1472` defines it. The build
uses `-fdata-sections` with `--gc-sections`, and a `volatile u32` that is only
ever incremented has no reader, so its section is collected. Counters survive
only because a marker block in `taskman_seam.c` names them. Add a new diagnostic
to a marker dump in the same change that adds the counter, or it measures
nothing.

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

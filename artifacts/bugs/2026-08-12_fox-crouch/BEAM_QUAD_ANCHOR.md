# Fox blaster quad — vertical anchor and scale vs source

Answered 2026-08-13 from source bytes and published globals. **No ROM was
built and no emulator was launched.** Root ROMs unchanged:
`smash64ds-battle-playable-hwtri.nds` sha256 `524448C9…` (12,225,536 B),
`smash64ds.nds` sha256 `54C07FAC…` (11,915,264 B).

Question (CONTRACT.md's own "Not done" list): does the beam quad hang from an
edge instead of straddling the beam's world Y, and do `scale_x = 6.333` /
`scale_y = 1.0` match the source particle's own size?

**Answer: no anchor error, no scale error. Both are source-exact.**

## 1. The source quad, byte for byte

`dFoxSpecial1_Blaster_WeaponAttributes.data` points at
`dFoxSpecial4_ReflectorDL_DisplayList`, relocData file **316**
(`decomp/.../src/relocData/210_FoxSpecial1.c:19-22`), whose payload is four
`Vtx` at 0x0 and a 9-command DL at 0x40 (`316_FoxSpecial4.c:11-22`).

Read from the shipped asset
`decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/FoxSpecial4`
(224 B; 0x40 o2r header + 0x10 reloc header, payload at file offset 0x50):

| file off | bytes | `ob` (s16 BE) | `cn` |
|---|---|---|---|
| 0x50 | `00 00 00 18 00 00` | **( 0, +24, 0)** | `DB 00 86 00` |
| 0x60 | `00 00 FF E6 00 00` | **( 0, −26, 0)** | `DB 00 86 00` |
| 0x70 | `FF E2 FF E6 00 00` | **(−30, −26, 0)** | `DB 00 86 00` |
| 0x80 | `FF E2 00 18 00 00` | **(−30, +24, 0)** | `DB 00 86 00` |

So the source quad **straddles local y = 0**, spanning −26…+24 — a 50-unit
band whose centre sits 1 unit below the projectile origin. It is *not* edge
anchored. In X it is anchored at the projectile (leading edge x = 0) and
trails 30 units behind. RGBA(219, 0, 134, 0) is the beam's own magenta.

## 2. The scales

- `scale.y` and `scale.z` are **never written** for this weapon.
  `wpFoxBlasterProcUpdate` (`wpfoxblaster.c:44-52`) touches only `scale.x`,
  and creation leaves `dGCScaleDefault = {1,1,1}` (`objman.c:107`) via
  `lbCommonInitDObj3Transforms` (`lbcommon.c:890`) from `wpmanager.c:268`.
  **`scale_y = 1.0` is exactly source.**
- `scale.x` starts at 1.0 and gains `WPBLASTER_ADD_SCALE_X = 16/3`
  (`wpvars.h:9`) per update, clamped at `WPBLASTER_CLAMP_SCALE_X = 160/3`
  (`wpvars.h:10`). First update ⇒ **6.33333** — the probe's `0x40caaaab`.
- Independent cross-check that the 30-unit local X extent is right:
  `30 × 16/3 = 160.0 = WPBLASTER_VEL_X` (`wpvars.h:8`). The quad lengthens
  by exactly one frame of travel per frame, so the tail stays pinned at the
  muzzle for the first ten frames (190 → 1600 world units). Any other local
  extent would visibly walk the tail off the gun.

## 3. The port reproduces it

`src/nds/nds_renderer.c:14806-14954`: `span_x = 30*sx`, `span_y0 = 24*sy`,
`span_y1 = 26*sy`; facing +1 gives `x0=tx, x1=tx−span_x, y0=ty+span_y0,
y1=ty−span_y1`, facing −1 the Rz(π) mirror. That is T·R·S applied to the four
source vertices, matching `nGCMatrixKindTraRotRpyRSca`
(`wpfoxblaster.c:20-22`); `rotate.z = atan2(vel.y, vel.x)` is 0 or π for the
admitted ±160 velocities. No half-height is baked into any offset.

The muzzle flash is centre-anchored too: `efManagerFoxBlasterGlowMakeEffect`
(`efmanager.c:5517-5530`) writes only `pc->pos`, and the source draw mirrors
the quad about the projected centre (`lbparticle.c:1769-1802`,
`xl = tx − (xh − tx)`), which is what the port's AOT half-extent table does
(`src/import/battleship_lbparticle.c:282-290`).

## 4. What the source geometry *does* say — the number the row turns on

Joint-17 world matrix at the fire tick, from the published globals
`gNdsFoxSpawnWorldMtx` == `gNdsFoxGunWorldProbeFloatMtx` (bit-identical;
globals, not the retracted stack read) in
`artifacts/verification/2026-08-12_fox-muzzle-alignment-c142.txt`:

```
row0 (local +X -> world)  0.960167766  -0.141958907  -0.222557232   |r0| 0.9958
row1 (local +Y -> world) -0.146019384  -0.985461652  -0.00206722319 |r1| 0.9962
row2 (local +Z -> world) -0.218805268   0.0346215703 -0.970864296
row3 (translation)      659.097107    231.915787    42.1739044
```

`row1` is essentially world −Y: the gun's local +Y is the grip direction and
the grip hangs down (local (−21, 54, 0) lands at world y **181.68**, below the
barrel). So the local offsets below are very nearly pure vertical sag.

| point | joint-17 local | world |
|---|---|---|
| shot + flash spawn (`ftfox.h:7`) | (60, 0, 0) | (716.707, **223.398**, 28.821) |
| gun bore centre | (60, −24, 0) | (720.212, **247.049**, 28.870) |
| muzzle face, local y min | (60, −42, 0) | (722.840, 264.788, 28.907) |
| muzzle face, local y max | (60, −6, 0) | (717.583, 229.311, 28.833) |

Gun vertices from `src/nds/nds_fox_gun.c:49-76` (source Vtx payload unchanged,
sha256-pinned by `scripts/fox_gun_bake.py`).

- **The beam and the flash spawn 23.651 world units below the bore centre.**
- Beam quad world y **197.398 … 247.398** (50.0 tall); muzzle face world y
  **229.311 … 264.788** (35.477 tall).
- Beam centre 222.398 is **24.651 below** the bore centre. Overlap is 18.087
  of the face's 35.477 (**51%**); **63.8% of the beam hangs entirely below the
  barrel.**
- The flash's peak half-extent is 165 (`battleship_lbparticle.c:288`), 3.4× the
  gun's 96-unit height, so the same 23.651 offset is ~14% of its radius. **The
  laser is the visible half of the owner's symptom; the flash is not.**

Every input above is source: the (60,0,0) spawn offset, the gun's own Vtx
payload, the quad's own Vtx payload, and the scale defaults. The one term with
no offline oracle is the joint-17 pose itself — and it multiplies **both** the
gun and the shot, so it cannot separate them. The sag is what BattleShip's own
data specifies, not a port defect.

## Verdict

`BLOCKED(decision: accept the source's 23.65-unit beam sag, or approve a
presentation-only delta)`. Nothing here is fixable at a port seam without
changing source-defined geometry, which `PROJECT_GOAL.md` reserves to the
owner. The two prepared alternatives, if the owner wants the change:

1. **Leave it.** Mechanically and visually what SSB64 draws.
2. **Draw-only offset of (0, −24, 0) in joint-17 local space** applied to the
   beam quad and the flash quad, leaving `wpFoxBlasterMakeWeapon`'s spawn,
   `attack_pos`, and collision untouched. Visual fidelity only; gameplay
   unchanged. Costs one `Vec3f` add per submit.

Option 2 must not be taken by an agent: it moves a source-exact telegraph.

## Not re-derived here

- Whether `pc->size` in EFCommon script 0x62 is a half-extent (assumed by
  `sNdsFoxBlasterGlowSize`); the source's own mirror construction is
  consistent with it, but the peak 165 was not checked against a capture.
- Fox's joint-17 SquatWait/SpecialN pose still has no offline oracle.

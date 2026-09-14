# P2-4 Hyrule / Inishie stage-hazard admission repair

Date: 2026-09-14

## Defect and repair

BattleShip `grCommonSetupInitAll` always creates the four common geometry layers,
clears yakumono state, calls `grMainSetupMakeGround`, restores yakumono state, and
then creates the common item/effect appear actors. Hyrule's tornado constructor and
Inishie's POW constructor each deliberately spin forever when their required map-
object count is zero or above ten.

The DS port had moved those two count checks outside the stage constructor and into
the shared `grCommonSetupInitAll` admission switch. A failed check therefore fell
through to `ndsGRCompatibilityNonPupupuSetup` and skipped the *entire* stage setup,
not just the malformed hazard. That made the defensive guard broader than the
source contract.

The checks now live at the stage-specific `grHyruleMakeGround` and
`grInishieMakeGround` seams. Valid staged data enters the imported source body
unchanged. Invalid Hyrule tornado data omits only the tornado GObj. Invalid Inishie
POW data still initializes the map headers, both scales and both Piranha Plants and
omits only the POW subsystem. The shared stage admission path always runs the common
setup for a valid loaded stage.

## Static/source check

`python scripts/stages/check_collision_parity.py --check` passes all nine VS stages.
The staged Mushroom Kingdom map has 36 map objects including five
`nMPMapObjKindPowerBlock` positions; the runtime Hyrule route sees four Twister
positions. No source or staged collision data was changed.

## Natural shipping-shell proof

Candidate shell ROM:

- `builds/build-p2-shell/smash64ds-p2-shell-hwtri.nds`
- SHA-256 `A22D169DAF1A07405A28287399D545A6FF82C19EE79BC28AC80CAF160FAAF6A2`
- ELF SHA-256 `EA030060B51A8EC95D072792E212144187A4F601E1F3DF47AFF7C7BFE6880CF3`
- base commit at build start: `e79b8aabd44`; unrelated owner dirty work was
  preserved in the integrated candidate.

Both probes used the existing ordinary Title -> Mode Select -> VS -> CSS -> SSS ->
VSBattle walk and selected the requested stage through the SSS input route.

### Mushroom Kingdom / Inishie

`scripts/diagnostics/probe-native-render-scene.ps1`, stage kind 8, 32 presents:

- scene/gkind `22/8`; natural SSS commit reached gkind 8.
- `DIAG_STAGE_HAZARD_GUARD=0,0,5,0`: five POW positions, zero refusals.
- `DIAG_PAKKUN=...draw=64...submitFail=0`: the Piranha native actor path is
  positively engaged after common setup.
- `DIAG_STAGE_OWNER=0,0,27`, native failure tuple all zero, and stage shortfall
  tuple all zero.
- capture: `artifacts/visibility/2026-09-14_inishie-ground-guard.png`.

### Hyrule Castle

Same probe, stage kind 4, 32 presents:

- scene/gkind `22/4`; natural SSS commit reached gkind 4.
- `DIAG_STAGE_HAZARD_GUARD=4,0,0,0`: four tornado positions, zero refusals.
- `DIAG_STAGE_OWNER=0,0,18`, native failure tuple all zero, and stage shortfall
  tuple all zero.
- capture: `artifacts/visibility/2026-09-14_hyrule-ground-guard.png`.

These captures prove the repaired natural stage entry and native output. They do
not close the remaining P2-4 visual-acceptance work on other stage presentation
dimensions.

## Integrated Boundary

Full log (local build artifact):
`builds/verify-p2-stage-hazard-guard-boundary-2026-09-14.log`.

The one widest relevant verifier was run once after the focused proofs:

- `p2_shell_loop`: PASS, one lap / 10 scene entries, deterministic menu
  high-waters flat, free floor 114,628 B, zero faults.
- `p2_battle_realtime`: PASS, 212-frame Pupupu realtime pacing smoke.
- `p2_fourcpu_stress`: PASS correctness, cadence, native-owner and memory gates;
  general-heap low-water 118,752 B; libc runtime high-water 32,896 B and top-chunk
  minimum 9,160 B under the 40,960 B reserve; weapon pool 10 entries / high-water
  2 / refusals 0; `gNdsITCommonDataRejectCount=0`; renderer native failures 0.
- Final verdict: `Boundary verification profile passed.`

Boundary ROM identities for this integrated run:

- shell loop: `410D3FC6C55161847F25213FF3AB157A4D92AF014A0B02ED366CD5C76EFA3D4D`
- realtime shell: `A22D169DAF1A07405A28287399D545A6FF82C19EE79BC28AC80CAF160FAAF6A2`
- four-CPU stress: `82F3F7366C779C6E12AEA8A06B5E3B490F2C824447E68D0054C74B90E979EF18`

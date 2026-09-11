# P2-3f47 Kirby hidden-part root-program recovery — 2026-09-11

## Scope

This closes only the previously first wide native-fighter blocker:

- fighter: Kirby
- model asset: `328`
- status: `0x116`
- Low-detail live root: `0x4728`
- failure: `REJECTED_PROGRAM`

It does **not** close all Kirby/copy-power coverage or all P2-3f47 acceptance.

## Before

The focused natural four-CPU probe rejected Kirby at presented frame **327**:

`KIRBYREJECT=327,status:0x116,...,head:1,program:0,decline:4,selected:9,tried:1`

The live source-order vector was:

`0x4728, 0x4860, 0x29a0, 0x2a08, 0x2a90, 0x2af8, 0x3858, 0x2b80, 0x2c28`.

Baseline witness: `builds/kirby-first-reject-console.txt`, SHA-256
`5991894BCC5F033698F0F19C576025EA15FCE97F5743CF559456CF9E1FEA3407`.

BattleShip's hidden-part construction makes this a complete 9-root source
program, not a seven-root replacement: SpecialN enables hidden joint 7 and
joint 19, while joint 7's body consumes the preceding head's vertex cache.

## Implementation/proof model

The generated Kirby alternate program now preserves the exact live source root
order and the position-faithful per-head body bake.  Runtime matrix metadata is
selected by the active Kirby root program instead of applying the canonical
seven-root metadata to a 9/10/1-root program.

The shipped-table geometry checker was also corrected to test what production
actually executes.  RAW runs use the current root matrix; CROSS runs consume the
physical GX slot encoded in each packed corner.  Reused resident rows therefore
do not need their historical `dense.matrix_binding` byte renumbered to the new
draw-order ordinal.  The shipped packed slot stream is compared root/run/corner
against the faithful source-order bake instead.

Host/source checks on the frozen candidate:

- `python scripts/fighters/test_kirby_trio_body.py` — **22 tests OK**, one
  superseded legacy resolver test intentionally skipped.
- `scripts/fighters/check_native_owner_geometry_closure.py` —
  `NATIVE_OWNER_GEOMETRY_CLOSURE_OK` for every owner/detail.
- `python scripts/fighters/generate_nds_native_owners.py --check` — current.
- `python scripts/fighters/generate_nds_native_owner_images.py --check` —
  generated files current.

## Frozen ROM

Exact four-CPU target:

`builds/build-p2-battle-core/smash64ds-p2-fourcpu-tickhud-hwtri.nds`

- size: **30,046,208 bytes**
- SHA-256: `661A2EC80D46B71F6598C5670E7B86FBC13D0530729CF7F8B10088D9327FCD91`
- ELF SHA-256: `6497699FEC121F80C72EEB02625F48FC48FA41731D5F53534CC794EAC7A8199D`

## Focused natural-path discriminator

Command:

`scripts/probe-p2-fourcpu-sparse.ps1 -NoBuild -Build build-p2-battle-core -FirstKirbyReject -Frame 512`

Result:

- `KIRBYREJECT_NONE_THROUGH=512`
- slot 3 / Kirby reject count stayed **0** through frame 512.
- This passes beyond the old natural failure at frame 327.

Artifact: `artifacts/verification/p2-3f47-kirby-root-program-512.txt`, SHA-256
`C9FEC52DA815E70312B1E8B8E65D940E4601F2D9CAA262AE2E286EA6207CA1CD`.

## Widest relevant verifier

`scripts/verify-p2-four-fighter-stress.ps1 -NoBuild -Build build-p2-battle-core`
completed the same Donkey/Samus/Link/Kirby natural window through frame **1973**,
clock **60 -> 1** (1,972 presented frames / 3,944 logic ticks).

The former sticky fighter failure `328:0x4728`, status `0x116`, is no longer the
first native-only failure.  The new first failure is independent item output:

- domain: **2**
- VSBattle scene: **22**
- Dream Land status/gkind: **6**
- identity: `0x03F50056` -> asset **86**
- root: **`0x17D8`**
- reason: **1 / `NO_PROGRAM`**

BattleShip source maps this to `86_ITCommonObject.c`:
`dITCommonObject_Gfx_0x17D8`, the **Sword joint 1 display list**.  This is an
existing P2-5 common-item/native-output ownership issue, not a Kirby root-program
failure.

Same-run resource invariants remained green:

- general heap free-min: **94,076 B**
- required safety floor: **25,600 B**
- margin: **68,476 B**
- graphics heap: 232 / 1,536 B, 0 overflow/no-room
- compact battle cores: 4/4, 92,200 B, 18 extern patches, 12 extern loads,
  zero failures
- compact ShieldPose: 4/4, 11,799 B, 36 native fixups, zero failures
- particle/effect/object allocator failure counters: zero

DamageSlash also remained lifetime-green: root mask 3, 136 draws, 120
triangle-bearing draws, 16 source-alpha-zero frames, 89 in-place texture
updates, and zero material/snapshot/texture/submit failures.

Verifier artifact hashes:

- tick HUD JSON: `E43D7219198CCA8F029B0308768D31972E47609D7A867FC10A95A8B8CFC853FC`
- memory JSON: `963E9F59E2E8FEE5F48705AD01BF65B20DA1F65D4439BA63DEEFD1A48B7CF9C9`
- coverage JSON: `9E7FEF55D7BE62E960A52C497F2684C9EEC20C801CB7EEFC821EA546352A6BC6`

## Remaining Kirby scope

Do not mark all Kirby/copy-power coverage closed.  A later natural witness at
frame **1324**, status `0x122`, head modelpart 10, shows the mixed-file Link-copy
program still rejecting.  The current generator contains unfinished `CopyLink`
program work, but runtime program-4 admission/ownership is not yet complete.

Witness: `builds/kirby-copylink-first-reject-detail.txt`, SHA-256
`4AA0C49F9781D63D4CC2739AE50BAEA98873D5A26184F9905147AB1B6C60477D`.

Therefore the precise verdict is:

- **CLOSED:** P2-3f47 SpecialN/hidden-part first blocker `328:0x4728`, status
  `0x116`.
- **OPEN:** later Kirby CopyLink mixed-file state and broader P2-3f47 acceptance.
- **FIRST WIDE BLOCKER NOW:** P2-5 ITCommonObject Sword asset `86`, root
  `0x17D8`, native `NO_PROGRAM`.
- P2-2p8 CPU optimization remains owner-deferred; timing from this run is not
  promoted as performance acceptance.

## Periodic normal build

`make TARGET=smash64ds BUILD=build` completed after the verification cycle and
republished `smash64ds.nds`:

- size: **53,107,712 bytes**
- SHA-256: `7ABC96BE6C18A3FA9C1421303B28A2C35FFFE016D3BCB6B23EF5243E00AEFF3D`

This is build-health evidence only.  It is not promoted as the accepted P2 ROM
while the native-only Sword `86:0x17D8` failure remains open.

# P2-3 Donkey low-detail model-part binding repair — 2026-09-11

## Outcome

The native Donkey owner now accepts the source low-detail model-part program
used by natural Up Smash (`AttackHi4`). The failure was not missing geometry,
asset residency, or a missing alternate owner program: one already-generated
Donkey model-part variant was attached to the wrong logical native binding.

The full native-only gate is still open. On the repaired candidate the first
wide native failure advances to SamusModel asset `320`, status `0xA6` (`Catch`),
root `0x57D8`, reason `REJECTED_PROGRAM`.

## Source contract and failure

BattleShip source authority:

- `decomp/BattleShip-main/decomp/src/relocData/212_DonkeyMainMotion.c`
- `decomp/BattleShip-main/decomp/src/relocData/213_DonkeyMain.c`
- `decomp/BattleShip-main/decomp/src/relocData/317_DonkeyModel.c`

`dDonkeyMainMotion_USmash` naturally executes these model-part changes before
the active hit:

- joint 16 -> modelpart 1
- joint 10 -> modelpart 1
- joint 12 -> modelpart 2

The low-detail native owner had already baked all three source replacements.
The first two resolved at their correct native bindings; the third did not:

- joint 10: `0x6188 -> 0x81D8`, native binding 4
- joint 12: `0x61F8 -> 0xAD08`, native binding 5
- joint 16: `0x6748 -> 0x7C88`, live native binding **8**

The generated metadata incorrectly recorded the last row as binding **7**.
High detail already recorded the same joint-16 model-part replacement at binding
8, which independently falsified the low-only binding-7 interpretation.

Focused pre-fix natural evidence:

`artifacts/verification/p2-3-donkey-attackhi4-reject.txt`

At presented frame 416:

```text
DONKEYREJECT=416,status:0xcf,battle_slot:0,program:0,decline:4,owner:3,selected:16,tried:1
DONKEYVALIDATE=4,3,1,8,31880,26440,16,4,4,33240
DONKEYROOT=8,asset:317,0x7c88,materials:1
DONKEYROOTJOINT=8,16
DONKEYPART=6,1
DONKEYPART=8,2
DONKEYPART=12,1
```

Validator code 4 is the exact root-offset mismatch. At root index/binding 8 it
observed `31880 == 0x7C88` and expected canonical `26440 == 0x6748`.

## Fix

`scripts/fighters/generate_nds_native_owners.py` changes exactly the source
metadata row for Donkey low detail:

```text
(7, 0x7C88) -> (8, 0x7C88)
```

This follows the existing model-part contract: `ftParamSetModelPartID` replaces
an already-selected JointTree root, so a variant must execute at the **same
logical matrix binding** as that live DObj. No status exception, generic
fallback, or new renderer path was added.

The shared generated owner output was refreshed by the normal build. The source
generator was already concurrently dirty for unrelated Kirby work, so the
Donkey correction was also validated from an immutable `HEAD` generator copy:
regenerating `HEAD` with only this one row changed produces the same Donkey
variant root with binding 8 instead of 7.

## Static/source checks

After the integrated build:

- `python scripts/fighters/generate_nds_native_owners.py --check` — PASS
- native owner geometry/primitive closure — PASS
- Donkey high: 16 canonical + 4 variants, 494 source triangles
- Donkey low: 16 canonical + 4 variants, 314 source triangles
- Donkey low vertex closure: 942 source corners preserved
- Donkey low matrix routing: all 942 corners routed to their source joint's GX
  palette slot
- Donkey low winding: 0 inconsistent shared edges
- primitive closure: 314 / 314 source triangles preserved in both checked modes

## Frozen candidate

Build:

`make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-battle-core`

ROM:

- `builds/build-p2-battle-core/smash64ds-p2-fourcpu-tickhud-hwtri.nds`
- bytes: **30,049,280**
- SHA-256: `CD764742FCFC24D916CC2896EE22D07BE4918C3B88E45BC34C37F41B819B1AA5`

ELF:

- bytes: **15,200,484**
- SHA-256: `F31F317C0403ED9E3F7E35BB1D4D55A8F9D85B2534A32DF34FAE59B854459FB4`

## Focused natural proof

Command:

`pwsh -NoProfile -ExecutionPolicy Bypass -File .\\scripts\\probe-p2-fourcpu-sparse.ps1 -NoBuild -Build build-p2-battle-core -FirstDonkeyReject -Frame 1536 -Artifact artifacts\\verification\\p2-3-donkey-modelpart-binding-1536.txt -TimeoutSeconds 300`

Artifact SHA-256:

`121923CCC692730EC4B10621C0C7518AD0874D69704A2A600EA8FD312F5C832B`

Terminal witness:

```text
DONKEYREJECT_NONE_THROUGH=1536
```

The same natural four-CPU path that rejected Up Smash at frame 416 before the
fix therefore crosses that state without any Donkey native-program rejection.

## Widest relevant verifier

Command on the same frozen ROM:

`pwsh -NoProfile -ExecutionPolicy Bypass -File .\\scripts\\verify-p2-four-fighter-stress.ps1 -NoBuild -Build build-p2-battle-core`

Coverage:

- start frame: 1
- timing start: 2
- end frame: **1,973**
- clock: **60 -> 1**
- elapsed: **59 / 60 s**
- presented delta: **1,972**
- logic delta: **3,944**
- roster: **Donkey/Samus/Link/Kirby**
- fighter draw mask: **0xF**

Resource and sibling gates remain green:

- general heap free-min: **94,076 B**
- safety-floor margin: **68,476 B**
- graphics heap: **232 / 1,536 B**
- `syMalloc` overflow: 0
- object-manager panic: 0
- pose-bind full: 0
- texture reject mask: 0
- DamageSlash: **136 draws / 120 triangle draws / 0 submit failures**

Verifier hashes:

- tick HUD JSON: `3E1F3293B1980CC551AE8DD70DC1888A416D5165F4CB869574729984B18728D3`
- memory JSON: `0B750027CCBD55FE5884152EE94D5932B28A6FE48A8D0B87B5BA539484881F29`
- coverage JSON: `6A648C887CFA0B0AC19AB1BB92B1C791481C61AE3EA265B7BB6E401C363FA054`

### Next native-only blocker

The first wide failure is no longer Donkey. It is now:

- domain: fighter
- fighter kind: **Samus** (`3`)
- asset: **320 / SamusModel**
- status: **`0xA6` / Catch**
- root: **`0x57D8`**
- reason: **2 / `REJECTED_PROGRAM`**

That failure is independent of this Donkey model-part correction and should be
handled through the existing Samus/fighter-production ownership work.

## Acceptance statement

Donkey's low-detail Up Smash model-part program is closed for this measured
natural lifetime. Do not reopen the binding unless contradictory source or
natural-path evidence appears.

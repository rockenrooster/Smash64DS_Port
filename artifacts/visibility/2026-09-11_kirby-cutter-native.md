# P2-3f47 Kirby Final Cutter native-output closure — 2026-09-11

## Outcome

Kirby's complete Final Cutter presentation is native on the measured four-CPU
battle path. The package closes all ten immutable BattleShip display-list roots
used by the Draw, Trail, Up and Down effects plus both immutable roots of the
travelling Final Cutter weapon.

The full-ROM native-only gate is **not** green yet. After the weapon closure, the
first wide native failure advances out of Kirby Final Cutter to Donkey's
`AttackHi4`: DonkeyModel asset `317`, root `0x5C18`, status `0xCF`,
`REJECTED_PROGRAM`.

## Source authority and ownership

BattleShip source authority:

- `decomp/BattleShip-main/decomp/src/relocData/348_KirbySpecial2.c`
- `decomp/BattleShip-main/decomp/src/ef/efmanager.c`
- O2R: `decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/KirbySpecial2`
  - asset ID: `348`
  - bytes: `10,592` (`0x2960`)
  - SHA-256: `170AA45C1FB013717928F18D0FD51830A087ED48112BCFDC8E7EB3C8C1EFC885`

BattleShip defines four Final Cutter effect descriptors. All source their DObj
trees from `gFTDataKirbySpecial2`; none has an MObj or MatAnimJoint. Draw keeps
its source attachment to fighter joint 17, while Trail/Up/Down retain their
source AnimJoints. The DS native owner therefore preserves the live DObj
attachment/transforms/animation and replaces only the exact immutable source
Gfx/texture packets.

The complete source-root set, with Draw deliberately kept first to preserve the
original focused-witness ordinal, is:

`0x27A0, 0x0C70, 0x0CE0, 0x11B0, 0x1218, 0x1280, 0x2210, 0x2270, 0x22D0, 0x2330`

Offline source compilation recovers exactly **10 groups / 53 triangles / 2
textures**. The only source textures are:

- CI4 32x32 image `348:0x2560`, palette `348:0x2538` (Draw)
- CI4 16x8 image `348:0x0BE0`, palette `348:0x0BB8` (Trail)

Up and Down are source-untextured geometry. No live material state is baked.

## Native implementation

The existing generated gameplay/effect packet owner is reused rather than
adding a parallel renderer:

- `scripts/3d_vfx/generate_nds_entry_effects.py`
  - pins KirbySpecial2 asset/hash;
  - emits the ten Final Cutter roots source-exactly;
  - appends them after existing roots so earlier ordinals remain stable.
- `src/nds/nds_entry_effects.generated.inc`
  - root count becomes 57;
  - group count becomes 101;
  - texture count becomes 56;
  - Cutter root range is `[47,57)`.
- `src/port/renderer_adapter_stage.c`
  - admits only exact KirbySpecial2 pointers for the ten source roots;
  - requires the live parent GObj to be an effect and `dobj->mobj == NULL`;
  - keeps source DObj transforms/AnimJoints live.
- `src/nds/nds_renderer_native_common.c`
  - maps asset 348 to the generated Cutter root range;
  - resets traversal state at independent Draw, Trail, Up and Down effect-tree
    boundaries.

This does not weaken native-only behavior and does not introduce a generic or
reference fallback.

### Texture lifetime

Generated totals after the complete Cutter family:

- total generated textures: **56**
- VSBattle startup-only textures: **41**
- VSBattle startup-only bytes: **18,528**

The startup-only count/bytes are unchanged from the accepted Sword lifetime
package. Both Cutter textures are gameplay-persistent and are therefore not
retired at `GO`.

## Host/source checks

All passed on the final source tree:

- `python scripts/3d_vfx/generate_nds_entry_effects.py --check`
- `python scripts/3d_vfx/test_native_entry_mask_words.py`
- `python scripts/3d_vfx/test_native_shield_reflector_packets.py`
- `python scripts/3d_vfx/test_native_catch_swirl_packet.py`
- `python scripts/3d_vfx/test_native_ko_reflect_packets.py`
- relevant `git diff --check`

## Frozen final candidate

Built with:

`make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-battle-core`

ROM:

- `builds/build-p2-battle-core/smash64ds-p2-fourcpu-tickhud-hwtri.nds`
- bytes: **30,048,256**
- SHA-256: `AF8B6971A27454655AF3E3AD7078213CB6A5974817B527CEF3EB2521A6D18546`

ELF:

- bytes: **15,199,364**
- SHA-256: `A7A8BE411F8C9BFBB4C1984863DAE6931EE80B1A2581C36565005D28339B2BBE`

## Focused natural-path proof

Command:

`pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\probe-p2-fourcpu-sparse.ps1 -NoBuild -Build build-p2-battle-core -FirstCutterReject -Frame 1536 -Artifact artifacts\verification\p2-3f47-kirby-cutter-family-native-1536.txt -TimeoutSeconds 300`

Artifact SHA-256:

`909A9DCB4C6462AA1241EC41241C9A4F4F1C9C8C55570C290A263B7CCE6EFFAD`

Exact terminal witness:

```text
CUTTERREJECT_NONE_THROUGH=1536
CUTTERFINAL=roots:54,6,6,2,2,2,4,2,4,4;entryDraw:736,fallback:0,texReject:0x0,releaseCount:41,releaseBytes:18528
```

Every one of the ten source roots therefore engages naturally in the CPU battle
window. There are zero Final Cutter family rejects, zero generated-owner
fallbacks, and zero texture rejects.

## Widest relevant verifier

Command, on the same frozen ROM:

`pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify-p2-four-fighter-stress.ps1 -NoBuild -Build build-p2-battle-core`

Coverage:

- start frame: 1
- timing start: 2
- end frame: **1,973**
- clock: **60 -> 1**
- elapsed: **59 / 60 s (0.983333...)**
- presented delta: **1,972**
- logic delta: **3,944**
- observed/expected roster: **Donkey/Samus/Link/Kirby**
- draw mask: **0xF**

Resource/lifetime checks remain green:

- general heap free-min: **94,076 B**
- safety floor: **25,600 B**
- margin: **68,476 B**
- graphics heap: **232 / 1,536 B**, overflow/no-room 0
- effect pool: **22 / 38** max active, free-min 16
- particle structs/generators/transforms: **53/112, 15/24, 27/80**, rejects 0
- `syMalloc` overflow: 0
- objman panic: 0
- texture reject mask: **0**
- battle-core externs: **18 patches / 12 loads / 0 failures**
- ShieldPose: **4 loads / 11,799 B / 36 fixups / 0 load/decode/fixup failures**
- DamageSlash remains green: **136 draws / 120 triangle draws / 0 submit failures**

Verifier hashes:

- tick HUD JSON: `0B5F71F7B24777E5BE1C355ACBCAACB0C255379A19D4B61427B4CC2A9EAEDD3C`
- memory JSON: `5B723F92A5F8551495A4CF4ED11C4A7E5782106EB9BB01AAB344A9969661E527`
- coverage JSON: `1EDB1D21A47600AB44FCFF8867F2659C7815ECF36E325BE295E4CBB45E1F1C6B`

### Next native-only blocker

The widest verifier no longer reports any Cutter root. Its first failure is now:

- domain: 2 (non-fighter/stage/effect path)
- scene: 22
- identity: `0x03F40148`
- asset: **328 (`KirbyModel`)**
- root: **`0x1D238`**
- material: 0
- reason: **1 / `NO_PROGRAM`**

BattleShip `328_KirbyModel.c` places `dKirbyModel_DL_0x1D238` in the DObjDLLink
at `0x1D368`, inside the DObjDesc beginning at `0x1D388`; KirbyMain's source
file-handle table points at that DObjDesc and its companion AnimJoint at
`0x1D410`. That root is the next P2-3f47 ownership investigation. CopyLink
remains independently open later and is not conflated with this blocker.

## Acceptance statement

### Travelling weapon closure addendum

The first wide blocker after the effect-family closure, KirbyModel
`328:0x1D238`, is the travelling Final Cutter **weapon**, not Stone and not a
fighter-body root. BattleShip's `dWPKirbyCutterWeaponDesc` points at
`gFTDataKirbyMain` + `llKirbyMainCutterWeaponAttributes`; the imported symbol is
`0x08`. KirbyMain's relocated handle at that slot points to the KirbyModel
DObjDesc at `0x1D388`, which submits the two source lists `0x1D238` and
`0x1D308`.

Source asset:

- `decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/KirbyModel`
- asset ID: **328**
- bytes: **120,948**
- SHA-256: `F25ADCA3C25B36D5C65BD00C4E0A5973D9E1C4519F8EAB67AD2B3008A55EA9CC`

Offline source compilation recovers exactly **2 groups / 3 triangles / 1
persistent texture** for the weapon roots. `0x1D238` contributes one
untextured triangle; `0x1D308` contributes two textured triangles. No live MObj
or MatAnim material state is baked.

The first admission attempt correctly failed closed because compact battle
packing does not publish the raw KirbyModel through `gFTDataKirbyModel`. A
focused live diagnostic proved the source weapon itself was valid:

```text
CUTTERLIVE=kmodel:(nil),dobj:0x23cd058,parent:0x23cdaf0,parentid:1012,mobj:(nil),wp:0x2356780,wpkind:4,cutterkind:4
```

The final admission follows the authoritative relocated display-list pointer
with `ndsRelocFindLoadedFileContaining`, requires asset **328**, exact root
`0x1D238` or `0x1D308`, live parent kind `nGCCommonKindWeapon`, exact
`nWPKindCutter`, and `dobj->mobj == NULL`. Weapon physics, collision, lifetime,
reflection, hit audio/effects and the live source DObj remain BattleShip-owned.

Generated totals after adding the weapon remain within the accepted lifetime
contract:

- roots: **59** total; weapon range `[57,59)`
- generated textures: **57** total
- VSBattle startup-only textures: **41**
- VSBattle startup-only bytes: **18,528**

The new weapon texture is gameplay-persistent and is not retired at `GO`.

#### Frozen weapon-closure candidate

ROM:

- `builds/build-p2-battle-core/smash64ds-p2-fourcpu-tickhud-hwtri.nds`
- bytes: **30,049,280**
- SHA-256: `B85CE5885DCEB7B8D7EA60D48C4D958CBAD5BB7CC814A0E2690AEC3D49D49483`

ELF:

- bytes: **15,200,484**
- SHA-256: `9CEF9F63807A70C6AA107C350CB47F6FC869FE04B583583EA246362A7DF3AA14`

Focused natural proof:

`pwsh -NoProfile -ExecutionPolicy Bypass -File .\\scripts\\probe-p2-fourcpu-sparse.ps1 -NoBuild -Build build-p2-battle-core -FirstCutterReject -Frame 1536 -Artifact artifacts\\verification\\p2-3f47-kirby-cutter-weapon-native-1536-final.txt -TimeoutSeconds 300`

Artifact SHA-256:

`BBB36393226EACE5975CFA7C583B85AB2C8C74D8E5D9E1BBD4B90A7F1DF1743F`

Exact terminal witness:

```text
CUTTERREJECT_NONE_THROUGH=1536
CUTTERFINAL=roots:54,6,6,2,2,2,4,2,4,4;entryDraw:776,fallback:0,texReject:0x0,releaseCount:41,releaseBytes:18528
CUTTERWEAPON=roots:20,20
```

Both weapon roots therefore engage naturally **20 times each** while all ten
effect roots remain engaged, with zero generated-owner fallback and texture
reject mask 0.

The required widest verifier on the same ROM again reaches frame **1,973** /
clock **1** with Donkey/Samus/Link/Kirby and draw mask `0xF`. Resource checks
remain green: general heap free-min **94,076 B** against the **25,600 B** floor
(**68,476 B** margin), graphics heap **232 / 1,536 B** with zero overflow/no-room,
`syMalloc` overflow 0, objman panic 0, pose-bind full 0, and texture reject mask
0. DamageSlash remains **136 draws / 120 triangle draws / 0 submit failures**.

Final verifier hashes:

- tick HUD JSON: `A49916A69E591ABA2D041F68CB5B9844AFED48DA9C92D3847E8A561851E62B7A`
- memory JSON: `A31B76043B18669DC4F020AAC068704CD4B84311A31F2CB631335339EBCBABD0`
- coverage JSON: `7D6BFD5B622067D6FA9112C8CFCCECC69EC11CFC7C6877F5AA25EB7C8587421F`

The first native-only failure is now fighter-domain DonkeyModel asset `317`,
status `0xCF` (`AttackHi4`), root `0x5C18`, reason **2 /
`REJECTED_PROGRAM`**. That is independent of Final Cutter and is the next wide
native-output blocker.

P2-3f47 remains open globally for Kirby CopyLink/remaining unique-state coverage,
but **Final Cutter Draw/Trail/Up/Down plus its travelling weapon is closed for
this measured natural lifetime**. Do not reopen this Final Cutter package
without contradictory natural-path evidence.

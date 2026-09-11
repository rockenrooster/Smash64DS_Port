# P2-3f47 Kirby Final Cutter native-output closure — 2026-09-11

## Outcome

Kirby's Final Cutter source effect family is native on the measured four-CPU
battle path. The package closes all ten immutable BattleShip display-list roots
used by the Draw, Trail, Up and Down effects, not only the first root exposed by
the native-only gate.

The full-ROM native-only gate is **not** green yet. On the same frozen candidate,
the first wide native failure advances to KirbyModel asset `328`, root
`0x1D238`, `NO_PROGRAM`, under the existing P2-3f47 Kirby package.

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

P2-3f47 is still open globally, but **Final Cutter Draw/Trail/Up/Down is closed
for this measured natural lifetime**. Do not reopen it without contradictory
natural-path evidence. Continue P2-3f47 at KirbyModel `328:0x1D238`.

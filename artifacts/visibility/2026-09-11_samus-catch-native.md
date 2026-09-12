# Samus Catch native fighter + grapple closure — 2026-09-11

## Outcome

Samus's ordinary `Catch` natural path is native for the measured four-CPU
battle lifetime, including both source-owned presentation halves:

1. the 21-root live SamusModel topology created by Catch's hidden grapple-arm
   joints; and
2. the SamusSpecial2 grapple-beam glow at source root `0x02E0`, including its
   looping two-frame source TEXID animation.

The full-ROM native-only gate remains open. On the same frozen candidate, its
first wide failure advances to Link (`fkind 5`) / LinkModel asset `324`, Catch
status `0xA6`, root `0x5B68`, `REJECTED_PROGRAM`.

## Source contract: Catch changes Samus topology

BattleShip source authority:

- `decomp/BattleShip-main/decomp/src/ft/ftmain.c`
- `decomp/BattleShip-main/decomp/src/ft/ftparam.c`
- `decomp/BattleShip-main/decomp/src/ft/ftdata.c`
- `decomp/BattleShip-main/decomp/src/relocData/216_SamusMainMotion.c`
- `decomp/BattleShip-main/decomp/src/relocData/217_SamusMain.c`
- `decomp/BattleShip-main/decomp/src/relocData/320_SamusModel.c`

`dFTSamusMotionDescs[Catch]` and `CatchPull` carry
`FTANIM_FLAG_ANIMLOCKS | 0x1FF80000`. The source `ftMainSetStatus` hidden-part
walk interprets those high bits as hidden-part IDs 3..11. For Samus those rows
create/reparent the grapple chain rooted at joints 17..24 (plus the non-drawing
joint 36 path), rather than merely replacing one already-selected display list.

The Catch motion then performs the source model-part mutations:

```text
joint 24 -> modelpart 1
joint 25 -> hidden
joint 17 -> modelpart 0
joint 18 -> modelpart 0
joint 19 -> modelpart 0
joint 20 -> modelpart 0
joint 21 -> modelpart 0
joint 22 -> modelpart 0
```

That grows the low-detail native root vector from canonical **14** roots to
**21**. The seven inserted drawable roots are:

```text
joint 17: 0x8D90
joint 18: 0x9140
joint 19: 0x9140
joint 20: 0x9140
joint 21: 0x9140
joint 22: 0x9140
joint 24: 0x8A70
```

The five `0x9140` entries deliberately reuse one immutable source geometry
bake under five different live DObj matrices.

### Pre-fix natural discriminator

`artifacts/verification/p2-3-samus-catch-reject.txt` stopped at presented
frame 430:

```text
SAMUSREJECT=430,status:0xa6,battle_slot:1,program:0,decline:4,owner:5,selected:21,tried:1
SAMUSVALIDATE=3,5,1,4294967295,21,14,21,0,0,0
```

Validator code 3 is the exact root-count mismatch: observed 21, expected 14.
The live root dump independently matched the source-derived Catch topology
above.

## Native fighter program

`scripts/fighters/generate_nds_native_owners.py` now treats the Catch-only
roots as a **program-only appendix**, not generic passive root variants. This is
an important fail-closed distinction: `0x8D90`, `0x9140`, and `0x8A70` are legal
only when the complete source Catch topology is present.

The generator derives the Catch program from:

- SamusMain's `FTHiddenPart` table;
- the Catch `0x1FF80000` hidden-part mask;
- SamusMain's `modelparts_container`; and
- the Catch motion's exact model-part events.

It then requires exactly 21 roots and exactly the three new immutable root
identities above. The existing root-program vertex-cache proof confirms every
new root is self-contained; this permits the five live `0x9140` DObjs to share
one source-exact geometry bake safely.

The production program intentionally captures every selected Catch DObj from
its **actual live source-tree root** instead of inventing a canonical hidden
parent schedule. Shipping production therefore keeps all live source matrices
while the generated program owns only immutable geometry/state.

## Source contract: grapple-beam glow

BattleShip source authority:

- `decomp/BattleShip-main/decomp/src/relocData/349_SamusSpecial2.c`
- `decomp/BattleShip-main/decomp/src/ef/efmanager.c`
- O2R: `decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/SamusSpecial2`
  - asset ID: **349**
  - bytes: **3,312**
  - SHA-256: `46A0BCC0BEB839C08C44E3A43569FBBE62A72D8C17AD90729E1762E7CF010503`

The grapple glow's DObjDLLink submits root `0x02E0`. Its display list owns the
fixed combine, tile, load-block, geometry and two triangles, but segment-E slot
0 supplies `G_SETTIMG` from one live MObj. That MObj has exactly
`MOBJ_FLAG_ALPHA`; its MatAnim loops TEXID between the two source IA8 16x16
images:

- TEXID 0 -> source image `349:0x0110`
- TEXID 1 -> source image `349:0x0008`

The generator compiles root `0x02E0` twice offline, once with each source image.
It requires geometry, matrix provenance and every immutable render-state field
to be identical between the two compiles; only image identity may differ. The
result emits the geometry once (**1 group / 2 triangles**) and retains two
preconverted DS texture frames. Runtime snapshots the live source MObj and uses
its current TEXID only to select between those two resident DS textures. No N64
texture/DL decoding was added to runtime.

Generated packet totals on the final source tree:

- groups: **104**
- triangles: **556**
- textures: **59**
- Samus grapple root ordinal: **59**
- Samus grapple texture slots: **57 / 58**
- startup-only texture lifetime remains **41 textures / 18,528 bytes**; the
  two grapple frames are gameplay-persistent.

## Static/source checks

All passed on the final source tree:

- `python scripts/3d_vfx/generate_nds_entry_effects.py --check`
- `python scripts/3d_vfx/test_native_entry_mask_words.py`
- `python scripts/3d_vfx/test_native_shield_reflector_packets.py`
- `python scripts/3d_vfx/test_native_catch_swirl_packet.py`
- `python scripts/3d_vfx/test_native_ko_reflect_packets.py`
- `python scripts/fighters/generate_nds_native_owners.py --check`
- relevant `git diff --check`

## Frozen final candidate

Built with:

`make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-battle-core`

ROM:

- bytes: **30,054,400**
- SHA-256: `476E86B2B9B0520922B3B53FAC1B7F0C5C4C5608AC58E057EF692F23436A90E5`

ELF:

- bytes: **15,204,744**
- SHA-256: `AC53F36EEA9AEFC23346E64705CD31C8A42B754C1BD6F08177F3DA9210EC30D2`

## Focused natural-path proof

Command:

`pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\probe-p2-fourcpu-sparse.ps1 -NoBuild -Build build-p2-battle-core -FirstSamusReject -Frame 1536 -Artifact artifacts\verification\p2-3-samus-catch-complete-native-1536.txt -TimeoutSeconds 300`

Artifact SHA-256:

`69D678C82BF10151007FE22062458D71DA588B70B7B88D7EAC99ABA9559C8A11`

Terminal witness:

```text
SAMUSREJECT_NONE_THROUGH=1536
SAMUSFINAL=grapple:12,entryDraw:788,fallback:0,texReject:0x0,releaseCount:41,releaseBytes:18528
```

This is positive engagement evidence, not a zero-counter proof: the source
grapple root actually submits **12** times in the natural window while both the
fighter and effect rejection traps remain silent.

## Widest relevant verifier

Command on the same frozen ROM:

`pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify-p2-four-fighter-stress.ps1 -NoBuild -Build build-p2-battle-core`

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

Resource/sibling gates remain green:

- general heap free-min: **91,164 B**
- safety floor: **25,600 B**
- margin: **65,564 B**
- graphics heap: **232 / 1,536 B**, overflow/no-room 0
- `syMalloc` overflow: 0
- object-manager panic: 0
- pose BindFull: 0
- texture reject mask: **0**
- native owner plan build/hit/mismatch: **703 / 6,366 / 0**
- DamageSlash: **136 draws / 120 triangle draws / 0 submit failures**

Verifier hashes:

- tick HUD JSON: `D7107D94865830698C4109B3776B9354F7EDFFDB325843E7D638968D3B1CB876`
- memory JSON: `8E6830295327CE6967464517B0B8D33FA200F5A73447DF2D401F8CEA9795E541`
- coverage JSON: `54D315DA5CC28DF91F2D0C5D33A2E63128E45A2AC7D9108B981C724DCCFD21AA`

### Next native-only blocker

The first wide failure is no longer Samus. It advances to:

- domain: fighter
- fighter kind: **Link (`5`)**
- asset: **324 / LinkModel**
- status: **`0xA6` / Catch**
- root: **`0x5B68`**
- reason: **2 / `REJECTED_PROGRAM`**

That is an independent Link root-program issue and is the next native-output
investigation. It does not weaken this Samus acceptance.

## Plain production build health

`make TARGET=smash64ds BUILD=build` completed from the settled source tree.

- `smash64ds.nds`: **53,116,928 B**
- SHA-256: `FAF832765C41A620FFD02E047A6F3F7F8FB6EF8D16C5D37A615B68477ADE6938`

This is a build-health artifact only; the full native-only gate remains red at
the Link blocker above.

## Selective-commit reproducibility check

The Samus-only staged package was also materialized from the immutable
`35ab1a83dfef` baseline into an isolated checkout and regenerated from source.
All static packet/owner checks above passed there, and the exact staged source
built successfully as `smash64ds-p2-fourcpu-tickhud-hwtri`:

- ROM: **28,335,104 B**, SHA-256
  `5C7FB3019369A610B399B475912946E8BA7CC349F2E4C337BCBD629423AD676B`
- ELF: **14,414,584 B**, SHA-256
  `0AF9C211A6E97665204301FDA5A597BFE7697157B26093E26D5B2496B2382F01`
- regenerated ignored fighter-owner IR: **5,324,413 B**, SHA-256
  `46E380CE4A8CA616C2504BBE5E49B087EB35067968BF1A8820BC2D44036170BA`

That isolated checkout intentionally excluded unrelated dirty integration work.
Its interpreter-only four-CPU harness did **not** reach the requested battle
frame marker even with a 900-second observation bound; it also hit no Samus
rejection breakpoint. Therefore that run is recorded only as a non-engaging
observation timeout and is **not** acceptance evidence. The natural-path and
wide acceptance claims above remain tied only to the frozen integrated ROM
`476E86B2...36A90E5`, whose complete input identity is recorded in this note.

## Acceptance statement

Samus Catch is closed for this measured natural lifetime, including the full
21-root fighter topology and the animated native grapple-beam presentation.
Do not reopen it without contradictory source or natural-path evidence.

# Meta Knight preparation recipe — 2026-09-30

Status: `IMPLEMENTED_NOT_ACCEPTED`. This records the preparation needed before
the Meta-enabled Make build. It does not establish a playable ROM, target pixels,
audio engagement, resource limits or performance acceptance.

The recipe was assembled read-only from the donor manifest, generated receipts,
producer argument parsers and the root integrator's execution summary. No command
below was run while writing this document. **Captured** means argv is present in
a receipt; **confirmed** means root supplied the executed arguments; **reconstructed**
means the command uses the current producer contract and frozen canonical paths
but has not been replayed as a cold preparation chain.

## Roots and immutable prerequisites

Base branch commit: `2e093297c5cabff53deeefd89d499aebfdf9f728`. Checkpoint must
also preserve this branch's source changes and producer scripts. Run preparation
serially, with all writes confined to the isolated checkout. `decomp/` remains
read-only. The donor ROMs are host conversion inputs and never DS runtime assets.

```powershell
$taskRoot = 'D:/Stuff/DevFolder/Smash64DS_Port/.worktrees/meta-knight'
$referenceRoot = 'D:/Stuff/DevFolder/Smash64DS_Port'
$pythonExe = 'C:/Users/Tyler/AppData/Local/Programs/Python/Python313/python.exe'
$extraRoot = "$referenceRoot/decomp/smashremix-plus-extra"
$remixRoot = "$referenceRoot/decomp/smashremix"
$sourceDir = "$extraRoot/extra_characters/MetaKnight"
$donorDir = "$taskRoot/builds/p4/meta-knight-donor"
$nativeDir = "$taskRoot/builds/p4/meta-knight-native"
$runtimeDir = "$taskRoot/builds/p4/meta-knight-runtime"
$modelIR = "$nativeDir/meta-knight-model-ir.json"
$baseFgm = "$taskRoot/assets/audio/fgm_phase_pack_ima.bin"
$legacyAdmission = "$taskRoot/builds/p4/legacy-admission.bin"
$env:NDS_REFERENCE_ROOT = $referenceRoot
$env:BATTLESHIP_O2R = "$referenceRoot/decomp/BattleShip-main/BattleShip_o2r"
$env:BATTLESHIP_RELOCDATA = "$referenceRoot/decomp/BattleShip-main/decomp/assets/us/relocData"
$env:META_KNIGHT_SOURCE_DIR = $sourceDir
Set-Location -LiteralPath $taskRoot
```

The reference roots above select only read-only assets and ignored decomp build
inputs. Tracked source, policy, layout validation and output roots remain in the
worktree. Both reference repositories must pass the preparation producer's clean
pin checks. The worktree must also have the read-only tracked BattleShip source
headers/TUs its source validators consume.

| Input/tool | Frozen identity |
|---|---|
| EXTRA revision | `96621afea26a83305abaf81add07dcf5a9c5fe3e` |
| Remix revision, including EXTRA's nested gitlink | `5e04fe7fcd023cd43c71f25f89bb6e810d254d55` |
| `docs/p4/source-lock.json` | SHA256 `eef937ae09a2077075df245cd11ff0bcc6d8d05fb589f92afb93c0b6982ea683` |
| Source ROM `D:/Stuff/DevFolder/Battleship/BattleShip/baserom.us.z64` | 16,777,216 B; SHA1 `e2929e10fccc0aa84e5776227e798abc07cedabf`; SHA256 `15592e79d3c5295cef4371d4992f0bd25bec2102fc29644c93e682f7ea99ef3d` |
| Python 3.13.15 executable above | SHA256 `85b71d8c6ec1905935f74be0c9869aae198d00e98f39df699ec66f9c5a84cecd` |
| EXTRA `Pipfile.lock` | SHA256 `846802ffb9f4395e7ab7aa769a0f0ad2185d51860f1df470d060a6e84363e350` |
| Remix `original.xdelta` | SHA256 `e892db9f57135ae2ba380e6db98516dd849dcabb4be5c553ea48158ec7dece1d` |
| Remix `xdelta.exe` | SHA256 `847f811f51bbbf31f25962169e552a09cb0751678c62c198bd9da8db86474361` |
| Remix `assembler/bass.exe` | SHA256 `fc08bea157e3c5f53f6d9c78c0335cffdc501e0f1a4c5aac3800194a76052f24` |
| Remix `assembler/rn64crc.exe` | SHA256 `4793bc38528de452ccf8d7dce2d39c2b44d935c1eb1f39ad8fd561f317d2ee0b` |
| ARM compiler `C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe` | 3,228,672 B; SHA256 `5e4c6997e5ff32b9a2a08a88f94c27111a902e9423c8c09943f96c9c9ff8b816` |
| Source `config.yaml` | SHA256 `88c1fdb83301e81c9a454f1e1f4dd3e2b32371ecac6c5860bcb14ef334c90127` |
| Source `main_reqlist.txt` | SHA256 `91adbcbb8d14fbd322aa0cd764b3ef95c01808b392a5ca8c2ca5506137485439` |
| Source `main.bin` | 2,416 B; SHA256 `9b51e7065e9b44a4a276ab96f18c69ba794ea1ca0e52adcaf9f5cae1422c4f07` |
| Source `character.bin` | 75,296 B; SHA256 `59cefdb861812208253763953d6bdbff25590ddf14b381b14b933063733eaf86` |
| Source `victory_theme.bin` | 1,244 B; SHA256 `bb61724e663797833394b0eec19fc8923110c66c1a3aac2042800ae6d24d08b6` |
| Legacy FGM input at `$baseFgm` | 6,968,728 B, 573 entries; SHA256 `52514d45dea6f27ec74750b83c2cbb8a971ef46ddd2b9cf03b85a4e7c0571cef` |
| Qualified twelve-row admission input | 261,168 B; FNV `0xDA643018`; SHA256 `cf229b0b1b76b7d51d8c78510a0d05932e6a81202b1c42cf825607418577c34a` |

The admission SHA256 was recovered in memory from the merged artifact's preserved
twelve directories and 260,960 legacy record bytes, using the inverse of
`build_from_legacy_payload`. Root's execution record identifies the historical
input as `D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2-fourcpu-tickhud/nitrofs/fighters/admission.bin`;
the current main artifact must still match the hash above before reuse.
Restore that qualified input to `$legacyAdmission`, or reconstruct it from the
legacy typed source and require the identities above before extension. The legacy
FGM artifact is also a prerequisite: restore its qualified bytes or use the normal
legacy audio preparation chain and require its exact hash. Do not use the merged
13-row admission or 601-entry FGM pack as their own legacy inputs.

## 1. Resolve the disposable donor

Reconstructed outer driver commands, using the documented source pins and ROM:

```powershell
& $pythonExe "$taskRoot/scripts/fighters/prepare_extra_donor.py" --phase preflight --extra-root $extraRoot --remix-root $remixRoot --rom 'D:/Stuff/DevFolder/Battleship/BattleShip/baserom.us.z64' --dest $donorDir
& $pythonExe "$taskRoot/scripts/fighters/prepare_extra_donor.py" --phase stage --extra-root $extraRoot --remix-root $remixRoot --rom 'D:/Stuff/DevFolder/Battleship/BattleShip/baserom.us.z64' --dest $donorDir
& $pythonExe "$taskRoot/scripts/fighters/prepare_extra_donor.py" --phase resolve --extra-root $extraRoot --remix-root $remixRoot --rom 'D:/Stuff/DevFolder/Battleship/BattleShip/baserom.us.z64' --dest $donorDir
```

`stage` requires a fresh empty destination; do not overwrite the retained donor.
The driver owns venv creation, hash-pinned dependencies, xdelta, appending only
MetaKnight, assembly, CRC and review extraction. Earlier failed review retries
are retained in the historical manifest; a new successful run needs no canary or
manual assembly edit.

The final successful internal commands below are **captured** in
`donor-manifest.json`; their manifest entries have `returncode: 0`:

```json
{
  "cwd": "D:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\builds\\p4\\meta-knight-donor\\extra",
  "argv": ["D:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\builds\\p4\\meta-knight-donor\\extra\\smashremix\\assembler\\bass.exe", "-o", "ssb64asm_extra_review.z64", "main.asm", "-sym", "review-symbols.log"]
}
{
  "cwd": "D:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\builds\\p4\\meta-knight-donor\\extra",
  "argv": ["D:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\builds\\p4\\meta-knight-donor\\extra\\smashremix\\assembler\\rn64crc.exe", "-u", "ssb64asm_extra_review.z64"]
}
{
  "cwd": "D:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\builds\\p4\\meta-knight-donor",
  "argv": ["C:\\Users\\Tyler\\AppData\\Local\\Programs\\Python\\Python313\\python.exe", "D:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\scripts\\fighters\\extra_resolved_actions.py", "--rom", "D:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\builds\\p4\\meta-knight-donor\\extra\\ssb64asm_extra_review.z64", "--symbols", "D:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\builds\\p4\\meta-knight-donor\\extra\\review-symbols.log", "--output", "D:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\builds\\p4\\meta-knight-donor\\resolved-actions.json"]
}
```

Expected donor identities: `original_extra.z64` SHA256
`042864cc0ce50f482d9607c30b41b0ef1a68e4c5121c4b5fda491ec3b12f7612`;
reference/action ROM `6b7948d4aab7f93b1f34ae667e7be371192c36b65fd58e01749c55167a203e47`;
review ROM `6a5f54d7dd5582586914d09bfd7b2fa17fce68ef3fbffb3f811ac55a7010f793`;
review symbols `a802044dade386de25b427ffcbf2c07b508eabcf39e4798897b4a4238e2f6249`;
resolved actions `8a0486a4bdc52c54d3e289faed629117ce0d313d49ee06b0d13d85f28ceb3cc5`.
The review has 252 actions, 225 main parameters, 15 menu parameters and 205
callback addresses. The historical donor manifest itself is SHA256
`e7b1864bde37a6d27d02e65e2231b0f543ce87192edc428613462a6e4df62b02`.

## 2. Convert native resources and complete geometry IR

These argument sets are **confirmed executed** by root; variables render the
recorded main/worktree paths:

```powershell
& $pythonExe "$taskRoot/scripts/fighters/extra_native_asset_adapter.py" --donor-dir $donorDir --native-source-root "$referenceRoot/decomp/BattleShip-main/BattleShip_o2r" --output-dir $nativeDir
& $pythonExe "$taskRoot/scripts/fighters/extra_resource_adapter.py" --source-dir $sourceDir --model-ir --model-asset-id 5456 --donor-main-id 5455 --donor-model-id 5456 --output $modelIR
```

Resource output is 172 containers plus `native-runtime-bindings.json` and
`nds_metaknight_native_assets.generated.h`. Preserve Main 5455, Model 5456,
native Motion `0x6000`, all source animations/dependencies and explicit offsets.
Do not truncate the 65,264-byte maximum main animation. IR must include the full
23-root storage union in each detail, the canonical live subset and both electric
skeletons; source joints/material ownership and inherited bindings remain explicit.

## 3. Compile layout metadata, runtime seams and lifecycle

The following producer invocations are **reconstructed**, not captured outer argv:

```powershell
& $pythonExe "$taskRoot/scripts/fighters/generate_meta_attribute_metadata.py" --bindings "$nativeDir/native-runtime-bindings.json" --donor-manifest "$donorDir/donor-manifest.json" --assets-root $nativeDir --compiler 'C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe' --decomp-root "$taskRoot/decomp/BattleShip-main/decomp" --libnds-include 'C:/devkitPro/libnds/include' --output-header "$nativeDir/nds_metaknight_attribute_metadata.generated.h" --output-json "$nativeDir/meta-attribute-metadata.json"
& $pythonExe "$taskRoot/scripts/fighters/generate_p4_runtime_data.py" --root $taskRoot --out-dir $runtimeDir --resolved-actions "$donorDir/resolved-actions.json" --native-bindings "$nativeDir/native-runtime-bindings.json" --donor-source "$donorDir/extra"
& $pythonExe "$taskRoot/scripts/fighters/generate_meta_lifecycle_source.py" --donor-manifest "$donorDir/donor-manifest.json" --actions "$donorDir/resolved-actions.json" --decomp-root "$taskRoot/decomp/BattleShip-main/decomp/src" --output-root "$taskRoot/builds/p4/meta-knight-lifecycle"
```

The attribute receipt **captures** the actual layout-oracle argv:

```json
["C:\\devkitPro\\devkitARM\\bin\\arm-none-eabi-gcc.exe", "-std=gnu11", "-march=armv5te", "-mtune=arm946e-s", "-mthumb", "-DARM9", "-D_LANGUAGE_C", "-DSSB64_TARGET_NDS", "-DREGION_US", "-DNDS_P4_METAKNIGHT=1", "-ID:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\include", "-ID:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\decomp\\BattleShip-main\\decomp\\src", "-ID:\\Stuff\\DevFolder\\Smash64DS_Port\\.worktrees\\meta-knight\\decomp\\BattleShip-main\\decomp\\src\\sys", "-IC:\\devkitPro\\libnds\\include", "-S", "-H", "-x", "c", "-o", "-", "-"]
```

Its stdin is constructed by `query_fields()` in the producer. Oracle assembly
SHA256 is `1b8c7e2c7169d8549788c3f5c00c84b6a2fad0bc8db3bf793be9d7f9ec8aeb52`;
all compiler/header identities are retained in `meta-attribute-metadata.json`.
The runtime emits four files under `$runtimeDir/nds/generated`, preserving
legacy runtime kinds/sentinels and Meta kind 29. Lifecycle copies are generated
from source and qualified before publication; they are never hand-edited.

## 4. Emit images, FPC/BEX, admission and the core contract

Image invocation is **confirmed executed** by root and appears in the Make
producer contract. Other invocations are **reconstructed** from their CLIs:

```powershell
& $pythonExe "$taskRoot/scripts/fighters/generate_nds_native_owner_images.py" --source-root $referenceRoot --repo-root $taskRoot --extra-model-ir $modelIR
& $pythonExe "$taskRoot/scripts/fighters/generate_preview_core_packs.py" --output-dir "$taskRoot/assets/fighters/preview_core" --kinds metaknight --meta-native-dir $nativeDir --meta-model-ir $modelIR
& $pythonExe "$taskRoot/scripts/fighters/generate_battle_core_packs.py" --output-dir "$taskRoot/assets/fighters/battle_core" --kinds metaknight --meta-native-dir $nativeDir --meta-model-ir $modelIR
& $pythonExe "$taskRoot/scripts/fighters/generate_nds_fighter_admission.py" --legacy-payload $legacyAdmission --meta-native-dir $nativeDir --out "$taskRoot/assets/fighters/admission.bin" --meta-report "$nativeDir/meta-texture-admission-2.json"
& $pythonExe "$taskRoot/scripts/fighters/generate_metaknight_core_contract.py" --native-dir $nativeDir --preview-dir "$taskRoot/assets/fighters/preview_core" --battle-dir "$taskRoot/assets/fighters/battle_core" --header "$nativeDir/include/nds/generated/nds_metaknight_core_contract.generated.h"
```

The producer contract also supports reconstructing the legacy payload with
`generate_nds_fighter_admission.py --no-header --out $legacyAdmission` before
extension, using the complete qualified legacy typed closure. This fallback
preparation step is **untested here** and requires its ignored legacy source
closure inputs; verify the exact legacy bytes/hash above before proceeding.

Preserve image slots 0–24; Meta is 25 and its two electric skeletons are 26/27.
Both `29.fpc` files are identical, as are both `29.ext` files. BEX must contain
all eleven source Main extern rows: dependency 232 at 52, dependency 351 at 8496,
and nine shield-pose rows for asset 331. Final admission has thirteen rows, 180
Meta records including electric variants, 281,344 bytes and FNV `0xD3A2073E`.
The earlier 174-record/base-only admission is superseded and invalid for use.

## 5. Convert own FGM and victory music, then emit audio pins

These are **reconstructed** invocations from the frozen receipts and CLIs:

```powershell
& $pythonExe "$taskRoot/scripts/fighters/extra_audio_adapter.py" --repo-root $taskRoot --reference-root $referenceRoot --source-dir $sourceDir --donor-dir $donorDir --base-pack $baseFgm --out-bin "$nativeDir/audio/fgm_phase_pack_ima.bin" --out-json "$nativeDir/audio/fgm_phase_pack_ima.json"
& $pythonExe "$taskRoot/scripts/fighters/generate_p4_audio_header.py" --pack "$nativeDir/audio/fgm_phase_pack_ima.bin" --metadata "$nativeDir/audio/fgm_phase_pack_ima.json" --output "$nativeDir/include/nds/generated/nds_p4_audio.generated.h"
& $pythonExe "$taskRoot/scripts/fighters/extra_bgm_adapter.py" --repo-root $taskRoot --reference-root $referenceRoot --source-dir $sourceDir --donor-dir $donorDir --out-bin "$nativeDir/audio/bgm_win_meta_knight_ima.bin" --out-json "$nativeDir/audio/bgm_win_meta_knight_ima.json" --out-header "$nativeDir/include/nds_p4_bgm.generated.h"
```

FGM extension retains all 573 legacy records and emits 601 entries, mapping
`0xBED77FC6`, with source appender/sound/decoder identities in its JSON receipt.
Victory music is Meta's own linked score 375: 207 notes, eight finite packets,
65,160 bytes. Its source CTL SHA256 is
`75b204f4edaa0fcd2f848c62fd29c866946c84608d3deca383b50b027684ad6b`;
source TBL SHA256 `ed6e2838e41d2f290980097b4545e0b70ac8972ce9fbf878a82ce6094793df51`;
reachable instrument/wave graph SHA256
`7fbd1b405051d73a2e45a0813c5b59e8efd3732e843143b5f7ff18a18da18a65`.
Audio receipts retain decoder/renderer/producer hashes and fidelity checks owed.

## Frozen output comparisons

These hashes were read from the retained preparation artifacts on 2026-09-30,
before the subsequent bounded capture/throw/bounce source repair, except the
lifecycle manifest updated after the integrator's successful nine-copy regeneration.
Receipts with absolute paths can change when relocating a checkout; compare
their pinned inputs and content hashes as well as the path-specific receipt hash.
Later producer/source changes require a coherent updated preparation receipt.

| Output (relative to worktree) | SHA256 |
|---|---|
| `builds/p4/meta-knight-native/native-runtime-bindings.json` | `b596583a1f8d3cf9fd655073e246d9b4a03c0bbd3b5f35ef6bff04ce2535f073` |
| `builds/p4/meta-knight-native/meta-knight-model-ir.json` | `a7e1372ccc70755901b443846a6debcf341c09c6337c45ddbc495699d44a450f` |
| `builds/p4/meta-knight-native/meta-attribute-metadata.json` | `1ad81db943068dc9a4d0f112346f11b630b92246d0244c56bc5f48099231d7c0` |
| `builds/p4/meta-knight-native/meta-texture-admission-2.json` | `cd7de95c158dcabf7f08b8bcc53ae958561926d441b24bbc48081556dc60b0e3` |
| `builds/p4/meta-knight-native/reloc_extra/MetaKnightMain` | `1752112cb765d4e6d00110d3921f5941aed25fb5194b76f4d1003674f16f249c` |
| `builds/p4/meta-knight-native/reloc_extra/MetaKnightModel` | `f8fee7e857baa30fcf842c584e7da91cedc7c7a973c3ad567ca950ac21930482` |
| `builds/p4/meta-knight-native/reloc_extra/MetaKnightMainMotion` | `b3f894c040ec2e8f4ddb96b98873a1d2fac073f0b665ff016d7256ab98c9aea0` |
| `builds/p4/meta-knight-native/nds_metaknight_attribute_metadata.generated.h` | `576911e4c2aa79d645b9757fd844a8e84cc02bb02e6234c70fd47e93e04580f0` |
| `builds/p4/meta-knight-native/nds_metaknight_native_assets.generated.h` | `ce4d3da4577a53ccbc5933c75b295a3f41ac4bc117d1cfa65abd0e0ee760d43e` |
| `builds/p4/meta-knight-native/include/nds/generated/nds_p4_audio.generated.h` | `6caa63138a6629bf9b071b4a69b032860e3b6f95c78a8125748ff4c84fd1c2bb` |
| `builds/p4/meta-knight-native/include/nds_p4_bgm.generated.h` | `1bff508d397166f70d908a0cf5e01a659877e2fba05a1712335329c2a889e7bf` |
| `builds/p4/meta-knight-native/include/nds/generated/nds_metaknight_core_contract.generated.h` | `28dc53f2b28b81af2cc2e9bca7bda4149e047b59b8da6018b7ccda3ca0491240` |
| `builds/p4/meta-knight-runtime/nds/generated/battleship_ftmain.generated.inc` | `12d6318db5087bb5005ea948428bff62a02072681980b72926a4d889888c638d` |
| `builds/p4/meta-knight-runtime/nds/generated/battleship_ftmanager.generated.inc` | `654d9edc9014e01d3f2fc238a1dd8ef656e7a37b5094a8930d54a0c4d7cfd725` |
| `builds/p4/meta-knight-runtime/nds/generated/battleship_ftcomputer.generated.inc` | `58122fad4af001b4a844d9b87277a1f292317d80710bd8b16ece60e3b6adaad6` |
| `builds/p4/meta-knight-runtime/nds/generated/nds_p4_runtime.generated.inc` | `ae975c9911d49198e0e662f6e4b2db13f81da52a9e51919720de0afc3fdc6d2b` |
| `builds/p4/meta-knight-lifecycle/lifecycle-source-manifest.json` | `3f28e068b66d2baf2633c7964fd3b4898529c2abf86ce6ace540f2caa45aa772` |
| `assets/fighters/preview_core/29.fpc` and `assets/fighters/battle_core/29.fpc` | `3df5eb37b95210b72d780c1239cf2f32b014b4a2b903b40a83943d8908585c07` |
| `assets/fighters/preview_core/29.ext` and `assets/fighters/battle_core/29.ext` | `f502ba2cecde6c788a5acefb6c93fc77442e3e2bec4f4b57f5d32da4520f7d20` |
| `assets/fighters/admission.bin` | `4b32480044e4525318c9e7479af7e93c0ec2c25e0c092e4a518050895682c305` |
| `include/nds/generated/nds_fighter_admission.generated.h` | `7d4cc0571b871401d125c02ddd8192a7a7f6797209fcb0d0bf15d761376bf3c9` |
| `include/nds/generated/nds_native_fighter_image.generated.h` | `ac3ee033f7b98b61c83ddb0a66dca2b8a3848ec5d3cacbf03b2b08c62813f5e8` |
| `builds/p4/meta-knight-native/audio/fgm_phase_pack_ima.bin` | `fbbc8db045ffef3b6ecd6fb0bda53d033eb3f9f188c43189fda8ef0a2098a621` |
| `builds/p4/meta-knight-native/audio/bgm_win_meta_knight_ima.bin` | `f6d59c02302ea231351f38ad43682605a8014d257ba871d5d5f53e95f13a2217` |

## Make gaps and checkpoint requirements

The lifecycle CLI in section 3 was confirmed executed after the bounded repair:
exit 0, nine source copies, producer SHA256
`cb81589c9b6edf52ee6a7cae958ba4fec331e301b3378015285e4335f0b45957`.

Make currently **requires this preparation**; it is not a cold regeneration
entry point for all Meta artifacts. Preserve the command classification above
instead of describing a normal Make invocation as sufficient reconstruction.

- Runtime include files, native resource/binding metadata, attribute metadata,
  audio/BGM headers and the core-contract header have no complete automatic
  producer edges. The lifecycle outputs have a producer but omit dependencies
  on all source TUs, source lock, review ROM and symbols they read.
- Image generation passes `--extra-model-ir`, but that ignored IR is absent from
  its generator prerequisite list. A changed IR may leave stale image C/header.
- Meta `reloc_extra/*` is enumerated with a wildcard; an absent directory makes
  the required resource inventory empty. Packaging must require/verify the
  bindings manifest's complete asset inventory, particularly native Motion and
  animation/dependency containers.
- Meta FPC/BEX and the thirteen-row admission Make recipes copy already prepared
  ignored assets. They do not run the Meta producer argument sets above. The
  FGM/BGM recipes likewise copy prepared private files.
- The donor command receipt is itself ignored. This document preserves key
  pins/commands; retain the donor, native bindings, attribute, admission, audio
  and lifecycle receipts with their hashes as checkpoint evidence. Do not
  commit ROMs, raw logs, packed assets or lab outputs as a substitute.

The current branch has not passed an end-to-end cold replay of this reconstructed
chain. That replay, native Meta-enabled ROM qualification and natural-input
CSS → SSS → match → Results/rematch remain due to the sole integrator.

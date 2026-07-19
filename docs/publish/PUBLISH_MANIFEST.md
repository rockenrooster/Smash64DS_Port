# Smash64DS publish manifest — TASK P1

Audit date: 2026-07-19

Audited commit: `c82519df89a4ab593d12610ce9cdafe2ea009057` (`master`)

Shipping target: `smash64ds-battle-playable-hwtri`

Machine-readable companion: `docs/publish/publish_manifest.json`

## Result

The current ROM is reproducible from the current checkout, but the public-source
delivery decision is blocked before TASK P2. The local BattleShip shell matches
upstream commit `62b513e9895ac5a4e833102e098afdfc05c9a48c`; its `decomp/` subtree does not
match that commit's pinned submodule. A pinned recursive clone therefore cannot
reproduce the current source closure or ROM without an additional, deliberate
source-delivery mechanism.

Do not start TASK P2, publish an upstream pin, or export a public tree until the
source-delivery choice is made. The two safe choices are:

1. vendor the exact filtered DECOMP-SOURCE closure with the decomp's upstream
   licensing limitation recorded; or
2. fetch the closest identified decomp base and apply a reviewed source-only
   patch set that reproduces the five DS-specific files and all required source
   identities.

The task's fallback policy requires Tyler to choose between those approaches.
No ROM, O2R, extracted asset, or generated content is included in this audit.

## Shipping ROM identity

The canonical direct build command is:

```powershell
make TARGET=smash64ds-battle-playable-hwtri -j4
```

The clean audit used an isolated object tree and output directory:

```powershell
make TARGET=smash64ds-battle-playable-hwtri `
  BUILD=build-build-publish-p1-closure-20260719 `
  NDS_OUTPUT_ROOT=D:/Stuff/DevFolder/Smash64DS_Port/builds/build-publish-p1-closure-20260719 `
  -j4
```

The object tree did not exist when the detached build began. Its first link
attempt found that the separately overridden output directory had not yet been
created; after creating only that isolated directory, the same invocation
resumed the clean tree through link and packaging. Exit code was 0. The result
is byte-identical to the root ROM:

| Property | Current value |
|---|---:|
| Output | `smash64ds-battle-playable-hwtri.nds` |
| Size | 14,688,256 bytes |
| SHA-256 | `C344CA8B89903A35678A6DC4A849F217B690CBCE16841FAC47974626899EB9DF` |
| Root ROM UTC timestamp | `2026-07-19T11:38:36.3036964Z` |
| Detached ROM UTC timestamp | `2026-07-19T12:03:22.5340348Z` |

`docs/P1_EXECUTION_BOARD.md` agrees with this current identity. The publish-task
prompt's older 14,669,824-byte / `DADB7C96...` expectation predates the retained
attack/hit A/V asset work and is stale; it must not be used as TASK P2's identity
gate.

## Build-input closure

The file-level closure is the union of:

- 146 compiler/assembler `.d` files from the clean object tree;
- every active Makefile and linker prerequisite, including the devkitARM rule
  include chain;
- all 296 files staged into NitroFS and their source paths;
- the exact external archive/start-file inputs named by the linker map; and
- the ARM7 payload and icon consumed by `ndstool`.

The Makefile invokes no repo-local generator during this shipping build. It
generates `nds_build_config.h` and `nds_scene_harness_config.h` inline. Port
asset generators therefore belong to the pre-build regeneration workflow, not
to the current Make dependency graph.

| Bucket | Files | Meaning |
|---|---:|---|
| PORT-CODE | 259 | 159 `src/` files, 97 `include/` files, `Makefile`, and two linker files |
| DECOMP-SOURCE | 232 | BattleShip/decomp C and headers fetched or otherwise supplied as source |
| ROM-DERIVED | 298 | 290 active O2R resources, six generated NitroFS payloads, and two generated native-owner content includes |
| GENERATED-METADATA | 3 | two transient build headers and the static-texture key/offset include |
| TOOL | 114 | devkitARM/libnds/calico headers, rules, start files, archives, ARM7 payload, and icon |
| **Total** | **906** | Every row is listed and classified in `publish_manifest.json` |

All 906 resolved paths existed during the audit. The active shipping target has
`NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=0`, so its relocData Makefile list is
empty. `decomp/.../assets/us/relocData/` is not an active shipping input; it is
an additional TASK P2 regeneration-exactness requirement from the publish
prompt.

No path under `decomp/sm64-nds`, `emulators/`, or melonDS appears in the file
closure or in any required asset generator. `sm64-nds` remains architecture
reference only. Emulator tooling contributes zero build inputs.

## ROM-derived inventory and regeneration

The canonical US ROM is 16,777,216 bytes. Both BattleShip `config.yml` and the
decomp's `smashbrothers.us.yaml` identify it by SHA-1:

```text
e2929e10fccc0aa84e5776227e798abc07cedabf
```

This is the NTSC-U v1.0 `NALE` big-endian dump normally named
`baserom.us.z64`. The local copy at the decomp source root matches that size and
hash; the audit did not copy or alter it.

### Extraction layers

BattleShip's Windows extraction sequence is:

```powershell
cmake -S . -B "build\us" -A x64 -DSSB64_VERSION=us
cmake --build "build\us" --config Release --target ExtractAssets
```

At the audited BattleShip source, `ExtractAssets` builds the pinned Torch
sidecar and runs the equivalent of:

```text
torch o2r baserom.us.z64 -s <BattleShip-source> -d <build>/extracted
```

This produces the zip-format archive
`<build>/extracted/BattleShip.o2r`. The DS Makefile does not consume that
archive; it consumes an unpacked `decomp/BattleShip-main/BattleShip_o2r/`
directory. No checked-in port or BattleShip script materializes that directory.
TASK P2 must add deterministic archive extraction and byte-compare the resulting
2,159-file tree.

Torch does **not** emit `decomp/assets/us/relocData/`. That 3,130-file tree is a
separate output of the decomp's extraction pipeline:

```powershell
# from the decomp source root, with baserom.us.z64 present
make extract VERSION=us
```

The target runs Splat, then `tools/relocData.py extractAll`, and produces
`assets/us/relocData/`. It is not part of the current shipping target's active
closure, but the P2 G1 instructions explicitly require reproducing and comparing
it.

### Port-generated outputs

| Outputs | Generator and exact source inputs | Verdict |
|---|---|---|
| Four BGM `.raw` + four `.json` files | `scripts/render-audio-bgm-pupupu.py`, invoked for sequence 0, 12, 16, and 22; reads O2R `S1_music_sbk`, `B1_sounds1_ctl`, `B1_sounds1_tbl` plus decomp `cseq_to_mid.py`, `decode_ctl.py`, and `audio_codec.py` | Complete and deterministic |
| `fgm_phase_pack_ima.bin/.json` | `scripts/render-audio-fgm-phase-pack.py`; reads five O2R FGM/sound-bank resources, three decomp audio decoders, source audio/mixer/scene/sine/action files, and the DS FGM header | Complete and deterministic |
| Static texture payload + metadata include | `scripts/generate_battle_playable_static_textures.py`, `generate_battle_playable_texture_census.py`, and `generate_pupupu_water_aot.py`; reads pinned stage/actor/Fox O2R resources and typed decomp declarations | Complete and deterministic |
| `nds_native_stage_owner.generated.inc` | `scripts/generate_nds_native_stage.py`; reads four pinned stage O2R resources and pinned decomp declarations | Complete, but must run before the public build because the output is content |
| `nds_native_fighter_owner.generated.inc` | `scripts/generate_nds_native_owners.py`; reads Mario/Fox O2R resources **and an embedded base64 profile export in the generator itself** | Gap: generator is not publish-safe yet |

There are 11 tracked `assets/` files. Six are active binary NitroFS inputs; five
JSON files are regeneration evidence rather than Make inputs. All 11 are
ROM-DERIVED and excluded from the public export.

The expected set was incomplete: two tracked native-owner includes also contain
ROM-derived geometry/state, and the fighter generator embeds the profile export
used to create one of them. These are additional ROM-derived findings, not
GENERATED-METADATA.

## Embedded-data audit

All 152 clean-build objects were scanned with the actual devkitARM `nm`. No
read-only or initialized-data symbol is 64 KiB or larger. That threshold does
not make smaller content arrays publishable; provenance still controls.

| Source | Classification | Evidence and disposition |
|---|---|---|
| `src/nds/nds_native_stage_owner.generated.inc` | CONTENT / ROM-DERIVED | Contains source geometry, corners, state and material programs. The built object includes a 4,992-byte vertex array. Exclude; regenerate from O2R with `generate_nds_native_stage.py`. |
| `src/nds/nds_native_fighter_owner.generated.inc` | CONTENT / ROM-DERIVED | Contains Mario/Fox vertices, corners, triangles and state programs. The built object includes a 6,492-byte dense-vertex array. Exclude; regenerate only after the generator gap is removed. |
| `scripts/generate_nds_native_owners.py` | CONTENT-bearing / ROM-DERIVED | Its `EXPORT` constant embeds base64 state, sequence and vertex data from a profile-2 export. Exclude in current form. |
| `src/nds/generated/battle_playable_static_textures.generated.inc` | GENERATED-METADATA | 6,432 bytes of built records containing keys, flags, offsets, sizes and hashes; texels live only in the excluded NitroFS payload. May ship. |
| `src/nds/battle_playable_static_textures.c` | PORT-CODE | Lookup/validation code only; it includes the metadata file and no texel/sample array. |
| `src/nds/nds_renderer.c` light-shade LUT | PORT-CODE / runtime state | The LUT is a 2,096-byte BSS cache. `ndsRendererHardwareGetLightShadeLut` computes its 128 RGB entries from live diffuse/ambient colors. It is not an initializer and carries no ROM content. |

## Upstream and divergence verdict

Upstream BattleShip is [JRickey/BattleShip](https://github.com/JRickey/BattleShip).
The best root match is:

```text
62b513e9895ac5a4e833102e098afdfc05c9a48c
```

Of its 413 ordinary tracked root blobs, 412 are present and byte-identical. The
only absent file is `.github/workflows/release.yml`, which is neither source nor
part of the build closure. Root source is therefore identical at that commit.

The recursive source tree is not identical:

- `libultraship` matches the pinned `b58e7063da285e01b3ac8e86d942c7cbf71b5483`
  exactly (430/430 blobs).
- `torch` matches the pinned `8060a909d9fa128d8c28db1ae2bcb6be7c446790`
  exactly (664/664 blobs).
- BattleShip commit `62b513e...` pins decomp commit
  `f961176767440d81959150d5685e8d20766419ce`. Only 120 of the 232 decomp files
  in the DS build closure match; 112 differ.
- The local decomp tree instead best matches VetriTheRetri/ssb-decomp-re commit
  `e6f3eee68dbe19fbac87914b613ff4ea6f29e251`: 3,383/3,388 tracked blobs match,
  with no missing files. The five modifications are DS-specific changes in:
  `src/mn/mncommon/mnstartup.c`, `src/mv/mvopening/mvopeningroom.c`,
  `src/sc/scmanager.c`, `src/sys/objhelper.c`, and `src/sys/taskman.c`.

All five modified files are in the shipping build closure. This is a real
source-delivery divergence, not output noise, and it triggers the TASK P1 stop
rule.

## License and attribution result

BattleShip's root `LICENSE` is MIT for its port-specific source and requires the
copyright and permission notice to accompany redistributed portions:

```text
Copyright (c) 2026 JRickey and contributors
```

Its scope notes explicitly do not grant that license to Nintendo/HAL content or
to the decompiled source subtree. The decomp upstream does not publish an
explicit license. A public NOTICE must preserve this distinction and must not
describe the decomp as MIT-licensed. libultraship and Torch carry their own MIT
notices; any filtered-vendor choice must retain the applicable notices.

## Reference toolchain

| Component | Audited version |
|---|---|
| PowerShell | 7.6.3 |
| Git | 2.54.0 |
| Python | 3.10.11 |
| GNU Make | 4.4.1 |
| devkitARM | r67.1-1 |
| arm-none-eabi-gcc | 15.2.0 |
| devkitarm-binutils | 2.45.1-2 |
| devkitarm-newlib | 4.6.0.20260123-5 |
| devkitarm-rules | 1.6.0-4 |
| libnds | 2.0.2-1 |
| calico | 1.2.0-1 |
| ndstool | 2.3.1-1 |
| general-tools | 1.4.4-1 |
| CMake (extraction host) | 4.3.3 |
| Ninja (available extraction host) | 1.12.1 |
| MSVC Build Tools | Visual Studio Community 2026 18.7.4 |

The DS build requires `DEVKITPRO=C:\devkitPro` and
`DEVKITARM=C:\devkitPro\devkitARM` in this reference environment. A stranger
build should accept any correctly installed compatible location and print the
detected versions.

## TASK P2 gap list

1. **Blocking source-delivery choice:** a recursive BattleShip pin is not exact.
   Choose filtered source vendoring or an explicit base-plus-patch delivery.
2. **Fighter generator content:** remove the embedded base64 profile export from
   `generate_nds_native_owners.py` and derive it deterministically from the
   user-ROM extraction, or supply a new build-time capture/generator that does.
3. **O2R materialization:** deterministically unpack `BattleShip.o2r` into the
   directory layout consumed by the DS Makefile and verify all 2,159 files.
4. **Separate relocData extraction:** run and qualify the decomp's `make extract
   VERSION=us`; Torch does not produce this directory. Record that it remains
   inactive in the current shipping target.
5. **Additional generated-content outputs:** regenerate the native stage and
   fighter includes before `make`, not only the 11 `assets/` files named in the
   original P2 prompt.
6. **Manifest refresh:** after resolving gaps 1-5, add the final `build.ps1`, pin
   or filtered-source mechanism, publish-safe fighter generator, and templates
   to `shipping_allowlist`; then re-run the closure and identity proof.

## Machine allowlist state

`publish_manifest.json` contains all 906 classified closure files and a
conservative 267-file P1-safe shipping allowlist: the exact 260 tracked
PORT-CODE/GENERATED-METADATA build files, six publish-safe regeneration scripts,
and `.gitattributes`. It intentionally excludes all assets, decomp/O2R paths,
both native-owner content includes, and the current content-bearing fighter
generator.

This is an audit allowlist, not the final P3 export list. TASK P2 must amend it
after the blocking source decision and regeneration gaps are resolved.

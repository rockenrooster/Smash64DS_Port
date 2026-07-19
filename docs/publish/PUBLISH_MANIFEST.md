# Smash64DS publish manifest — TASK P1/P2

Audit date: 2026-07-19

P1 audit commit: `c82519df89a4ab593d12610ce9cdafe2ea009057` (`master`)

P2 implementation commit: `84dc33dbf4957d88b644f24af5980ff50c35d1b6`

Shipping target: `smash64ds-battle-playable-hwtri`

Machine-readable companion: `docs/publish/publish_manifest.json`

## Result

TASK P2 is verified. Tyler selected the closest-upstream-base plus reviewed
source-patch option. `DECOMP_PIN.txt` now pins the BattleShip shell, its
libultraship and Torch submodules, VetriTheRetri's closest decomp base, the five
DS-specific source identities, the canonical ROM identity, and the reference DS
ROM identity. `scripts/publish/patches/ssb-decomp-re-ds.patch` contains only the
five reviewed source changes and is itself pinned by SHA-256.

The supported build entry point is:

```powershell
.\build.ps1 -Rom "C:\path\to\baserom.us.z64" [-Jobs N] [-Clean] [-DecompPath <BattleShip-root>]
```

The script checks prerequisites and ROM identity, acquires or reuses the pinned
source, extracts O2R and relocData, regenerates all ROM-derived port content,
builds `smash64ds-battle-playable-hwtri`, and reports its identity. It never
ships the ROM, O2R archive/tree, relocData, the 11 derived asset files, or either
native-owner content include.

| P2 gate | Result |
|---|---|
| G1 regeneration | PASS: 2,159/2,159 O2R files, 3,130/3,130 relocData files, and 16 derived port outputs had zero byte differences |
| G2 ROM identity | PASS: 14,688,256 bytes; SHA-256 `C344CA8B89903A35678A6DC4A849F217B690CBCE16841FAC47974626899EB9DF` |
| G3 failures | PASS: missing devkitPro, missing ROM, and wrong-hash ROM each returned nonzero with the designed message |
| G3 idempotence | PASS: a complete warm rerun succeeded and reproduced the same ROM identity |
| Requested sanity | PASS: `.\scripts\verify-dev-fast.ps1 -Build` completed its fixtures, build contracts, published-ROM check, and realtime smoke |

No ROM, O2R, extracted asset, or generated content is included in the publish
allowlist.

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
directory. `build.ps1` now materializes the archive with
`System.IO.Compression.ZipFile`, requires exactly 2,159 files, and replaces only
that validated generated directory. G1 found zero path, size, or SHA-256
differences against the pre-P2 tree.

Torch does **not** emit `decomp/assets/us/relocData/`. That 3,130-file tree is a
separate output of the decomp's extraction pipeline:

```powershell
# from the decomp source root, with baserom.us.z64 present
make extract VERSION=us
```

The upstream Makefile explicitly rejects native Windows, while its VPK0 helper
is published only for Linux and macOS. `build.ps1` therefore runs the same
pinned Splat split directly, then uses
`scripts/extract-battleship-relocdata.py`, a source-licensed host adapter for the
same VPK0/table extraction. It produces `assets/us/relocData/`; G1 found zero
path, size, or SHA-256 differences across all 3,130 files. The tree remains
inactive in the shipping target.

### Port-generated outputs

| Outputs | Generator and exact source inputs | Verdict |
|---|---|---|
| Four BGM `.raw` + four `.json` files | `scripts/render-audio-bgm-pupupu.py`, invoked for sequence 0, 12, 16, and 22; reads O2R `S1_music_sbk`, `B1_sounds1_ctl`, `B1_sounds1_tbl` plus decomp `cseq_to_mid.py`, `decode_ctl.py`, and `audio_codec.py` | Complete and deterministic |
| `fgm_phase_pack_ima.bin/.json` | `scripts/render-audio-fgm-phase-pack.py`; reads five O2R FGM/sound-bank resources, three decomp audio decoders, source audio/mixer/scene/sine/action files, and the DS FGM header | Complete and deterministic |
| Static texture payload + metadata include | `scripts/generate_battle_playable_static_textures.py`, `generate_battle_playable_texture_census.py`, and `generate_pupupu_water_aot.py`; reads pinned stage/actor/Fox O2R resources and typed decomp declarations | Complete and deterministic |
| `nds_native_stage_owner.generated.inc` | `scripts/generate_nds_native_stage.py`; reads four pinned stage O2R resources and pinned decomp declarations | Complete, but must run before the public build because the output is content |
| `nds_native_fighter_owner.generated.inc` | `scripts/generate_nds_native_owners.py`; deterministically parses the exact Mario/Fox O2R joint trees and display lists | Complete and deterministic; embedded profile content removed |
| Decomp `assets/us/relocData/` | pinned Splat plus `scripts/extract-battleship-relocdata.py`; reads the canonical ROM-derived `relocData.bin` | Complete and byte-identical on native Windows |

There are 11 tracked `assets/` files. Six are active binary NitroFS inputs; five
JSON files are regeneration evidence rather than Make inputs. All 11 are
ROM-DERIVED and excluded from the public export.

The expected set was incomplete: two tracked native-owner includes also contain
ROM-derived geometry/state. They remain excluded and are regenerated from the
user's extracted O2R. The fighter generator no longer embeds any profile export.

## Embedded-data audit

All 152 clean-build objects were scanned with the actual devkitARM `nm`. No
read-only or initialized-data symbol is 64 KiB or larger. That threshold does
not make smaller content arrays publishable; provenance still controls.

| Source | Classification | Evidence and disposition |
|---|---|---|
| `src/nds/nds_native_stage_owner.generated.inc` | CONTENT / ROM-DERIVED | Contains source geometry, corners, state and material programs. The built object includes a 4,992-byte vertex array. Exclude; regenerate from O2R with `generate_nds_native_stage.py`. |
| `src/nds/nds_native_fighter_owner.generated.inc` | CONTENT / ROM-DERIVED | Contains Mario/Fox vertices, corners, triangles and state programs. The built object includes a 6,492-byte dense-vertex array. Exclude; regenerate only after the generator gap is removed. |
| `scripts/generate_nds_native_owners.py` | PORT-CODE | Parses exact user-ROM O2R inputs at build time; contains no embedded state, sequence, vertex, triangle, or texture payload. May ship. |
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

All five modified files are in the shipping build closure. TASK P1 therefore
stopped correctly. TASK P2 resolves the divergence by fetching the identified
`e6f3eee...` base and applying the reviewed five-file patch. The patch SHA-256
is `377151B88E60DDE59F05AD3290B991CFA3B8090F8237E7B00B16037A5B24E259`;
the five post-patch file hashes are independently pinned in `DECOMP_PIN.txt`.

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

## TASK P2 gap resolution

1. **Source delivery:** resolved by the pinned base-plus-five-file-patch path.
2. **Fighter generator:** embedded profile content removed; exact O2R parsing is
   deterministic and reproduces the owner include byte-for-byte.
3. **O2R materialization:** deterministic 2,159-file zip extraction added and
   byte-qualified.
4. **relocData:** native-Windows Splat/VPK0 adapter added; 3,130 files reproduce
   exactly and remain inactive in the shipping target.
5. **Generated content:** all 11 tracked assets, static metadata, native stage,
   native fighter, and both consumed-field manifests regenerate before `make`.
6. **Manifest:** human and machine inventories refreshed after G1/G2/G3.

## Machine allowlist state

`publish_manifest.json` contains all 906 classified closure files and a
272-file P2-safe shipping allowlist: the exact 260 tracked
PORT-CODE/GENERATED-METADATA build files, eight publish-safe regeneration
scripts, `.gitattributes`, `build.ps1`, `DECOMP_PIN.txt`, and the reviewed source
patch. It intentionally excludes all assets, decomp/O2R/relocData paths, and
both native-owner content includes.

This is the input allowlist for the P3 public-export rehearsal; P3 may add only
its reviewed public documentation, license, notice, and packaging files.

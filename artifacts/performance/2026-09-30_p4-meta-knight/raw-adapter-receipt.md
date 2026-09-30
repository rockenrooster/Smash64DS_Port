# Raw EXTRA model adapter receipt — 2026-09-30

Status: `SOURCE_INVENTORY_NOT_NATIVE_ADMISSION`. This host prerequisite has no
ROM, DS gameplay, native visual or performance acceptance claim.
The native geometry/image-provider checkpoint below supersedes the earlier
geometry conversion gap; runtime and visual acceptance remain due.

## Implemented producer boundary

`scripts/fighters/extra_resource_adapter.py` reads explicit `--source-dir` or
`META_KNIGHT_SOURCE_DIR` inputs. It never discovers/changes donor checkouts, assigns
DS file IDs, enables a fighter or emits an emulator payload. Its CLI can emit
stable sorted JSON; the integrator owns that producer invocation.

- Decodes big-endian threaded pointer words: upper u16 is next slot in words,
  lower u16 is target offset in words; `0xFFFF * 4` is the byte-head sentinel.
- Rejects unaligned/truncated/out-of-range heads/next slots/internal targets,
  cycles, overlapping chains and request cardinality mismatches. Symbolic
  requests and explicit donor-file IDs retain their original identities/order.
- Preserves live external references to offset zero. Main slot `0x238` targets
  the character's material dispatch at zero; it is not a null pointer.
- Identifies FTAttributes, both DObjDesc trees, setup masks, canonical/hidden
  roots, material records, passive alternatives, accessory and stock art.
  Span provenance and unresolved reachability remain visible in the inventory.
- `RawResource.as_native_resource(asset_id, dependency_ids, native_stage_module)`
  constructs the existing O2RResource/PointerRef shape in memory only after the
  caller provides every numeric asset identity. Missing IDs and booleans fail.

## Observed pinned source facts

Read-only input directory:
`D:/Stuff/DevFolder/Smash64DS_Port/decomp/smashremix-plus-extra/extra_characters/MetaKnight`.
The worktree EXTRA submodule is empty, so host tests use this explicit path.

| Resource | Bytes | Internal / external fixups | SHA256 |
|---|---:|---:|---|
| `main.bin` | 2,416 | 15 / 62 | `9b51e7065e9b44a4a276ab96f18c69ba794ea1ca0e52adcaf9f5cae1422c4f07` |
| `character.bin` | 75,296 | 582 / 0 | `59cefdb861812208253763953d6bdbff25590ddf14b381b14b933063733eaf86` |

Main requests contain 51 `${CHARACTER}` rows, two explicit legacy requests
(`00E8` moveset, `015F` music) and nine `014B` shield-pose rows. No character
external request list is needed for this raw model resource.

FTAttributes is at main `0x624`, 840 bytes. Its commonparts pointer targets
main `0x234`; high/low trees start at character `0x3CB0` / `0x8438`, each with
31 descriptors plus the depth-18 sentinel. Setup words `0xEF7CFFC0, 0` select
22 descriptors, hence 23 initial live nodes including TopN. Each initial
detail has 13 roots with 291 direct triangles. These counts exclude later
wing/modelpart execution and are not frame geometry/performance measurements.

FTHiddenPart records include wing joints 30–33 and auxiliary joint 34. The
adapter inventories four non-null hidden wing roots for each detail and
retains zero/transform-only records without silently classifying them as dead.
The passive table has ten source records: one high/low alternative at joint12
and four high/low alternatives at joint34. The accessory at main `0x21C`
references character `0xAED0`; its eight commands contain no direct triangles.

Stock provenance: main `FTAttributes + 0x340` -> main FTSprites `0x41C` ->
character Sprite `0x123F8`; logical size 8x10, CI4, attr `0x220`. Bitmap
`0x123E8` has source width 8, stored stride16, height10 and pixel buffer
`0x122A8` (80 bytes). The six stock LUT pointers at main `0x404..0x418` target
character `0x122F8`, `0x12320`, `0x12348`, `0x12370`, `0x12398`, `0x123C0`.
Each CI4 lookup needs 32 palette bytes. Sprite's declared n_tlut=256 is retained
as source metadata; no invented adjustment is made. Emblem pointer is `0x125D8`.

## Host verification

Command:

```powershell
python scripts/fighters/test_extra_resource_adapter.py --source-dir D:\Stuff\DevFolder\Smash64DS_Port\decomp\smashremix-plus-extra\extra_characters\MetaKnight -v
```

37 tests passed (helper final run: 0.109s). They include synthetic malformed
chains/requests, explicit-ID bridge negatives, pinned binary hashes and exact
source roots/material alternatives/stock footprints, repeated JSON equality,
and six copied-fixture corruptions: invalid config head, missing attr marker,
first/tree sentinel corruption, non-finite transform and foreign target bounds.
The helper initially caught four boolean-ID subtest failures; IDs now require
`type(value) is int`, and the regressions passed. `py_compile` and focused
`git diff --check` passed. No generator, donor build or ROM build was invoked.

## Remaining conversion seam

The existing owner compiler still loads source through `P2_O2R_ASSETS` and
owner dictionaries (`generate_nds_native_owners.py:_build_source_export_for_owners`,
`load_o2r_payload`, `decode_joint_topology`, `build_dense_geometry`). No global
maps were monkeypatched and no fabricated O2R IDs were assigned. A compact
typed input provider must supply resource, ordered roots/bindings, source
high/low/setup metadata, passive/hidden root sets and actual material semantics.
Reuse its native state/action/triangle/run/epoch/dense/image emitters afterward.
`build_p2_root_set_runtime_context` is the useful independent-root-vector shape
for the first canary; it still needs that source provider.

The adapter inventory explicitly leaves callbacks/actions, inherited motions,
shield poses, costume mapping, actual hidden/modelpart reachability, texture
closure and runtime metadata unqualified. Resolving the pinned donor with the
supported ROM remains necessary before gameplay admission. Standalone model
inspection does not replace that resolution or satisfy CSS-to-playable-match.

## Native geometry/image-provider checkpoint

`NativeOwnerSource` now supplies a hash-checked typed O2RResource/raw payload to
the ordinary native owner compiler without changing its catalogs. Both Meta
Knight detail unions compile with the caller-assigned model asset ID 5456:

| Detail | Union roots | Triangles | Dense vertices | States | Epochs | Runs | State-only roots | Native IR SHA256 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| High | 23 | 407 | 713 | 62 | 39 | 35 | 4 | `7280e482fa04e8ea835341f7015dd3270814360c26e71cddf010e02cd294e8ea` |
| Low | 23 | 407 | 713 | 62 | 39 | 35 | 4 | `7f452f40d1a26d773b8097f6d8acd7b8b8a13067f4e5eb213aa6388e73b7bdb4` |

Storage coverage is 13 canonical roots/291 triangles, four hidden wing roots/
six triangles, five passive alternatives/110 triangles and one control-only
accessory. It is not a draw-all union or a frame polygon count. Nineteen source
joints supply compact bindings; live draw selection must match source offsets
and source joints.

Actual source differences required producer handling: tail gSPVertex writers
feed the next welded joint and now retain zero-run action epochs; repeated
complete wing LIGHTCOL prefixes collapse to final colors only when every update
precedes the first vertex writer; FC121805/FF17FFFF requires the existing
per-triangle opaque source-alpha proof. Exact new policy pairs FC123245/00400087
and FC321803/FF17FFFF retain their words at families 6/7, with immutable white
RGBA proofs. Their material/texture consumer and visual qualification is a
separate task, particularly the first pair's source alpha behavior.

`generate_nds_native_owner_images.py --extra-model-ir` consumes the verified
inventory, restores JSON primitive-mode keys, appends image slot 25 after all
25 legacy slots and emits compiler-derived high/low sizeof byte macros. It
also produces `nds_native_metaknight.generated.inc`: exact source roots,
source-joint/binding/material references, canonical 23-node construction
hierarchy, thirteen canonical root indices and hidden-parent records.
No static live hierarchy is invented for the complete storage union.

Production input/output roots are separate (`--source-root` read-only references,
`--repo-root` output worktree). A necessary existing input-root bug was repaired:
skeleton O2R lookup now forwards its explicit repo and uses root-scoped caches;
default callers retain their original cache. The initial helper was interrupted
before editing those two files, so the authorized fix was completed serially.

The producer accepts raw or processed request lists. Numeric local donor IDs
must be explicitly supplied with `--donor-main-id 5455 --donor-model-id 5456`
(or `source_resource_ids` in the host API); absent mappings never guess local
resources. The native model ID is an independent caller argument.

Host provider/image/source-root suite: 49 tests passed in 9.914s. The full real
legacy owner/skeleton/hat header test preserves all legacy struct layouts and
slot definitions while appending Meta Knight slot 25, both paths and exact
sizeof macros. Existing Link Boomerang executable tables match across default
and explicit source routes in both details. Synthetic negatives cover source
identity, geometry/cache/light/color proofs and saved inventory corruption.
All modified modules pass py_compile and focused diff checks. The raw suite
also passed after the typed bridge's payload hash addition.
The raw suite was extended for processed numeric request lists after that new
input mode landed: 42 tests passed in 0.171s. Numeric request `1550` with the
explicit local mapping preserves source model/stock addresses and binary hashes;
missing identities and duplicate/boolean/out-of-range IDs reject. The prior
49-test full-header/provider evidence was reused because those producers did
not change during this input-mode addition.

No production writer, generator CLI, image compiler or DS build was invoked by
this subtask. Numeric compiled image byte counts, runtime selection, resources,
natural-input match evidence and performance gates remain the integrator's work.

## Electric skeleton closure invalidator

Donor admission proved electric color-animation family 24 reaches skeleton IDs
2 then 1. The source selector at Main `0x618` points at two 31-entry FTSkeleton
arrays (`0x428`, `0x520`). Its scalar 10 is the live common-joint gate used by
`ftDisplayMainDrawAll`, not unused padding. Both variants render source joints
6, 10, 11, 14, 15, 23 and 28. Skeleton 2 joint 6 retains NOFOG flag `0x40`.
All roots, pointer domains, extents and draw flags are validated in the adapter.

| Skeleton | Roots | Triangles | Dense | States | Epochs | Runs | High / Low IR SHA256 |
|---|---:|---:|---:|---:|---:|---:|---|
| 1 | 7 | 201 | 178 | 17 | 12 | 22 | `cde729dcabf5aeff4236c27f5bac972afde2c5648d9025c4f3821552bc0c9c52` / `31153679c6b45f799baba4f67d94da7ba61d3082b26806b840dfcadb96e968b5` |
| 2 | 7 | 186 | 134 | 18 | 11 | 23 | `8a6eac3de1c9e81779c4892ed07eb4f01991cccc7894dbe8952ccd6ad2966245` / `76f98838776e6375cfd66a8a1e2bbcac680f9324b7ac23f87975db38b5be10d0` |

Both detail views compile exact source skeleton geometry. They share the body's
19-joint binding domain; the seven roots use bindings 0, 2, 3, 6, 7, 10 and 12.
The image registry preserves Meta Knight slot 25 and appends skeleton slots
26/27, for 28 total slots. Image input now rejects an inventory missing either
source-required skeleton. The original 25 slots and structs remain unchanged.

Skeleton 1's FC327E05/FF17F9FF alpha variation has the same per-triangle opaque
source-alpha proof before its exact RGB-equivalent family-1 canonicalization.
New skeleton heads reach additional textures; this was reported to native asset
conversion as a concrete admission/pack invalidator. That worker reports six
additional admission records and complete typed root coverage; its receipt owns
those producer claims.

Skeleton 2 roots at joints 14, 23 and 28 call material slot 0 while their own
common DObjs have no MObj. Source `gcDrawMObjForDObj` returns before replacing
segment E, so those roots inherit the actual preceding binder. The adapter
retains empty own chains and explicit inherited-binding/required-slot metadata;
the runtime validates the captured material DObj, not a guessed previous joint.
Generated `MaterialInherited` and `RequiredMaterialSlots` arrays carry this
contract. Parallel `MaterialAssets` records retain typed source ownership;
offset `0x188`, for example, belongs to source CHARACTER 5456, not MAIN 5455.

After this invalidator, 49 raw tests and 54 provider/image tests passed (0.164s
and 10.951s). One focused inheritance test was then added to each suite and
passed in 0.017s / 0.212s; unchanged full-header evidence was reused. Total host
coverage is 50 raw and 55 provider/image tests. Modified modules pass py_compile
and focused diff checks. No production writer or target build ran here.

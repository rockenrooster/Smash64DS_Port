# External review: deriving a fighter's live-after-setup set

**Status:** architectural decision and implementation recommendation  
**Date:** 2026-09-04  
**Repository basis:** `rockenrooster/Smash64DS_Port`, current `master` at commit `ee5985acb3343f4b4567a4623ad99f8f0f415e09`  
**Scope:** P2 step 2, four distinct fighter kinds in a VS battle on Nintendo DS

## Decision

Proceed with a pack, but change the unit of thought.

The safe solution is **not** to discover a minimal subset of bytes that can remain behind the current raw-file pointer ABI. It is to compile the source files offline into a **closed, typed, semantic post-setup fighter ABI**, then make the battle build incapable of reading the discarded raw representation.

That distinction is the answer to the rare-path problem:

- A relocation-only trim leaves ordinary C pointers into partially retained files. An omitted rare target can still become a wild pointer.
- A semantic pack contains typed references, explicit counts, explicit feature predicates, and validated target ranges.
- Battle code is migrated to that ABI, so an unconverted raw-file access is a compile/link/CI failure rather than an untested runtime possibility.
- The loader validates every required object and reference before allowing the battle to start.
- Poisoning and broad play scripts remain useful, but only as secondary falsifiers.

The proposed **6–12 KB Kirby model fragment is not sufficient evidence that P2 fits**. More strongly, a model-only pack is mathematically incapable of closing the present hole:

| Quantity | Bytes |
|---|---:|
| Worst-four current main closures | 577,424 |
| Raw model files within those closures | 271,040 |
| Everything left after deleting all four models for free | **306,384** |
| Optimistic four-kind pack ceiling for a 32 KiB floor, including the already measured 36,864-byte binary loss but no other four-player growth | **175,604** |
| Excess after perfect model deletion | **130,780** |

The pack remains the right architectural shape **only if it replaces the whole post-setup main closure**, including the live metadata currently reached through Main, Model, ShieldPose, and Special files. It is not enough to replace geometry while leaving the other closure members resident.

Whether that whole-closure semantic pack fits is not yet proven. The required reduction is about 69.6% before accounting for other four-player growth. That is aggressive, but plausible because the existing closure contains raw N64 geometry, all-costume presentation data, setup scaffolding, file padding, relocation-oriented pointer structures, and a DS-native renderer representation at the same time. The correct next task is therefore a **census-only semantic pack estimator**, not the runtime loader and not a poison harness.

The estimator should produce a hard go/no-go result before all twelve fighters are integrated:

- **Green:** worst legal four-kind battle pack is at or below an exact generated budget, provisionally no more than about 150–160 KiB.
- **Yellow:** it is below the current hard optimistic ceiling of 175,604 bytes but leaves inadequate room for unmeasured four-player growth; recover static/scene/VRAM memory before runtime integration.
- **Red:** it exceeds 175,604 bytes before other growth; the pack alone cannot preserve a 32 KiB floor in the measured configuration, so another lever is mandatory.

There should be no 495 duplicated payload packs. Generate reusable semantic atoms and factored manifests, then enumerate all 495 exactly-four-kind sets—and all 793 one-through-four-kind sets—offline for size proof. A selected match takes the union of atom IDs and stores each atom once.

---

## Answers to the five sub-questions

| Question | Decision |
|---|---|
| **1. Derivation method** | Use a schema-driven semantic object compiler. Build a typed object graph from compiled relocData symbols and relocations, add explicit rules for implicit/computed edges, root it in a generated post-setup consumer contract, translate each object into a declared runtime disposition, and close the graph for each legal match configuration. Relocation closure and dynamic witnessing are inputs/checks, not the proof. |
| **2. Cross-fighter references** | Use canonical atom identities and set union. A Kirby capability manifest and a Link base manifest refer to the same Link atom, so it is loaded once when both need it. Roster-conditional copy assets are capabilities; unconditional cross-owner dependencies remain base dependencies. Do not duplicate large donor fragments or retain whole donor files. |
| **3. Completeness** | Establish a closed-world boundary: all post-setup consumers use generated pack types/accessors; every source object and edge has a disposition; every dynamic selector has a generated domain; the binder validates every reference before GO; raw fighter loads are disabled afterward; a recursive pointer-domain audit rejects pointers into staging/raw ranges. Poison/watchpoint testing is a secondary falsifier. |
| **4. Sufficiency** | A model-only pack is definitely insufficient. A whole-closure semantic pack may be sufficient, but the repository does not yet contain the selected-costume and semantic-object census needed to claim that. The estimator must include native-owner bytes, pack metadata, loader/static growth, shared dependencies, and any motion-bank policy. |
| **5. Layout** | Keep `SIM_HOT`, `RENDER_HOT`, and resident `WARM_COLD`, plus a distinct `SETUP_UPLOAD_TRANSIENT` section and separate scene/UI resources. Lay out match-local dense arrays in the order their hot loops consume them, align to the platform cache-line constant, and keep large immutable native/texture data out of the ARM9 static image. |

---

# 1. Define the problem as semantic reachability, not observed byte liveness

For a legal match configuration \(C\), the required set is:

\[
L(C) = \text{all semantic values that any legal post-setup execution can reach or select}
\]

A useful mechanical formulation is:

\[
L(C) = \mu X.\left(R(C) \cup \operatorname{targets}(E_{\mathrm{semantic}}(X, C))\right)
\]

where:

- \(R(C)\) is the set of post-setup roots enabled by the roster, selected costumes, detail policy, game mode, item mode, and scene;
- \(E_{\mathrm{semantic}}\) includes ordinary pointer relocations **and** array extents, sentinel-terminated streams, computed offsets, script-selected variants, and capability rules;
- the least fixed point closes those edges until no new object is added.

The pack is then not necessarily a byte subset of the original files:

\[
P(C) = \bigcup_{o \in L(C)} \tau(o)
\]

where \(\tau\) is an explicit translation such as:

- retain a small typed table;
- copy a scalar into a dense runtime structure;
- replace a raw display-list target with a native-render root ID;
- convert an N64 material description to a compact DS material record;
- upload selected texels to VRAM and retain only a handle;
- copy a setup descriptor into per-instance state, then discard its source;
- move Results-only sprite data into a Results scene pack.

This distinction matters. Asking for the **minimal bytes of the old representation** forces the generator to preserve all the old representation's unsafe pointer behavior. Asking for a **complete semantic replacement** allows the generator to make unsupported or missing references impossible.

## 1.1 Use typed object granularity, not individual bytes

Do not attempt byte-perfect liveness inside every small control structure.

For example, `FTAttributes` is 0x348 bytes. Four complete copies are only 3,360 bytes. It is safer to preserve its scalar semantics wholesale—or generate an equivalently complete scalar facade—than to save a few hundred bytes by proving each scalar field dead.

Apply the same policy to small tables:

- retain the complete valid `FTModelPart` row domain;
- retain complete joint-to-part selector tables;
- retain complete shield scripts if they are small;
- retain complete thrown-status and skeleton selector tables;
- retain all entries when a dynamic index is not cheaply and statically bounded more tightly.

Be aggressive only on the large contributors:

- raw `Gfx` and `Vtx`;
- all-costume texel and palette banks;
- N64 rendering state already represented by a native owner;
- setup-only DObj/MObj scaffolding;
- raw animation or material streams that can be compacted;
- file padding and relocation-oriented pointer arrays.

A safe object-level over-approximation can still save hundreds of kilobytes. A byte-minimal proof is unnecessary and would make the project much riskier.

## 1.2 Define an explicit setup barrier

The generator and verifier need one named lifetime boundary, for example:

```text
FIGHTER_PACK_BARRIER:
    all distinct-kind pack blocks loaded
    all pack references validated
    all fighter-kind runtime facades bound
    all selected textures uploaded
    all per-instance setup copies complete
    all setup-only staging released/poisoned
    no fighter raw reloc file remains resident
    battle may now enter GO
```

An object is setup-only only when all of its semantic outputs have crossed this barrier into one of:

- the immutable match pack;
- per-kind runtime state;
- per-instance state;
- VRAM;
- another explicitly resident battle subsystem.

“Setup read observed once” is not enough. The barrier makes the claim testable.

---

# 2. What the current repository establishes

The current source already provides unusually good raw material for this method.

## 2.1 The relocation corpus is typed enough to build an object graph

On current `master`, `decomp/BattleShip-main/decomp/relocData.md` states that the US corpus is rebuilt from typed C and byte-compared to the original ROM. It also says the authoritative pointer chains are regenerated from compiled object-file relocation records and the cross-file symbol index.

That means the pack generator should treat these as primary inputs:

1. compiled relocData ELF symbol tables;
2. compiled relocation records such as `R_MIPS_32`;
3. exact O2R payload bytes and extern IDs;
4. typed declarations or a Clang-generated declaration manifest;
5. semantic decoders for structures whose reachability is not represented by a relocation at every element.

The checked-in `.reloc` text can remain an excellent audit report and cross-check, but it should not be the sole authority if the current decomp pipeline derives chains from compiled objects.

The existing `scripts/fighters/generate_fighter_production_manifest.py` already provides much of the file-level substrate:

- it parses FTData roots;
- scans O2R headers;
- resolves extern IDs;
- computes transitive closure and exact aligned allocation sizes;
- recovers semantic file aliases;
- records source hashes and structural counts.

Extend that script family rather than starting a disconnected generator.

## 2.2 The repository already has a consumer-manifest precedent

`scripts/fighters/generate_nds_native_owners.py` contains `SOURCE_CLOSURE_POLICIES` and emits a consumed-field manifest with explicit classifications such as:

- `immutable_generation`;
- `live_camera_dependent`;
- `live_camera_independent`;
- `callback_visible_mutation_output`.

That is the correct pattern for the new proof. Expand it from native-renderer fields to every post-setup fighter-asset consumer.

The important property is not the exact classification names. It is that a new source field read causes generation or CI to fail until it is consciously classified.

## 2.3 The raw root graph is demonstrably live after creation

The current code confirms that the pack cannot stop at creation outputs.

Examples include:

- `ftParamSetModelPartID`, `ftParamResetModelPartAll`, and `ftParamSetModelPartDetailAll` re-read model-part descriptors and common-part fallback data;
- the port's `ndsFTParamApplyModelPartCurrent` deliberately rebuilds the live DObj's display selection, MObj chain, material animation, and flags from those descriptors;
- shield logic reads `shield_anim_joints`;
- electric/color-skeleton display reads `attr->skeleton`;
- HUD and Results paths read `attr->sprites`, the stock sprite, and costume LUTs;
- Kirby Copy selects donor-specific part data after setup;
- detail switching can reapply every joint.

Therefore, “copy the DObj tree once and free the raw data” is not a valid general lifetime rule. Those later readers must either receive packed equivalents or be redirected to a new native semantic operation.

## 2.4 The existing native owner is resident data, not free data

The native owner images already leave `.rodata` and load from NitroFS, which is the correct direction. They still consume battle RAM. One existing four-owner trace records:

```text
gNdsNativeOwnerImageLoadCount = 4
gNdsNativeOwnerImageBytes     = 36,276
```

The new capacity ledger must include these bytes. Preferably the unified match pack either:

- embeds the selected low-detail native owner data once; or
- references the existing loaded owner allocation without duplicating it.

Do not count raw-model savings while omitting the replacement owner's resident cost.

## 2.5 Four-player detail policy can simplify the pack, but only if enforced

The native renderer source documents the source game's low-detail selection for three or more fighters. That creates a major opportunity:

- four-player battle pack: low-detail owner only;
- CSS/preview/Results: scene-specific high-detail resources;
- no mid-match high-detail raw or native representation.

However, the current model-part code still has a general high/low selector. The pack may omit high detail only after the four-player mode establishes a checked invariant such as:

```c
NDS_ASSERT(fp->detail_curr == nFTPartsDetailLow);
```

and the static state-transition audit proves no battle path can select high detail in that configuration. Until that invariant is landed, both detail variants are legally reachable and both belong in the closure.

---

# 3. The mechanical derivation pipeline

## 3.1 Build an exact source-object index

For every relocData file in the twelve-fighter universe, generate records like:

```text
ObjectKey:
    file_id
    symbol
    symbol_offset
    byte_size
    source_type
    element_count
    source_hash
```

Obtain:

- file ID and exact payload from O2R;
- symbol start and size from ELF;
- type and array dimensions from Clang AST, DWARF, or a generated declaration manifest;
- source provenance from the typed relocData C file.

Resolve an interior pointer as:

```text
(source object, field offset)
    -> (target object, target addend)
```

Never leave a pointer represented merely as “address somewhere in file 0x139.”

A stable initial atom ID should use source identity, for example:

```text
(file_id, symbol, subobject_index, translation_kind, variant)
```

This is enough to deduplicate the same donor object across fighters. Content hashing can be added later, but it is not necessary for the first correct implementation and becomes more complicated for cyclic graphs.

## 3.2 Extract explicit relocation edges

For each pointer relocation, record:

```text
Edge:
    source_atom
    source_field
    target_atom
    target_addend
    target_type
    nullable
    feature_predicate
```

Cross-file references are ordinary edges. They are not special cases and do not assign ownership.

This immediately handles the Kirby examples:

```text
Kirby model-part descriptor
    -> Link boomerang display object

Kirby model-part descriptor
    -> Fox unknown display object
```

Both targets get their own canonical IDs independent of which fighter's manifest discovered them.

## 3.3 Add semantic edges that relocation records cannot express

Relocation records identify pointer slots, not necessarily the entire target extent or every computed access. The schema layer must add rules for at least the following categories.

| Source form | Required extent/edge rule |
|---|---|
| `DObjDesc[]` | Explicit symbol array bound or generated joint count; parent/order validation. |
| `FTModelPartDesc` / `FTModelPart` | Explicit dimensions for joint, part ID, and detail ID. Include the full valid table domain unless a stronger static selector proof exists. |
| `MObjSub **` tables | Typed count when available; otherwise a schema-declared NULL terminator and maximum bound. |
| `AObjEvent16/32` | Parse through the legal end opcode; emit an explicit record count and reject missing/overlong terminators. |
| `Gfx` lists | Parse through `G_ENDDL`, including called/branched lists and any segment/material dispatch. In the battle pack, most of these should terminate in `NATIVE_REPLACE`, not copied raw commands. |
| Sprite / texture / palette | Derive byte extent from format, dimensions, stride, frame count, palette count, and selected costume. |
| Computed-offset tables | Add a named schema edge/range. A relocation landing near the table is not sufficient. |
| Script-selected model parts | Parse all motion/event scripts and collect the legal selector domain, or retain the whole small table. |
| Color-skeleton selectors | Parse all color-event setters and validate every selected skeleton ID. |
| Kirby Copy donor selection | Feature edge conditioned on the set of fighters that can legally appear/be acquired in the mode. |
| Setup copies | Record the destination semantic object, then allow the source to become transient only after the copy is validated. |

Current `relocData.md` explicitly describes cases where palettes are reached by computed offsets and no chain pointer lands on the palette set itself. That is a concrete reason the raw relocation closure cannot be the complete proof.

### Rule for unknown objects

Use a deliberate fallback policy:

- an unknown **small** object may be retained whole and marked `CONSERVATIVE_OPAQUE`;
- an unknown **large** object is a generator failure, not silently promoted to the whole source file.

For example, set a review threshold such as 256 or 1,024 bytes. The exact value is a policy decision; the essential property is that one untyped large range cannot quietly erase the expected savings.

## 3.4 Generate the post-setup consumer contract

The consumer side determines the graph roots.

Do not try to prove reachability through the full C call graph. Fighter status dispatch, callbacks, script opcodes, and imported source functions make that unnecessarily fragile. Instead:

1. take every translation unit linked into the battle scene;
2. scan all typed accesses rooted in fighter file-backed state;
3. classify each access by lifetime and replacement.

At minimum, the scanner should find:

- `fp->attr` and aliases assigned from it;
- `fp->data`;
- `dFTManagerDataFiles`;
- `gFTData*` file bases;
- `p_file_main`, `p_file_model`, `p_file_shieldpose`, and `p_file_special*`;
- `lbRelocGetFileData`, `lbRelocGetStatusBufferFile`, and fighter-related file IDs;
- raw `Gfx *`, `DObjDesc *`, `MObjSub *`, `AObjEvent*`, `Sprite *`, texture, and palette pointers derived from fighter files;
- casts to byte pointers and pointer arithmetic involving those roots;
- writes that copy such pointers into a persistent GObj, DObj, MObj, fighter-kind global, or per-instance structure.

A Clang AST/IR scanner is preferable to regex. The existing generated consumed-field work can supply the style and CI contract.

Each discovered consumer must have a row like:

```yaml
consumer: ftParamSetModelPartID
source_path: decomp/.../ftparam.c
access_path: FTAttributes.modelparts_container.modelparts_desc[].modelparts[][]
phase: post_setup
selector_domains:
  joint: common_part_joint_range
  modelpart: table_bound
  detail: four_player_low_only
disposition: packed_modelpart_record
runtime_accessor: ndsFighterPackApplyModelPart
```

The generator must fail when:

- a new consumer appears;
- a known consumer reads a new field;
- a field is classified both setup-only and post-setup;
- an access escapes through an unclassified cast or pointer arithmetic expression.

## 3.5 Give every source object one explicit disposition

The output should contain a **disposition ledger**, not merely a list of included ranges.

Recommended disposition kinds:

| Disposition | Meaning |
|---|---|
| `PACK_SIM` | Persistent simulation-hot semantic data. |
| `PACK_RENDER` | Persistent render-hot semantic/native data. |
| `PACK_COLD` | Persistent but infrequent battle capability data. |
| `COPY_KIND` | Copied once into a per-kind runtime facade; source can be discarded. |
| `COPY_INSTANCE` | Copied into every fighter instance during setup; source can be discarded afterward. |
| `NATIVE_REPLACE` | Raw N64 presentation data is replaced by an existing/generated native object or logical render ID. |
| `VRAM_UPLOAD` | Source bytes are loaded/uploaded before GO; only a compact handle remains in main RAM. |
| `SCENE_SPLIT` | Not a battle lifetime object; loaded by CSS, Results, or another scene. |
| `SETUP_TRANSIENT` | Needed while constructing runtime objects, then released and poisoned at the barrier. |
| `DROP_PROVEN` | No post-setup semantic consumer and no retained target. |
| `CONSERVATIVE_OPAQUE` | Retained whole under a small-object policy. |
| `UNCLASSIFIED` | Hard generator failure. |

For every source symbol or schema-defined subobject, emit:

```text
source file / symbol / offset / size
source type
incoming references
outgoing references
consumer locations
selector predicates
lifetime
disposition
packed atom ID
packed byte count and alignment
dedup identity
reason/proof rule
```

A complete pack build has zero `UNCLASSIFIED` rows.

## 3.6 Translate the unsafe pointer ABI

On disk, never store absolute C pointers. Use section-relative references or atom IDs with explicit counts.

A possible format is:

```c
typedef u16 NDSAtomID;
typedef u32 NDSPackRef;

typedef struct NDSFighterPackHeader
{
    u32 magic;
    u16 schema_version;
    u16 atom_count;
    u32 config_hash;
    u32 source_manifest_hash;
    u32 section_offset[4];
    u32 section_size[4];
    u32 fixup_offset;
    u32 fixup_count;
} NDSFighterPackHeader;
```

A reference should encode one of:

- null;
- atom ID;
- section plus validated relative offset;
- a typed logical ID such as native model-part root, material program, or VRAM texture handle.

Every variable-length array must carry or derive a validated count. Do not reproduce a raw pointer and depend on an implicit sentinel unless the binder first validates the sentinel within a declared maximum extent.

### Transitional facade

To minimize source churn, the first runtime version can build a normal `FTAttributes` facade per distinct kind:

- copy all scalar fields;
- point pointer fields only at validated pack objects;
- use compact/native replacement records behind modified seams;
- share this facade across mirrors.

That facade is cheap. What must disappear is the assumption that its pointers lead into raw reloc files.

The final form should make the distinction clearer, for example:

```c
typedef struct FTBattleKindData
{
    FTAttributeScalars scalars;
    const NDSModelPartTable *modelparts;
    const NDSShieldPoseBank *shield;
    const NDSSkeletonVariantTable *skeletons;
    const NDSBattleStockIcon *stock_icon;
    const NDSNativeFighterProgram *render_program;
} FTBattleKindData;
```

Do not copy linked status tables into this pack merely for layout while retaining the linked copies. Moving static data into a pack helps capacity only when the linked copy is removed or the representation becomes smaller.

## 3.7 Close the graph for a match configuration

Define the configuration key from actual legal capabilities:

```text
mode
stage
unique fighter kinds
selected costume set per kind
detail policy
item mode / allowed item set
possible spawned fighter kinds
Kirby-copy donor set
scene lifetime
optional high-detail policy
motion residency policy
```

Then:

```python
required = set()

for kind in config.unique_kinds:
    required |= kind_base_manifest[kind]

for kind, costumes in config.costumes_by_kind.items():
    for costume in costumes:
        required |= costume_manifest[kind, costume]

if KIRBY in config.unique_kinds:
    for donor in config.legal_copy_donors:
        required |= copy_capability_manifest[donor]

required |= mode_manifest[config.mode]
required |= item_manifest[config.item_mode]
required |= battle_common_manifest

required = semantic_closure(required, config)
required = deduplicate_by_canonical_atom_id(required)
layout = place_by_section_and_hot_order(required)
validate_all_edges(layout, config)
```

The closure and layout can be computed entirely offline for every enumerated configuration. The runtime then reads a compact ready-made manifest and performs a bounded placement/fixup pass; it does not rediscover the raw graph.

---

# 4. Cross-fighter references: use atom union, not ownership

## 4.1 Reject the four naive choices as primary representations

| Choice | Assessment |
|---|---|
| Full per-roster payload pack | Correct but duplicates large data across 495 exact-four rosters and complicates fixes. Reject as the payload representation. |
| Shared whole donor segment | Avoids ROM duplication but retains unrelated donor bytes in RAM. This recreates file-level closure and loses the main saving. |
| Copy donor fragment into the borrower | Semantically safe, but duplicates the fragment when the donor is also selected unless a later dedup layer recognizes it. |
| Keep external raw pointer | Unsafe; absent donor residency produces exactly the rare wild-pointer failure being avoided. |

## 4.2 Recommended factoring

Use:

1. a ROM-side canonical semantic atom store;
2. small base manifests per fighter kind;
3. small manifests per selected costume;
4. capability manifests for copy hats, specials, weapons, items, or mode-specific resources;
5. optional precomputed exact-roster **manifest lists**, not duplicated payloads.

The same source object has the same atom ID wherever referenced. Therefore:

```text
Kirby copy manifest for Link  ─┐
                               ├─> atom 0x1234: Link boomerang native fragment
Link base manifest             ─┘
```

The match union contains atom `0x1234` once.

The final layout can still replicate a tiny descriptor deliberately for locality. Such replication must be explicit and charged to the size ledger. Large geometry, texture, native packet, or script payloads should never be replicated implicitly.

## 4.3 Roster-conditional and unconditional dependencies must be distinguished semantically

Do not infer conditionality from the target fighter's name.

Examples:

- A Kirby Copy hat that can only be selected after copying a live Link is a roster/capability-conditioned dependency in ordinary VS mode.
- A motion table that unconditionally uses a resource authored under another fighter's namespace is a base dependency even when that other fighter is absent.
- A mode that can spawn a fighter not present on the CSS roster must include that fighter in `possible_spawned_kinds`.
- A debug command capable of forcing any Copy kind either needs all copy capabilities or must be disabled/asserted in the production pack-only build.

The generator should require every cross-owner edge to declare one of:

```text
unconditional
roster_kind_present(kind)
possible_spawn_kind(kind)
selected_costume(kind, costume)
feature_enabled(feature)
scene_is(scene)
```

A bare cross-owner edge with no predicate is unconditional.

## 4.4 Kirby's absent donor entries must be explicit invalid values

Do not leave a table slot containing a pointer to an unloaded donor.

Translate it to something like:

```c
typedef struct NDSKirbyCopyPart
{
    NDSAtomID modelpart_atom;     /* NDS_ATOM_INVALID when capability absent */
    u16 native_root_variant;
    u16 material_program;
} NDSKirbyCopyPart;
```

At load time:

```text
for every legal copy donor in this match:
    entry must be present and valid

for every illegal/absent donor:
    entry must be explicitly invalid
```

At the Copy acquisition seam, assert that the requested donor capability is present. This converts a future source-rule violation into an immediate diagnostic.

## 4.5 There are 495 payload combinations, but only 793 proof cases

For twelve fighters:

```text
C(12, 4) = 495 exactly-four-distinct-kind sets

C(12,1) + C(12,2) + C(12,3) + C(12,4)
= 12 + 66 + 220 + 495
= 793 one-through-four-kind sets
```

Enumerating 793 unions is trivial for a build tool. Storing 793 duplicated data images is unnecessary.

It is acceptable to emit 793 small reports or NitroFS manifest files if that materially simplifies loading. They do not belong in ARM9 `.rodata`.

---

# 5. Completeness argument

The acceptance case should be a set of independent proof obligations. No single dynamic test is asked to establish completeness.

## 5.1 Obligation A: every post-setup consumer crosses the new ABI

After migration, battle-linked code must not be able to obtain an unvalidated pointer into a fighter raw file.

Acceptance:

```text
unclassified_post_setup_consumer_count == 0
legacy_fighter_file_root_access_count   == 0
```

Practical enforcement:

- put legacy raw fighter-file access behind setup-only modules;
- expose generated pack types/accessors to battle code;
- use a pack-only build flag that omits raw Main/Model/Shield/Special assets;
- fail the link/map audit if forbidden raw fighter symbols appear in the battle overlay;
- fail CI when the AST consumer manifest changes unexpectedly.

This is stronger than a call-graph proof because it covers rare function-pointer callbacks too.

## 5.2 Obligation B: every source object has a disposition

For each source object reachable from a declared root, exactly one disposition must exist.

Acceptance:

```text
unclassified_atom_count                == 0
large_conservative_opaque_atom_count   == 0
source_object_disposition_conflicts    == 0
```

If a structure is copied during setup, the ledger must identify both:

- the source object;
- the validated destination object that preserves its semantics.

“Not included” without a reason is invalid.

## 5.3 Obligation C: every reference and selector is bounded

The generated schema must know:

- target type;
- target extent;
- nullability;
- alignment;
- section;
- feature predicate;
- selector dimensions.

At load time, validate:

```text
offset + minimum_target_size <= section_size
array_count <= generated_max
joint_id < joint_count
modelpart_id < modelpart_count[joint]
detail_id < detail_count
costume_id is loaded
skeleton_id < skeleton_count
copy donor capability is present
script terminator appears within declared extent
all required native roots/materials/textures resolve
```

A two-pass binder handles cycles:

1. place every atom and construct the atom-ID-to-address table;
2. resolve and validate all references.

Only after pass 2 succeeds may the battle start.

## 5.4 Obligation D: no persistent pointer points into discarded storage

Immediately before the barrier, recursively inspect all persistent roots:

- per-kind data;
- every fighter instance;
- DObjs, MObjs, AObjs;
- active weapons/effects initialized during setup;
- renderer binding state;
- HUD stock/icon state.

Classify every pointer into an approved domain:

```text
PACK_SIM
PACK_RENDER
PACK_COLD
INSTANCE_HEAP
STABLE_CODE_OR_STATIC
VRAM_HANDLE / NON-CPU ADDRESS
NULL
```

Reject pointers into:

```text
RAW_FIGHTER_FILE
PACK_STAGING
TEXTURE_UPLOAD_STAGING
FREED_SETUP_ARENA
UNKNOWN
```

Acceptance:

```text
live_pointer_outside_allowed_domains == 0
```

A generated pointer-field descriptor can drive this audit. It should include the pointer-bearing fields of `FTAttributes`, DObj, MObj, AObj, fighter instance state, and any port-side renderer caches.

## 5.5 Obligation E: the raw loader cannot silently repair an omission

After the setup barrier:

- lock the fighter reloc loader;
- count every file open/read;
- make a fighter Main/Model/Shield/Special request a hard diagnostic in verification builds;
- make a mandatory animation read a hard diagnostic if the selected policy promises a resident motion bank.

Acceptance:

```text
fighter_asset_reads_after_barrier  == 0
fighter_raw_loads_after_barrier    == 0
mandatory_page_faults              == 0
```

This prevents an incomplete pack from appearing correct merely because the old loader fetched the missing object.

## 5.6 Obligation F: dynamic witnessing falsifies the static proof

Now build the poison-and-witness harness.

Use it to answer, “Did the static model miss something?” rather than, “Did this finite script happen to prove everything?”

Recommended behavior:

1. load a reference/raw arm and a pack-only arm from the same input stream;
2. after the pack barrier, fill all staging and abandoned raw ranges with distinct invalid patterns;
3. on the emulator, install read watchpoints on those ranges where practical;
4. run a generated status/move/item/throw/copy/KO/respawn/pause/Results/rematch matrix;
5. compare deterministic gameplay state hashes and selected render/material state;
6. fail on any raw-range read or divergence.

On retail DS there is no MMU that can make every stale read trap immediately. That is why the static boundary and load-time validation are the primary proof. Emulator watchpoints are still very valuable.

---

# 6. Capacity verdict

## 6.1 The exact current budget equation

Let:

- \(F_{P1} = 72{,}148\) bytes: measured shipped free floor;
- \(R_{P1} = 173{,}088\) bytes: current Mario+Fox raw closure;
- \(D_{\mathrm{static}} = 36{,}864\) bytes: already measured arena loss from the larger four-kind binary;
- \(W\): complete four-kind resident replacement pack, including native owner data and retained pack metadata;
- \(D_{\mathrm{other}}\): all other four-player growth not present in the shipped floor;
- \(D_{\mathrm{binder}}\): net linked `.text/.rodata/.bss` growth from the runtime binder/catalog after deleting any superseded code/static data.

Then the projected free floor is:

\[
F_{\mathrm{new}}
= 72{,}148 + 173{,}088 - 36{,}864
  - W - D_{\mathrm{other}} - D_{\mathrm{binder}}
\]

\[
F_{\mathrm{new}}
= 208{,}372
  - W - D_{\mathrm{other}} - D_{\mathrm{binder}}
\]

For a 32,768-byte safety floor:

\[
W \le 175{,}604 - D_{\mathrm{other}} - D_{\mathrm{binder}}
\]

Therefore **175,604 bytes is not a target**. It is the current optimistic upper bound before any additional growth.

For comparison, if the 36,864-byte binary loss were ignored, the 32 KiB ceiling would be 212,468 bytes. That more generous number is no longer the correct planning budget for the known four-kind binary.

## 6.2 Reduction required

| Resident-pack target | Required saving from 577,424 bytes | Reduction |
|---|---:|---:|
| 212,468 B, ignoring known binary loss | 364,956 B | 63.2% |
| 175,604 B, known optimistic cap | 401,820 B | 69.6% |
| 160 KiB provisional engineering target | 413,584 B | 71.6% |
| 150 KiB provisional engineering target | 423,824 B | 73.4% |

At 175,604 bytes, the average is only 43,901 bytes per distinct kind before shared-common effects. That is why keeping most of each non-model closure unchanged cannot work.

## 6.3 Model-only proof

The four raw models total 271,040 bytes, or about 46.9% of the 577,424-byte worst-four closure.

Even a physically impossible perfect result—zero replacement bytes—leaves 306,384 bytes. That is:

- 93,916 bytes above the older, more generous 212,468-byte ceiling;
- 130,780 bytes above the current known 175,604-byte optimistic cap.

For Kirby alone:

```text
Current closure                       204,208
Raw model portion                    -120,864
Non-model closure remaining            83,344
Estimated replacement model         +  6–12 KB
Estimated resulting closure          ~89–95 KB
```

That would be a major saving, but it is still about twice the current average per-kind allowance. The other closure members must also be semantically reduced or another memory source must be recovered.

## 6.4 What must be included in \(W\)

The estimator must charge:

- complete packed per-kind semantics;
- selected-costume main-RAM remnants;
- low-detail native owner programs;
- shared donor/cross-fighter atoms;
- resident pack headers and lookup records;
- per-kind runtime facades;
- any retained material animation programs;
- any battle-resident stock icon data;
- padding/alignment in the final layout;
- deliberate hot-record replication.

It must separately report:

- transient setup/upload peak;
- binder `.text/.rodata/.bss` delta;
- raw/static data deleted by the new path;
- per-instance growth;
- four-player pool growth;
- compact motion bank, if required by the chosen post-GO I/O policy.

Do not hide any of these in “free floor.”

## 6.5 Keep the main-closure decision separate from the motion-bank decision

The immediate 404,336-byte comparison is about main closures. Existing motion files have a different source lifetime: individual motions are acquired into reusable per-instance figatree storage.

The generator should report two profiles:

### Profile A: P2 main-closure replacement

Includes:

- Main closure semantics;
- native low-detail owner;
- selected costume;
- Shield/Special resources that are members of the main closure;
- current per-instance animation scratch unchanged.

### Profile B: no fighter asset reads after GO

Adds:

- compact immutable motion/event bank for every zero-notice status;
- any special/weapon resource otherwise loaded on demand;
- all metadata required to switch status without file I/O.

If Profile B is an accepted standing law, its bytes must be in the final capacity decision. Do not let it silently enter later after Profile A was declared to fit.

## 6.6 Required census before runtime work

Build a read-only estimator that does the following for all twelve fighters:

1. indexes every typed source object in each core closure;
2. resolves explicit and schema-defined implicit edges;
3. classifies all post-setup consumers;
4. translates large raw presentation data to the existing native representation;
5. enumerates exact selected-costume texel and palette spans;
6. accounts for VRAM-uploaded versus main-RAM-retained bytes;
7. adds native owner image bytes and all pack overhead;
8. unions cross-fighter dependencies by canonical atom ID;
9. enumerates all 793 one-through-four-kind sets;
10. reports worst set, worst costume combination, worst feature profile, and exact byte ledger.

Suggested gate:

```text
unclassified atoms or consumers:
    STOP; no size verdict is valid

worst pack > 175,604 - measured_other_growth - binder_delta:
    RED; pack alone cannot meet the 32 KiB floor

worst pack in approximately 150–175 KiB:
    YELLOW; fit depends on explicit secondary recovery

worst pack <= approximately 150–160 KiB:
    GREEN enough to build the runtime proof, subject to same-build low-water verification
```

The 150–160 KiB range is provisional. The generator should replace it with an exact threshold after measuring a four-slot, pack-disabled skeleton build that isolates \(D_{\mathrm{other}}\) and \(D_{\mathrm{binder}}\).

---

# 7. What to do if the estimator is yellow or red

Use additional levers in this order.

## 7.1 Direct selected-texture upload to VRAM

For each selected costume:

```text
NitroFS compressed block
    -> small bounded decode window or direct decoder
    -> VRAM destination
    -> discard source window before GO
```

Retain in main RAM only:

- texture handle/VRAM address;
- dimensions/format;
- material binding metadata;
- animation frame table if it cannot reside elsewhere.

Do not retain an all-costume source atlas after upload.

## 7.2 Split battle from CSS, entry, and Results lifetimes

The raw `FTSprites` graph is read by several scenes, but that does not require one allocation to survive all scenes.

Examples:

- CSS portrait/name/high-detail model: CSS pack;
- battle stock icon: tiny battle UI record or pre-uploaded OAM/VRAM resource;
- Results stock sprite/emblem/high-detail model: Results pack;
- four-player battle: low-detail owner only;
- optional entry cosmetics: load before entry, retain only if reused, otherwise release before the battle barrier.

Scene transitions are legal load boundaries. This is not gameplay-time paging.

## 7.3 Enforce low-detail-only battle residency

For a four-player match, remove both raw and native high-detail fighter geometry only after the detail invariant is explicit and verified.

If this is not enough, consider a more compact four-player-only native packet or model simplification. That is preferable to retaining high-detail assets that the source policy never selects.

## 7.4 Recover battle static address space

The ARM9 image competes with the arena one-for-one.

Measure and move or remove:

- scene-exclusive menu/preview/diagnostic data;
- generated tables that can live in NitroFS;
- duplicated native/raw renderer representations;
- battle-inactive code/data through overlays;
- scratch buffers whose lifetimes can be proven non-overlapping.

Every pack-related linked table must be included in `D_binder`. A 100 KB NitroFS manifest is cheap; a 100 KB `.rodata` manifest gains nothing.

## 7.5 Compact the motion/event representation

If the no-post-GO-read profile is required and pushes the result over budget, the next semantic target is a per-kind compact motion bank shared by mirrors:

- direct evaluation;
- explicit track lengths;
- compact joint indices;
- compact event records;
- no raw file acquisition when an action begins;
- small mutable cursor/pose state per instance.

Keep this as its own ledger section so its cost and saving are visible.

## 7.6 Restrict distinct kinds only after representation and lifetime work

Reducing simultaneous distinct kinds is a product concession. It should follow:

- semantic closure replacement;
- selected-costume VRAM residency;
- scene lifetime separation;
- low-detail enforcement;
- static overlay recovery;
- compact motion representation.

Runtime gameplay paging remains the wrong lever.

---

# 8. Pack layout for capacity and the 41.9% stall result

Use four battle sections plus separate scene resources.

## 8.1 `SIM_HOT`

Contents should be limited to values consumed repeatedly by 60 Hz gameplay:

- dense scalar fighter attributes used by physics/status code;
- compact collision and hurtbox descriptors that are not already copied per instance;
- gameplay-joint parent/binding schedule;
- compact animation/event tracks needed by current gameplay state;
- throw/capture lookup records;
- immediate special/weapon gameplay attributes;
- model-part state only when it affects gameplay;
- small immutable tables indexed every combat step.

Do not duplicate linked status tables here unless the linked copy is removed and the move is a measured net improvement.

Preferred layout:

```text
kind 0 hot scalar block
kind 1 hot scalar block
kind 2 hot scalar block
kind 3 hot scalar block

then structure-of-arrays tables in actual loop order
```

A four-kind dense index is preferable to chasing twelve-entry global tables and file pointers.

## 8.2 `RENDER_HOT`

Contents:

- selected low-detail native owner program;
- native root/epoch/run records;
- joint-to-render-binding schedule;
- matrix patch offsets;
- active model-part logical IDs and flags;
- compact material state;
- texture/VRAM handles;
- dynamic color/flash patch records;
- final packet metadata.

Exclude:

- raw N64 display lists;
- raw source vertices;
- all-costume texel banks;
- source-file ownership/range-discovery data used only by the generic renderer;
- high-detail four-player data under the enforced low-detail policy.

Order data by the renderer's actual consumption sequence. Avoid interleaving simulation fields that cause the two hot loops to evict one another from the 4 KB data cache.

## 8.3 `WARM_COLD`

Still resident, but not on every-frame paths:

- shield pose scripts;
- rare model-part variants;
- Kirby Copy capability records;
- electric/color-skeleton variants;
- entry/rebirth records retained through battle;
- rare special presentation descriptors;
- infrequent item/weapon anchors;
- diagnostic names or provenance only when a verification build requests them.

A cold object can still be mandatory. “Cold” is a layout property, not a license to page it after GO.

## 8.4 `SETUP_UPLOAD_TRANSIENT`

Contents that may exist only before the barrier:

- DObjDesc topology used solely to instantiate live DObjs;
- source MObj descriptions translated into compact runtime material state;
- selected texture source/compressed bytes;
- fixup stream;
- decompression state;
- source-to-runtime mapping tables;
- verification-only provenance records.

Rules:

- never load the old whole closure to produce the pack;
- decode directly into final destinations;
- use independent compression blocks;
- stream fixups when possible;
- free/poison this section before entering GO;
- measure peak, not only final size.

## 8.5 Separate scene/UI packs

Do not place these in the battle pack unless battle consumes them:

- CSS portraits and high-detail previews;
- Results-only models, emblems, and sprites;
- scene transition art;
- non-battle announcer/menu resources.

The battle may keep a tiny stock icon record, but not the full scene sprite graph merely because both originated in the Model file.

## 8.6 Alignment and reference width

The renderer comments document 32-byte cache lines and a 4 KB data cache. Use a named platform constant and assert it rather than scattering literal `32`s through the generator.

Recommendations:

- align each hot section and frequently streamed array to one cache line;
- use 16-bit offsets inside sections proven below 64 KiB;
- use 24/32-bit offsets or atom IDs for larger/cross-section references;
- store count next to every variable-length reference;
- separate rarely read fixup/provenance metadata from hot records;
- profile hot-record order before and after final placement.

## 8.7 ROM deduplication and RAM locality are separate decisions

A canonical atom should exist once in the ROM object store.

At match load, the final RAM layout may:

- place it once and reference it from multiple kinds; or
- replicate a very small record when that removes a costly hot indirection.

Any replication must be:

- explicit in the manifest;
- bounded by a small total cap;
- charged to the exact pack size;
- prohibited for large native streams, textures, or animation banks.

---

# 9. Recommended implementation sequence

## Phase 0: census-only compiler

No runtime behavior changes.

Build:

```text
scripts/fighters/generate_fighter_pack_atoms.py
scripts/fighters/audit_fighter_pack_consumers.py
scripts/fighters/estimate_fighter_match_packs.py
```

Possible outputs:

```text
docs/p2/generated/FIGHTER_PACK_OBJECTS.generated.json
docs/p2/generated/FIGHTER_PACK_CONSUMERS.generated.json
docs/p2/generated/FIGHTER_PACK_DISPOSITIONS.generated.json
docs/p2/generated/FIGHTER_PACK_WORST_SETS.generated.json
docs/p2/generated/FIGHTER_PACK_SIZE_REPORT.generated.md
```

Required result:

- exact raw bytes by object class;
- exact translated bytes by section;
- selected-costume accounting;
- native-owner accounting;
- all cross-owner edges;
- all 793 set sizes;
- zero unclassified large objects;
- a worst-set proof.

This phase answers whether the pack is sufficient. It is not throwaway work; the same manifests become the production generator inputs.

## Phase 1: establish the ABI and binder

Implement:

- versioned pack header;
- typed atom/ref records;
- two-pass placement and fixup;
- all load-time validators;
- per-kind runtime facade;
- pointer-domain audit;
- loader lock after the barrier;
- pack/static/binder byte telemetry.

Keep the binder generic over generated records, but do not add general-purpose runtime graph discovery.

## Phase 2: Kirby proof

Use Kirby because it stresses:

- large raw model;
- copy capabilities;
- cross-fighter references;
- model-part resets;
- high/low detail;
- stock LUTs;
- material and skeleton variants.

The proof should include at least:

- selected low-detail body;
- every legal model-part selector;
- all copy donors possible in the chosen test roster;
- shield;
- electric/color skeleton;
- battle stock icon;
- KO/respawn;
- Results scene split;
- raw-range poison/watchpoint;
- reference-vs-pack state comparison.

Do not claim P2 capacity from Kirby alone.

## Phase 3: worst four before the rest of the roster

Add:

- Yoshi;
- Fox;
- Link.

Then build the exact worst-four union and compare against the generated budget.

Stop here if it is red. Do not spend time integrating the remaining eight fighters into a representation that cannot fit.

## Phase 4: all twelve and all 793 sets

Once worst-four passes:

- generate all kind/costume/capability manifests;
- enumerate all legal set unions;
- assert the generated worst set;
- run pack validation for every set in host tests;
- run representative runtime tests for every fighter;
- run the deepest dynamic matrix on the worst sets and Kirby sets.

## Phase 5: remove the compatibility escape routes

After qualification:

- remove raw fighter Main/Model/Shield/Special loads from the battle build;
- remove generic raw display-list ownership discovery for admitted fighter bodies;
- require logical model-part/native root IDs;
- make post-barrier fighter file reads a standing failure;
- preserve a reference/raw comparison build only as a development oracle.

---

# 10. Concrete acceptance laws

## 10.1 Build-time structural laws

```text
fighter_pack_unclassified_consumers            == 0
fighter_pack_unclassified_atoms                == 0
fighter_pack_large_opaque_atoms                == 0
fighter_pack_unresolved_refs                   == 0
fighter_pack_invalid_selector_domains          == 0
fighter_pack_source_hash_mismatches            == 0
fighter_pack_duplicate_atom_id_conflicts       == 0
fighter_pack_all_793_manifests_validate        == 1
fighter_pack_worst_set_bytes <= generated_pack_budget
```

## 10.2 Load-time laws

```text
pack_header_valid                              == 1
pack_schema_version_supported                  == 1
pack_config_hash_matches_match                 == 1
pack_all_section_ranges_valid                  == 1
pack_all_refs_valid                            == 1
pack_all_required_capabilities_present         == 1
pack_all_selected_costumes_present             == 1
pack_pointer_domain_audit_failures              == 0
pack_setup_transient_bytes_at_barrier           == 0
```

## 10.3 Post-barrier runtime laws

```text
fighter_raw_asset_loads_after_barrier          == 0
fighter_asset_reads_after_barrier              == 0
mandatory_motion_reads_after_barrier           == 0  # only for Profile B
generic_fighter_body_renderer_hits             == 0
live_pointer_outside_allowed_domains           == 0
general_heap_low_water                         >= 32,768
taskman_arena_alloc_fail_delta                 == 0
```

## 10.4 Differential/falsifier laws

```text
poisoned_raw_range_reads                       == 0
reference_vs_pack_gameplay_state_hash_mismatch == 0
reference_vs_pack_selector_state_mismatch      == 0
reference_vs_pack_material_state_mismatch      == 0
```

The dynamic laws cannot prove every rare path, but they can disprove the static model quickly.

---

# 11. Specific treatment of the known live structures

## 11.1 `FTAttributes`

Recommended:

- preserve all scalar semantics in a per-kind packed/scalar record;
- keep pointer fields as validated typed refs or build a facade;
- classify each pointer target separately;
- do not retain the Main file merely to preserve the 0x348-byte struct.

Likely section:

- scalars frequently read: `SIM_HOT`;
- rare audio/display scalars: `WARM_COLD` or facade;
- setup-only descriptors: `SETUP_UPLOAD_TRANSIENT` / `COPY_INSTANCE`;
- scene sprites: split by scene.

## 11.2 Damage collision descriptors

`ftManagerMakeFighter` copies descriptors into `fp->damage_colls`.

If the consumer audit finds no later direct read of `attr->damage_coll_descs`, classify:

```text
source descriptor: SETUP_TRANSIENT
destination fp->damage_colls: INSTANCE_HEAP
```

The pointer-domain audit then proves no live pointer still targets the source descriptor.

## 11.3 DObj hierarchy and common parts

Separate:

- topology needed to instantiate live DObjs;
- selectors needed later for detail/model-part changes;
- presentation payload.

Possible result:

```text
DObjDesc transforms/topology     -> SETUP_TRANSIENT
joint parent/binding schedule    -> PACK_SIM or PACK_RENDER
model-part selector table        -> PACK_COLD / small complete table
raw Gfx/Vtx                      -> NATIVE_REPLACE
MObj/material semantics          -> compact PACK_RENDER/PACK_COLD
```

Because detail/model-part changes occur later, do not discard the selector semantics merely because the initial DObjs exist.

## 11.4 Model parts

Replace this source-time record:

```c
{ dl, mobjsubs, costume_matanim, main_matanim, flags }
```

with a pack-time record such as:

```c
typedef struct NDSFighterModelPart
{
    u16 native_root_variant;
    u16 material_program;
    u16 costume_anim_program;
    u16 main_anim_program;
    u8 flags;
    u8 reserved;
} NDSFighterModelPart;
```

Then make `ndsFTParamApplyModelPartCurrent` consume this record.

The logical native root ID, not a raw `Gfx *`, should identify the selected geometry. This removes the cross-file raw pointer while preserving late model-part switching.

## 11.5 Shield animation joints

These are legal post-setup readers.

Pack:

- a validated table of joint-script IDs;
- compact AObjEvent32 programs with explicit lengths;
- all eight legal angle entries or a statically proven subset.

This belongs in resident cold/warm data, not a setup-only range.

## 11.6 Electric/color skeletons

Pack:

- logical skeleton variant IDs;
- joint mapping;
- native render roots/material changes required by each variant;
- explicit variant count.

Do not keep raw skeleton display lists merely because `attr->skeleton` currently contains pointers to them.

## 11.7 Stock icons and LUTs

Split by scene:

- battle: selected costume's stock icon pixels/palette uploaded to the appropriate graphics memory, with a tiny resident handle;
- CSS: CSS pack;
- Results: Results pack, loaded at scene transition.

Do not retain all costume LUTs in battle unless the match can change costume dynamically.

## 11.8 Kirby Copy

Pack a sparse capability table keyed by donor kind.

For each legal donor:

- model-part/native root;
- compact material data;
- any gameplay attachment or special attributes;
- any required sound/effect capability under the selected residency policy.

The table entry exists only when the mode's possible donor set includes that kind. The runtime asserts capability presence on acquisition.

---

# 12. Risks and unresolved measurements

The following are genuine unknowns and should remain explicit.

## 12.1 Selected-costume texture bytes

The existing 6–12 KB Kirby estimate did not enumerate the selected costume's exact texture/palette requirement. This is the most important missing measurement.

The estimator must distinguish:

- bytes uploaded to VRAM and discarded;
- bytes retained for animated texture frames;
- material metadata;
- duplicate palettes shared across parts/costumes;
- high versus low detail;
- copy-hat textures/materials.

No fit claim is credible without this census.

## 12.2 Other four-player growth

The 36,864-byte binary loss is known. Other deltas may include:

- two additional fighter instances;
- joints/parts;
- per-instance animation scratch;
- larger collision working sets;
- HUD state;
- effects, weapons, and audio pressure;
- pack binder code/data.

Measure a same-commit, same-feature, four-slot skeleton build to derive the actual `D_other`.

## 12.3 Native owner high/low duplication

The generated image types currently describe high and low owner variants. Four-player residency should count only low detail after the invariant is proven. Until then, count both.

## 12.4 Main-closure versus no-I/O motion profile

A whole-closure pack may pass while a complete resident motion bank does not. Report both profiles and make the project choose the standing law explicitly.

## 12.5 Rare computed references

The typed reloc corpus substantially improves the problem, but typed source does not automatically imply all semantic extents are explicit reloc edges. The schema report must list every computed-offset or sentinel rule. A new unrecognized pointer arithmetic pattern is a CI failure.

---

# Final judgment

The correct primary method is:

> **Compile a conservative, typed semantic object closure offline; make the battle runtime consume only that closure; validate it completely before GO; and make raw-file access impossible afterward.**

The correct factoring is:

> **Per-kind, per-costume, and per-capability manifests over canonical shared atoms, with build-time enumeration of all roster unions—not 495 duplicated payload packs.**

The correct completeness claim is:

> **Every post-setup consumer is inside the new ABI; every source object and edge has a disposition; every selector is bounded; every reference is validated; and no live pointer or post-GO load can escape back to discarded raw storage.**

The size judgment is:

> **A model-only pack cannot solve P2. A whole-main-closure semantic pack might, but it must reduce the worst four from 577,424 bytes to no more than `175,604 - other_growth - binder_delta`, and preferably into roughly the 150–160 KiB range. The current evidence is insufficient to say that it will. Build the census-only compiler first and stop before runtime integration if its worst-set report is red.**

That census is the next defensible P2 step. It is both the proof technique and the go/no-go measurement.

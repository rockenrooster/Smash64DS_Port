# Review: deterministic DS texture and VRAM residency

**Status:** decision recommended  
**Written:** 2026-09-04  
**Repository basis:** `rockenrooster/Smash64DS_Port`, current `master` at `ee5985acb3343f4b4567a4623ad99f8f0f415e09`  
**Scope:** the 3D texture cache, texture/palette VRAM, particle atlases, and main-engine VRAM-bank allocation. This is separate from the 4 MB main-RAM fighter pack.

## Decision

The current fixed `44 static / 79 dynamic` partition should **not survive as the residency policy**.

It should also **not be replaced by a general gameplay-time LRU**. An LRU can choose which texture loses; it cannot make an overfull simultaneous working set fit, and its miss path adds exactly the allocation, upload, conversion, and global invalidation behavior this project has been removing from gameplay.

The recommended system is:

> **A deterministic, generated scene-residency plan, committed before `GO`, with stable texture handles for the entire residency epoch and a small, explicitly bounded optional-presentation region.**

The existing fixed arrays, direct slot indexing, generated static records, and prepared-run texture handles are useful implementation machinery and should be reused. What must go is the idea that an anonymous global static/dynamic high-water partition decides admission while rendering.

The final shape is:

1. Every converted texture is described by generated metadata separating **texel storage**, **palette storage**, and **texture view/state**.
2. Every fighter, stage, item set, effect bank, HUD, and 1P encounter emits a generated **residency fragment** listing all textures it may require during that residency epoch.
3. Match or scene setup unions the selected fragments, deduplicates them, proves all independent constraints, and creates a deterministic placement plan.
4. Required textures are uploaded and all native-owner references are patched before gameplay.
5. Residency is then locked. Required content cannot allocate, upload, evict, convert, or perform NitroFS I/O after `GO`.
6. A missing required resource prevents the scene from starting and names the exact failed constraint. It never demotes the stage to the generic renderer.
7. Only content explicitly declared optional may be omitted or replaced, and that decision is local to its owner.

This gives a mechanical answer to “does this fit?” and makes the two current silent failures impossible by construction.

---

## Findings from the current source

### The `82 dynamic` versus `79 dynamic` discrepancy is historical, not an unexplained three-slot shortage

The terminal sentence in `src/nds/nds_renderer_preamble.c` is stale. The history is:

| Revision state | Total slots | Static slots | Dynamic slots | Meaning |
|---|---:|---:|---:|---|
| Before `8da5257c2528` | 114 | 32 | 82 | This is the state for which “78 measured + 4 headroom = 82 dynamic” was true. |
| `8da5257c2528` | 114 | 35 | 79 | Three live Dream Land source keys moved into the generated static corpus. The total did not grow. |
| `6178b43d052d` | 123 | 44 | 79 | Nine DeadExplode variants became preloaded static records, and the total grew by the same nine, deliberately retaining 79 dynamic entries. |
| Current `master` | 123 | 44 | 79 | The compiled result. |

So the shipped cache is not accidentally three entries short of the last measured configuration. The later changes transferred three entries from the dynamic side to the static side, then preserved the resulting 79-entry dynamic capacity.

That resolves the arithmetic question, but it does **not** validate 79 as a future P2 capacity. The conclusion is encoded in commit history and contradictory prose rather than in a generated set proof. Ness, Kirby, items, Pokémon, new stage effects, or a new texture view can invalidate it without changing any counter that the current checker pins.

The comment should be corrected immediately, but the larger fix is to generate both counts from the selected scene plan rather than maintain either number by hand.

### The current four-fighter reject is consistent with slot exhaustion, but static source inspection cannot prove the final discriminator

The source does establish this chain:

- `renderer_adapter_stage.c` publishes outer reason `6` when `ndsRendererPrepareNativeStageOwner()` returns false.
- `ndsRendererNativeStagePrepareRun()` returns false when a textured run cannot resolve its source-frame texture.
- Under the gated trace, that becomes PrepareRun reason `2`, then owner reason `300 + run_index`.
- The texture resolver already has separate reject bits and a first-failure census for free, live, pinned, current-frame, and evictable cache entries.

That makes the working theory credible, especially because this is the only identified fighter-count-sensitive branch. It still does not prove whether the particular 60 rejects are:

- software texture-view slots exhausted by pinned/current-frame entries;
- texture VRAM bytes exhausted;
- texture VRAM fragmented or blocked by format-specific placement;
- a generated-name or `glTexImage` failure after an attempted eviction;
- or another texture-resolution invariant.

One focused first-fault capture can classify that. It should record the inner reject bit, requested asset/view, requested byte footprint, free cache entries, pinned/current-frame/evictable counts, and bank allocator state. That measurement is useful diagnosis, but it is **not a prerequisite for the architectural decision**: every case above is a late-admission failure and is eliminated by the proposed pre-`GO` plan.

### “Texture capacity” is at least five independent constraints

The current discussion tends to collapse several resources into one number. They are not interchangeable:

1. **Texture storage bytes** in the texture-mapped banks.
2. **Texture view/key slots** in the software resident table.
3. **Texture palette bytes and bases** in banks F/G.
4. **Format-specific auxiliary placement**, especially DS 4x4-compressed textures, whose texel blocks and per-block index data have coupled slot-placement requirements.
5. **Atlas geometry and palette quality**, where enough total free texels does not imply that a required rectangle can be placed or that adding it preserves the accepted shared palette.

There are also secondary ceilings: generated-name count, lookup-table load, metadata RAM, alignment, maximum dimensions, and dedicated non-cache texture owners.

This is why a high-water count can pass while a picture silently changes. A correct admission report must prove every axis independently.

### The stage failure boundary is currently too large

At present, one run’s failed texture resolve causes `ndsRendererPrepareNativeStageOwner()` to fail, then the adapter clears the native stage owner’s validity. A leaf resource failure therefore invalidates the owner that carries thousands of static stage commands.

That is not graceful degradation. It is a failure-containment bug.

The static stage core, stage-local moving actors, stage particles, and unrelated fighter/effect owners need separate admission and validity domains. A stage-core texture can still be mandatory, but its failure must be detected before gameplay. Once admitted, no dynamic leaf is allowed to revoke that proof.

### The particle atlas’s current packing algorithm is not the central problem

The historical shelf packer was already replaced by first-fit-decreasing placement over an occupancy bitmap on 2026-08-14. That recovered previously stranded space. The remaining defect is not primarily “shelf versus first fit”; it is **best-effort admission**.

The packer is currently allowed to produce a different valid-sized set when the inputs change. Therefore:

- identical byte totals do not imply identical content;
- identical admitted/excluded counts do not imply identical content;
- a deterministic heuristic does not imply a stable content contract;
- and a measured-live list can only protect behavior that a completed test happened to exercise.

The atlas representation is useful. Silent displacement is not.

---

## Required invariants

The new system should publish these as code and CI invariants, not design intentions.

### Residency invariants

1. **All mandatory resources for a residency epoch are admitted before the epoch begins.**
2. **No mandatory texture allocation, deletion, eviction, conversion, or NitroFS read occurs after `GO`.**
3. **Every mandatory draw references a stable planned handle.**
4. **Every planned handle references a verified texel placement and, when applicable, a verified palette placement.**
5. **A residency plan is immutable for the lifetime of its scene or match.**
6. **The plan hash, generated asset hash, owner-table hash, and runtime-loaded plan generation agree before the first GX submission.**
7. **Anonymous headroom is not relied upon for correctness.** Any reserved bytes or slots have a named owner and purpose.
8. **Unknown texture demand is an invariant violation, not a cache miss.**

### Failure-containment invariants

1. A required-resource admission failure prevents scene entry; it does not start gameplay with missing art.
2. A texture resource failure is never a reason to route the complete stage through the generic renderer.
3. An optional owner may fail only according to a generated, explicit fallback policy.
4. No owner may begin GX submission until its required handles have passed preflight.
5. No fallback path may begin after that owner has partially submitted a frame.
6. The first nested cause is always retained. An outer code such as `6` may classify the subsystem, but may not overwrite the inner constraint, owner, run, or asset.

### Content invariants

1. Script reachability closes over texture dependencies mechanically.
2. Required atlas membership is exact by ID, not by count.
3. Optional exclusions are exact by ID and reason, not by count.
4. Adding a required asset either adds it to the admitted set or fails the build/plan. It may not silently displace another required asset.
5. Format substitution, palette sharing, downscaling, frame decimation, or other fidelity changes are explicit generated choices with their own acceptance evidence.

---

## Proposed architecture

### Overview

```text
                         BUILD/HOST
source assets
    │
    ├── DS converters ──> texel blobs + palette blobs
    │
    ├── owner scanners ─> residency fragments
    │
    └── effect scanner ─> script → texture dependency closure
                              │
                              v
                     canonical texture manifest
                              │
                    exhaustive admission checker
                              │
             placement certificates + fit reports + ROM data

                       SCENE/MATCH LOAD
exact match/encounter configuration
    │
    └── select and union residency fragments
                              │
                      deterministic scene plan
                              │
          set VRAM bank profile → upload → verify → patch owners
                              │
                         RESIDENCY LOCK
                              │
                              v
                            GAMEPLAY
owner-local handle → TEXIMAGE/PLTT bind → GX submission
(no lookup miss, allocation, eviction, conversion, or I/O)
```

### 1. Separate texture storage from texture views

The current cache key mixes several concepts that consume different resources. The manifest should give them distinct identities.

#### `TextureStorageId`

Represents bytes in texture VRAM:

- final DS format;
- width and height;
- texel payload bytes;
- compressed-format auxiliary bytes, if any;
- alignment and bank-placement constraints;
- immutable versus fixed-footprint mutable storage;
- hash of the final uploaded bytes.

Two source textures with identical final storage may share one `TextureStorageId`.

#### `PaletteStorageId`

Represents bytes in texture-palette VRAM:

- entry count and alignment;
- exact palette bytes;
- transparency/color-zero semantics;
- animation or mutability policy;
- hash of the final uploaded bytes.

Exact identical palettes share a `PaletteStorageId`. Deliberate cross-texture quantization is a separate conversion decision, not automatic deduplication.

#### `TextureViewId`

Represents how a draw interprets storage:

- `TextureStorageId`;
- `PaletteStorageId` or no palette;
- palette base;
- wrap/clamp/mirror flags;
- color-zero transparency;
- texture-coordinate generation mode;
- any renderer semantic that presently makes two cache keys distinct.

This distinction matters. Two views can share every texel byte and still require different bind state. Conversely, several fighter costumes can often share one indexed texel plane while selecting different palettes. A single “texture count” cannot model either case.

A compact runtime view can be close to:

```c
typedef struct NDSResidentTextureView
{
    u32 teximage_param;
    u16 palette_base;
    u16 flags;
} NDSResidentTextureView;
```

The exact ABI should be measured, but planned views do not need full source keys, LRU ages, pin flags, resident-key pointers, or miss-recovery state. Most metadata remains generated ROM data. As an order-of-magnitude check, 256 eight-byte views occupy 2,048 bytes; even a 12-byte view table is only 3,072 bytes before owner maps. This can be materially smaller than growing the current roughly 24 KB cache structure and gives room for more planned handles without increasing main-RAM pressure.

### 2. Generate one residency fragment per content owner

Each independently selectable content unit emits a `ResidencyFragment`. At minimum:

- battle-common renderer textures;
- HUD and common battle presentation;
- each stage core;
- each stage’s dynamic actors and particle scripts;
- each fighter kind, detail level, costume, and palette/shade family;
- each entry/KO/respawn effect family;
- each enabled item family;
- the complete enabled Poké Ball outcome set;
- each 1P encounter or bonus stage;
- title, menu, results, and other non-battle scenes.

A fragment contains:

- required `TextureViewId`s;
- required storage and palette dependencies;
- atlas cells and scripts;
- dedicated texture owners;
- fixed mutable surfaces;
- tier: mandatory or explicitly optional;
- optional fallback view/owner, if one exists;
- residency epoch;
- source provenance for diagnostics.

The fragment must cover **all states reachable during the epoch**, not just the setup pose or a measured soak. Fighter material-animation tracks, alternate model parts, entry objects, weapons, item states, stage actors, KO/results transitions, and particle spawn graphs all contribute.

This is analogous to a link closure, but over final DS graphics resources rather than main-RAM bytes.

### 3. Build the scene plan from the exact configuration

The planner’s input is the exact scene configuration:

```text
scene kind
stage
four fighter kinds
detail level
costume/shade for each live instance
team/color constraints
item mode and enabled item set
enabled Poké Ball outcomes
1P encounter/boss variant
HUD/presentation mode
VRAM bank profile
```

It then:

1. selects the relevant fragments;
2. expands all dependencies;
3. canonicalizes and deduplicates storage, palettes, and views separately;
4. places mandatory atlas cells;
5. places mandatory texture storage under real bank and format constraints;
6. places mandatory palettes;
7. assigns stable runtime handles;
8. optionally admits declared optional groups in deterministic priority order;
9. emits an exact placement certificate and a plan hash.

A selected match should therefore be known to fit **before its first asset upload**, not merely before a particular frame.

### 4. Use residency epochs, not gameplay-time paging

The natural epochs are:

- title/menu shell;
- character/stage select;
- one VS match;
- one 1P encounter or bonus stage;
- results;
- other explicit loading-screen transitions.

Only one stage needs to be resident for a VS match. Master Hand, Metal Mario, Giant DK, Fighting Polygon Team, and bonus stages do not need to be unioned into one campaign-wide plan; each encounter has a loading boundary.

Within a live match, however, a texture that can become necessary on the next source update belongs to that match’s closure. If “items on” promises the complete source item and Poké Ball tables, the planner must admit that complete set; silently narrowing the random table is a gameplay change, not residency management. The closure includes:

- every action/material state of the selected fighters;
- every enabled item that may spawn;
- every enabled Poké Ball result;
- every common combat/KO/respawn effect;
- every stage-local actor/effect that may activate.

Mutual exclusion does not save VRAM unless there is a real, accepted transition at which storage may be replaced. Merely observing that two effects rarely overlap is not a residency proof.

### 5. Replace the cache policy with a planned resident table

The runtime table should have two logical regions:

```text
[ mandatory scene-plan views ][ explicitly optional/preloaded page views ]
```

A temporary compatibility lane may remain while content is migrated, but shipping P2 core content must not depend on it.

The important properties are:

- slot assignment happens once at load;
- handles do not move during the epoch;
- no runtime free list is needed;
- no runtime resident-key pointer is needed;
- no LRU age is needed for planned views;
- owner programs store or index their stable handle directly;
- the current hash lookup remains only for unconverted/debug compatibility content and is absent from accepted core paths.

Thus the good part of the current design—fixed arrays and cheap direct addressing—survives. The hard-coded global split does not.

Generated static records can still receive the fastest path. “Static” becomes a property of a storage record, not a compile-time claim to the first 44 slots. The selected scene plan decides which records are instantiated and where.

### 6. Commit and lock residency before `GO`

The loader should perform this sequence:

1. verify the plan and asset hashes;
2. install the scene’s VRAM-bank profile;
3. create or reserve texture names in generated order;
4. upload each texel blob;
5. upload and assign each palette;
6. verify the actual texture pointer, palette pointer/base, footprint, and non-overlap against the placement certificate;
7. patch stage/fighter/item/effect owner tables with stable handles;
8. prepare native owners;
9. publish one scene-plan generation;
10. set `NDS_TEXTURE_RESIDENCY_LOCKED`;
11. permit `GO`.

Every mutating texture seam should check the lock:

- `glGenTextures`;
- `glDeleteTextures`;
- `glTexImage2D` or wrappers;
- cache release/evict/discard;
- N64-to-DS conversion;
- texture/palette NitroFS reads;
- atlas creation.

In diagnostics, a post-lock mutation should stop at the first call and publish a structured fault. In shipping code, it should never be reachable for mandatory content. Fixed mutable textures may update bytes only inside a predeclared allocation and only through their declared update path; their footprint and handle remain unchanged.

---

## Mechanical admission and the generated fit report

### Prove all resource axes

For a plan \(P\), the checker must separately prove:

```text
required view count       <= resident view capacity
required texel placement  fits the selected texture-bank topology
compressed auxiliary data fits its required coupled bank regions
required palette placement fits banks F/G
required atlas rectangles fit their sheets
all required scripts have packed bytecode and texture dependencies
all dedicated owners have names, storage, palettes, and handles
runtime metadata fits its main-RAM budget
```

The texture-bank proof must use exact generated footprints and alignment. It must not treat A/B—or a future A/B/C/D mapping—as one anonymous byte sum. DS 4x4-compressed textures in particular have coupled texel and per-block index storage in specific texture slots. A report that says only “190 KB of 256 KB” is not a placement certificate.

### Required placement must not rely on a heuristic

First-fit-decreasing is appropriate as a fast candidate generator, but it can produce a false “does not fit” result for rectangles even when another placement exists. Since packing is offline or load-time-only, the required set should use an exact or bounded-complete solver:

- bitset-backed branch and bound for the power-of-two atlas cells;
- CP-SAT/ILP during the host build; or
- another complete search whose emitted placement is independently checked.

The output is a simple certificate: sheet/bank, offset, dimensions, format, and dependency hashes. A small checker—not the solver—becomes the trusted gate.

Optional content may use a deterministic heuristic after all required placements are fixed. It may never move or replace a required member.

### Make the legal configuration domain explicit

The admission checker needs a generated definition of what P2 promises:

```text
VS:
  any legal four-player fighter selection from all 12
  every legal costume/team assignment
  each completed VS stage
  items off
  items on with the complete enabled item/Pokémon table

1P:
  every enumerated encounter and bonus stage
  the exact fighter/boss/variant set for that encounter

Other:
  character select, title, results, and any scene with its own bank profile
```

Twelve ordered fighter slots are only \(12^4 = 20,736\) kind selections before deduplication. Multiplying by stages is still practical on a host, and many configurations collapse to the same unordered resource union. Costume families can likewise be memoized by actual storage/palette identity rather than by raw player labels.

The DS loader should run the same deterministic union/placement rules for the selected configuration. CI should exhaustively run the legal domain, cache equivalent unions, and publish the worst witness for every constraint.

### Example report shape

```json
{
  "plan_id": "vs/dreamland/kirby-ness-samus-link/items-on",
  "plan_hash": "…",
  "vram_profile": "battle_384k",
  "result": "FAIL",
  "constraints": {
    "views": {
      "used": 127,
      "capacity": 123,
      "headroom": -4
    },
    "texture_banks": {
      "used_bytes": 347136,
      "capacity_bytes": 393216,
      "largest_legal_hole_bytes": 8192
    },
    "palette": {
      "used_bytes": 29184,
      "capacity_bytes": 32768
    },
    "atlas": {
      "required_cells": 41,
      "placed_cells": 40,
      "unplaced": [30],
      "minimum_additional_bytes": 1024
    }
  },
  "largest_contributors": [
    "fighter/kirby/low/costume-2",
    "items/pokemon/all",
    "stage/dreamland/actors"
  ],
  "first_unsatisfied_constraint": "resident texture views"
}
```

The values above are illustrative; the real generator supplies them. The important behavior is that failure names a configuration, a resource axis, a witness set, and the smallest known shortage. No stress run is required to discover what changed.

### Replace anonymous headroom with named reservations

“Measured 78 plus 4” is a useful temporary engineering guard, but it is not a content contract. In the final system, the closure is exact. Any spare capacity should be one of:

- uncommitted headroom reported as informational only;
- an explicit optional page;
- a fixed mutable surface;
- a diagnostics reservation;
- or a named future-content reservation.

Correctness must not depend on four unnamed slots catching an unenumerated texture. An unenumerated texture is a manifest bug and must fail as such.

---

## Local degradation and stage containment

### Split the native stage by ownership, not by fallback implementation

The stage should have at least these independent validity domains:

1. **Stage core:** immutable map geometry, core materials, and core textures.
2. **Stage actors:** Whispy, flowers, moving platforms, or equivalent live stage objects.
3. **Stage effects:** stage-local particles and presentation.
4. **Other owners:** fighters, items, Pokémon, common effects, HUD.

The stage core is mandatory and fully resolved during scene admission. Its prepared run table references stable handles. No fighter, item, particle, or stage-actor allocation can invalidate it.

A stage actor may also be mandatory, but it has its own preflight and failure identity. If a diagnostic fault is injected into that actor, only that actor is withheld; the core packet remains valid. In a correct shipping plan, both were admitted and neither fails.

### Do not use generic rendering as a resource-pressure fallback

The generic renderer is an implementation fallback for unsupported rendering semantics. It is not an acceptable memory-management strategy.

Resource pressure must produce one of two outcomes:

- **Before gameplay:** the required plan is rejected with an exact admission error.
- **During gameplay, optional owner only:** the owner is omitted or uses its declared resident fallback.

There is no valid branch in which a texture allocation miss causes thousands of already-supported static stage commands to be reinterpreted every frame.

### Preflight owners before the first GX write

At frame start, construct the active owner list. Each owner has a generated required-handle list or compact bitset. Preflight verifies:

- the scene-plan generation;
- each handle’s validity;
- any mutable-resource generation owned by that owner;
- the owner’s declared optional admission state.

Only then are owners submitted.

This is not a per-frame texture allocator. All resources are already resident. With 128 planned views, an owner mask is four 32-bit words; with 256 views it is eight. Even ten active owners therefore require at most 40 or 80 wordwise OR/tests before any optional diagnostic detail. The check can be reduced or compiled to assertions after the plan path is proven.

The critical rule is that a failed owner is identified **before** it writes GX state.

### Failure policy by class

| Resource class | Admission rule | Runtime response |
|---|---|---|
| Stage core, fighters, enabled items/Pokémon, HUD, required gameplay/effects | Must fit | Scene does not start if missing. No generic fallback. |
| Required stage actor/presentation | Must fit, but separate owner | Scene does not start if missing; injected runtime fault cannot invalidate stage core. |
| Explicit optional cosmetic/presentation | Admit after all mandatory content | Use named resident fallback or omit that owner only; increment a persistent fault counter. |
| Diagnostic/lab content | Dedicated reservation or no admission | Never competes with shipping core. |
| Undeclared request after lock | Invariant violation | Latch structured first fault; do not allocate or globally fence the renderer. |

“Local degradation” therefore does not mean silently weakening required P2 fidelity. It means the failure boundary is local, and only an asset already declared optional is allowed to degrade.

### Preserve the nested cause permanently

Replace the single reason word with a structured first-fault record similar to:

```c
typedef struct NDSTextureResidencyFault
{
    u32 plan_hash_lo;
    u16 owner_class;
    u16 owner_id;
    u16 run_or_cell;
    u16 constraint;
    u32 texture_view_id;
    u32 requested_bytes;
    u32 used;
    u32 capacity;
    u32 inner_reason_mask;
    u32 outer_reason;
} NDSTextureResidencyFault;
```

Outer reason `6` can remain as “native stage owner failed,” but the first inner cause must always be published, independent of `NDS_TASK36_REJECT_TRACE`.

---

## Knowing demand before a frame

### The exact future frame is not generally knowable at match load

Gameplay state, CPU choices, item spawns, particle scripts, and material animation determine which subset is drawn on a particular future frame. Trying to predict that subset recreates paging policy and test-coverage risk.

### The complete reachable epoch closure is knowable

The stronger and safer answer is to enumerate every texture that may be requested during the match or encounter. This is mechanically derivable from:

- generated native owner roots/runs;
- all material-animation tracks and variants;
- fighter costumes and shades selected for the match;
- effect constructor to script IDs;
- particle script spawn graphs;
- script to texture IDs;
- stage actor programs;
- item and Pokémon outcome tables;
- entry, KO, respawn, sudden-death, and results transitions that share the epoch;
- dedicated renderer owners.

Once this closure is resident, the per-frame working set no longer controls correctness.

### A per-frame set is still useful as a proof, not as an allocator

For diagnostics, each generated owner can publish a bitset of handles used by its currently selected root/material state. ORing those sets before GX submission gives:

- a cheap “all requested handles belong to the plan” assertion;
- an exact owner witness if an unplanned state appears;
- a way to decide whether an already-preloaded optional owner is admitted.

It must not trigger upload or eviction.

---

## Particle atlas decision

### Keep atlases; remove best-effort membership

A small number of atlas texture names is advantageous on this hardware and directly reduces view-slot pressure and bind traffic. The four separate 8,192-byte allocations also reflect an already measured allocation shape.

The representation should remain, but its admission contract should become:

1. derive the exact required script closure for the selected residency epoch;
2. derive every required texture/frame dependency;
3. place all required cells or fail;
4. verify the emitted placement certificate;
5. only then consider optional cells;
6. emit exact admitted and excluded sets, with reasons and hashes.

There is no eviction operation in this model.

### Use scene-specific atlas variants

Do not grow one global atlas toward the union of all P2 content. Generate variants such as:

- common battle effects plus the selected stage’s effects;
- items-on common effects plus the enabled item/Pokémon set;
- a specific 1P encounter’s common and boss effects;
- menu/results variants;
- optional cosmetic variants.

ROM size is cheap, and loading boundaries already exist. A stable `AtlasVariantId` selects the four-sheet payload and its texture-ID-to-cell table before `GO`.

The runtime script lookup remains by source texture ID, but resolves through the selected variant’s generated table. UVs and sheet handles are immutable for that epoch.

### Make the current ImpactShock case a build error

For the reported atlas:

- capacity: 32,768 texels/bytes in A3I5;
- used: 31,872;
- free: 896;
- one 32×32 cell: 1,024;
- immediate arithmetic shortage: 128.

A required Yoshi’s Island cell may not replace required texture 30 merely because both make the totals read the same. The solver has only acceptable outcomes:

- find another valid placement;
- use a different pre-approved representation, such as a stage-local sheet or dedicated pre-resident texture, if the complete scene plan still fits;
- select a larger atlas allocation that the complete plan proves;
- or fail and report the 128-byte/cell witness.

A dedicated 32×32 texture is not automatically better: it trades atlas geometry pressure for one more resident view/name and its own palette state. The planner can compare those axes explicitly.

### The three Stock effects must enter through the same dependency gate

If `efManagerStockSnapMakeEffect`, `efManagerStockStealStartMakeEffect`, and `efManagerStockStealEndMakeEffect` are restored, scripts `0x26`, `0x75`, and `0x76` become required roots. Their complete script and texture closure must be marked required before the public forwarders are enabled.

If that closure does not fit, the build must fail. Wiring a forwarder while its required scripts remain absent would merely change a known inert stub into another silent visual failure.

### Required atlas checks

The checker should pin:

- selected atlas variant and configuration hash;
- exact required script IDs;
- exact packed script IDs;
- exact required texture IDs;
- exact admitted texture IDs;
- exact optional/excluded texture IDs and reasons;
- every texture’s sheet, rectangle, frame mapping, and palette;
- hashes of final texel and palette bytes;
- no rectangle overlap;
- no out-of-range UV;
- no required texture whose lookup returns a sentinel;
- accepted image-error bounds for any lossy format or shared-palette choice.

Counts remain useful report fields, but no count is an acceptance condition by itself.

---

## VRAM-bank ceiling

### Banks C and D are currently two general 16-bit compositor surfaces

The current setup is not simply “one full-screen stage background consuming 256 KB.” `nds_platform.c` creates:

- BG2: one 256×256 16-bit bitmap at base 0;
- BG3: one 256×256 16-bit bitmap at base 8.

Each surface is 128 KB, so each consumes one full bank. The code also uses these as general scene-owned background/foreground overlay layers, with direct pixel writes, affine transforms, title-fire use, and scene transitions.

Therefore, reclaiming C/D is a compositor and scene-bank-ownership change, not a free remap of one static picture.

### Recommended bank strategy: scene-specific VRAM profiles

VRAM ownership should be part of the scene plan. For example:

| Profile | A | B | C | D | Intended use |
|---|---|---|---|---|---|
| Existing battle | texture | texture | main BG | main BG | 256 KB texture |
| Candidate battle | texture | texture | main BG | texture | 384 KB texture |
| Aggressive battle | texture | texture | texture | texture | 512 KB texture; both overlay layers replaced |
| Menu/title | scene-specific | scene-specific | main BG | main BG | Preserve compositor-heavy scenes |

Bank roles change only at scene boundaries after both engines and DMA users are quiescent. The loader then creates the residency plan against that profile’s real bank topology.

### Reclaiming D is the first worthwhile capacity experiment

A single additional texture bank raises battle texture capacity from 256 KB to 384 KB: a 128 KB or 50% increase. That is large enough to be worth a focused prototype.

The most plausible route is to fit the battle’s required 2D presentation into C:

- two 256×256 8-bit bitmap surfaces are 64 KB each by byte arithmetic;
- or use generated tiled/affine 4bpp/8bpp assets where appropriate;
- or retain one bitmap layer and move sparse foreground duties to a separately proven mechanism.

However, the arithmetic is not the proof. Two 8-bit layers may have palette-sharing constraints, different transparency behavior, and conversion costs. Current arbitrary 16-bit pixel writes cannot simply continue into an indexed surface without a palette/index conversion path, and a live conversion pass after `GO` would spend the performance that the bank was meant to save.

The candidate is acceptable only if static imagery is generated directly in its final form and every remaining dynamic layer operation has an exact, bounded implementation.

### Reclaiming both C and D is not the first move

Mapping A/B/C/D as texture memory doubles the nominal capacity to 512 KB, but it requires replacing both main 2D overlay layers across every affected scene.

A textured 3D background quad is possible in principle, but it:

- consumes part of the newly reclaimed texture memory;
- consumes 3D fill and polygon state;
- changes ordering and blending relative to translucent stage/fighter geometry;
- does not by itself replace the foreground compositor;
- and adds another required texture owner to the same residency plan.

It may still be a useful stage-specific representation, especially at 4bpp/8bpp or compressed format, but it must win an end-to-end visual and timing proof. It is not an architectural substitute for admission control.

### Capacity should follow the plan, not precede it

The order should be:

1. implement the manifest and admission report at the current 256 KB;
2. obtain exact failing configurations and constraints;
3. apply format/storage/view deduplication;
4. prototype the 384 KB battle profile if needed;
5. consider the 512 KB profile only if the report still proves it necessary.

Adding banks first would move the failure threshold but leave both silent-failure mechanisms intact. The residency redesign is required even if all four banks eventually become textures.

---

## Format, compression, and palette recovery

### Raw DS storage choices

Ignoring alignment and palette overhead for the moment:

| DS representation | Raw payload | Saving versus 16bpp direct | Important limitation |
|---|---:|---:|---|
| Direct color | 16 bpp | 0% | Largest; one-bit alpha. |
| A3I5 / A5I3 | 8 bpp | 50% | 32 or 8 indexed colors with per-texel alpha. |
| 256-color palette | 8 bpp | 50% | Palette storage; color-zero transparency only. |
| 16-color palette | 4 bpp | 75% | Palette storage; 16 colors. |
| 4-color palette | 2 bpp | 87.5% | Palette storage; 4 colors. |
| DS 4×4 compressed | 3 bpp total for texel blocks plus per-block auxiliary index data, before palette overhead | 81.25% raw saving | Lossy/block constraints and coupled texture-slot placement. |

The compressed figure comes from 32 bits of indices plus 16 bits of auxiliary palette/mode data per 4×4 block: 48 bits for 16 texels. The plan must use exact generated bytes and hardware placement rather than multiplying every image by a nominal bpp.

### Highest-value rules

1. **Preserve source indexed textures as indexed textures.** Expanding a source CI4 image to direct color multiplies its texel footprint by four.
2. **Separate texel planes from costume palettes.** One fighter index texture can serve several costumes by changing `PLTT_BASE`, provided the renderer treats palette state as a view rather than baking a new texel allocation.
3. **Deduplicate exact palette bytes globally within the scene.**
4. **Deduplicate exact final texel bytes globally within the scene.**
5. **Use A3I5/A5I3 for effects that truly need graded alpha.** The particle atlas already exploits this.
6. **Consider 4×4 compression first for larger opaque or one-bit-alpha surfaces**, not tiny UI/effect cells or art whose block artifacts violate the visual oracle.
7. **Use direct color only where the accepted error threshold rules out the smaller candidates.**

The repository has already demonstrated the scale of the first rule: retaining a set of static CI4 sources as PAL16 instead of storing them at two bytes per texel recovered 74,496 bytes. That is evidence that format discipline is a first-order lever.

### Shared palettes across fighters

There are two very different operations:

- **Exact sharing:** two textures use identical palette bytes. This is safe and should be automatic.
- **Joint quantization:** several textures are recolored into one newly optimized shared palette. This is lossy and must be an explicit generated candidate with screenshot/pixel-error acceptance.

Shared palettes alone often save tens or hundreds of bytes, not tens of kilobytes. Their larger value is enabling one indexed texel allocation to serve several costumes or variants. The manifest’s storage/view split is what exposes that saving.

Palette animation, palette base, color-zero transparency, and source alpha semantics remain part of the view. Similar-looking palettes are not interchangeable.

### Format choice cannot solve slot pressure by itself

Converting a texture from 16bpp to 4bpp may cut its VRAM footprint by 75% while consuming the same number of software views. Failure one can therefore remain with most texture VRAM free.

The format report must show both:

```text
storage-byte delta
palette-byte delta
view/slot delta
```

The architecture, not just the converter, is what allows storage aliases and palette-only costume variants to reduce slot pressure.

### Make format selection mechanical

For each source texture, the generator should emit allowed candidates:

```text
candidate format
exact texel bytes
exact auxiliary bytes
exact palette bytes
required bank topology
maximum and mean image error
alpha error
accepted/rejected reason
storage/view sharing opportunities
```

The planner may choose only candidates already accepted for that asset class. It may not solve a budget by silently downscaling, decimating animation frames, reducing alpha, or changing a shared palette.

---

## Runtime cost on ARM946E-S

Precise cycle counts require the project’s normal target measurement; they should not be invented from source. The relative code shapes are clear.

| Structure | Steady-frame work | Miss/transition work | Judgment |
|---|---|---|---|
| Current fixed static/dynamic cache | Static direct path is cheap; dynamic path builds/compares keys, probes lookup state, and stamps use age. | May scan/release entries, create names, convert/upload, and invalidate prepared proofs. | Acceptable compatibility machinery; unsafe admission policy. |
| General LRU | Similar or greater hit metadata. | Scan up to the resident-table size, delete/allocate/upload, perturb generations, and potentially thrash every frame. Cannot solve a simultaneous working set larger than capacity. | Reject for battle core. |
| Hard per-owner runtime partitions | O(1) local lookup. | Avoids cross-owner eviction but strands unused capacity and still fails late inside an owner. | Do not use as the global policy. |
| Load-time global plan with owner guarantees | Stable handle load and normal bind-state comparison only. | O(number of selected resources) once during loading. | Recommended. |
| Per-frame full pre-resolve | Walk every unique frame texture and repeat key resolution before drawing. | Still cannot make missing bytes appear without upload/eviction. | Reject as primary policy. |
| Per-frame owner bitset preflight | A bounded number of word ORs/tests plus one plan-generation check; no allocation. | None. | Useful debug/fault-containment proof; optional on proven hot paths. |

The planned path should be cheaper than the current dynamic resolver:

```text
generated run/owner index
    → resident view table load
    → compare/bind TEXIMAGE_PARAM
    → compare/bind PLTT_BASE
```

No source key reconstruction, hash probe, cache age write, generation recovery, or allocation retry is required.

A rigid compile-time slot quota per owner is unnecessary. The load-time solver gives each selected mandatory owner all of its required views, then packs the union globally so shared textures and unused capacity are not stranded. Owner ranges may still be used in the generated plan for diagnostics and locality.

---

## Migration plan

### Phase 0: contain the two current silent failures

1. Correct the cache-sizing history: document that 82 became 79 when three keys moved static, and generate/assert the arithmetic from one source of truth.
2. Publish the complete nested first texture fault unconditionally. Keep outer reason `6`, but include inner reason, run, requested view, requested bytes, and the first-failure cache/bank census.
3. Stop treating texture-resource failure as permission to invalidate the complete stage native owner. Preserve an independently prepared stage-core validity proof.
4. Make the particle checker compare exact required/admitted/excluded sets for every configuration variant, not counts.
5. Add permanent counters/gates for post-`GO` texture creation, upload, deletion, eviction, conversion, and NitroFS reads.

These changes make today’s failures loud even before the full planner lands.

### Phase 1: generate the canonical manifest

Extend the existing texture, native-owner, and particle generators to emit:

- `TextureStorageId`;
- `PaletteStorageId`;
- `TextureViewId`;
- exact footprints and bank constraints;
- source provenance;
- owner residency fragments;
- required/optional tier and fallback policy;
- hashes.

Generate a complete report for the current Mario/Fox/Dream Land configuration first. It must reproduce the existing admitted content exactly before it becomes an admission authority.

### Phase 2: build the host admission checker

Add a tool such as:

```text
scripts/generate_nds_texture_residency.py
scripts/check-nds-texture-residency.ps1
docs/optimization/NDS_TEXTURE_RESIDENCY.generated.json
```

It should:

- enumerate the declared legal configuration domain;
- memoize equivalent resource unions;
- solve all mandatory atlas/bank placements;
- report worst-case slot, texel, compressed-auxiliary, palette, and atlas witnesses;
- fail on any unsupported promised configuration;
- emit simple placement certificates.

No new fighter, item, Pokémon, effect script, or stage texture is admitted to P2 without changing this report intentionally.

### Phase 3: introduce stable pre-`GO` handles

At scene load:

- execute the selected plan;
- upload in generated order;
- verify placement;
- patch prepared stage and fighter runs to direct handles;
- lock residency.

Initially, the current cache can hold these planned entries. Remove its miss path from accepted native owners one owner class at a time:

1. stage core;
2. fighters;
3. common effects/atlas;
4. stage actors;
5. items/Pokémon;
6. 1P encounter owners.

The dynamic compatibility resolver remains only as a diagnostic indicator of unfinished migration.

### Phase 4: split owner validity domains

Refactor stage submission so core, actors, and effects have separate preflight/commit units. Perform the same separation for dedicated fighter overlays and item/effect owners.

Fault-injection tests must prove that invalidating one optional leaf does not change stage-core native execution or another owner’s handle table.

### Phase 5: generate atlas variants

Replace the single best-effort shared membership decision with exact per-epoch variants. Preserve the measured four-by-8-KB allocation shape unless the complete scene plan and runtime proof approve another shape.

Bring scripts `0x26`, `0x75`, and `0x76` into the required closure before restoring the public Stock-effect forwarders.

### Phase 6: evaluate scene-specific VRAM-bank profiles

With exact reports in hand:

1. test the existing 256 KB profile;
2. prototype a C-only battle compositor and D-as-texture profile;
3. verify all overlay users and visual semantics;
4. compare the resulting 384 KB plan matrix;
5. pursue the full 512 KB profile only if a concrete remaining witness requires it.

---

## Acceptance gates

The residency work is complete only when all of the following are automated.

### Build/host gates

- Every promised VS and 1P configuration has a generated plan with `PASS`.
- Required texture, palette, script, and atlas dependency closure is complete.
- Required atlas placement is exact and independently verified.
- Required texture-bank placement is exact and independently verified.
- No required asset is present in an excluded or sentinel set.
- Every optional exclusion has an ID, owner, and reason.
- Changing a required set changes the plan hash.
- The checked-in report matches generated assets and owner tables.
- Worst-case witnesses and headroom are published per independent constraint.

### Load-time gates

- Selected plan hash matches ROM metadata.
- VRAM bank profile matches the plan.
- Every upload lands at a verified legal location.
- Every palette base is legal and non-overlapping.
- Every owner handle is patched before native preparation completes.
- Required admission completes before `GO`.
- A deliberately reduced capacity causes a named load failure, not partial scene entry.

### Gameplay gates

- Zero mandatory texture-cache misses.
- Zero post-`GO` `glGenTextures`, `glTexImage2D`, and `glDeleteTextures`.
- Zero post-`GO` texture conversion and texture/palette NitroFS reads.
- Zero cache eviction/release/discard for planned entries.
- Zero undeclared `TextureViewId` requests.
- Zero stage-core fallback caused by resource pressure.
- Plan generation remains constant for the residency epoch.
- Existing semantic, visual, and performance gates remain green.

### Fault-containment gates

- Remove one mandatory handle: scene admission fails before first GX submission and names the asset.
- Remove one optional handle: only that owner uses its declared fallback or is omitted.
- Corrupt one atlas placement: certificate verification fails before upload.
- Corrupt one plan hash: scene entry fails.
- Force one dynamic stage actor invalid after setup: stage core remains native and unchanged.
- Add a same-sized atlas cell that would displace another required cell: build fails despite identical totals and counts.
- Request an undeclared texture after the lock: the first structured fault is retained and no allocation is attempted.

---

## Direct answers to the six sub-questions

### 1. Structure

Do **not** replace the current cache with a general LRU.

Replace the admission policy with a deterministic per-scene/per-match plan. Retain fixed arrays, direct mapping, generated static records, and stable prepared-run handles. Remove the hard-coded `44/79` policy and the planned path’s dependence on the 128-entry hash lookup.

Use global load-time packing with owner-specific guarantees, not rigid runtime owner quotas. Reserve a small separate region only for explicitly optional, already-preloaded presentation pages or diagnostics.

Steady per-frame cost becomes direct handle binding; plan construction and allocation cost occur only during loading.

### 2. Local degradation

Split the static stage core from stage actors and effects. Resolve and lock the stage core before gameplay. A dynamic leaf may never clear the stage-core proof.

Preflight each owner before its first GX write. Required-owner failure blocks scene admission. Explicitly optional-owner failure omits or substitutes that owner only. Generic rendering is not a memory-pressure fallback.

### 3. Knowing demand before the frame

The exact subset used on every future frame is not generally knowable, but the complete reachable texture closure for the selected match or 1P encounter is knowable and is the correct admission unit.

Compute it at load from generated owner fragments, material tracks, script closures, selected costumes, stage, item/Pokémon policy, and encounter. Once that set is resident, frame demand cannot oversubscribe residency.

A frame bitset remains useful as a low-cost assertion, not as a paging trigger.

### 4. The atlas

Keep atlases. Remove silent best-effort membership.

Required scripts and textures are placed first and must all fit. The packer emits an exact placement certificate. Optional cells are considered only afterward under named deterministic priority. Generate scene/encounter atlas variants rather than one P2-global union.

An addition that would remove ImpactShock must fail before producing a ROM, even when byte totals, frame totals, and counts are unchanged.

### 5. The ceiling

The 256 KB split is not necessarily the final best split, but reclaiming banks does not remove the need for deterministic admission.

The most promising next capacity lever is a **battle-specific 384 KB profile** using D as texture and fitting required 2D composition into C. It buys 128 KB while preserving one main BG bank. It requires exact proof because the current C/D users are two general 16-bit compositor surfaces, not one static image.

A 512 KB A/B/C/D texture profile is a later, higher-risk option. A 3D background quad spends some reclaimed texture and changes blending/order, so it is not the first substitute.

### 6. Compression and format

Format choice can recover a great deal of byte capacity:

- 16bpp to 8bpp: about 50% raw texel saving;
- 16bpp to PAL16: about 75%;
- 16bpp to PAL4: about 87.5%;
- DS 4×4 compressed: about 81.25% raw texel/auxiliary saving before palette/alignment effects.

The actual recoverable amount cannot be answered safely until the manifest evaluates each final DS texture. Source indexed textures, exact texel deduplication, and palette-only costume variants should be the first targets.

Format changes alone do not reduce the number of texture views, so they cannot solve software-slot exhaustion without the storage/view redesign.

---

## Final judgment

The fixed-partition cache was a reasonable local optimization for a measured P1/P2 snapshot. It is not a scalable content-admission system.

Its central sizing rule—measure a stress high-water and add anonymous headroom—cannot answer whether all promised P2 configurations fit, cannot distinguish storage bytes from views and palettes, and cannot prevent a same-sized content substitution from changing the picture. A larger cache or more texture VRAM would postpone the next silent failure without changing those properties.

The correct replacement is **not more dynamic caching**. It is less dynamism:

- generated final DS assets;
- generated owner closures;
- exact per-scene admission;
- deterministic placements;
- stable direct handles;
- immutable pre-`GO` residency;
- owner-local failure boundaries;
- and build/load failures for required content that does not fit.

Accordingly:

> **Retire the current fixed static/dynamic partition as policy. Preserve its fixed-storage and direct-indexing ideas inside a generated scene-residency plan. Do not add a gameplay-time LRU.**

Ness, Kirby, items, Pokémon, and the remaining stages should enter only through this admission path. The first complete plan matrix will then answer whether 256 KB is sufficient, whether the 384 KB profile closes the remaining witnesses, or whether specific assets still require format or representation work—without learning the answer from a missing effect or a four-million-tick stage frame.

---

## Evidence reviewed

Repository source and history:

- `src/nds/nds_renderer_preamble.c`
- `src/nds/nds_renderer_assets.c`
- `src/nds/nds_renderer_textures_effects.c`
- `src/nds/nds_renderer_native_owners.c`
- `src/port/renderer_adapter_stage.c`
- `src/nds/nds_platform.c`
- `src/nds/nds_battle_hud.c`
- `scripts/generate_nds_particle_banks.py`
- `scripts/check-nds-particle-banks.ps1`
- `docs/optimization/NDS_PARTICLE_BANKS.generated.json`
- `src/import/battleship_efmanager.c`
- `src/port/battle_playable_compat_stubs.c`
- commit `8da5257c2528e494cd3217e1b848af6fa314d6bf`
- commit `6178b43d052da5ca6cf856d9354ee0e194de5eca`

Hardware/API references:

- devkitPro/libnds `include/nds/arm9/videoGL.h`
- devkitPro/libnds `include/nds/arm9/background.h`
- GBATEK sections “DS 3D Texture Formats,” texture palette base, VRAM bank control, and main-engine bitmap backgrounds

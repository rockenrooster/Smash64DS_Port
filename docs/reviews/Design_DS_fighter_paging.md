Verdict

The owner is right that this should be solvable. The agent is right that ordinary byte shaving will not solve it.

The mistake would be jumping from those observations to general-purpose runtime demand paging. The original architecture already loads each fighter’s main closure before the match; only individual animation files are later loaded into a reusable per-instance figatree buffer when status changes.

That strongly suggests the proper direction is:

Recover enough battle address space, eliminate duplicate source/native representations, and load a compact match-specific working set before GO.

Not:

Fault arbitrary fighter pages from NitroFS while the match is running.

The size of the actual hole

Using the supplied measurements:

Working Mario + Fox closure: 173,088 B
Worst four closure: 577,424 B
Additional closure residency: 404,336 B
Current free floor: 66,144 B

With no other changes, the projected floor is:

66,144 - 404,336 = -338,192 B

To retain the required 32,768-byte safety floor, the minimum recovery is:

404,336 - 66,144 + 32,768 = 370,960 B

And 370,960 bytes is only the lower bound. It does not include the additional per-instance state, joint/parts allocations, animation scratch, four-player audio pressure, or any larger item/effect requirements caused by going from two instances to four.

I would set the engineering target at at least 512 KiB of net additional battle headroom, not 371 KiB. That gives room for the compact replacement data and avoids producing another configuration that technically boots but has no useful safety margin.

This is large relative to the current 1.32 MB arena, but only about one eighth of total DS main RAM. The Nintendo DS has 4 MB of main RAM plus separately banked VRAM, although that main RAM is shared by the system and ARM processors and VRAM has mapping constraints.

What the N64 comparison proves—and what it does not

The N64 comparison is valuable because the original game supported the same gameplay with 4 MB and essentially the same closure-loading model. It is evidence that a workable lifetime and overlay arrangement exists.

It does not prove that the current port’s memory map automatically fits:

The current ARM9 static image and BSS consume far more of the 4 MB before the battle arena is created.
The port may have both an original N64 representation and a generated DS-native representation resident simultaneously.
Debugging, compatibility, preview, framebuffer, cache, and renderer buffers did not necessarily exist in the same form on N64.
Dedicated DS VRAM only helps if textures and framebuffer-like data are actually moved there and their main-RAM source copies are discarded.

So I would tell the agent to use the N64 build as a memory-map oracle: instrument the original four-heavy-fighter match and record its scene overlay size, arena size, closure allocations, animation heaps, framebuffers, audio heap, and low-water mark. Compare those categories against the DS build. That will identify where the port is spending hundreds of kilobytes the original did not.

The owner’s “scale it to 0.8×” point

Scaling from 320×240 to 256×192 is 0.8 on each axis, but pixel count becomes:

0.8 × 0.8 = 0.64

So an equivalent uncompressed pixel buffer can shrink by 36%, not 20%:

320 × 240 × 2 = 153,600 B
256 × 192 × 2 =  98,304 B
Savings         =  55,296 B

That is useful for framebuffers, decoded backgrounds, preview surfaces, and some textures.

It does nothing directly for:

animation tracks;
collision data;
fighter attributes;
status/moveset data;
skeleton topology;
code;
model coordinates;
display-list control data.

For geometry, the corresponding operation is not scaling the coordinates by 0.8. It is polygon/vertex reduction or use of the original low-detail model. The current renderer already has separate high- and low-detail fighter owner images, and the source policy uses low detail for larger fighter counts.

Also, some of the obvious large-buffer savings have already been spent. The repository records a framebuffer collapse as already recovered, while the audio effect cache was shown to be undersized rather than wastefully large.

So the 0.8× argument is a valid supporting optimization, but not the central solution.

Recommended architecture
1. A compact, match-resident pack

At character-selection completion, build a manifest from:

unique selected fighter kinds;
selected costumes;
stage;
item mode;
required common effects;
fighter voice banks;
any Kirby copy assets reachable in this match.

Then load the resulting mandatory pack completely before the match starts.

A sensible resource split is:

Resource	Residency policy
Battle common data	Resident for the entire battle scene
Fighter attributes, status tables, skeleton and collision metadata	Resident per selected unique fighter kind
Fighter geometry	Low-detail DS-native model resident; no original N64 geometry retained
Fighter textures	Only selected costumes loaded and uploaded to VRAM
Animation	Compact immutable motion bank per selected kind, shared by mirrors
Animation runtime state	Small per-instance evaluation or decode scratch
Weapons and special-move resources	Resident for every zero-notice action available to selected fighters
Voice/SFX data	Selected compressed banks plus a bounded playback cache
High-detail geometry and nonessential cosmetics	Optional cache with a guaranteed low-detail fallback

Mirrors should share all immutable kind data. Only fighter state, current pose, animation cursors, and similar mutable data should be per instance.

2. Generate the compact representation offline

Do not load the original closure, compact it in RAM, and then free it. That creates exactly the temporary peak that is already failing.

The build pipeline should instead produce the final DS representation directly:

decomp assets
    ↓ offline generator
compact fighter-core block
compact low-detail geometry block
compact motion block
selected-costume texture blocks
special/weapon/effect blocks
    ↓
NitroFS

ROM size is cheap, so favor:

uncompressed or independently block-compressed final data;
direct reads into final destinations;
no giant temporary decompression buffer;
no runtime N64 display-list conversion;
no runtime asset graph traversal.

The repository already does the broad version of this for native fighter rendering: .fighter_image objects are extracted into NitroFS rather than linked into the ARM9 static image, and the runtime loads images for fighters used by the scene.

The next step is to make those images replace unnecessary portions of the original fighter model closure instead of coexisting with them.

3. Bind once; do not add indirection to hot loops

Because the mandatory pack remains resident until the battle ends, it does not need pageable handles in every draw, animation, or collision access.

Use relative offsets in the on-ROM pack, then during setup:

Allocate final blocks.
Read them.
Validate sizes and checksums.
Perform one fixup/bind pass.
Write final pointers into the fighter-kind runtime tables.
Never move or evict those blocks until scene teardown.

That preserves essentially the same hot-path pointer access as today.

Handles, generation numbers, reference counts, and fallback checks are only needed for optional evictable resources, such as high-detail presentation data.

4. Do not page the existing reloc files directly

The existing files contain relocated direct pointers. The loader resolves internal and external references into absolute addresses, and the status table lets later resources reuse those addresses.

Therefore, evicting one raw file can leave pointers to freed memory in:

another loaded file;
a DObj or MObj;
a fighter-kind global;
a live weapon or effect;
renderer state.

The mechanically safe paging unit in that representation is at least a strongly connected component of the relocation graph, plus all objects that retain pointers into it. In practice, that may expand toward most of the closure.

So the answer is not “choose a better 4 KiB page size.” It is replace the raw reloc ABI with generated semantic blocks whose cross-block references are controlled.

The strongest first experiment: Kirby’s model

Kirby is the ideal proof because his raw model member alone is 120,864 bytes.

Build a low-detail KirbyBattlePack containing only:

joint hierarchy and parent relationships;
setup descriptors actually read during fighter creation;
attachment points;
material state still consumed by gameplay or the DS renderer;
collision-relevant model metadata;
selected-costume texture data;
the existing native low-detail rendering program.

Then bypass the original KirbyModel file entirely.

In a proof build:

Instrument or watch every access to the original model address range.
Fill the abandoned range with a poison pattern after setup.
Exercise entry, every movement/status, every throw/capture, all Copy powers, Stone, Final Cutter, items, KO, respawn, pause, Results, and rematch.
Treat any post-setup read as a missing field in the generated pack.
Once the read count is zero, remove the raw allocation rather than merely reusing its memory informally.

Repeat for the other heavy fighters. The raw model files for the worst set are:

Kirby   120,864
Link     73,584
Yoshi    44,256
Fox      32,336
----------------
Total   271,040 B

Those values are present in the generated production census.

You will not recover all 271,040 bytes because some skeleton, material, and texture information must survive. But this is still the largest obvious fighter-local opportunity, especially because a DS-native owner representation is already required for performance.

Animation needs a different treatment

Animation is the one place where the source already behaves somewhat like demand loading: on a status change, the current motion is loaded into the fighter’s reusable figatree heap.

That was reasonable on the source platform. It is not acceptable as an unpredictable DLDI miss at 30 FPS.

The appropriate DS replacement is:

one compact, directly evaluable AOT motion bank per selected fighter kind;
shared by all mirrors;
one small mutable pose/evaluation workspace per fighter instance;
no raw animation-file acquisition after GO.

Avoid decompressing a whole motion on the frame an action starts unless measurements prove the decoder fits comfortably in the worst frame. Ideally, evaluate the compact tracks directly.

The metric should not be “data touched by this frame.” It should be:

All data reachable with zero reliable prefetch notice.

An input, collision, grab, throw, reflector, item pickup, or damage transition can select a new state immediately. Those resources belong to the hard-resident set even when the present frame does not touch them.

Keeping cartridge reads off the gameplay path

The safest and simplest policy is:

CSS / loading screen:
    compute manifest
    allocate final arenas
    load mandatory blocks
    perform fixups
    upload textures
    warm or bind animation data
    validate residency
    then enter battle

After GO:
    mandatory asset read count must remain zero

Add a standing verifier law:

asset_reads_after_go == 0
animation_file_loads_after_go == 0
mandatory_page_faults == 0
general_heap_low_water >= 32768

The repository has already measured an end-to-end uncached animation acquisition at approximately 3,873,969 ticks in the affected configuration—about 3.46 times the entire 1.12M-tick presented-frame budget. The experiment’s P95 rose from about 1.19M to 3.45M ticks when the cache was effectively removed.

That is not necessarily the raw card-bus transfer cost, but it is the number that matters: it measures the actual path that would execute in the game.

NitroFS access also differs between direct cartridge environments and homebrew devices using filesystem/DLDI access, so a read that behaves acceptably on one device or emulator is not a reliable hard-deadline primitive.

Background I/O is suitable only for something with a permanent fallback, such as optional high-detail geometry. It should never gate collision, animation, a move effect, or a required sound cue.

Recover static address space too

The other half of the solution is reducing the approximately 2.7 MB resident image/BSS footprint that leaves only about 1.32 MB for the battle arena.

The preferred order is:

Mutually exclusive scene overlays. CSS, menus, intro, battle, Results, diagnostics, and preview systems should not all occupy main RAM simultaneously.
Move remaining per-fighter immutable tables from .rodata to fighter packs.
Compile out battle-inactive preview/debug storage from shipping battle configurations.
Lifetime-overlay large scratch buffers whose non-overlap is mechanically proven.
Move texture ownership to VRAM and release main-RAM upload sources.
Consider fighter-code overlays only after measuring how much code and read-only fighter data they would recover.

Because ROM size is cheap, an extreme but stable option is prelinked code/data banks for selected fighter-kind sets. There are only:

C(12,1) + C(12,2) + C(12,3) + C(12,4) = 793

possible sets of one through four unique fighter kinds. Mirrors do not add another immutable kind pack. I would not start with 793 executable overlays, but the number demonstrates that ROM-for-RAM specialization is finite and mechanically enumerable.

At minimum, the build should enumerate all 793 data manifests across every stage/item configuration and prove their decoded resident size. “Any four fit” should be a generated assertion, not a manually chosen stress case.

Estimator results (2026-09-05, corrected accounting): `estimate_fighter_pack.py --ledger --strict` now completes with zero source-location disagreements across all 81 unique closure payloads and 793 kind sets; the size verdict remains **RED**. The original 302 disagreements were reconciled through pinned O2R hashes/chains, 279 B of proven zero alignment, source-parent offsets, and complete initializer or extraction-recipe evidence. The final eight cases additionally require exact full-file agreement with the reference include extractor's declaration-order layout: Link's complete pointer/null table, actual incoming targets/readers for the Sector texels and item fragments, and enclosing two-palette split provenance for Kirby. Generated includes are compared in full when present; absent includes use the reference's lossless raw-span recipe, not a claimed independent file comparison. Both identical Kirby palettes remain separate complete 32-byte atoms; each reconciliation records its full-span hash. Profile A's worst set remains Captain/Link/Pikachu/Yoshi at **312,921 B**, or **137,317 B above** the optimistic ceiling before other growth. Profile B adds complete raw motion members without assumed compression: Captain/Pikachu/Ness/Kirby is worst at **1,929,961 B**. Kirby's 188 motion members total **399,264 B**, separate from 10,924 B of core commands. Legal common/team costumes include ID 4 for Mario/Donkey/Samus/Kirby and 4/5 for Captain. Exact four-slot VRAM occupancy, compact motion encoding, and final replacement-record sizes remain unmeasured. The 66 focused host tests pass, including corrupted-body, changed-reader, wrong-pointer, missing-parent and layout-mismatch controls. No runtime RAM gain or Kirby admission is established.

Ranked concession order

Only after compact match packs, representation removal, and scene overlays have been measured should presentation concessions begin.

No-fidelity-loss changes: remove duplicate N64/DS representations, load only selected kinds and costumes, use the source low-detail fighter model for three/four-player matches, move scene-exclusive data into overlays.
Representation changes: compact AOT animation tracks, compact skeleton/material metadata, DS-native texture formats, selected-match Kirby hats and Copy assets, shared immutable data for mirrors.
Memory placement: texture VRAM residency, carefully audited VRAM or ARM7-accessible storage, lifetime-sharing of staging and transition buffers.
Presentation-only reductions: lower-resolution fighter textures, more aggressive model decimation, no high-detail model in four-player mode.
Cosmetic reductions: lower particle density, fewer simultaneous cosmetic effects, reduced voice quality or variety. Required gameplay telegraphs remain intact.
Content restrictions: restrict particular item sets, stage combinations, or the number of unique fighter kinds.
Platform or core requirement changes: DSi-only memory, fewer than four fighters, roster cuts. These belong last.

Shrinking the existing audio cache should not be an early concession because the repository has already demonstrated that cache misses can dominate frame time and that its cue working set exceeds the current cache.

What I would send back to the agent

Treat any four fighters as solvable on retail DS without reducing fighter count. Do not implement gameplay-time page faults. First reproduce the original N64 battle memory ledger, then build an offline-generated, match-resident DS pack. Start with Kirby: replace the raw 120,864-byte model member with the existing low-detail native owner plus only the skeleton, material, attachment, collision and selected-costume data that runtime consumers actually read. Prove the original model range has zero post-setup reads. Load all mandatory selected-match resources before GO and make any NitroFS/DLDI resource read after GO a verifier failure. Target at least 512 KiB of net additional battle headroom, enumerate all 793 unique fighter-kind sets, and report the true worst set before considering visual concessions.

So: yes, it is solvable—but primarily through source-style scene residency plus DS-native match packs, not through per-frame demand paging, and not merely by multiplying asset dimensions by 0.8.

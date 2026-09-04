# Review: character-select residency and the DS storage path

**Date:** 2026-09-04  
**Project:** Smash64DS_Port  
**Reviewed revision:** `master`, `4f6beb07f90cda7a4ce7663fc5b6594360f4af2a`  
**Disposition:** architecture recommendation and diagnostic plan; not a measured fix.

`master` was newer than `p2-pikachu` and `codex/r2-runtime2` when inspected. This review is pinned to the revision above, rather than assuming the historically active runtime branch is still current. Repository behavior described below comes from that revision. The reported two timeouts were not reproduced here. The exact linked Calico/FatFs revisions and build configuration remain to be established from the failing ELF/toolchain; the upstream implementations inspected are identified in the references.[^revision]

## 1. Decision

**Do not implement lazy CSS by putting a synchronous `ftManagerSetupFilesAllKind(kind)` inside selection-change handling. That moves an entry-time failure into an interactive frame, and the current audio implementation makes that especially unsafe.**

Approve three separable changes:

1. **A generated, indexed storage bank with contiguous load units**, shared infrastructure for menu assets and the future battle semantic pack. Eliminate runtime path/header/dependency discovery from normal pack loading. Investigate and, where necessary, fix the *outer ROM file's* seek behavior as well; another archive inside NitroFS does not automatically fix that layer.
2. **A preview-only representation**, rooted in the actual CSS construction, rotating preview, confirmation pose, costume and material behavior—not the gameplay closure. Prefer loading all twelve compact preview descriptions into main RAM at the menu boundary **if a complete census proves they fit**. Keep only visible instances and their planned textures active. This is the preferred final interaction, not a compromise to avoid lazy loading.
3. **A latest-selection-wins preview service** as the bounded lazy policy when the compact twelve-kind set does not fit. Selection itself never waits. Cache hits appear immediately; misses use a correct portrait while a dwell-qualified request is loaded. All visible distinct kinds are pinned. No speculative FIFO of old cursor positions is allowed.

There is also a particularly attractive **menu-specific audio option**: preload the existing Battle Select ADPCM container into RAM. It is **157,372 bytes, about 153.7 KiB**, not its 628,352-byte PCM-equivalent stream length. Keep the existing packet playback implementation and change only its byte source. Subject to the menu memory census, this removes BGM storage contention during both CSS interaction and any stage-select interval that continues the same track.[^bgm-format]

This is not an instruction to build every fallback immediately. **Build the storage probe and preview census first.** If exact compact eager previews plus RAM-backed menu music fit, there is no reason to build a general gameplay-file eviction system for CSS. If they do not, implement bounded lazy loading of those same preview packs—not arbitrary eviction out of the battle loader.

| Question | Judgment |
|---|---|
| Files or bytes? | Access pattern, metadata and outer-file seeking are stronger hypotheses than a 900 KB payload threshold. The timeout cause is still unproven. |
| Packed archive? | Yes, with a generated index and cohort-local layout. Also address the backing ROM seek path. |
| Rapid selection changes? | Immediate selection, latest-wins requests, cache hits without delay, a short dwell on misses, and a truthful placeholder. |
| Audio sharing? | Prefer a complete RAM copy of this small menu track when it fits. Otherwise use one explicit I/O admission policy with audio deadlines, not independent competing readers. |
| Lazy or smaller eager set? | Smaller exact previews first; eager all-twelve residency wins if the measured complete set fits. Lazy is the capacity fallback. |
| Other scenes? | Reuse storage and lifetime infrastructure. Give each scene its own roots and loading policy; do not impose CSS hover behavior everywhere. |

The previous decisions remain intact: no gameplay asset paging, battle-pack construction stays behind its estimator gate, and required battle texture residency locks before `GO`.[^pack-estimator][^texture-plan]

## 2. What the evidence establishes—and what it does not

The eager ladder is real. The shipping CSS setup loads every admitted kind, allocates four `figatree_heap`s, and prepares preview residency for all admitted kinds. The original-scene proof path has its own all-playable-kind loop; changing only the shipping ladder would leave that second path eager.[^css-entry][^css-proof]

The generated main-closure census supports the twelve-kind sum of **1,188,080 bytes** and the Mario/Fox sum of **173,088 bytes**. These are allocation/closure figures, not a measurement of transferred storage bytes. The report's nine- and ten-kind totals likewise do not include every possible native-image, animation, metadata, buffering or transient cost. The manifest's **1,916-file total** and **77 borrowed Kirby animation files** are accepted here as reported census figures; this review did not independently parse the full manifest.[^fighter-census][^pack-estimator]

There are several distinct counts:

```text
shipped files
    != distinct files touched by CSS
    != fopen calls
    != payload-load completions
    != underlying ROM reads/seeks
    != FAT-sector or data-sector reads
```

The reloc API separately exposes header reads, external-ID reads and payload reads, several of which reopen the same path. A completed-payload counter cannot reveal a stall before the payload, or account for all metadata traffic.[^reloc]

The supplied observation places execution in `get_fat` when inspected. It does **not** identify the calling operation, prove that the same invocation ran for 3,000 seconds, or prove an infinite FAT-chain loop. `clst=14918` is a cluster identifier, not a count of clusters already traversed. A normal but repeatedly repeated lookup, a long outer-file seek, a media/driver fault, a malformed offset, corruption, and a debugger-stopped target can all produce substantially different situations around that observation.

**The debugger confound is material, not a footnote.** Report host wall time, target-running time and actual I/O progress separately. Do not divide 904,656 by 3,000 and call the result DS throughput. The reads did not finish; the amount actually transferred is unknown; the emulator may have spent part of the interval stopped or severely slowed by instrumentation. Two repeatable failures justify investigation but do not remove a repeatable harness error.

The healthy arena counters argue against the known arena-overflow halt. They do not exclude corrupted filesystem objects, a separate libc allocation failure, stale cache pointers or invalid asset metadata. Similarly, the reported **29,696-tick battle P95 DLDI effect** is a workload-level measurement, not a maximum latency for one CSS read.

## 3. The important filesystem distinction: NitroFS is already inside an archive

The file-backed implementation inspected has this structure:[^nitrofs][^calico-fd][^calico-names][^fat-driver]

```text
fopen("nitro:/.../fighter_asset.o2r")
    NitroFS: resolve a pathname to a NitroFS file ID
    NitroFS handle: { file ID, logical position }

fread(asset_handle, ...)
    NitroFS file table: translate to an absolute ROM byte range
    Calico NitroRom file backend:
        lock one shared ROM mutex
        seek one shared backing fd when its position differs
        read that backing fd
        unlock
    libdvm / FatFs
    DLDI-backed block device
```

There is a separate direct-card backend. **Its success is not proof that the file-backed DLDI path is healthy.** Identify the active backend in the test rather than inferring it from the `nitro:/` prefix.[^calico-fd]

### 3.1 Hundreds of NitroFS assets are not hundreds of FAT files

In libdvm 2.1.0, a NitroFS open resolves a name and stores a file ID and position. Its seek implementation adjusts that logical position. The FAT cluster chain encountered below it can be the chain of the **entire `.nds` file on the host volume**, not an individual fighter asset.[^nitrofs]

The inspected Calico backend already opens that ROM once. All NitroFS handles share the backing fd, its tracked position and a mutex around seek-plus-read. Thus opening a single `nitro:/preview.bank` does not, by itself, give it an independent physical read cursor. Audio, asset data and pathname lookup traffic can all move the shared backing cursor.[^calico-fd]

The distinction also prevents confusing two unrelated structures called FAT: NitroFS's table of file start/end offsets and the storage volume's cluster-allocation chains.

### 3.2 Why pathname count and layout can still be expensive

Calico caches the NitroFS file table and directory descriptors at mount. In the inspected implementation, it does **not** cache all directory name lists. Path resolution starts a directory iterator and scans names; an iterator entry involves a short header read followed by a name read through the ROM interface.[^calico-names]

Consequently, opening many names in the same directory can cause quadratic *directory-entry visits* when each lookup restarts at the beginning. That is not automatically quadratic physical I/O: sector caches matter. Separately, bouncing between the name table, far-away payloads and music can turn small logical requests into expensive backing-file seeks.

A useful diagnostic model is:

```text
load time = pathname work
          + backing-file seek / cluster-chain work
          + actual device transfers
          + relocation / binding / construction
          + lock / scheduling / instrumentation delay
```

This is why **“per-byte versus per-file” is too narrow**. There is also a per-offset/per-seek-history cost. Adding a fighter changes content, pathname population, ROM layout and possibly the host file's allocation. A threshold coinciding with 904,656 resident bytes is not evidence of a filesystem limit at that size.

**Working hypothesis:** excessive metadata and outer-ROM seeking deserve the first performance probe. **Unresolved fact:** the supplied evidence cannot yet distinguish pathological progress from a genuine filesystem/driver/harness failure. Packing is justified infrastructure, not a demonstrated cure for the timeout.

## 4. The cheapest measurement that settles the direction

### 4.1 First add one observable operation boundary

Add a small fixed RAM trace and an always-readable active-operation record. Before each open, seek, read, relocation and preview-construction phase, publish an increasing sequence number, caller class, asset/kind, offset, requested bytes and target timestamp. On return, publish result, actual bytes and elapsed ticks. Record both the last completed operation and the current incomplete one.

Use aggregated counters for NitroFS path-entry visits, backing-ROM seek distance/direction, `get_fat` calls and block reads. Do **not** set a breakpoint on every FAT access. Do **not** log to the same filesystem inside the timed operation. Bound the trace memory, count lost records and dump it after the measurement. Timing must handle wraparound; a 3,000-second run must not be interpreted from a single unextended 32-bit timer delta.

Keep caller identity explicit: `CSS_HEADER`, `CSS_PAYLOAD`, `NATIVE_IMAGE`, `ANIMATION`, `BGM`, and `SCENE_OTHER`. Otherwise a sampled filesystem stack may be attributed to the wrong consumer.

### 4.2 Run a small probe, not another full 50-minute shell lap

Use the approved profiling environment with the **same DLDI-backed launch route** as the failing run, initially without an attached debugger. Retain the exact ROM and storage-image hashes and verify a running-target heartbeat. Start with one suspect kind and its actual request list.

| Probe | Hold constant / change | Interpretation |
|---|---|---|
| Open/close only | Same paths; request no payload | Separates pathname/handle work from asset bytes. A one-byte variant additionally exercises the first data access. |
| Loose versus packed replay | Same logical bytes and dependency order; eliminate per-asset names using a pre-read index | Tests path/metadata overhead without changing fighter behavior. Count actual lower-level bytes too. |
| Ordered versus original offsets | Same ranges; read in ascending ROM order, then original order | Large difference implicates seek history/locality rather than payload volume. |
| One payload, several sizes | Same already-resolved object; vary transferred bytes | Estimates the transfer component without multiplying opens. |
| Audio off versus streaming versus RAM-backed | Same asset requests and binary routes where feasible | Separates audio contention and service scheduling from asset loading. Muting volume alone is not “audio off.” |
| Nine versus ten admitted, same ten-kind ROM | Disable the tenth load without removing its shipped assets | Separates admission work from layout/name-table changes caused by rebuilding the ROM. |

Run cold and warm variants. First collect phase totals and worst single-operation duration; only expand the matrix if those do not discriminate. A probe that already identifies one nonreturning read does not need to finish the entire roster.

Follow-up controls: vary kind order and skip the suspect kind; replay the same requested ranges against a clean, independently validated storage-image copy; compare near and far backing-file offsets; then test the packed path with outer-file fast seeking where supported. Changing compiler/link placement is another confound, so prefer route switches in the same test binary where possible.

### 4.3 If it still stalls

Attach only to inspect a target already shown not to progress. Capture the complete caller stack, active operation record, relevant file position/size, both processors' states, thread/mutex ownership, and interrupt-enable state. Determine whether `get_fat` was reached by `f_lseek`, `f_read`, directory enumeration, or map construction. Match source and symbols to the failing ELF before assigning meaning to `ff.c:1181`.

Inspect the backing file's chain on an offline copy: bounds, termination, cycles, cross-links, recorded size and fragmentation. Check returned header sizes/offsets before following them. Never “fix” an invalid chain by silently treating it as EOF.

A rising completed-byte count indicates slow progress. Rising chain walks with little useful I/O suggests amplification. A fixed pending driver request, repeated invalid chain, blocked lock owner or stopped emulator requires a correctness/harness fix before any interactive loader is admitted.

**The first deliverable is a trace attributing the delay, not a claim that 900 KB is too large.**

## 5. Storage shape: pack load units, not merely filenames

Use a small number of scene/cohort banks with a generated index. A bank should be opened or resolved once per appropriate residency epoch. The index is generated from the existing asset pipeline, validated against the bank build ID, then read from memory during loading. Do not repeat closure discovery through file headers.

A useful entry describes:

```text
stable asset / atom ID, representation version
bank ID, bank-relative offset, stored length, decoded length
alignment, integrity check, dependency-list range
binding / fixup-list range, scene capability classification
```

Serialize the format explicitly rather than dumping native C structures or live pointers. Validate overflow-safe ranges, alignment, count limits, decoded sizes, declared imports, cycles where disallowed, and destination capacity before binding. A short read or checksum mismatch must leave the pending object unpublished.

The logical API should resemble `read_at(bank, offset, destination, length)` even when the backend implements it with serialized seek-plus-read. This establishes ownership; it is **not** a promise that a POSIX `pread` implementation exists or bypasses the shared ROM backend.

### 5.1 Generate the metadata answers too

The existing reloc helpers reopen files to obtain headers and extern IDs. Packing payloads while keeping those discovery calls simply preserves part of the bad access pattern. Have the pack generator emit the relevant sizes, dependency IDs and fixup descriptors. Compatibility helpers may initially answer from this generated table while the old consumer interfaces remain.[^reloc]

Retain logical asset IDs independently of NitroFS file IDs, which may change when the ROM is built. Either resolve the few bank names once and use bank-relative offsets, or generate a post-ROM-build binding verified against the final image. Never hand-maintain absolute ROM offsets.

### 5.2 Make one preview a small number of contiguous ranges

An archive holding the old files in an arbitrary order replaces opens with scattered seeks. Instead, cluster the geometry, material/texture sources and menu animations consumed together. Read monotonically within an admitted preview bundle between scheduling points. Deduplicate immutable atoms where worthwhile; bounded duplication of small donor data on disk can be a reasonable trade for fewer seeks. Count the resulting resident union correctly.

Start uncompressed where it is small enough. Add independently decodable chunks only when measurements justify them. A monolithic compressed roster forces unrelated decoding; a single large compressed fighter block prevents cheap cancellation and creates a large CPU commit cost.

The battle semantic pack can share this bank reader, format validation, atom IDs and tooling. It has **different roots and a different lifetime**. Do not create hundreds of match-combination payload archives; the existing estimator explicitly prescribes factored atoms/manifests. Shared infrastructure does not turn storage, main RAM and VRAM into one budget.[^pack-estimator]

### 5.3 Fix outer-ROM seeking at the right layer

If the probe attributes the delay to repeated backing-chain traversal, evaluate a supported fast-seek/extent map for the **outer ROM file**. FatFs provides an in-memory cluster link map when `FF_USE_FASTSEEK` is enabled. Its documented table cost is `2 × (fragments + 1)` DWORDs, or `8 × (fragments + 1)` bytes with 32-bit DWORDs. Construct and validate it at a load boundary, account for its actual size, and retain it for the backing file's lifetime.[^fatfs-fastseek]

That requires cooperation from the linked backend. A `FILE *` for `nitro:/preview.bank` is not the underlying FatFs file object; casting it to one is invalid. Confirm the linked configuration and expose an explicit supported hook if needed.

A map removes repeated allocation-chain lookup, not media latency, lock contention or corruption. Logically consecutive bytes in an archive are not guaranteed physically consecutive sectors. Conversely, even a contiguous backing file can suffer repeated chain walking without an appropriate mapping strategy. Separate backing-ROM handles can reduce shared-position interference, but simply opening two NitroFS bank handles does not create them. Measure before adding that complexity.[^calico-fd]

## 6. Make a CSS pack, not a smaller collection of gameplay files

The source preview has a much smaller behavioral domain than a playable fighter. Its unconfirmed model rotates; confirmation brings it to a facing orientation and chooses a kind-specific demo win status. Construction also preserves yaw across replacements and applies costume, shade, scale and CPU coloring. Those behaviors—not attacks, items and every gameplay animation—define the first preview roots.[^source-preview]

For example, the selected status is Win4 for Fox/Samus, Win1 for Donkey/Luigi/Link/Captain, Win2 for Yoshi/Purin/Ness, Win3 for Mario/Kirby, and the default Win1 for Pikachu. This makes the selected animation an explicit, enumerable dependency rather than a reason to carry a fighter's complete motion collection.[^source-preview]

Generate `CSSPreview(kind, legal_variant)` from the existing semantic-object work. Its root contract must cover construction inputs, the initial preview motion, the selected demo status, all reachable animation/material events, model parts actually used by those states, required native geometry, legal costumes/shades, and CPU/team presentation. Include shared donor atoms by identity. Do not take “Purin borrows 77 files overall” to mean “CSS needs all 77,” or assume it needs none without following its menu roots.

**Restricted behavior is not permission to guess at dead bytes.** Unknown consumers or unclassified data are a STOP. The estimator already has typed object sources, relocation information and explicit semantic-edge requirements; reuse that machinery with a different root set.[^pack-estimator]

The present preparation helper warms the initial submotion animation and, for the generated native-image fighters, both owner-image details. Both details must be counted until a CSS-specific detail-selection proof permits omitting one. Loading only an initial pose is insufficient if confirmation later reaches a different animation or texture.[^css-prepare][^ftmanager]

### 6.1 Do not violate the manager's residency invariant

`ftManagerSetupFilesAllKind` treats a non-null main pointer as evidence that the kind's model/motion/special closure is available. The DS wrapper also retries globally deferred effect descriptors after loading. A compact preview subset must **not** masquerade as that full residency state.[^source-manager][^ftmanager]

Use a preview-specific asset view/binder, or an explicitly separate, checked preview-mode binding contract. Reuse source animation/rendering behavior where its inputs are fully covered. Do not publish a partial pack into the ordinary `p_file_main` invariant and hope an incidental consumer never touches a removed field. Likewise, do not accidentally bind global effect descriptors to menu memory that will later be evicted.

### 6.2 The mechanical eager-versus-lazy decision

Generate a report containing, for every kind and legal presentation variant: stored bytes, retained immutable bytes, mutable instance bytes, construction scratch, required textures/palettes/views, maximum decode/bind work units, and the number of storage ranges.

Define:

```text
H = menu shell, portraits, names, fixed infrastructure and reserved headroom
I = four live instance workspaces, including their figatree / pose state
A = audio backing and playback buffers, counted exactly once
X = indices, filesystem mapping/cache costs and loader bookkeeping
U(S) = union of retained preview atoms for kinds/variants in set S
W = maximum staging, decode, bind and safe-publication overlap

Eager requirement = H + I + A + X + U(all twelve) + W
Lazy requirement  = H + I + A + X + max U(up to four live kinds) + W
                   + explicitly reserved non-visible cache capacity
```

Compare against the **measured menu-owned budget**, not a nominal 4 MB and not a free-floor number from an unrelated phase. Code growth, stacks, libc allocations and already resident renderer data still consume main RAM. The reported 910,504-byte free floor is useful evidence for the failing phase, not an automatic budget available everywhere.

Evaluate two audio profiles, streaming and complete RAM-backed Battle Select. If the all-twelve exact preview set fits with RAM music and safe construction overhead, choose it. Otherwise evaluate eager previews with streaming music, then live-kind lazy previews with RAM music. Prefer the simplest passing profile that preserves responsiveness; use lazy plus streaming only when the memory report requires it and its latency tests pass.

Main-RAM eager preview data does **not** require twelve models or twelve texture sets resident in VRAM. Construct four instances and use a menu-specific texture plan for the simultaneously visible variants. Switching textures from RAM can still require budgeted conversion/upload; it merely avoids storage latency. Do not borrow the battle VRAM profile without accounting for menu compositor ownership.[^texture-plan]

No all-twelve compact size has been measured in this review. Claiming it fits would be speculation. The decision rule, however, is firm: **do not pay the complexity of arbitrary file eviction until a preview-only census shows that the simpler resident representation fails.**

### 6.3 Reduced or pre-rendered previews

Start with exact source-derived geometry and the restricted menu animation domain. Quantized tracks, reduced geometry or pre-rendered sprites are separate fidelity decisions. A sprite sequence must reproduce rotation, confirmation motion, costume and material variation; its aggregate frames and palettes are not automatically smaller than a model with short animation tracks.

Use the already resident portrait as the temporary loading/error representation. Do not silently turn permanent portrait-only previews into a successful full-3D admission result. A reduced 3D tier is preferable to an unbounded loader only after explicit visual approval and a measured size/cost report.

## 7. Lazy policy when the compact resident set still does not fit

### 7.1 What the player sees

Portrait, name, puck, costume/team indication, selection sounds and confirmation state change on the original input cadence. They do not wait for a file. A valid resident preview is attached without a dwell delay. On a miss, the old model is removed from that slot and the **new fighter's portrait** occupies the preview region until the correct 3D model is ready. Do not leave Mario turning beneath a newly selected Fox name.

Use **150 ms as an initial dwell candidate**, measured in target time, not a hard-coded count of presented frames. It is a proposed interaction parameter, not a measured hardware limit. A sweep that never dwells should produce no new full preview jobs. Confirmation bypasses the dwell and promotes the requested preview, but does not block input or the ability to proceed to the next legal loading boundary.

Maintain lightweight per-slot yaw/selection-timeline state separately from the GObj. On publication, apply the latest costume, shade, CPU/team state and source-selected status. Preserve the source's rotation/facing behavior at its intended update cadence; do not make it dependent on disk completion. Where exact animation-time reconstruction is unavailable, explicitly test and document the delayed-appearance behavior instead of claiming perfect timeline equivalence.[^source-preview]

Do not delay the name announcement until I/O finishes, replay a confirmation sound on completion, or require a 3D preview to exist before the selection becomes authoritative.

### 7.2 One current request per slot, not a queue of cursor history

Keep these separate:

```text
logical slot: wanted kind/variant, selected state, yaw/timeline, request generation
published preview: handle, source revision, resident generation
loader job: destination ownership, bytes/work completed, subscribed slots
scene: lifetime epoch
```

The existing sync stores kind/player/selected state and then performs rebuilds synchronously. An asynchronous conversion must not use that stored “last state” as proof that a model was built: otherwise a pending request can be mistaken for a completed one on the next frame.[^css-sync]

A suitable state machine is:

```text
ABSENT -> DWELL -> QUEUED -> READING -> BINDING -> READY -> PUBLISHED
                             |           |         |
                             +------ stale / failed ------> discard safely
```

Each selection change supersedes the previous slot request. Check scene epoch and request generation before starting work, after each work unit, and before publication. If an in-flight operation cannot be cancelled, let that one operation return; discard its obsolete result and do not continue the old bundle. Coalesce identical immutable kind requests across slots, cancelling the shared load only when no current subscriber needs it.

Give failed requests a recorded terminal state and a bounded retry policy. Returning to the same broken kind must not trigger an open/read/retry storm every frame; persistent failures remain on the portrait until an explicit retry condition or a new validated scene epoch.

Start with one active preview transfer. Prioritize confirmed requests, then fairly service stable visible requests. A moving slot must not indefinitely starve a stable second slot. Directional prefetch is disabled initially: it competes with real requests and audio, and prediction cannot repair an unbounded read path. Add it only when traces show useful hits, bounded waste and spare capacity.

### 7.3 Cache capacity is bytes plus ownership, not just N

Four identical fighters need four mutable instances but can share immutable kind data. Four distinct fighters need the union of four preview packs. A count of four cache entries is insufficient unless the worst permitted byte/variant combination fits.

Pin all published users, pending bind users and in-flight destinations. Evict only unreferenced, non-visible content. With all four kinds visible, replacing one kind requires either an explicitly budgeted old/new overlap or detaching that slot to its portrait before reclaiming its old storage. **Do not quietly assume a fifth full fighter staging allocation.** The portrait policy makes detach-before-replace a valid, local operation.

### 7.4 Actual reclamation must exist

The taskman general heap is a bump region. Destroying a preview or clearing `p_file_main` does not return its loaded closure to that region. Repeated lazy loads into it will still grow without bound.[^allocator]

Reserve a reusable preview arena/pool at menu entry. Immutable preview blocks, scratch and per-slot workspaces need explicit ownership and bounded lifetimes. Retain the four figatree heaps while reusing the existing constructor; any reduction of those heaps is a separate measured optimization. The current destroy wrapper releases pose ownership while retaining reusable scene backing, so preserve that behavior.[^ftmanager]

Before reclaiming any block, detach all consumers and remove its bindings: GObj/DObj/MObj references, pose registrations, animation cache entries, relocation/status mappings, native owner bindings, material memoization, texture-source references and any deferred descriptors. Shared atoms need reference ownership or a separately pinned shared pool. Never rewind the whole scene arena while other previews remain live.

Cache validity must include the taskman/scene generation, not merely whether an address lies inside the arena. This repository already records a heap-generation mechanism specifically because address-range/cursor tests admitted stale animation-cache pointers after a scene reset.[^allocator]

## 8. “Budgeted asynchronous loading” needs an actual latency contract

A byte budget limits total requested work. It does not bound the time of the next `fopen`, `fseek`, `fread`, decompression step or native binding operation.

It is practical to retain synchronous reads of small ranges and yield **between** them. It is not necessary to interrupt libc halfway through a read. But before enabling demand reads in an interactive scene, characterize the worst observed execution and scheduling behavior of the individual read/seek unit on the required backend. A 512-byte read can still initiate an expensive outer-chain walk.

Use a staged loader:

```text
resolve/validate metadata at the boundary
    -> admit a bounded read unit
    -> return to UI/audio service
    -> admit bounded decode/fixup work
    -> return
    -> prepare required texture/native bindings
    -> publish at a render-safe boundary
```

Choose transfer units from measurements, including seek and cache behavior; do not prescribe “4 KB per frame” as a guarantee. Start with bounded, sector-aware units and allow measured coalescing. Include unaligned tails, stdio read-ahead and any bounce-buffer copy in the accounting.

Two implementation options are legitimate. A cooperative main-thread pump is simplest when each call demonstrably fits the available frame slack. A worker can keep the main thread responsive while waiting on storage, but only after verifying filesystem/driver reentrancy, shared locks, scheduling and interrupt behavior. Lower thread priority alone does not bound a mutex-held operation, an IRQ-disabled section, or a driver that does not return.

A watchdog checked after `fread` returns cannot rescue a `fread` that never returns. Do not free its destination, kill its thread or force-unlock the filesystem underneath it. A truly hung device may require controlled scene/device failure; software above that call cannot honestly promise continued interaction without a lower-layer timeout/cancellation contract.

**Admission rule:** if acceptable individual-operation behavior cannot be established, do not initiate optional filesystem reads while CSS is interactive. Use eager compact data at the boundary, or retain the portrait until an explicit loading boundary. Dwell and a spinner are latency-hiding policies, not remedies for an unbounded driver.

## 9. Audio: the present call graph matters more than old comments

At the reviewed revision, the timer callback posts to a mailbox. `ndsAudioBgmWorkerMain` calls `ndsAudioBgmHandleSeam`, which starts the prepared next channel and marks the old buffer for refill. **`ndsAudioBgmUpdate`, on the main thread, calls `ndsAudioBgmServiceRefills`, which performs the packet read.** Nearby historical priority comments describe a refill-worker arrangement, but that is not what the current function bodies do.[^bgm-seam][^bgm-refill][^bgm-update]

Therefore, a long synchronous CSS load can starve the refill service even without two simultaneous filesystem callers. It can also delay seam handling through critical sections or device-service interference. Do not “solve” this by raising the current worker's priority and assuming the packet reads moved with it.

### 9.1 Quantify the actual reserve

The format uses two 8,196-byte playback buffers. A maximum-length packet contains 16,384 samples at 22,050 Hz, about **743 ms**. Including its eight-byte record header, a full packet implies about **11,041 stored bytes/second**, not the **44,100 PCM-equivalent bytes/second** diagnostic constant.[^bgm-format]

Two initially prepared full packets represent approximately 1.486 seconds of sound, not a permanent 1.486-second refill allowance. After a seam, the retired buffer must be prepared before its next use. Partial packets and loop-boundary packets can be shorter. Calculate slack from the **actual queued sample counts and playback position**, not from buffer capacity or a historical comment.

There are two independent deadlines: producing the next packet before it is needed, and servicing the timer/worker/channel-start seam promptly. Plenty of buffered audio does not permit a long interrupt-disabled section at a seam.

### 9.2 Preferred CSS option: keep Battle Select's compressed container in RAM

Load and validate the 157,372-byte container at a legal menu boundary. Retain the existing packet headers, ADPCM payloads, loop-record offsets, volume behavior, channel switching and two playback buffers. Read packets from a memory byte provider instead of from NitroFS. No PCM expansion or audio re-encoding is required.[^bgm-format]

This needs an explicit source abstraction, not a dummy non-null `FILE *`: the current update function returns early when its file pointer is null. Replace that ownership test with a validated file-or-memory source contract. Account for the complete backing copy **in addition to** the existing playback buffers unless the accounting already includes them.[^bgm-update]

Place the cache in a clearly owned menu-session lifetime if CSS and stage select retain the same BGM. A scene-arena reset must not invalidate a still-playing memory source. Quiesce all users before releasing or replacing it. On first entry, prime the track only after required boundary loading; on re-entry, preserve the existing track position where the menu contract requires continuity.

This removes storage contention, but **does not make a blocked main-thread refill service disappear**. Keep returning to audio update, or separately validate a bounded memory-refill scheduling change. Do not claim that a RAM copy alone makes indefinite synchronous asset loads safe.

### 9.3 Streaming fallback: one admission policy, audio first

Route optional preview reads and audio refill requests through one explicit device-ownership/admission policy. Existing callers must not bypass it with surprise header, animation or owner-image reads. A mutex prevents corrupt interleaving; it is not a deadline scheduler.

At each scheduling point, service urgent audio work before starting an optional asset unit. For a proposed read unit `q`, require:

```text
audio slack > worst read/seek/lock time(q)
            + worst audio refill/prepare time
            + scheduling uncertainty and guard
```

For a main-thread pump, also require enough frame slack for that work and the remaining mandatory UI work. These are empirical admission bounds with margins, not a proof that every possible storage device has a finite maximum latency. If the configured backend violates them, disable optional reads and expose the fault rather than continually increasing a timeout.

Do not hold an outer archive lock across decode, relocation, model construction or texture preparation. Do not perform filesystem operations in the timer IRQ. Preserve the seam worker's ability to run, and measure seam lateness as well as buffer underruns. Refill production and preview loading must make progress without either waiting on a lower-priority owner that cannot run.

The CSS sync already records why per-hover BGM suspend/reprime was removed, including a tested 15-rebuild, zero-payload-read run. Keep that as a regression scenario; it is evidence for that tested configuration, not a universal zero-I/O proof for the expanded roster or all later animation callbacks.[^css-sync]

## 10. Scope: share infrastructure, not one scene's policy

| Scene | Residency and storage policy |
|---|---|
| VS character select | The preview-only bank and immediate selection contract described above. All-twelve compact eager data if proven to fit; otherwise bounded live-kind loading. |
| Stage select | Keep the small selection UI and available thumbnails resident. Audit whether an actual 3D stage preview is required and which source assets it consumes. Load that representation after dwell if needed; load the full selected stage at the confirmed transition, not on every highlight. |
| Results | Build a Results-specific set for the known participants and required win/lose/draw presentation. It has a legal boundary and no arbitrary roster sweep. Do not assume the battle pack includes Results assets; the accepted semantic-pack plan explicitly separates scene presentation data. |
| 1P interstitials | The next opponent/scene is usually determined by campaign state before combat starts. Generate the interstitial's own manifest and admit it at the transition. Cover boss/variant roots explicitly; do not turn campaign progression into gameplay paging. |
| Battle | Consume the accepted semantic pack and pre-`GO` texture plan. No CSS cache dependency and no optional-loader escape hatch after `GO`. |

The Results wrapper already contains separate setup/transition timing instruments and records a historical case where apparent load delay was actually reveal timing and per-frame cost. Reuse that separation rather than automatically blaming all scene delay on storage.[^results]

Stage-select and unfinished 1P behavior require a call-site/root audit; this review does not assert that their complete closures or current loads have been measured. The reusable boundary is **bank reader + validation + scene epoch + generated demand report**, not a global LRU holding arbitrary source files across every scene.

## 11. Repository-bound implementation order

### Step A — establish progress and identify the backing path

Instrument the existing reloc operations, native-image/animation loads and backing-ROM operations. Run the small matrix in §4. Record linked library versions/configuration, launch backend and storage-image identity. A nonreturning low-level operation must be resolved or excluded from interactive use before moving loads into CSS.

### Step B — introduce indexed storage without changing residency semantics

Add the generated bank/index and reader adapter behind the existing asset API. Preserve logical IDs and exact content. Initially replay the same eager request list through both backends to isolate the storage-shape effect. Remove metadata discovery from the bank path. Add outer-ROM mapping only if the trace requires it and the linked backend supports a safe integration.

This A/B is important: changing pack semantics, allocation, audio and selection behavior simultaneously would prevent attributing the improvement or a new failure.

### Step C — produce the preview census and choose a profile

Extend the semantic estimator with CSS roots and legal variants. Report main RAM, transient peak, textures/palettes/views, load ranges and materialization work. Evaluate all twelve eagerly resident versus every legal up-to-four-kind live set, including four-instance duplicate-kind cases. Include native-image details, the four figatree heaps and both audio profiles. Unknown roots stop admission.

If the eager profile passes, load it at the menu boundary and keep selection-time storage at zero. If it fails, the same report supplies the sizes and dependencies for the lazy service. Do not bypass the battle estimator's existing go/no-go conditions while implementing this menu work.[^pack-estimator]

### Step D — change preview orchestration, not the read-only reference

The principal seam is `src/import/battleship_mnplayersvs.c`: replace both eager paths with the selected scene profile; separate desired state from published state in preview sync; turn preparation into a request/readiness contract. No blocking `ensure` is called from input, update, draw or the commit path.

The `ftManagerMakeFighter` wrapper currently contains native owner-image ensure calls. Those are not an acceptable hidden miss path for asynchronous commit. Establish complete preview readiness beforehand and provide a checked no-I/O construction/bind path. Preserve pose release and existing render/material invalidations on replacement.[^ftmanager][^css-sync]

Keep all adaptations in port code, generators and explicitly managed library integration. **`decomp/` remains unchanged.**

### Step E — integrate audio and publication

Add the file-or-memory BGM source contract if the RAM-backed profile was selected. Otherwise connect all reads to the admission policy. Stage completed previews outside published ownership, validate their dependency/texture readiness, and atomically attach them at a safe frame boundary. Scene exit invalidates requests immediately, but backing memory is not reclaimed until in-flight operations and consumers have actually quiesced.

### Step F — reuse the infrastructure at other boundaries

Apply the same indexed reader and scene-manifest accounting to Results, stage transition and campaign interstitials as their roots are audited. Do not broaden the first CSS patch into an unmeasured rewrite of every scene loader.

## 12. Acceptance gates

| Gate | Required evidence |
|---|---|
| Correct diagnosis | Exact backend and active operation identified; unhalted target progress distinguished from host timeout; no unaccounted filesystem call in the relevant paths. |
| Storage correctness | Loose/banked logical content and binding equivalence; malformed sizes, offsets, imports, checksum failures and short reads leave no partially published preview. |
| Responsiveness | Sweep all twelve at one selection change per update, reverse direction, alternate two kinds, confirm during load and cancel at every loader stage. No historical FIFO backlog; no input call waits for readiness. |
| Capacity | Worst legal four distinct kinds and four instances of one kind, with costume/team/CPU variants. Peak includes overlap and scratch. Repeated cycling reaches a stable memory plateau, not bump-heap growth. |
| Lifetime | CSS → stage select → CSS; CSS → battle → Results → menu; scene exit during each read/bind stage. No completion from an old epoch can attach or write into reused memory. |
| Source presentation | Initial rotation, replacement yaw, confirmation status, grab/unselect rebuild, CPU tint, costume/shade and team behavior checked for every admitted kind. Purin/Kirby donor ownership tested explicitly. |
| Audio | Repeated short-packet and loop-boundary tests; no new seam misses, unsafe writes, timer-event drops or hover-triggered suspend/reprime. Compare actual audio continuity, not just a “playing” flag. |
| Local failure | Optional preview failure names kind, asset, phase and reason, leaves a correct placeholder, and does not invalidate another slot or the entire renderer. A required battle asset failure prevents battle admission. |
| Texture readiness | Preview publication requires all planned handles to be valid. Count slots, bytes, palettes and placement, not only main-RAM fit. No global native-owner fallback from a local preview miss. |
| Gameplay isolation | Required gameplay asset reads, allocations, conversions and uploads remain zero after `GO` under the accepted plans. Classify permitted BGM streaming separately; a blanket “all filesystem reads zero” counter would conflate the two. |

Test the required DLDI-backed route, not only direct-card access, including supported storage layouts that exercise seek cost. Keep source/build hashes and target timing evidence with the result. Percentiles describe observed service quality; also record maxima, outstanding operations and explicitly injected stalls. A P95 does not establish the individual-operation bound needed for interactive admission.

For the source-sized kind domain there are 793 nonempty distinct-kind subsets of size at most four; the existing estimator already enumerates that shape. Extend it with the legal preview variant and instance counts rather than treating the four heaviest independent totals as a proof of every shared-dependency case.[^pack-estimator]

## Final judgment

**The CSS should stop being a loader for the whole gameplay roster. It does not necessarily have to become a streaming screen.** Exact preview-only data, eagerly resident when it fits, is the best DS outcome. A latest-wins lazy service is the principled fallback for that smaller domain.

The storage evidence points toward **name lookup and backing-ROM seek amplification**, not a credible 900 KB throughput threshold, but the timeout remains undiagnosed until an unhalted operation trace distinguishes progress from failure. The discovered shared backing cursor makes “one archive, one open” incomplete as a latency argument.

The concrete sequence is: **trace the underlying operation; pack and index storage; measure the CSS semantic set; select eager or bounded lazy preview residency; exploit the small existing ADPCM menu track where RAM permits; and keep both input and preview publication free of surprise I/O.**

---

## References and evidence boundaries

All project links below are pinned to the reviewed commit. References to prior measurements in source comments are identified as recorded measurements, not new results. The libdvm tag matches the reported version label; the Calico commit is inspected upstream evidence, not a claim about the failing ELF's exact linked revision.

[^revision]: [Reviewed project commit, 2026-09-04](https://github.com/rockenrooster/Smash64DS_Port/commit/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a). Branch comparison used repository branch heads: `p2-pikachu` at `ad4b17c5fd15b17c344e5e864b1c3aef8c7bd039` and `codex/r2-runtime2` at `3e95ab113814603840d9a9009c1b82469892674a`.

[^css-entry]: [CSS entry: eager ladder, four heaps and preparation, `battleship_mnplayersvs.c:475–560`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/import/battleship_mnplayersvs.c#L475-L560).

[^css-proof]: [Original-scene proof setup and second eager loop, `battleship_mnplayersvs.c:1125–1175`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/import/battleship_mnplayersvs.c#L1125-L1175).

[^css-prepare]: [Preview residency helpers, `battleship_mnplayersvs.c:131–372`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/import/battleship_mnplayersvs.c#L131-L372). Initial submotion animation and native owner-image preparation are distinct from the main-closure census.

[^css-sync]: [Preview synchronization, rebuild triggers and recorded zero-payload-I/O regression result, `battleship_mnplayersvs.c:620–820`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/import/battleship_mnplayersvs.c#L620-L820).

[^fighter-census]: [Generated fighter file-size census, `nds_fighter_production.generated.h:7–21`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/include/nds/generated/nds_fighter_production.generated.h#L7-L21). Totals in this review are arithmetic from those rows or explicitly attributed to the supplied problem report, not measured transfer totals.

[^reloc]: [Separate header, extern-ID and payload acquisition paths, `nds_reloc_assets.c:580–870`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/nds/nds_reloc_assets.c#L580-L870).

[^source-preview]: [Source costume selection, selected demo statuses, rotation and fighter construction, `mnplayersvs.c:1500–1695`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayersvs.c#L1500-L1695). This reference was inspected read-only.

[^source-manager]: [Source manager load invariant and closure binding, `ftmanager.c:281–360`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/decomp/BattleShip-main/decomp/src/ft/ftmanager.c#L281-L360).

[^ftmanager]: [DS manager wrapper: generated sizes, deferred effects, native-image ensure and pose release, `battleship_ftmanager.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/import/battleship_ftmanager.c).

[^allocator]: [Bump-region and heap-generation contract, `battleship_sys_malloc.c:1–150`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/import/battleship_sys_malloc.c#L1-L150).

[^bgm-format]: [BGM rate and track metadata, `nds_audio_bgm.h`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/include/nds/nds_audio_bgm.h), especially [Battle Select and packet/buffer constants, lines 255–280](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/include/nds/nds_audio_bgm.h#L255-L280). The durations/rate in §9 are derived from these constants; actual shorter packet durations must use each record's sample count.

[^bgm-seam]: [Timer, channel seam and worker bodies, `nds_audio_bgm.c:625–725`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/nds/nds_audio_bgm.c#L625-L725).

[^bgm-refill]: [Refill implementation and adjacent historical priority comments, `nds_audio_bgm.c:725–940`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/nds/nds_audio_bgm.c#L725-L940). Current call sites, not those historical comments, establish which thread performs reads.

[^bgm-update]: [Current main-thread update/refill call graph, `nds_audio_bgm.c:1290–end`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/nds/nds_audio_bgm.c#L1290).

[^pack-estimator]: [Accepted battle semantic-pack estimator and object/atom methodology, `P2-2-pack-estimator.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/docs/p2/P2-2-pack-estimator.md). CSS uses separate roots and accounting; this review does not override its runtime implementation gate.

[^texture-plan]: [Accepted scene-specific texture/VRAM policy, `P2-texture-residency.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/docs/p2/P2-texture-residency.md).

[^results]: [Results setup/transition/reveal diagnostics, `battleship_mnvsresults.c:55–130`](https://github.com/rockenrooster/Smash64DS_Port/blob/4f6beb07f90cda7a4ce7663fc5b6594360f4af2a/src/import/battleship_mnvsresults.c#L55-L130). Its historical timings are not new measurements or a claim about current Results performance.

[^nitrofs]: [devkitPro libdvm 2.1.0, `source/nitrofs.c`](https://github.com/devkitPro/libdvm/blob/3006b93714b2cac7e052dbc44903b3ef29d2965a/source/nitrofs.c): `_nitroFS_file`, `_nitroFS_open_r`, `_nitroFS_read_r`, `_nitroFS_seek_r`.

[^calico-fd]: [devkitPro Calico, `source/nds/arm9/nitrorom.c`, inspected commit `81b75e314d57ed1784545e28554e567f26f572f1`](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/source/nds/arm9/nitrorom.c): `NitroRomFd`, `_nitroromFdReadImpl`, `_nitroromFdRead`, `nitroromGetSelf`. Confirm the failing build uses this behavior before treating it as deployed evidence.

[^calico-names]: [Calico, same inspected commit, `source/nds/nitrorom.c`](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/source/nds/nitrorom.c): cached file/directory descriptors, `nitroromReadIter`, `nitroromResolvePath`.

[^fat-driver]: [libdvm 2.1.0, `source/fat_driver.c`](https://github.com/devkitPro/libdvm/blob/3006b93714b2cac7e052dbc44903b3ef29d2965a/source/fat_driver.c): FatFs-backed devoptab and read/open implementation.

[^fatfs-fastseek]: [FatFs author documentation: `f_lseek` and fast seek](https://elm-chan.org/fsw/ff/doc/lseek.html). Configuration support and safe integration with the actual linked libdvm/FatFs build must be verified; the documented feature is not evidence that this project currently enables it.

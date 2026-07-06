# Memory Overlay Plan

This is a read-only planning scout for the memory gate needed before adding
fighter and stage breadth. No build, verifier, emulator, or snapshot was run.
Filesystem byte counts come from existing `build/nitrofs` and
`decomp/BattleShip-main/BattleShip_o2r` files; source behavior is cited with
file:line references.

## Current Main-RAM Owners

The current DS taskman arena is an explicit host allocation, not the original
N64 overlay tail. When `NDS_IMPORT_BATTLESHIP_FTMANAGER` is enabled, the arena
is `0x130000` bytes; otherwise it falls back to 1 MiB
(`src/port/diagnostics.c:6624-6628`). The arena is allocated with `calloc`,
rounded to 16-byte alignment, and exposed through `ndsTaskmanArenaStart()` and
`ndsTaskmanArenaSize()` at `src/port/diagnostics.c:6665-6686`. The handoff note
records that `0x130000` was chosen because imported fighter manager plus the
inherited stage proof exceeded the old 1 MiB arena, not because it is a final
strategy (`docs/KNOWN_ISSUES.md:1075-1077`).

Original taskman behavior matters because reloc payloads and scene buffers land
in the general heap. BattleShip initializes the general heap from scene setup in
`decomp/BattleShip-main/decomp/src/sys/taskman.c:267`, allocates through
`syTaskmanMalloc` at `decomp/BattleShip-main/decomp/src/sys/taskman.c:273`,
resets the graphics heap at `decomp/BattleShip-main/decomp/src/sys/taskman.c:281-288`,
and initializes display-list/RDP output buffer state at
`decomp/BattleShip-main/decomp/src/sys/taskman.c:292-383`. The setup fields are
defined in `include/sys/taskman.h:38-55`.

VSBattle's original setup requests:

| Buffer | Bytes | Citation |
|---|---:|---|
| DL buffer 0 per context, `sizeof(Gfx) * 7680` | 61,440 | `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:31-37` |
| DL buffer 1 per context, `sizeof(Gfx) * 2560` | 20,480 | `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:31-38` |
| Context count | 2 | `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:31-35` |
| Graphics heap per context, `0xD000` | 53,248 | `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:39` |
| RDP output buffer, `0xC000` | 49,152 | `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:40-41` |

That is 270,336 bytes for two contexts of DL0 + DL1 + graphics heap, plus
49,152 bytes for the RDP output buffer. These allocations are part of the scene
taskman budget and should not be double-counted against the explicit arena if
the DS port keeps them inside the arena.

The DS renderer itself does not add a large always-on framebuffer allocation in
the current source path. Hardware triangles are off by default at
`src/nds/nds_renderer.c:7-11`. The optional hardware texture scratch is behind
that flag and would be 128 * 128 * `u16` = 32,768 bytes if enabled
(`src/nds/nds_renderer.c:72-75`, `src/nds/nds_renderer.c:159-160`). Traversal
state is a per-render stack/local structure beginning at
`src/nds/nds_renderer.c:163`.

## Current Battle Reloc Payloads

The Makefile stages the current battle-relevant NitroFS reloc sets from Pupupu
stage files, stage-scout files, Mario/Fox fighter files, and VSBattle/interface
files at `Makefile:571-682`. Reloc payloads are staged into the `.nds` through
the NitroFS rule at `Makefile:743`.

Existing `build/nitrofs/reloc` file sizes measured during this scout:

| Group | Files | Bytes | Notes |
|---|---:|---:|---|
| Pupupu active stage set | 5 | 203,348 | `GRPupupuMap`, `StageDreamLand`, `ExternDataBank103`, `ExternDataBank104`, `MiscDataBank152`; source list at `Makefile:571-576`. |
| Mario/Fox fighter slice | 63 | 267,582 | `FTManagerCommon`, Mario/Fox core files, selected Mario/Fox animations, and selected extern data; source list starts at `Makefile:584`. |
| VSBattle/interface set | 9 | 209,456 | `IFCommon*` and `SYKseg1Validate`; source list at `Makefile:649-653`. |
| Total current battle staged payloads | 77 | 680,386 | Filesystem metadata under `build/nitrofs/reloc`; no build was run. |

These file bytes are a conservative resident-pressure indicator, not a separate
arena allocation total. The current reloc loader reuses an existing loaded file
when possible, otherwise uses a static buffer or `syTaskmanMalloc(asset_size,
0x10)` at `src/port/reloc_backend_assets.c:1985-2013`, so most live loaded
reloc assets consume the taskman arena.

VSBattle naturally loads the interface/common files before battle start:
`scVSBattleSetupFiles` calls `lbRelocLoadFilesListed(dGMCommonFileIDs,
gGMCommonFiles)` at
`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattlefiles.c:23-37`.
`dGMCommonFileIDs` lists eight interface files in
`decomp/BattleShip-main/decomp/src/gm/gmcommon.c:11-20`.

Fighter loading is also naturally resident today. VSBattle allocates fighter
manager data at `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:162`,
sets up each player kind at
`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:177`, and gives each
spawned fighter a figatree heap from
`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:199`. The fighter
manager loads common files through `syTaskmanMalloc` at
`decomp/BattleShip-main/decomp/src/ft/ftmanager.c:166-168`, loads each main
fighter file at `decomp/BattleShip-main/decomp/src/ft/ftmanager.c:285`, and
loads status-buffer files in `decomp/BattleShip-main/decomp/src/ft/ftmanager.c:306-332`.
It computes the figatree heap from the largest animation file size at
`decomp/BattleShip-main/decomp/src/ft/ftmanager.c:201-206` and allocates the
per-kind figatree heap at `decomp/BattleShip-main/decomp/src/ft/ftmanager.c:364`.

For the current staged Mario/Fox subset, the largest NitroFS animation payload
is `FTMarioAnim029` at 3,264 bytes. That is a current lower-bound figatree
scratch size for this slice. If future setup points `gSCManagerFighterFileSizes`
at broader animation sets, the O2R max-file table below is the safer budget
source.

## DS 4 MiB Budget

Retail DS main RAM is 4,194,304 bytes. The current explicit battle arena is
1,245,184 bytes. Current staged battle payload files sum to 680,386 bytes; most
of that is expected to live inside the arena when loaded through
`syTaskmanMalloc`, so the important number is arena occupancy, not arena plus
payloads as separate RAM.

| Owner | Current bytes | Budget interpretation |
|---|---:|---|
| DS main RAM total | 4,194,304 | Retail hardware target. |
| Taskman arena capacity | 1,245,184 | Explicit scene heap capacity from `src/port/diagnostics.c:6624-6686`. |
| VSBattle taskman renderer buffers | 319,488 | Two contexts of DL0/DL1/graphics heap plus RDP output, source-sized from VSBattle setup. |
| Current battle reloc payload files | 680,386 | Existing NitroFS bytes that are likely arena occupants once loaded. |
| Current figatree scratch lower bound | 3,264 per active fighter | Largest staged Mario/Fox animation file; future full-set maxes are larger. |
| Optional HW texture scratch | 0 now, 32,768 if enabled | Behind `NDS_RENDERER_HW_TRIANGLES`; defaults off. |

With the current 1,245,184-byte arena, renderer buffers plus current battle
payloads consume about 999,874 bytes before allocator overhead, fighter/object
runtime allocations, particles, weapons, effects, and scene allocations. That
leaves roughly 245 KiB of arena headroom on this scout's numbers. This is enough
for small proof slices, but not enough for broad fighter/stage residency.

The extra 4 MiB DSi/debug-cart option changes what can be cached, not what the
retail plan should require. With 8 MiB, the port could keep more decompressed
fighter cores, one extra stage set, or a converted audio cache resident for
debugging. It still should stream/evict on scene boundaries because retail DS
must remain the correctness target.

## Fighter Projection From BattleShip_o2r

Core fighter payloads below are filesystem metadata from
`decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main`. A core set means
`Main`, `MainMotion`, `Model`, `ShieldPose`, and `Special*` files only; it does
not include per-animation files.

| Fighter | Core bytes | Files |
|---|---:|---:|
| Mario | 52,474 | 7 |
| Fox | 69,124 | 8 |
| Donkey | 77,144 | 5 |
| Captain | 100,848 | 6 |
| Kirby | 156,626 | 5 |
| Link | 105,300 | 7 |
| Luigi | 42,054 | 4 |
| Ness | 77,694 | 7 |
| Pikachu | 78,960 | 7 |
| Purin | 70,800 | 5 |
| Samus | 83,986 | 7 |
| Yoshi | 65,716 | 6 |
| Boss | 15,392 | 3 |

Full animation payloads are much larger, also from filesystem metadata under
`decomp/BattleShip-main/BattleShip_o2r/reloc_animations`:

| Animation set | Total bytes | Files | Largest file |
|---|---:|---:|---:|
| Captain | 446,992 | 152 | 10,864 |
| Donkey | 351,552 | 153 | 11,648 |
| Fox | 367,744 | 158 | 4,976 |
| Kirby | 406,464 | 186 | 7,888 |
| KirbyCopy | 166,384 | 59 | 8,320 |
| Link | 336,304 | 144 | 7,408 |
| Luigi | 36,480 | 12 | 6,800 |
| Mario | 360,320 | 143 | 6,304 |
| MasterHand | 86,080 | 34 | 13,952 |
| Ness | 418,080 | 151 | 8,528 |
| Pikachu | 407,072 | 141 | 10,832 |
| Purin | 35,952 | 8 | 7,520 |
| Samus | 323,600 | 150 | 6,720 |
| Yoshi | 395,056 | 142 | 9,440 |

Projection:

| Addition | Core-only bytes | Full animations too | Budget result |
|---|---:|---:|---|
| Add Link + Samus cores | 189,286 | +659,904 | Core-only is plausible; all animations are not. |
| Add four typical cores, e.g. Donkey + Link + Samus + Yoshi | 332,146 | +1,406,512 | Requires active-fighter core residency plus animation streaming. |
| Add Kirby core | 156,626 | +406,464 | Kirby is the largest core and should be a dedicated memory gate. |
| Load all playable full animation sets | About 0.98 MiB cores plus several MiB animations | Not viable | Must be per-scene/per-fighter streaming. |

The port should never load all per-fighter animation files resident for a
breadth milestone. Keep active fighter cores resident, and stream or prefetch
only the animation/status groups needed by the live state machine.

## Stage Projection From BattleShip_o2r

Stage map/visual pairs below are filesystem metadata from
`decomp/BattleShip-main/BattleShip_o2r/reloc_stages`. These totals exclude
stage-specific external banks until each original stage setup proves the exact
`ExternDataBank*` and `MiscDataBank*` files it needs.

| Stage | Map | Visual payload | Map + visual bytes |
|---|---|---|---:|
| Castle | `GRCastleMap` | `StageCastle` | 159,298 |
| Hyrule | `GRHyruleMap` | `StageHyruleWallpaper` | 159,322 |
| Inishie | `GRInishieMap` | `StageInishieWallpaper` | 159,484 |
| Jungle | `GRJungleMap` | `StageJungle` | 159,328 |
| Last | `GRLastMap` | `StageLastWallpaper` | 159,276 |
| Metal | `GRMetalMap` | `StageMetalWallpaper` | 159,308 |
| Pupupu | `GRPupupuMap` | `StageDreamLand` | 159,298 |
| Sector | `GRSectorMap` | `StageSector` | 159,410 |
| Yamabuki | `GRYamabukiMap` | `StageYamabukiWallpaper` | 159,966 |
| Yoster | `GRYosterMap` | `StageYoshi` | 159,300 |
| Zebes | `GRZebesMap` | `StageZebes` | 159,326 |

Current Pupupu proves why map + visual is not the whole stage budget: the staged
Pupupu active set is 203,348 bytes once `ExternDataBank103`,
`ExternDataBank104`, and `MiscDataBank152` are included. The current stage-scout
set also stages `GRInishieMap`, `GRHyruleMap`, `StageCastle`, and
`ExternDataBank113` through `Makefile:578-582`, but those are scout assets, not
a final resident multi-stage strategy.

Useful external-bank sizes from `BattleShip_o2r/reloc_extern_data` filesystem
metadata:

| Bank | Bytes |
|---|---:|
| `ExternDataBank103` | 12,304 |
| `ExternDataBank104` | 17,586 |
| `ExternDataBank105` | 57,264 |
| `ExternDataBank106` | 17,776 |
| `ExternDataBank107` | 27,872 |
| `ExternDataBank108` | 63,024 |
| `ExternDataBank109` | 47,200 |
| `ExternDataBank110` | 21,120 |
| `ExternDataBank111` | 47,572 |
| `ExternDataBank112` | 66,240 |
| `ExternDataBank113` | 26,848 |
| `ExternDataBank114` | 76,208 |
| `MiscDataBank152` | 14,160 |
| `MiscDataBank153` | 7,764 |
| `MiscDataBank154` | 1,792 |
| `MiscDataBank155` | 5,224 |
| `MiscDataBank156` | 144 |
| `MiscDataBank157` | 3,616 |
| `MiscDataBank158` | 3,376 |
| `MiscDataBank159` | 11,056 |

Projected active stage cost is therefore about 159 KiB plus 0-90 KiB of extern
banks for ordinary stages, with Pupupu's current exact active set at about
199 KiB. Two resident stages would waste 160-250 KiB that should be reserved for
active fighters, animation streaming, and audio.

## Streaming And Eviction Strategy

Use the scene boundary as the hard ownership line. Menus, opening movie assets,
and previous stage/fighter payloads should be evicted before VSBattle begins.
Within battle, keep only these resident classes:

| Class | Residency rule |
|---|---|
| Scene common/interface | Load once for VSBattle; evict on scene exit. |
| Active stage | One map + one visual payload + proven extern banks. Evict before loading another stage. |
| Active fighter cores | Keep core files for fighters that can spawn in the current match. Do not load non-participating fighters. |
| Animation payloads | Stream by status group or maintain a tiny LRU of current/next animations. Size figatree heaps to the largest loaded animation needed by the active kind. |
| Effects/items/weapons | Add only when their original manager TUs are imported; gate each manager against arena high-water marks. |
| Audio | Do not load raw `B1_sounds1.tbl` and `B1_sounds2.tbl` resident with battle. The audio scout shows those two raw sample tables alone are about 4.14 MiB. Audio must stream, convert, or cache selectively. |

Implementation shape:

1. Add an explicit reloc cache generation for scene ownership. A file loaded for
   menu/opening must not remain pinned by pointer identity when battle starts.
2. Make stage selection produce a stage asset manifest from original setup data:
   map, visual payload, extern banks, hazard extras.
3. Make fighter selection produce a fighter asset manifest: core files, common
   files, selected starting animation groups, and max figatree scratch.
4. Load manifests into the taskman arena and record high-water usage. Reject a
   slice if it cannot prove enough arena headroom for object/runtime allocation.
5. Add debug-cart/DSi optional cache only as an optimization layer. Retail 4 MiB
   behavior must stream/evict the same way.

Status: slices 1 and 2 landed in the live port. The current mode-163 ledger
reports arena headroom `207900`, resident reloc `653968` bytes
(`stage=202816`, `fighter=242480`, `if=208672`), stale menu/opening bytes
`0/0`, and direct-route last eviction `0/0`.

## Effect Manager Gate Decision

BattleShip `efManagerInitEffects` allocates the effect struct pool, creates the
effect display roots, and unconditionally loads `EFCommonEffects1/2/3`
(`decomp/BattleShip-main/decomp/src/ef/efmanager.c:1734,1754-1756`). Those
three common effect banks add 94,944 resident bytes. Current mode-163 headroom
is about 208 KiB, only about 77 KiB above the fixed 128 KiB reserve, so loading
the full common effect bank set would miss the reserve by roughly 18 KiB.

For the effect-manager gate, grow the explicit taskman arena from `0x130000` to
`0x150000` and keep the reserve assertion unchanged. This is still within the
retail DS 4 MiB target; current renderer/DL/RDP buffers are already ledgered
from VSBattle setup, and the hardware texture scratch remains opt-in rather
than an always-resident arena owner. The pre-gate mode-163 ledger reported
`head208224 reloc646352`; after the effect-manager and reflector defaults it
reports `head240332 reloc747472 stage202816 fighter241280 if208672 stale0/0`.

Common particle script/texture banks stay non-resident in this slice. They are
about 326 KiB together and are reached from `efDisplayInitAll` through
`efParticleGetLoadBankID`; keep particle calls on diagnostic/no-op shims until
a dedicated particle asset gate can stream or budget them explicitly.

## Gate Before Breadth

Before adding more fighters or stages, the port needs a memory gate that reports
at least:

| Measurement | Required source |
|---|---|
| Taskman arena capacity and high-water used | DS taskman seam plus original `syTaskmanMalloc` region. |
| Resident reloc payload bytes by owner | Reloc loader registration table. |
| Current scene renderer/taskman buffers | Scene setup fields from original taskman setup. |
| Active fighter core bytes and animation cache bytes | Fighter manifest and `gFTManagerFigatreeHeapSize`. |
| Active stage bytes and extern-bank bytes | Stage manifest. |
| Audio cache bytes | Audio backend manifest once audio work starts. |

This gate should be a natural-runtime verifier/capture, not a one-bit branch
proof. It should fail when arena headroom drops below a fixed reserve, e.g.
128 KiB for runtime object churn on current slices, with the reserve revisited
after item/effect/weapon managers become live.

## Proposed /task-Sized Slice Sequence

1. Done: `/task Memory ledger gate`: Add a read-only runtime memory ledger for battle
   that reports taskman arena capacity, high-water usage, loaded reloc owners,
   renderer/taskman buffer sizes, active fighter payload bytes, stage payload
   bytes, and figatree heap size. Gate current Mario/Fox/Pupupu without changing
   gameplay.
2. Done: `/task Scene-owned reloc eviction`: Add scene generation ownership to the
   reloc cache and evict menu/opening/stage-scout payloads before VSBattle owns
   the arena. Gate by proving current battle still loads naturally and stale
   scene files are not retained.
3. `/task Active stage manifest`: Replace staged multi-stage residency with one
   selected stage manifest. Start with Pupupu exact files, then add Hyrule or
   Inishie by importing the relevant original setup data and proving only that
   stage's assets are resident.
4. `/task Active fighter manifest`: Keep Mario/Fox as the first manifest, then
   add one new fighter core plus selected startup/wait/run/jump/damage animation
   groups. Gate on arena headroom and figatree max-file sizing before expanding
   to the next fighter.
5. `/task Animation streaming cache`: Replace resident status animation piles
   with a tiny current/next-status cache and LRU eviction. Gate on natural
   fighter status transitions rather than synthetic one-branch proofs.

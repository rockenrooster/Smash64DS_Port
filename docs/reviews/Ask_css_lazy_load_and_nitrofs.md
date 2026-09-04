# Hard problem: the character select must go lazy, and the DS read path may not survive it

Status: open, and now blocking. Written 2026-09-04 for external review.

This is the third residency problem, and it is **not** either of the two already
answered. `Review_Deriving_Fighter_Live_After_Setup_Set.md` covers the 4 MB main
RAM battle pack. `Review_DS_Texture_VRAM_Residency.md` covers VRAM and the
texture cache. This one is about **storage**: what the character select reads,
when it reads it, and whether the DS file path can do it at all.

## What happened

Adding a tenth fighter to the shipped ROM made the Boundary shell lap hang. The
harness walked `Startup(27) → Title(1) → ModeSelect(7) → VSMode(9)`, reached
step 14 — the transition into the character select — and stopped there with the
ARM9 parked in:

```
0x020ecf0c in get_fat (clst=14918, obj=0x2294cec)
    at .../libfat/src/libdvm-2.1.0/fatfs/source/ff.c:1181
```

It never came back, on the run **or** its retry: two consecutive 3000-second
harness timeouts. The arena was healthy throughout — `LOOPF 3` free 910,504,
high-water 387,928 — so this is not the allocator. It is inside the filesystem,
walking a FAT cluster chain.

## Why the character select reads so much

`ndsMNPlayersVSSetupFighterFiles`
(`src/import/battleship_mnplayersvs.c:496-527`) calls
`ftManagerSetupFilesAllKind` once for **every admitted fighter**, so the select
screen loads the entire roster before it can draw. Main-closure bytes, from
`include/nds/generated/nds_fighter_production.generated.h:10-21`:

| Roster | CSS-resident closure bytes |
| --- | ---: |
| P1 pair | 173,088 |
| nine fighters (shipped) | 832,288 |
| ten fighters | **904,656** ← hangs |
| all twelve | 1,188,080 |

**The source does this too.** `decomp/.../mn/mnplayers/mnplayersvs.c:4759-4762`
is literally `for (i = nFTKindPlayableStart; i <= nFTKindPlayableEnd; i++)
ftManagerSetupFilesAllKind(i);`. The N64 could afford all twelve resident; the
DS cannot. So going lazy is a sanctioned DS adaptation, not a divergence — but
the source gives no guidance on how, because it never had to.

Beyond the closures, the screen also warms one animation file per admitted kind
(`battleship_mnplayersvs.c:143-172`, `ndsR2AnimCachePreloadFighterFile`), and
Jigglypuff in particular borrows 77 of Kirby's animation files.

## The thing I most want judged: is this bytes, or is it files?

The port reads every reloc asset as an **individual file**:
`fopen(entry->path, "rb")` at `src/nds/nds_reloc_assets.c:620, 662, 730, 802,
1182`. The manifest counts **1,916 NitroFS files across the twelve fighters**,
150-200 each.

904,656 bytes is not a lot. It should not take 3000 seconds. Two very different
explanations, with very different fixes:

1. **Per-byte cost** — the reads are simply slow and the total is genuinely
   large. Fix: read less, or read it earlier.
2. **Per-file cost** — each `fopen` walks a path and traverses a FAT chain, and
   the cost is superlinear in file count or in offset. The hang being *inside*
   `get_fat` points here. Fix: stop having hundreds of files. One packed
   archive with a generated offset index, opened once.

I cannot separate these from the evidence I have, and I would rather be told
which than guess. **A confound I want named rather than hidden:** the measurement
was taken under gdb with the emulator attached, and this project has a recorded
case of a gdb-halted emulator reading as a total performance collapse. I do not
know how much of the 50 minutes is instrumentation. What I *am* confident of is
the shape — it is stuck in FAT traversal, and it did not finish twice.

There is also a DLDI dimension: DLDI is required for retail parity and is
measured to cost about 29,696 ticks at P95 in battle, so the storage path is
already known to be expensive here.

## The design problem, if lazy is the answer

A specification pass produced a concrete change list — delete the eager ladder,
convert the prepare-resident helpers to `ensure(kind)` on selection change, add
the eviction path that does not exist today (`ftManagerSetupFilesAllKind` only
ever loads), keep the four `figatree_heap`s, and hold residency at N >= live
preview slots rather than N = 1, LRU-evicting only non-visible kinds.

**But it also found the hard part, and it is not the eviction.** The character
select lets the player sweep a cursor across the roster, and
`mnPlayersVSPuckProcUpdate:3560-3566` calls `UpdateFighter` on *every* kind
change. Today that rebuild is zero-I/O because everything is resident. Lazy
loading puts a NitroFS read **inside a frame**, during a screen that is also
streaming BGM off the same device. A fast sweep across twelve portraits would
issue twelve loads.

So the real question:

> **How should a DS character select present a 3D preview per fighter without
> holding the whole roster resident, when the storage path is slow enough to
> hang, the player can change selection every frame, and audio is streaming from
> the same device?**

Candidate directions I can see, none of which I am confident enough to build:

- **Decouple selection from residency**: portraits and name plates always
  resident (they are small); the 3D preview loads only after the cursor *dwells*,
  with a placeholder until it arrives.
- **Asynchronous, budgeted loading**: a fixed byte budget per frame, so a load
  spans frames and never blocks one. Needs an interruptible reader, which
  `fopen`/`fread` is not.
- **Predictive prefetch** along cursor direction.
- **A different storage shape**: one packed per-fighter blob, or one packed
  roster blob, with a generated index — turning N opens into one open plus N
  seeks, and making the preview load a contiguous read.
- **Preview from something cheaper than the fighter closure**: a pre-rendered or
  reduced preview asset generated offline, so the select screen never loads
  gameplay data at all.

## Sub-questions

1. **Files or bytes?** Which explanation fits a `get_fat` hang at ~900 KB across
   roughly 80-150 opens, and what measurement would settle it cheaply?
2. **Storage shape.** Is a packed archive with a generated index the right answer
   for this project generally — not just for the character select but for the
   match-resident pack the other review prescribes? Would that make the two
   problems one?
3. **Latency hiding.** Which of the directions above is right for a screen where
   selection can change every frame, and what does the player actually see while
   a preview loads? What does SSB64 itself do that we can keep the feel of?
4. **Audio interaction.** BGM streams from the same device during this screen.
   How do a lazy loader and a streaming audio reader share it without either
   starving?
5. **Is lazy even the right lever?** The alternative is to make the *resident set*
   small enough that eager still works — a reduced preview representation for all
   twelve, rather than full closures for a few. Which is the better DS answer?
6. **Scope.** Does this same treatment need to reach stage select, Results, and
   the 1P campaign's interstitials, or is the character select uniquely bad?

## What is already decided, so it need not be re-argued

- Runtime paging **during gameplay** is refused (first review). This is a menu
  screen with legal load boundaries, which is a different question.
- The battle-side four-kind residency answer is a generated semantic pack
  (`docs/p2/P2-2-pack-estimator.md`).
- Texture and VRAM residency becomes a deterministic pre-`GO` scene plan
  (`docs/p2/P2-texture-residency.md`).

## Repository orientation

- Eager ladder: `src/import/battleship_mnplayersvs.c:496-527`; its proof twin at
  `:1156-1159`; preview residency helpers `:131-372`; preview sync `:641-802`;
  puck update in the decomp at `mnplayersvs.c:3560-3566`.
- Load seam: `ftManagerSetupFilesAllKind` /
  `ftManagerSetupFilesMainKind`, `decomp/.../ft/ftmanager.c:281-360`;
  the allocation is `:285`.
- File opens: `src/nds/nds_reloc_assets.c:620, 662, 730, 802, 1182`.
- Per-fighter closure and NitroFS file counts:
  `scripts/fighters/fighter_production_manifest.json`.
- `decomp/` is read-only reference throughout.

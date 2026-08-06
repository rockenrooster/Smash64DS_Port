# CSS Panel Backdrop Garbled — Investigation Handoff (2026-04-29)

**Issue:** [#3 Character Select backdrop behind each port renders garbled
sometimes](https://github.com/JRickey/BattleShip/issues/3) — reporter
demonstrates garble that varies depending on what HMN/CPU changes were
made before re-entering VS mode.

**Status:** Not fixed in this pass. Symptom is state-dependent and
needs in-game reproduction before any change is made. This document
captures the leading theories and the code paths a future session
should instrument.

## Reproduction signal from the reporter's video

Frame extracted at ~5s shows the four port slots:
- 1P (Samus, HMN): backdrop renders nearly featureless gray instead of
  the saturated red/blue/yellow/green seen on N64.
- 2P (Kirby, HMN): backdrop is a horizontal-striped pattern that looks
  like a different sprite's bitmap data — *not* the panel sprite.
- 3P (Mario, CP): partial pattern, somewhat correct color tone.
- 4P (Pikachu, CP): close to the N64 reference (green CP card).

The pattern of "first slot mostly gray, middle slots stripe-garbled,
last slot ok" plus "depends on prior HMN/CPU toggles" reads like a
texture-cache or palette-staging staleness, not a one-shot fixup
mismatch.

## What the game does

`mnPlayersVSMakeGate(player)` in `src/mn/mnplayers/mnplayers/mnplayersvs.c`:

1. Loads `llMNPlayersCommonRedCardSprite` as the panel SObj source —
   the *same* Sprite for all four players.
2. Calls `mnPlayersVSSetGateLUT(panel, color_id, pkind)` which **only
   swaps the LUT field** to recolor the panel:
   ```c
   SObjGetSprite(sobj)->LUT =
       PORT_REGISTER(lbRelocGetFileData(void*, sMNPlayersVSFiles[0],
                                        man_offsets[player]));
   ```
3. Each HMN↔CP toggle and each team-color change calls
   `mnPlayersVSSetGateLUT` again with a fresh `PORT_REGISTER` →
   accumulates new tokens, never reuses or frees the old ones.

Render side: `lbCommonPrepSObjAttr` emits a runtime
`gDPLoadTLUT(... PORT_RESOLVE(sprite->LUT))`. Fast3D's `GfxDpLoadTlut`
calls `portRelocFixupTextureAtRuntime(src, byteCount)` (idempotent) to
un-BSWAP32 the palette before staging it.

## Theories ranked by likelihood

### Theory A — libultraship texture cache stale across LUT swap

The cache key in libultraship is `{addr, fmt, siz, sizeBytes, masks,
maskt, w, h}`. When we swap LUT but keep the same panel bitmap, the
*bitmap* cache entry is reused (correctly) but the renderer composites
that bitmap with whichever palette is currently staged. If the staging
side has a stale palette pointer left over from a prior frame, the
panel bitmap shows up colored with the wrong LUT.

To validate: instrument `GfxDpLoadTlut` to log
`{addr, byteCount, first 16 bytes post-fixup}` and correlate with the
panel draws. If the same panel draw cycles through different LUT
addresses across frames but the rendered output sticks at the first
one, this is the bug.

### Theory B — `mnPlayersVSSetGateLUT` writing to a Sprite that gets re-fixed

`SObjGetSprite(sobj)` returns `&sobj->sprite`, the SObj's per-instance
copy. That copy is the one rendered. So the LUT write is local to the
SObj — which should be safe.

But: `lbCommonPrepSObjAttr` reads `sprite->bmfmt` to decide if a LUT
load is needed (`G_IM_FMT_CI` branch). The Sprite copy was set up by
`gcAddSObjForGObj` from the source RedCard Sprite. If the source's
`bmfmt` is wrong (because of a missed pass2 fixup or because the file
was reloaded but `lbCommonPrepSObjAttr` reads sobj->sprite from the
*old* layout), the LUT load wouldn't happen and the panel would render
with whatever palette was staged previously.

To validate: log `sobj->sprite.bmfmt` at every panel draw and verify
it's `G_IM_FMT_CI` (`= 2`). If it ever reads non-CI, Sprite struct
corruption is the issue.

### Theory C — Heap reuse stale fixup idempotency

`lbRelocLoadAndRelocFile` evicts `sStructU16Fixups` for the heap range
before memcpy (see `port/bridge/lbreloc_bridge.cpp:378`). Should
prevent stale fixups, but the panel sprites are loaded into a heap
that's reused across CSS visits, so a missed eviction would leave
`portFixupSprite` skipping the new load → fields read wrong.

To validate: add a "skipped due to idempotency" counter inside
`portFixupSprite` and dump it on each scene start. If it's nonzero for
mnPlayersVS files on a re-entry, the eviction range is missing
something.

### Theory D — Pointer-table generation crossing during scene reset

`portRelocResetPointerTable` increments `sGeneration` (mod 0xFF) on
each scene transition. If a token registered in the previous CSS
visit is still referenced by some long-lived data — e.g., a static
SObj that doesn't go through the scene heap — its `PORT_RESOLVE`
returns NULL. The renderer would silently load palette index 0 or
similar.

To validate: enable `portRelocResolvePointerDebug` for the panel SObj
draws and watch for `invalid/stale token` warnings tied to LUT
addresses.

## Where to start

1. Run the build with `SSB64_TEX_FIXUP_LOG=1` and reproduce the bug
   (re-enter VS mode several times with HMN/CP toggles between).
2. Diff the per-frame `tex_fixup.log` entries for the panel sprite's
   LUT range against a clean first-entry. If LUT bytes differ between
   visits at the same addr, theory A or C; if the LUT load entry
   stops appearing, theory B.
3. The reporter already attached an `ssb64.log` and a `gbi trace`
   (`/tmp/css.log`, `/tmp/css.gbi` if you grabbed them) — these were
   captured *during* the garbled state and span four CSS re-entries
   (scene 16). Compare frame-zero TexRect / LoadTlut sequences in
   FRAME 0 vs FRAME 4 of the trace; they should be byte-identical
   once you mask out the always-changing pointer-table tokens.

## Caveats

- The `ssb64.log` from the reporter doesn't show any error or
  warning; the renderer is silent on the bug. Diagnostic instrumentation
  is required.
- The Mupen+GLideN64 reference image has fully-saturated panel colors
  on the very first CSS entry, which the port does *not* — even on a
  fresh launch, the colors look slightly off (compare the gray 1P
  Samus card in the bug video frame to the saturated red 1P card on
  N64). So part of the bug may be present from frame 1, not just
  after re-entries; the re-entries amplify it.
- Skipping a "patch and pray" fix here. Without an in-process repro
  and a confirmed root cause, any change risks breaking a working
  path (e.g., the wallpaper-cache work in `dk_intro_wallpaper_*.md`).

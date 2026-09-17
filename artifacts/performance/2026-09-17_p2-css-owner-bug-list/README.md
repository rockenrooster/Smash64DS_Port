# The owner's seven CSS defects: four causes, and three of them are one screen's load policy

Owner, 2026-09-17, verbatim:

```
-CSS Bugs still present and need to be fixed:
    -Low FPS/Flashing during gate openings
    -fighter 3d previews not visible for:
        -Yoshi
    -music pauses/reset when rendering new 3d fighter previews (moving around cursor)
    -delay between cursor hover and 3d fighter preview rendering.
    -Kirby not selectable
    -Jigglypuff not selectable
    -Ness not selectable
```

Seven reports, four distinct causes. **Every number below is measured**, from one
shipping-configuration probe of the character select, not inferred.

## The instrument

`scripts/menus/probe-p2-shell.ps1`, the shipping-cadence shell walk
(`NDS_HARNESS_FAST_LOGIC=0`, `NDS_RENDERER_PROFILE_LEVEL=0`), stop 5 =
`PlayersVS`. One CSS visit, **1,651 presented frames**. Ticks are bus cycles, so
one 60 Hz frame is **558,566**.

## Cause 1 — Kirby / Jigglypuff / Ness are gated by a note whose own release condition was met five days later

`Makefile:716` pins `NDS_P2_SHELL_ROSTER ?= 7`. The ladder beneath it had rungs
for eight fighters and **no rung for Kirby or Ness at all**; Jigglypuff sat on
rung 8, unreachable. The comment explaining that is dated 2026-09-04:

> BACK TO 7. Rung 8 (Jigglypuff) makes the character select read ten full
> fighter closures from NitroFS — 904,656 B against 832,288 at rung 7 — and
> the Boundary shell lap then hung inside libfat `get_fat` […] **The eager
> load is the real defect, not Jigglypuff** […] **Raise this again once the
> character select stops loading every roster member at once.**

That condition was satisfied on **2026-09-09** by `d99a89f8741`, *"Give
character select four fighter slots instead of the whole roster"*, with
`435ebf00d6d` and `11014557337` either side of it. CSS entry now takes four
fixed 80 KiB closure blocks (`NDS_PLAYERS_VS_RESIDENT_BLOCKS`), one 64 KiB
shared tree, and an animation-cache arena reserved by
`ndsR2AnimCacheReserveCSSWorkingSet` — which is `ndsR2AnimCacheArenaEnsureSetup`
and nothing else. **No CSS entry allocation scales with roster size any more.**
The ladder was simply never re-raised.

Extended to rung 9 (Ness) and rung 10 (Kirby).

## Cause 2 — the shipping CSS has the blocking loader; the sliced one is compiled out

This is the single most consequential finding, and it explains **two** of the
owner's reports at once.

`NDS_PLAYERS_VS_COMPACT_PREVIEW` is 1 exactly when
`NDS_RENDERER_HW_TRIANGLES && NDS_RENDERER_PROFILE_LEVEL < 2` — which is the
shipping shell target. In `ndsMNPlayersVSPreviewAcquireResidentKind`
(`battleship_mnplayersvs.c`), that arm runs the whole closure in **one frame**:

```c
ndsAudioBgmSuspendForBlockingLoad();          /* :1094 */
ftManagerSetupFilesAllKind(fkind);            /* the entire closure */
... ndsMNPlayersVSPreviewPrepareResidentKind(...);
ndsAudioBgmResumeAfterBlockingLoad();         /* :1116 */
```

The `#else` arm — the one that does **not** ship — is fully sliced:
`ndsRelocExternTreeSliceBegin` / `SliceStep` at
`NDS_PLAYERS_VS_LOAD_CHUNK_BYTES` (8 KiB) and `..._CHUNK_NODES` (4) per CSS
tic, with the bind deferred to its own final tic so prepare never rides the
last payload tic.

**The good loader exists and only the oracle/profile build gets it.**

### What that costs, measured

| | value | in 60 Hz frames |
|---|---:|---:|
| worst CSS frame (`MSMAX w3`) | **4,808,448** | **8.61** |
| of which preview phase (`CSSPHASE preview` max) | **4,799,488** | 8.59 |
| panel/door sync phase max (`CSSPHASE sync`) | **3,244,224** | 5.81 |
| update phase max (`CSSPHASE update`) | 2,444,352 | 4.38 |

`preview max / frame max = 99.8%`: **the worst frame on the character select is
a preview closure load and essentially nothing else.**

### Cadence, from the VBlank histogram

`MSVB3 5 1492 113 35 11 max=9` — 1,492 frames took one VBlank, 113 took two,
35 took three, 11 took four or more, and the worst took **nine**.

**159 of 1,651 CSS frames (9.6%) miss 60 Hz, and the worst frame presents at
6.7 FPS.** That is the owner's "Low FPS" as a number.

### The music, as a count

`CSSBGMFENCE 5 suspend=8 resume=8 seammiss=0 error=0`

**Eight suspend/resume pairs in one CSS visit.** They are balanced and nothing
underran — so the stream never dies, which is why the owner says "pauses"
rather than "stops". But `ndsAudioBgmSuspendForBlockingLoad` calls
`ndsAudioBgmKillSound()`, and resume re-reads *two* packets and restarts the
hardware from the stream cursor. The audible gap is at minimum the load frame
itself, which the table above puts at up to 8.6 frames = **143 ms**, plus the
resume. Eight times per visit.

The fence is correct and was added for a real reason (`nds_audio_bgm.c:2181`
documents the measured alternative: seam misses 1, error stops 1, music gone
for the rest of the screen). **The defect is the blocking load it is protecting
against, not the fence.**

### The hover delay, as a count

`NDS_PLAYERS_VS_PREVIEW_DWELL_TICKS` is **13**, and its comment says why: a
portrait cell is 45 source pixels, the cursor advances 4 px/tic, so a cell can
be the requested kind for at most 12 consecutive tics at full browsing speed —
requiring a 13th means **an uninterrupted sweep never starts a load at all**.

That is a sound debounce for a *blocking* load and it is exactly what the owner
feels. Measured: `CSSRESREL 5 … dwell=9/3/70/6` — 9 requests, 3 skipped,
**70 tics held**, 6 committed. 70 tics is **1.17 s of deliberate waiting** in
one visit, before any load begins.

And the loads it gates do not amortise: `CSSRESACT 5 acq=10 hit=0 cached=0
load=7 finish=7 … retry=3 fail=0` against `CSSRESREL 5 rel=5 retire=7/7
reuse=3 exit=4`. **Ten acquires, zero cache hits, seven loads, seven retires.**
Every acquire missed and nothing survived to be reused.

Slicing the compact arm the way the non-compact arm is already sliced addresses
the load frame, the eight BGM gaps and the reason the dwell has to be 13, all
from one change.

## Cause 3 — the door slide re-reads the whole panel from NitroFS, per slot, per tic

`ndsMenuShellCssStepDoors` (`nds_menu_shell_css.c:891`) slides `door_offset` by
2 per tic between 0 and 41, so ~21 tics per gate. On **every mid-slide tic**, for
**every sliding slot**, it re-blits the slot's entire panel as the underlay and
then draws both door halves over it:

```c
blit = sCssPanelSurface[i];
if (ndsUiKitBlitSurfaces(&blit, 1u) != FALSE) { gNdsMenuShellCssPanelBlitCount++; }
... ndsUiKitDrawCachedSub(...) x2
```

`ndsUiKitBlitSurfaces` has **no cache**. `ndsUiKitBlitOneSurface`
(`nds_ui_kit.c`) opens the surface pack, streams row slices with
`ndsRelocAssetStreamRead`, FNV-folds every byte to check the hash, and
`DC_FlushRange`s each slice. Every call is a real NitroFS read.

The size is stated by this file's own neighbour, `ndsMenuShellCssSyncPanels`:

> a panel is **7,738 B of NitroFS** and the character select's own worst frame
> already sits at **71% of the 60 Hz budget** before one is read, so the screen
> ENTRY (a load frame) writes all four and everything after it writes one a
> frame.

`SyncPanels` takes a `budget` argument for exactly this reason. **`StepDoors`
does not go through it**, so a slide can issue up to four full-panel NitroFS
reads in a single frame on a screen already at 71% of budget.

Measured: `CSSACT 5 … doors=80` — 80 door-slide frames in this visit — and
the sync phase's worst frame is **3,244,224 ticks = 5.81 frames**.

The underlay being re-read is **identical every tic** — `sCssPanelSurface[i]`
does not change during a slide. It is re-read only to repaint pixels the door
halves trampled on the previous frame.

## Cause 4 — Yoshi's preview: two producers, one number, 1,232 bytes apart

**Found and measured; the first fix was WRONG and hung the character select, so
it is reverted.** The cause below is arithmetic and stands. The remedy did not.

> **Read the correction at the end of this section before acting on it.**

`ndsRendererValidateNativeFighterOwner`
(`nds_renderer_native_fighter_production.c:1171-1176`) opens with

```c
if ((asset_data_size != expected_asset_data_size) ||
    (root_count != expected_count))
{
    NDS_NATIVE_FIGHTER_VALIDATE_REJECT(3u, …);
}
```

— **before the root loop**. For a compact CSS preview the `asset_data_size` it
is handed is the pack header's section-1 `source_bytes`
(`ndsRelocNativeSourceSize`, `reloc_preview_pack.c:127-132`, reached from
`renderer_adapter_fighter.c:3742-3747`). A reject sets
`native_owner_enabled = FALSE`, and at `NDS_RENDERER_PROFILE_LEVEL 0` a
declined owner draws **nothing**.

Parsing every built pack's header against the owner's emitted size:

| kind | pack `source_bytes` | owner `asset_data_size` | delta |
|---|---:|---:|---:|
| mario / fox / donkey / samus / luigi / link / captain / kirby / pikachu / purin / ness | — | — | **0** |
| **yoshi** | **45,488** | **44,256** | **+1,232** |

Eleven agree exactly. Yoshi is off by 1,232, and 1,232 is the weld.

### Why only Yoshi

`load_o2r_payload` returns the payload **extended** with synthetic welded DLs
for a pair-mode owner (`_extend_payload_with_pairs`), and caches the
pre-extension length as `raw_length`. Yoshi is the only character-select kind
in `OWNER_DL_PAIR_MODE` — the other three members are 1P/Boss owners with no
preview pack — so he is the only fighter for whom the two lengths differ at
all. The owner emitter published `raw_length` (correct: the runtime loads the
unextended asset from NitroFS). The preview pack recorded
`len(load_o2r_payload(...))` and wrote **that** into `source_bytes`.

Neither producer's text mentions the other, so no search relates them. This is
the **third** instance of that shape found today, after Kirby's copy and Yoshi's
throw.

**In-match Yoshi was never affected** — the battle pack declares 44,256 — which
is exactly why the defect could sit in the character select unnoticed while
Yoshi played fine.

### The fix

One shared helper, `owner_asset_data_size(payload, owner_name)`, now answers
both ends, and the pack writer keeps the two extents apart:

- `model_span_extent` (**extended**) bounds every offset the pack stores —
  Yoshi's welded roots `0xace0` / `0xae68` sit at and above the raw end, so the
  span/cell/slot bounds must keep using it.
- `model_source_bytes` (**raw**) is what goes in the header, because that is
  what the validator compares.

Regenerating changed **two bytes of one file**: offsets 108 and 109 of
`06.fpc`, the low half of section 1's `source_bytes`. Every other pack is
byte-identical and Yoshi's pack is identical from byte 128 on. Nothing else
moved.

### And a check, because this drift was silent

`scripts/fighters/check_preview_pack_owner_sizes.py` reads each pack's header
field and each owner's emitted size — including Mario's and Fox's literals,
parsed out of the validator rather than restated — and fails on any
disagreement. Proved both ways: it reports the exact defect against the stale
packs and passes against the regenerated ones. Registered in `verify-all.ps1`
with `$expectedVerifiers` moved 17 → 18.

### CORRECTION: lowering `source_bytes` hangs the CSS, and it is reverted

`source_bytes` has a **second** runtime consumer I did not check.
`reloc_preview_pack.c:401` bounds every span's `source_offset` by the same
field, and its failure path is `ndsPreviewPackLoadHalt` — a `for (;;)` spin,
**not** an abort. Measured on the built packs:

| pack | `source_bytes` | model spans | max source end | out of range |
|---|---:|---:|---:|---:|
| original | 45,488 | 3 | 45,488 | **0** |
| my "fix" | 44,256 | 3 | 45,488 | **1** |

Yoshi's third model span ends at 45,488, so a 44,256 bound puts it out of range
and the ROM spins forever. The character select never enters.

**The symptom matched exactly and I misread it twice.** Both probes carrying the
change reached VS Mode and stopped — `css=0/0`, no crash marker, arena identical
to the control, timeout. I first blamed CPU contention from running three jobs
at once. The rung-8 probe then ran alone and stalled identically, which ruled
that out: the only thing both builds shared and the clean rung-7 control lacked
was this change.

Reverted; all twelve packs are byte-identical to the pre-change tree, verified
by compare rather than asserted. **One field cannot be both the span extent and
the raw asset size.** The span bound is load-bearing, so it keeps the field, and
the real fix is a pack **format** change — the raw size as its own header word,
with a version bump — which is not being rushed in behind a hang.

Yoshi's preview is invisible again. That is the right trade: a hung character
select is far worse than a blank one, and it is the state the owner already
reported rather than a new one.

`check_preview_pack_owner_sizes.py` keeps its value rather than going
permanently red: `KNOWN_MISMATCHES` records Yoshi by name with both numbers and
the reason, so the run prints the live defect and still **fails** on any fighter
not listed. A second drift cannot hide behind the first one's exception.

**The root-alias fix and the generator assert below are untouched and remain
correct** — a different defect, not reading this field, each proved both ways.

### The same investigation found a second, worse defect in today's own fix

Chasing *why* Yoshi's owner size was 44,256 led to `faf3a7782e8` (2026-09-06),
which states its intent plainly:

> The owner bake published the compiler-extended payload size (`0xb1b0`) and the
> synthetic welded-list offsets as root identities. It now publishes the raw O2R
> size (`0xace0`) and the source post-list offsets while keeping the welded
> programs for the geometry oracles.

So the owner deliberately moved to the raw size **and** gained
`runtime_root_aliases` to map each weld's synthetic offset back to the source
post-list offset the live DObjs provide. That alias is applied where the
canonical table is emitted:

```python
roots = [(aliases.get(row[0], row[0]), *row[1:]) for row in context["roots"]]
```

**The root-program tables, added later, never picked it up.** Yoshi's Catch and
Throw programs — written earlier today to fix his invisible grab and B attack —
published `0xace0` and `0xae68` as root identities against an asset that
**ends** at `0xace0`. Those are addresses the loader can never produce, and
`ndsRendererValidateNativeFighterOwner`'s bounds helpers reject them. That fix
could not have worked, which is consistent with `BUGS.md` still recording it as
not yet seen on screen.

| table | before | after |
|---|---|---|
| `sNdsNativeYoshiCatchRoots` | max `0xae68`, **2 out of bounds** | max `0x3148`, none |
| `sNdsNativeYoshiThrowRoots` | max `0xae68`, **2 out of bounds** | max `0x7d10`, none |
| `sNdsNativeYoshiCatchRootsLow` | max `0xb0d0`, **2 out of bounds** | max `0x6738`, none |
| `sNdsNativeYoshiThrowRootsLow` | max `0xb0d0`, **2 out of bounds** | max `0x7d10`, none |
| `sNdsNativeYoshiRoots` (canonical) | in bounds | unchanged |

`0xace0 → 0x2248`, `0xae68 → 0x2c50` high; `0xaf70 → 0x5bc8`,
`0xb0d0 → 0x6308` low. **Eight lines change in a 132,000-line generated file
and nothing else moves.**

Aliasing quietly would leave the next one to be found the same way, so the
shared helper **asserts**: a root offset at or past `asset_data_size` is a bake
error that names itself. Proved both ways — with the alias in place the output
is byte-identical, so the assert is inert rather than a second change; with the
alias removed the generator fails with

```
yoshi high canonical: root offset 0xace0 is at or past asset_data_size 0xace0
```

## The walk now selects every fighter

Owner, 2026-09-17: *"the CSS walk should go over and select ALL fighters."*

It did not, and that is why both defects above could exist unnoticed. The walk
committed whatever its roster-independent pixel wander landed on — measured,
`CSSFTRKIND 5 mask=0x23b`, i.e. Mario, Fox, Luigi and Samus: **four of nine
admitted kinds**. Yoshi was never among them.

`ndsMenuShellCssWalkTourStep` now parks slot 0 on each admitted kind for 48
tics, sized from the two things standing between a kind change and a drawn
preview: the 13-tic dwell before a load may begin, then a residency budget of
one action per tic through retire/load/prepare (the probe measured `retry=3`
per acquire). It uses **direct slot assignment** for the reason
`ndsMenuShellCssWalkRestoreGate` already documents — an A press over a locked
cell is *refused*, so press counts are roster-dependent and re-tuning them per
rung broke this walk once already, while assigning the slot never touches a
cell. START is suppressed until the tour finishes, and the existing snapshot
still restores Mario/Fox, so what the gate commits is unchanged.

The probe prints two new lines:

```
CSSTOUR    kind=<mask parked on>  drew=<mask that produced triangles>  done=N
CSSTOURTRI <per-kind triangle counts, FTKind order>
```

`kind` and `drew` must be **equal**. A bit in `kind` and not in `drew` is a
fighter whose 3D preview draws nothing, and `CSSTOURTRI` names which one. This
is the instrument that was missing: it converts "fighter N's preview is
invisible" from an owner playtest into a counter.

### What this run does *not* prove

`CSSFTRKIND 5 mask=23b mario=188/… fox=1652/… luigi=113/… samus=113/…` — the
scripted walk previews Mario, Fox, Luigi and Samus only, so it never exercised
Yoshi either way. `CSSPKT 5 … decline=0` says no owner was declined for those
four, which is consistent but is not evidence about Yoshi.

The size mismatch is measured and is gone. Whether a **second** defect also
blanks him is untested: Yoshi's pack carries root identity cells only for
`0x2398, 0x5cf8, 0x7d10, 0x8300…0x9350`, none of them the canonical body roots,
and that arrangement last passed on the raw path (2026-09-06,
`YOSHI_COMPARE expected_size=0xace0 size=0xace0`, 18/18 roots) rather than
through a pack. If Yoshi is still invisible, the witness to read is
`gNdsNativeFighterValidateRejectCode`: **3 means this fix did not take; 4 with
observed `0xffffffff` means the runner-up is live.**

## Kirby/Jigglypuff/Ness: attributed, and it is the same wall as everything else today

**The rung-7 control is clean.** It reaches the character select with no SIGILL
and no ABORT; its run ended on my own `-Hits 7` timeout waiting for later
stops, not on a crash. Rung 10 dies there. So the crash is the roster change,
and the mechanism is the one this campaign has hit three times already:

| | rung 7 | rung 10 | delta |
|---|---:|---:|---:|
| `gNdsTaskmanArenaChosenSize` | 1,240,832 | 1,154,816 | **-86,016 = exactly 21 pages** |
| heap free at the CSS | 378,096 | 279,152 | -98,944 |
| image `.text` | 1,531,868 | 1,598,380 | +66,512 |
| image `.data` | 223,224 | 238,752 | +15,528 |

**+82,040 B of image costs 21 arena pages**, and the character-select exit then
wanders into `SIGILL pc=0x00000b64` -- a low address with no symbol, which is a
wandered CPU rather than a faulting instruction, so the exception site names
nothing. Same family as the Yoshi egg and Vulcan Jab; just large enough to
crash instead of merely reject.

The per-roster entry-effect emitter (8,458 B returnable) is nowhere near
covering 82 KB, so it is not the lever here.

**But the ladder is cumulative, so rungs 8 and 9 are separate questions.**
Kirby is the expensive member -- thirteen copy hats plus the copy state
machines -- and Jigglypuff alone (rung 8) or Jigglypuff plus Ness (rung 9) may
well fit. Bisecting is now cheap because the walk got faster.

## Superseded: what the rung-10 SIGILL looked like before attribution

The rung-10 ROM builds clean (exit 0, zero `error:`, all ten `NDS_P2_*` flags
set, `NDS_P2_ITEM_CORE 1`) and reaches the character select, plays it for 1,651
frames, commits a fighter (`CSSCOMMIT 5 n=1`) and exits it
(`MSSCENE 5 enters=5 exits=5`). Then:

```
Program received signal SIGILL, Illegal instruction.
0x00000b64 in ?? ()
MSSTOP n=5 pc=00000b64 cpsr=200000b7
ABORT lr=01fffbc4 spsr=200000b7
```

`pc` in low memory with no symbol is a wandered CPU, not a faulting
instruction, so the exception site names nothing. **Attribution is pending the
rung-7 control run on the same probe** — until that lands it is not known
whether this is a rung-10 regression or a pre-existing CSS-exit defect the
roster change merely inherited.

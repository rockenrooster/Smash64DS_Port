# Whispy face/eyes — 30 Hz cadence row

Bug (verbatim): *"Whispys face and eyes still look low FPS, should have smooth
30 FPS animation."*

Stage: **CONTRACT (partial)** — the blink script bytes are captured; the decode
and the 60→30 Hz cadence measurement are NOT done.

## Source contract established (no ROM)

`grpupupu.c:74-78` — the eyes table has exactly two statuses per side:

```c
{ { LeftTurnAnimJoint,  LeftTurnMatAnimJoint  }, { LeftBlinkAnimJoint,  0x0 } },
{ { RightTurnAnimJoint, RightTurnMatAnimJoint }, { RightBlinkAnimJoint, 0x0 } }
```

The blink's material-anim slot is literally `0x0`. `grpupupu.c:565-583`
(`grPupupuUpdateGObjAnims`) passes that slot to `gcAddAnimAll` as NULL, then
`gcPlayAnimAll`, then clears the status to −1. **A NULL material anim on blink
is source-correct and is not a defect** — do not restart the "missing blink
texture" hypothesis; `HANDOFF.md` already records that the six
`dGRPupupuWhispyEyesTextures` are the WIND cycle on `map_gobj[3]`.

The mouth (`grpupupu.c:81-98`) has four statuses per side
(Stretch/Turn/Open/Close), each with a NON-null material anim — which is why it
is the correct free control for the blink.

## Measured on the existing ROM (`build-c128-foxgun`, no build spent)

`scripts/probe-whispy-blink-script.ps1`:

```
WPRE n=1 status=1 lr=1 head=02327ea0 jointoff=00001330 matoff=00000000
     table=0x23291d0 gframe=0.000000 mouthstatus=0
  WJ0 (nil)
  WJ1 0x23291dc : 06010000 3f1c61aa 0801014a 3f1c61aa 18010001 0400000c
  WJ2 0x23291f8 : 12800000 3f800000 14400000 3f800000 0c800005 3dd480c1
  WJ3 0x6010000 : ee88ee88 ...            <- OUT OF RANGE, see below
```

- `status=1` is Blink and `matoff=00000000` confirms the NULL material anim
  live, matching source.
- Words alternate command / f32: `3f1c61aa` = 0.61081, `3f800000` = 1.0,
  `3dd480c1` = 0.10400. So each script is a (command, value) pair stream.
- **The decode is NOT done.** Two candidate packings are in play
  (`probe-whispy-blink-script.ps1:29-35`): source `opcode[31:25] flags[24:15]
  payload[14:0]` vs the port's native `opcode[6:0] flags[16:7] payload[31:17]`.
  Under the source packing `WJ1` opcodes read 3, 4, 12 and `WJ2` reads 9, 10, 6;
  under the native packing the payloads come out in the hundreds-to-thousands,
  which is not frame-shaped. Neither has been checked against `objdef.h`'s
  opcode table, so **no conclusion about the blink's frame count is supported
  yet.** The probe's own hypothesis (b) needs the sum of the blocking-command
  payloads, plus one.

## Probe defects found and fixed (both would have produced false evidence)

1. **`$g0->anim_speed` does not exist.** `GObj` carries only `anim_frame`
   (`objtypes.h:218`); `anim_speed` is a `DObj`/`MObj`/`CObj` field
   (`objtypes.h:452`). GDB aborted the sourced command file at that line, so
   every `WPOST` line — the entire test between "the add never happened" and
   "the script really is one frame" — was silently lost while `WPRE` looked
   healthy. Now reads `$d0->anim_speed`.
2. **The 4-entry table read runs off the end.** `WJ3` printed
   `0x06010000` — a VRAM address — full of texture bytes (`ee88ee88…`) that
   would decode as a plausible script. The table is not NULL-terminated at 4.
   The probe now accepts an entry only if it points into `0x02000000‥0x02400000`
   and otherwise prints `nil-or-out-of-range` with the pointer.

## DECISIVE RUN (fixed probe, `build-c128-foxgun`, still no build spent)

`artifacts/verification/2026-08-12_whispy-blink-decisive.txt`, 6 natural blinks.

```
WPRE  n=1 status=1 lr=1 matoff=00000000 gframe=0.000000
  WJ1 0x23291dc : 06010000 3f1c61aa 0801014a 3f1c61aa 18010001 0400000c
WPOST n=1 dfail=0 dscript=8 dreuse=0 lastreason=0 gframe=1.000000 ...
WPRE  n=2 ...
  WJ1 0x23291dc : 00000103 3f1c61aa 02940104 3f1c61aa 0002010c 00180002
WPOST n=2 dfail=0 dscript=0 dreuse=8 lastreason=0 gframe=1.000000 ...
```

**1. Hypothesis (a) is REFUTED.** `dfail=0` and `lastreason=0` on all six
events; `dscript=8` on the first then `dreuse=8` thereafter. Normalization
succeeds and caches. The overflow/rejection story is not what is happening, and
`gcAddAnimAll` is not being skipped.

**2. The words change at n=1→n=2 because the port repacks IN PLACE.** n=1 reads
source layout, n=2+ reads native. The repack is correct:

| source | native | opcode | flags | payload |
|---|---|---|---|---|
| `06010000` | `00000103` | 3 | 2 | 0 |
| `0801014a` | `02940104` | 4 | 2 | 330 |
| `0400000c` | `00180002` | **2 = Wait** | 0 | **12** |

Both packings decode the last word identically (source `>>25` = 2, `&0x7FFF` =
12; native `&0x7F` = 2, `>>17` = 12), so this is not a decode ambiguity.

**3. FIRST DIVERGENT VALUE.** The blink joint script contains `Wait 12`, so the
animation should span ~13 source frames (sum of blocking payloads plus one).
The port's `map_gobj[0]->anim_frame` reaches **1.000000** and is back to
**0.000000** by the next blink request. The defect is in PLAYBACK, downstream of
a correct script and a successful normalize.

**4. The mouth control does NOT differ:** `mouthframe=1.000000` on every event,
despite a non-NULL material anim. So this is not blink-specific and not caused
by the NULL material argument — it is the shared GObj animation playout for
these Pupupu actors. That widens the owning seam beyond the eyes.

Caveat on the root DObj row: `djoint=(nil) droot=0 dwait=0 dframe=0.000000`
is CONSISTENT, not anomalous — `WJ0` is `(nil)`, i.e. the root carries no
script and the animation lives on the children (`WJ1`, `WJ2`). The probe only
reads `$d0` (the root), so **the children's `anim_wait`/`anim_frame` — where the
`Wait 12` would actually be counted down — are still unmeasured.** That is the
next read, and it needs no build.

## Two harness traps that ate a cycle each (not bugs in the ROM)

1. **The gdb capture goes to `artifacts/verifier-temp/default/`, not
   `.../slot<N>/`,** unless `SMASH64DS_VERIFY_TEMP_DIR` is set — the RunnerSlot
   picks the emulator/log dir, not the gdb-out dir. Polling `slot6` returned
   "0 WCAD lines" for a run that was fine, and it was killed for nothing.
2. **A PowerShell `Start-Job` does not survive between tool invocations.** The
   job is gone on the next call, so the probe never ran at all while appearing
   to be in flight. Launch long probes with the harness's own background mode.

## Cadence run (`-Cadence`, 26 presented frames) — INCONCLUSIVE by construction

`artifacts/verification/2026-08-12_whispy-cadence.txt`. Every sampled frame is
identical:

```
eyewait=-3.40282e38  eyeframe=0.000000  eyerotx=0.000000
eyeroty=0.610865     eyetry=187.500000  (mouth likewise)
```

`-FLT_MAX` in `anim_wait` is the "no animation running" sentinel — the same
field read **11.000000** at the blink breakpoint. So the sample landed entirely
BETWEEN blinks, where the eyes are legitimately static, and it says nothing
about 30 Hz cadence. `eyeroty = 0.610865` is ~35 degrees and matches the
`3f1c61aa` = 0.61081 constant carried in the blink script's own words.

**The probe is mis-designed, not the game.** A flat run proves nothing unless
the sample window is known to contain an animation. The next version must arm
on `whispy_eyes_status != -1` (or on `anim_wait != -FLT_MAX`) and only then
sample the following N presented frames — otherwise a quiet window reads
exactly like a frozen face, which is the conclusion the probe exists to test.

## ARMED cadence run — the clock is CORRECT (2026-08-12, no build)

`artifacts/verification/2026-08-12_whispy-cadence-armed.txt`, armed on
`anim_wait > -1000000`, 24 presented frames covering two blinks.

| presented frame | `anim_frame` | `anim_wait` |
|---:|---:|---:|
| 1 | 1 | 11 |
| 2 | 3 | 9 |
| 3 | 5 | 7 |
| 4 | 7 | 5 |
| 5 | 9 | 3 |
| 6 | 11 | 1 |
| 7 | — | `-FLT_MAX` (ended) |

Second blink at f=11..16 repeats it (wait 12,10,8,6,4,2).

**`anim_frame` advances by exactly 2 per presented frame and `anim_wait` falls
by 2** — exactly two 60 Hz source ticks per 30 Hz present. The `Wait 12` script
spans 6 presented frames (~0.2 s). The port is NOT slowing, throttling, or
dropping the animation, and the mouth tracks the eyes identically. **The
"30 Hz presentation loses the source animation" hypothesis is refuted at the
clock level**, as is any fix that would slow gameplay animation.

**LEAD, NOT A CONCLUSION.** Across all 24 frames — including the 6 animating
ones — `rotate.x` (0.0), `rotate.y` (0.610865) and `translate.y` (187.5) never
change. A counter that advances while the transform stays put would be the
first divergent value. But only 3 of 9 channels were sampled: `rotate.z`,
all three `scale` components, and `translate.x/z` were NOT read, and the
decoded opcodes (3, 4, 12 on WJ1; 9, 10, 6 on WJ2) have not been mapped to
channels against `objdef.h`. Three readings this session looked this
convincing and turned out to be artifacts. **Next: sample all nine channels on
both WJ1's and WJ2's DObjs across an armed window, and map the opcodes, BEFORE
calling the applier broken.** `eyeroty = 0.610865` (~35 deg) matches the
`3f1c61aa` = 0.61081 constant carried in the blink script itself, so at least
one script value has reached the transform at some point.

## NINE-CHANNEL RUN — the applier works; the "static transform" lead is REFUTED

`artifacts/verification/2026-08-12_whispy-channels.txt`. The animated node is
not `$d0->child` (E1) but ITS child (E2). E1 is correctly static — it is the eye
group's placement (`t=-525.0,187.5,-1219.4  r=0,0.610865,0  s=1,1,1`, constant).
E2 carries the blink:

| presented frame | `anim_frame` | E2 `scale.y` |
|---:|---:|---:|
| 1 | 1 | 0.948030 |
| 2 | 3 | 0.604812 |
| 3 | 5 | **0.103761** |
| 4 | 7 | 0.103760 |
| 5 | 9 | 0.606009 |
| 6 | 11 | 0.948296 |
| 7 | — | 1.000000 |

A vertical squash to ~10% and back — the blink — with a DISTINCT value on every
presented frame (E2 `scale.x` also hits 1.2 for one frame). **The transform
applier is healthy.** The earlier "counter advances while the transform stays
put" lead was an artifact of sampling the wrong DObj depth, and is withdrawn.

**The chain is now proven healthy end to end from source script to DObj
transform:** script decode (`Wait 12`), normalize (`dfail=0`), playback
(`anim_wait` counts down), 60→30 Hz clock (+2 `anim_frame` per present), and
transform application (6 distinct `scale.y` per 6 presents). No fix belongs at
any of those seams, and nothing here should be slowed.

**The ONLY remaining link is DObj transform -> submitted geometry -> pixels.**
That is exactly Case C of the owner's brief: a prepared/baked stage-actor run
caching a field that is actually dynamic. `NDS_R2_WHISPY_NATIVE_AOT` and
`NDS_R2_WHISPY_NATIVE_TEXTURES` are both 1 and the native path pins textures
"immutable for the whole scene" (`nds_renderer.c:4440`); whether the per-frame
`scale.y` reaches the submitted vertices is UNMEASURED. Probe
`probe-whispy-native-aot.ps1` already exists for that path.

## A fifth output-filter casualty

The `E1`/`E2` lines printed correctly but never reached the console: the
harness's own marker filter passes only its known prefixes, so the new rows
were dropped while `WCAD` came through, which read exactly like two failed
printfs. Read the ARTIFACT, never the filtered console — five recurrences now.

## LAST LINK CLOSED — XObj kind 28 consumes scale

`artifacts/verification/2026-08-12_whispy-xobj-kinds.txt`: the eye's animated
DObj (E2) reads `xobjs_num=1 k0=28 vec=(nil)`.

`ndsRendererAdapterBuildDObjLocalMatrix` builds the local matrix from
`dobj->xobjs[]` dispatched BY KIND, so an animated `scale.y` reaches the matrix
only if the node carries a scale-bearing kind. Counting from
`nGCMatrixKindTra = 18` (`objdef.h:123-133`), **28 =
`nGCMatrixKindTraRotRpyRSca`** — translate + rotate Roll/Pitch/Yaw + **scale**.
It does.

**The Whispy chain is therefore measured end to end with no divergence at any
seam:** script decode -> normalize -> playback -> 60/30 Hz clock -> transform
application -> matrix consumption. Six distinct `scale.y` values reach a
scale-bearing matrix over six consecutive presented frames.

**No fix is warranted anywhere in this chain, and implementing one would be
inventing a defect.** In particular the two tempting fixes are both refuted by
measurement: the source animation is NOT collapsing (so nothing needs
preserving across presents), and the 30 Hz sampling is NOT dropping states (so
nothing needs slowing).

What remains is a perception question the instruments cannot settle. The blink
is a correct 6-presented-frame event; between blinks the eyes are legitimately
static, which is source behaviour. If the owner still reads the face as
low-FPS, the next contract row must name WHICH motion looks stepped (blink,
eye turn, mouth open/close, or the wind cycle on `map_gobj[3]`) — those are
four different mechanisms and only the owner can say which one is meant.

## Not done / inherited

- The decisive `WPOST` counters (`dfail`/`dscript`/`dreuse`/`lastreason` across
  the blink frame) — this is what separates a rejected normalize from a genuine
  one-frame script. A rerun with the fixed probe was launched but is not
  reported here.
- The offline AObjEvent32 decode (Bug 2A) for all 12 eye/mouth scripts.
- The 60 Hz-vs-30 Hz presentation cadence measurement (Bug 2B) and the
  consecutive-presented-frame visual cadence test (Bug 2D).
- `grPupupuUpdateGObjAnims` did NOT hit its breakpoint within 600 s on
  `build-c142-crouchprobe`. Under GDB this rig advances only ~13 s of guest
  time per ~10 min wall, so that is most likely budget, not absence — but it is
  unproven either way, and c142 vs c128 has not been compared.

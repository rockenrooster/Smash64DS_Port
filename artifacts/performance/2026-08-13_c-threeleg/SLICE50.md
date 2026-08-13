# Slice 50 KEPT — the stage stops re-proving its texture bindings. Gate 1,210,880

**One leg of the three shipped.** `RESIDUE.md` §4 row 0 / `FLAG_SWEEP.md` §4 named a
three-leg composition with a 20,562 ceiling. Leg A shipped and pays. **Leg B is
blocked by a RAM guard and leg C does not exist as briefed** — both refuted below
with the evidence, so nobody re-derives them.

## Verdict

**KEEP.** A/B/A on the `BOTH_CPU 1` gate arm, 1,600 samples from frame 438, whole
match, DLDI ON, `NDS_TICK_HUD_DRAW=1`, mode 163, `slips=0` on all three arms.

| bucket | A control | **A2 control, separate link** | B candidate | Δ B−A |
|---|---:|---:|---:|---:|
| **`WORK-H` P50** | 939,392 | **939,392** | 923,392 | **−16,000** |
| **`WORK-H` P95** | 1,219,520 | **1,219,520** | 1,210,880 | **−8,640** |
| `WORK-H` mean | 967,552 | 967,552 | 947,305 | −20,247 |
| **`STG` P50** | 172,928 | 172,928 | 161,600 | **−11,328** |
| **`STG` P95** | 177,216 | 177,216 | 165,888 | **−11,328** |
| `STG` mean | 173,313 | 173,313 | 161,991 | −11,322 |
| `FTR` P50 / P95 | 303,232 / 330,048 | same | 298,048 / 324,032 | −5,184 / −6,016 |
| `WAIT` P50 | 197,824 | 197,824 | 211,136 | **+13,312** |
| VBlank 2 / 3 / 4 / 5+ | 1705 / 304 / 17 / 12 | same | **1743 / 267 / 15 / 13** | **+38 / −37 / −2 / +1** |
| max VBlank interval | 26 | 26 | 26 | 0 |

**A2 is the third arm and it returned EXACTLY to A — every bucket, both
percentiles, the whole histogram, delta 0.** Its ELF hash differs from A's
(the link is not byte-reproducible), so this is a genuine second link of the same
source and not a re-run: the instrument reproduces to **0** for unchanged code,
and the entire B delta is the change's. `compare-tick-hud-arms.py` warns of a
~14,080 placement term for separately-linked arms; between A and A2 it measured
**0**, so that term is a property of *changed code*, not of linking.

**Judge this one at `STG` and P50, not at P95.** `STG` moved **−11,328 at P50, at
P95 and at the mean simultaneously** — a flat uniform shift of exactly the lane
the deleted work lived in, which is what a flat deletion looks like and what
placement noise cannot imitate. P95 is the noisiest statistic here: two candidate
binaries differing by ONE line (the write-seam drop, below) measured P95 1,186,816
and 1,210,880, a **24,064** spread, against a 3,648 spread at P50 and **512** in
`STG`. The P95 figure banked above is the CONSERVATIVE of the two.

`WAIT` rising +13,312 against `WORK-H` falling 16,000 is the corroboration: on a
VBlank-paced ROM the work removed reappears as idle time. And 37 frames moved
from 3 VBlanks to 2, which is the switch's own stated objective.

## What shipped

`ndsRendererNativeStagePreparedTextureValid` re-proved the prepared stage run
table against the texture cache **~195 times a frame**. Per-line attribution
(`../2026-08-13_c-flagsweep/gxrecord-and-texvalid-owner-line.txt`) priced it at
**9,369 tk/fr at 5.34 cycles per instruction** — not compares, but two cold arrays
(`runs[]`, then the cache entries it names) dragged through a 4 KB D-cache every
frame. Slice 44's shape, whose lever was *stop touching the objects*.

Exact call decomposition, entry-PC counts off `c123-pc-cycles.csv` / 1601 regions:

| caller | calls/frame |
|---|---:|
| `ndsRendererCommitNativeStageSegment` — sweep of all 54 runs | 54 |
| `ndsRendererNativeStagePreparedTexturesValid` ← the reuse key (inlined) | 53.97 |
| `ndsRendererTask36ReplayTexturesValid` — replay table sweep | ~54 |
| `ndsRendererNativeStageBeginRun` — per-run last gate | ~33 |
| **total** | **194.88** |

Replaced by **one epoch compare**, keyed on
`sNdsRendererHardwareTextureKeyGeneration`, which every (re-)key already stamped
and which `ndsRendererHardwareReleaseTexture` now stamps as well.

## Leg A's invalidation table — where every writer signals (§3.12 checklist)

The proof reads exactly four things: `entry != NULL`, `entry->ready`,
`entry->name`, `entry->key_generation`. Every writer of those, enumerated:

| invalidation source | where it now signals |
|---|---|
| re-key of an entry (3 sites: static prepare, dynamic resolve, dynamic upload) | already bumped the epoch before this change — `sNdsRendererHardwareTextureKeyGeneration++` beside `entry->key_generation = …` |
| release / evict / discard / static teardown | **new**: one bump in `ndsRendererHardwareReleaseTexture`, which is the SOLE seam — `EvictTexture`, `AllocTexture`'s recycle, `DiscardTextureCache` and `ReleaseBattleStaticTextureEntries` all route through it, and its `memset` is what clears `ready` and `name` |
| a fresh `entry->ready = TRUE` (7 sites) | needs no bump: every one is guarded by `entry->name == 0` + `glGenTextures`, i.e. a slot that is free or was just released. A prepared run cannot be VALID against a slot with `ready == 0`, so no live certificate can reference it |
| prepared table rewritten | `ndsRendererNativeStagePrepareRun` — **the write seam**, this function is the table's only writer; plus the instance beside `gNdsR2StagePrepareBuildCount` |
| replay table rewritten | `ndsRendererTask36ReplayReset` and the per-run `run->prepared = …` capture |
| scene / match entry (taskman arena rewind) | the battle prepare calls `ndsRendererHardwareDiscardTextureCache` first, which releases every entry — ≥24 pinned static textures are live after any match, so the epoch always moves. Independently, the table is keyed on topology generation + stamp (R2-07 E2) |

**No pointer is trusted across a boundary** (§3.12's actual failure mode): the
certificate is a monotonic `u32` in `.bss` that is never reset, so a rewound arena
handing back the same addresses cannot make a stale certificate read current.

Only the POSITIVE answer is cached; a failing table re-sweeps every frame exactly
as before, because a table becomes valid again by being rebuilt and that is not an
epoch event.

## Engagement, both sides

**Control side** is the c123 profile above: **194.88 calls/frame**, 9,369 tk/fr.

**Candidate side**, `-ExtraGlobals` on the same run that produced the buckets:

| counter | value | per frame | predicted |
|---|---:|---:|---|
| `gNdsR2TexProofFastCount` | 20,370 | **9.995** | ~10 (1 reuse key + 8 commits + ~1 replay) |
| `gNdsR2TexProofSweepCount` | **8** | 0.004 | <50 for the whole match |
| `gNdsR2TexProofSweepFailCount` | **0** | 0 | 0 |
| `gNdsR2TextureEpochBumpCount` | **0** | 0 | 0 mid-match |

**195 calls/frame became 8 full sweeps in a 2,038-frame match.** `EpochBump = 0`
inside the window confirms the design's measured premise: the texture cache is
static across a match (`ndsRendererHardwareAllocTexture` executes **3 times** in
the whole run, `FindTexture` 134, and Release/Evict/Discard never appear in the
profile at all). A per-frame `Sweep` count would have meant the epoch was moving
and the lever had regressed to the old shape; it did not.

**Negative control / invariants, identical on all three arms:**
`gNdsR2StagePrepareReuseCount` **2,037**, `gNdsR2StagePrepareBuildCount` **2** —
the reuse decision is unchanged, so the certificate is not buying its win by
reusing something the control rebuilt. End-of-match
`gNdsBattleTextHudP0Damage` **0**, `P1Damage` **58**, `P0Stock` **1**,
`P1Stock` **1** on A, A2 and B: **the same match, no divergence.**

## Fidelity

Frame-locked pair on the simulation clock —
`EXACT_LOCK=gSCManagerBattleState->time_remain,40,38` on **both** arms (the
presented-frame counter drifts between arms in proportion to the speed change, so
the clock lock is the only valid cross-build lock).

**Game viewport 400x296 = 118,400 px: PIXEL-IDENTICAL on both tics, max channel
delta 0.** The only drift anywhere on the top screen is **12 px on scanlines
y=298-299**, x=178-180/186-187/205 — the tick-HUD's own digit row, which must
differ because the arms have different tick counts. Same finding slice 44
recorded. Captures and diffs:
`../../visibility/2026-08-13_c131-texproof/`.

## Cost

Both arms measured, `boot-headroom.txt`: **text +564, data +0, bss +32, total
+596**. Proven headroom **176,736 → 176,128** against a cliff ~176 KB away. The
bss is the two certificates plus the four engagement counters; the text is their
helpers. The change deletes per-frame *reads*, which is why a +564-byte binary
still measures −16,000 P50: at the memo's 1.85 cyc/byte the added text is worth
roughly 1,043 ticks of `FTR` mean, and `FTR` fell 5,184 anyway.

**Boundary GREEN** (`boundary.log`, full console capture — `verify-all`'s
sub-verifiers write to the real console, so a `*>` redirect or a `Tee` pipeline
captures only the dispatcher line; read that file, not a filtered tail).
0 `Exception:`, 0 `FAIL`, 0 `RED`. Root ROMs `smash64ds.nds`
`54c07fac…a68a` and `smash64ds-battle-playable-hwtri.nds` `524448c9…adee` are
byte-identical to their values at cycle start, before and after.

**It went RED first, and both failures were checker debt rather than defects** —
`check-gbi-decode-fixtures.ps1` pinned the reuse key's old helper name and the
Commit gate's per-segment loop. Both are re-pinned in the new form and
**strengthened**: the release-seam epoch bump and the write-seam certificate drop
are now their own assertions, because either regressing would be silent — stale
native geometry drawn from a recycled texture slot.

## Leg B — BLOCKED by the texture-cache RAM guard, not by its design

`glBindTexture` is **5,544 tk/fr** (54.89 binds/frame at ~101 ticks of libnds
`DynamicArray` lookup, entry-PC counted) and the direct-bind replacement does ship
for three textures (`NDSRendererWhispyNativeBinding`). Generalising it needs the
palette word and libnds's two software-state fields cached per cache entry:
**12 bytes x 69 entries = 828 B**.

`nds_renderer.c`'s own `_Static_assert` refuses it:

```
texture cache storage must stay at or under the 48x292 budget   /* 14,016 B */
```

Current occupancy is **13,944 B — 72 bytes of slack.** The guard's comment names
the reason: *"+14KB of bss took gNdsTaskmanGeneralHeapFreeMin under the GObj cap
and the ROM stopped booting."* Routing around a guard installed by that failure is
not an agent call. **The build failed on this assert, which is the guard working.**

Two shapes survive for a later cycle, both needing their own boot-headroom read:
store the cached words in the prepared runs instead (54 stage + fighter runs,
outside this assert), or a bounded side table for the few textures that actually
carry the 54.89 binds. Note also that the fast path must re-emit the
`NDS_TASK29_GX_TEXTURE_BIND` record, because a stage bind can happen inside a
Task 36 capture window.

## Leg C — REFUTED as specified: there is no unconverted per-corner writer left

The brief's "pure `#if` extension to the 8 named GX sites" does not exist. Checked
each:

- The colour / texcoord / vertex writers on the fighter path were **already
  converted by slice 1** — `ndsRendererHardwareWriteFighterColorWord`,
  `…FighterTexCoordWord`, `…FighterVertex16Words` are what
  `EmitProductionPrimitiveGroups` and `EmitProductionCrossRun` already call, and
  `NDS_RENDERER_GX_RECORD_FIGHTER` compiles them to bare GX stores in the gate
  configuration.
- `ndsRendererHardwareWriteNormalWord` — the one writer in those functions not
  named "Fighter" — **contains no capture test at all**; it is a bare
  `GFX_NORMAL = value`.
- `ndsRendererHardwareWriteColorWord`'s only fighter-side call sits inside
  `#if NDS_LAB_CULL_PROBE`, which is 0.

The residual `:1281` charge in those owners comes from the **`gl*` macro
redirection block** (`nds_renderer.c:1501-1523`): every `glBegin`, `glEnd`,
`glPolyFmt`, `glMatrixMode`, `glTexParameter` … in this TU is `#define`d to an
`ndsRendererTask29Gl*` wrapper that carries the test. Those wrappers are shared
with the stage and effect paths, so converting them is a **per-caller split with a
text price at 1.85 cyc/byte** — which is exactly the "1,669 in four genuinely
shared helpers … should be priced separately" bucket `FLAG_SWEEP.md` §4.1 already
put them in. It is not an `#if`.

Additionally, for the three **effect** sites of the eight, the fighter writers
would also elide the `sNdsEffectPacketArmed` test at `:1284`, which is **live**
around an effect display list — that would silently break the effect-packet
instrument in every tick-HUD ROM, i.e. in the gate ROM.

## What this cycle did NOT do

- Did not ship legs B or C; both refuted above rather than deferred silently.
- Did not convert `ndsRendererNativeStageBeginRun`'s gate to the whole-table
  proof unconditionally — it consults the certificate and **falls back to the
  original per-run proof whenever no certificate is current**, so the last gate at
  the point of use never weakens.
- Did not re-profile. The 9,369 prediction is c123; the measured `STG` shift is
  **11,328**, 21% above it, and the difference is not attributed.

## Residual risk for the stress/soak cycle — READ THIS

**Boundary proves ONE match.** The certificate survives a Results->START restart by
three independent guards (epoch moved by the discard's releases; topology
generation+stamp rebuild; the write-seam drop), and no guard keys on a pointer.
But the whole class §3.12 names is *"a guard that LOOKS self-invalidating"*, and
the protection this change removes — a per-frame re-proof — was self-healing
against invalidation paths nobody enumerated, whereas a certificate is only as
complete as its enumeration. The enumeration above is structural (those four
fields have no other writers), which is the strongest form available without a
second-match run.

**Ask the soak to exercise the second match deliberately** and read
`gNdsR2TexProofSweepFailCount` and `gNdsR2TextureEpochBumpCount` at the end: a
non-zero `SweepFail` after a restart is the signal, and `EpochBump` must be
non-zero across a scene boundary (it was 0 *within* the match window, which is
correct, and it must NOT be 0 across an entry).

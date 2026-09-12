# Handoff

## Latest integration checkpoint — 2026-09-11

This block supersedes the older pack-gate/native-output restart text below until
the broader documentation refresh is integrated. The compact Donkey/Samus/Link/
Kirby battle-core candidate recovered the tested-window capacity, and subsequent
native-output packages closed DamageSlash, Kirby hidden-part SpecialN, Sword,
Final Cutter effects + weapon, Donkey low-detail Up Smash, and now **Samus Catch**.

Samus Catch is source-defined as one feature: the 21-root hidden grapple-chain
fighter program plus SamusSpecial2 root `349:0x02E0` and its two-frame TEXID
MatAnim. The frozen integrated ROM proves natural engagement through frame 1,536
(`grapple:12`, fallback 0, texture reject 0); the one-minute four-CPU verifier
reaches frame 1,973 / clock 1 with the resource gates green and advances the
first native-only failure to **Link Catch**, LinkModel `324:0x5B68`, status
`0xA6`, `REJECTED_PROGRAM`. Permanent evidence:
`artifacts/visibility/2026-09-11_samus-catch-native.md`.

Resume at that existing P2-3 Link/fighter-production package; do not reopen
Samus Catch without contradictory source or natural-path evidence. P2-3f47
CopyLink remains independently open. Preserve all-ROM native-only rendering,
30 Hz menus, active 1P, and the current optimization/raster deferrals.

Current: ACTIVE. Native-only contract owns every ROM; 1P unpaused 09-10. Zero
native failures is not zero owner bugs -- NO_PROGRAM fires only when *no* owner
claims a list, so an owner that draws nothing passes it. A clean checkout now
builds `smash64ds.nds` (51,395,584 B); `docs/VERIFYING.md` owns the prerequisite
inventory, and an incremental build proves nothing before publishing.

## Four-fighter RAM critical path

**Startup and the GObj latch are CLOSED in the current integration build.** The
four-distinct-kind stress ROM reaches frame 64 with all four pose slots bound;
`gNdsTaskmanGeneralHeapFreeMin=53,128`, `sGCCommonsMaxNum=-1`, 60 active GObjs,
objman panic 0 and allocator overflow 0. The reduced 1,536 B graphics heap reads
16 B peak, overflow 0, no-room 0. The sparse probe now publishes those witnesses.

The remaining blocker is the **pack gate**. A 2026-09-10 shipping-shell,
pack-disabled skeleton rebaseline halts before battle while loading the fourth
raw tree: 77,360 B requested against 23,732 B free after the first three trees
spent 300,304 B. Even deleting those three trees for free and charging zero for
fighter 4 / later startup / binder leaves a relaxed 32 KiB-floor ceiling of
291,268 B. Object-granular liveness over the manifest's transitive extern
closure removes file-loader baggage (largest proof: Yoshi reaches only
2,288 B of the 79,584 B indexed `ITCommonObject`). The estimator's current
worst raw set is Captain+Link+Pikachu+Kirby at 361,362 B; the stricter useful
lower endpoint is Donkey+Captain+Link+Kirby at **351,776 B after all
still-unresolved banks leave for VRAM**, so the direct minimum shortfall is
**60,508 B**. `artifacts/performance/2026-09-10_pack-skeleton-ceiling/CEILING.md`
is the capacity evidence. The older 175,604 / 227,380 figures are historical,
not today's exact shell ceiling. Kirby's hats paid 103,652 (`4d8d9d27179`).

**No lever reaches green.** The largest, low-only at 112,388 B, is DEAD: every
draw reads `detail_curr`, and the dead-up-fall (`ftcommondead.c:529`) and pause
zoom (`ifcommon.c:2955`) both raise it to High in an ordinary match.
Quantization is ~15 K not ~80 K, and the 175,604 allowance is itself optimistic
with two growth deductions never measured -- skeleton build queued. What remains
is census-scale: Kirby's source carries 3,160 Vtx entries to Link's 1,094, one
display list at 272 verts where no other fighter exceeds 32.

## Character select -- bisected, attributed, being fixed

Worst frame is a synchronous fighter load, proven not inferred:

    MSMAX 5 w3=11701888   MSMAXAT 5 f3=219/c0
    MSMAXRES 5 load=1 finish=1 retire=0 dwell=0 payload=6

First bad change `d99a89f8741` moved the closure load onto counted browse
frames; `435ebf00d6d` cut the rate and kept the magnitude by design. Animcache,
audio and the packet flatten are **exonerated with reasons** (BUG_NOTES) -- do
not re-open. Slice in flight; FPS HUD `b242a60acaa`, latch `705e39b4be0`.

## Known state, do not re-derive

Items **25 of 45** owners; Capsule and the first Pokemon batch in flight. There
is no shared branch executor to widen -- each kind owns its triple, and Capsule
is the only remaining kind needing two sibling roots.

Stage visibility: Yoster floor visible, **platform subset submit-proved only**;
Inishie bricks and Congo barrel unproven on screen. **Zebes shafts DRAW** (5
runs, 10 tris, alpha 31), merely hard-edged -- an appearance question long
misfiled as missing geometry. Every stage capture predates the texture corpus
regenerated 09-10, and nothing maps a run to its pixels; per-run crop specified.

Collision parity passes nine stages vs source, host-side, 0.188 s. Audio is
mostly counter-verifiable; Link's absent voice is 4 cues, not ~12. Yoster BGM
garble is out of candidates -- needs an owner listen. Boundary: `p2_shell_loop`
(floor red, 1,968 B vs 32,768), `p2_battle_realtime` 163, `p2_fourcpu_stress`.

## Rules

CodeGraph first; other docs lookup-only. Bank verbose evidence outside the restart
surface. One build at a time, never `make` from a writer, no `-j`/`MAKEFLAGS`. Agent scratch under `builds/` is **gitignored** --
settle findings into the owning doc: `docs/BUGS.md`, board, `docs/p2/BUG_NOTES.md`.

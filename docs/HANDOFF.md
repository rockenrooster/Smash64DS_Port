# Handoff

Current: ACTIVE. Native-only contract owns every ROM; 1P unpaused 09-10. Zero
native failures is not zero owner bugs -- NO_PROGRAM fires only when *no* owner
claims a list, so an owner that draws nothing passes it. A clean checkout now
builds `smash64ds.nds` (51,395,584 B); `docs/VERIFYING.md` owns the prerequisite
inventory, and an incremental build proves nothing before publishing.

## The three RAM constraints, which were one thing for too long

1. **Frame-0 startup OOM.** `ifCommonPlayerTagMakeInterface` fails, ~108 B
   requested against ~88 B free, `sGCCommonsMaxNum` still -1 -- so NOT the
   latch. Gates every four-fighter measurement. Fix in flight (SObj pre-seed).
2. **Frame-45 GObj latch.** Predicted clear at ~30,596 B; proof owed, and it
   cannot run until (1) lands.
3. **Pack gate.** Worst four-fighter set 402,984 B vs a 175,604 B allowance --
   short **227,380 B**. Kirby's hats paid 103,652 (`4d8d9d27179`).

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

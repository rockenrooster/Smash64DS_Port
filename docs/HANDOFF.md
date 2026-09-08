# Handoff

Current: ACTIVE. Contract: docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md — every
new ROM, diagnostics and profiling included, must exclude reference renderers and
software scene compositors. Host reference tools are allowed.
**1P campaign remains paused.** Main Menu/VS/VS Options/Option/Backup Clear accepted;
owner symptoms are in docs/BUGS.md and the evidence in docs/p2/BUG_NOTES.md.

## Current checkpoint

Pushed through a6b52979936. Landed 2026-09-08, each built and pushed:
- Fast-logic battle draws are bracketed in the SObj preview frame and the lower
  battle HUD no longer rides the text console's flag: together, `p2_shell_loop`
  goes from 11 NULL-GObj sprite failures to **zero** at 35,604 B free.
- Particle env colour reaches the KO pillar quad as a baked palette variant; review
  caught that assigning one onto a sheet clobbers that sheet's palette for good, so
  each sheet now keeps a base-palette name.
- Near-plane census skips rigid bindings (its Yoster reading was an artefact).
- Cross-matrix emit is noinline/cold (inlined it overflowed ITCM by 448 B), and a
  ground actor emitting zero triangles now records a native failure.

**`p2_shell_loop` is RED on one thing:** three per lap, domain STAGE, reason
BAD_ASSET, from `ndsRelocCopyMObjSubForAttachment` declining a Dream Land battle
MObjSub whose flags read 0 (`reloc_backend_compat_shims.c:3639`) -- an old
failure the sticky first record hid, not a regression. Candidate ROM:
builds/build-p2-shell-loop/smash64ds-p2-shell-loop-hwtri.nds. The other Boundary
arms, `p2_battle_realtime` and `p2_fourcpu_stress`, are unmeasured since.

## Measured on the ROM, 2026-09-08

`builds/resume-20260908/stage-witness-probe.ps1 -Gkind N -Tag <stage>` walks to a
stage, enters battle, reads its witnesses at two cameras.
- **Saffron gate**: reached every frame, emits zero triangles every time (seen=480
  reject=480); its display list has no native program (root 0x420, the file-160
  rejection), so format/alpha/blend are all downstream of that.
- **Congo barrel**: submitted every frame, opaque, textured, non-degenerate, zero
  native failures; witness is object space, world position owed.
- Saffron ran 19.6 FPS against Congo's 29.1; the per-stage gap is a live lead.

## Active integration

1. Inishie scale-platform plumbing is committed (3a565df230f) but **incomplete**:
   the packet is byte-identical, so the platforms are still absent.
2. Item particles alias bank 0 and draw another effect's: `gITManagerParticleBankID`
   is never assigned and `lbparticle.c:2549` masks without an identity test.
3. Castle roof: near-plane and near-fan dead, eight roof triangles carried and
   admitted; the SOURCE triangle count is still undecoded.
4. Zebes acid is flat both sides, so the dome is shading: the generator averages
   corner alphas and invents values (0xf3/0xe8) the source does not have.

## Preserved work and operating rules

Broad unrelated dirty work (1P, tags, pipes, Pakkun, assets, user P3/P4 docs) must
be preserved; do not resume campaign or redo CSS repairs.
**Launch codex as `-m "chatgpt-web/extra-high" -c model_reasoning_effort="xhigh"`;**
the config default gpt-5.6-luna is a spent budget. GLM truncated every run today.
Muse (`swarm-*`, up to 5) and Claude subagents work; launch writers only between
builds, never let one run `make`, one build at a time, no -j/MAKEFLAGS.
CodeGraph first; a restart reads this file and the board, others lookup-only.
Bank verbose output and bounded UTF-8 log reads; start each cycle with verify-all.ps1 -Profile Boundary -List and git status --short.

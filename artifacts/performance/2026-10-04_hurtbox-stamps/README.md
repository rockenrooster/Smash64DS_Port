# Hurtbox reject caches keyed per fighter instead of a global latch epoch: measured, slower, 2026-10-04

The fixed-point hurtbox reject (`src/port/nds_p2_hurtbox_reject.c`) keys its
world and box caches on `gNdsP2HurtboxLatchEpoch`, bumped at every FTParts
latch clear (~17 a frame: each fighter's two resets a tick, every SetStatus).
Hypothesis: a victim's cached worlds die when any OTHER fighter resets, so
weapon/item tests and attackers after a mid-phase SetStatus rebuild worlds
that are still valid. Change: a per-battle-slot stamp drawn from the counter,
bumped only for the fighter owning the cleared joints (all slots when it
cannot be named); caches compare against the victim's stamp (build
`build-gate-1004h`).

| arm | WORK P50 | WORK P95 | > 1.12M |
|---|---:|---:|---:|
| control (`../2026-10-04_owner-validate-pool/gate-v1`) | 845,952 | 1,178,752 | 151 |
| per-fighter stamps (`gate-h1`) | 847,488 | 1,184,768 | 154 |

Paired by frame: median +1,216, mean +1,490; frames over 1.0M +1,536.
Digest identical (no stale world was ever used). The cross-fighter
invalidations were not where the kernel's builds come from -- each victim is
tested after its own update, so its worlds are rebuilt either way -- and the
bump path (an ITCM shim calling into main RAM to name the fighter) costs more
than the reuse saved. Reverted.

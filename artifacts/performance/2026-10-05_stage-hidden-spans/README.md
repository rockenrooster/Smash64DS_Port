# Stage GX fast commit across hidden bindings (Sector Z's segment 0), 2026-10-05

`src/nds/nds_stage_gx.exec.inc` (word `gNdsStageGxFastHidden`). The one-pass
GX commit declined any stage segment one of whose bindings was hidden; Sector
Z's segment 0 holds the owner-hidden wing-platform collision proxy (r58), so it
declined on every frame and drew run by run (`ndsStageGxDraw`, ~49K cycles a
band frame in the clean profile, plus per-run texture binds). The per-run path
skips a hidden binding's runs whole -- no words, no state, no depths -- and
every run's words open with its own material state, so a segment is now built
for the hidden subset in force: the left-out runs' patches are skipped and
their words cut out of the DMA as span breaks (at most four spans; a segment
whose hidden subset keeps moving goes back to declining after eight rebuilds).

Clean sweep ROM `build-lab-clean1005i`, Sector Z, word 0 (`hs0`) -> 1 (`hs1`):
P50 935,872 -> 911,680, P95 1,303,232 -> 1,283,904, over 1.12M 274 -> 223,
paired median -23.2K, replay digest identical; the lab decline counters
(`gNdsLabStageGxFastWhy`) read 0 for every reason with the word on.

FIFO identity: lab sweep ROM `build-lab-sweep1005h` (stage GX hash of every
word sent to the FIFO, per logic frame; scratchpad `stghash.ps1`), Sector Z,
logic frames 1,000-1,400: the per-run arm (`stghash-hs0.log`) and the span arm
(`stghash-hs1.log`) hash identically on all 201 sampled frames, as does the
span-state hash.

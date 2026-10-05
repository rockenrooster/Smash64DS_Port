# Sector Z degenerate stage bindings in the GX fast commit: measured, not kept, 2026-10-05

Hypothesis: Sector Z's stage segment 0 declines the fast commit because a
binding's composed matrix is degenerate (all-zero w column) and fails the near
box. An experiment word skipping such bindings in the near test and composing
them to an outside matrix (`build-lab-sweep1005b`, lab sweep ROM) changed
nothing: same ROM `c0` -> `c1` P50 1,011,328 -> 1,011,200, paired median -64,
digest identical; the official gate (`gate-q1`, `build-gate-1005q`) was
neutral (1,109,888 P95, 1,861 two-VBlank presents). The patch was reverted.
The decline's real cause, found with lab counters later that day
(`../2026-10-05_fflower-route/szwhy-g1`), is the permanently hidden
wing-platform proxy binding.

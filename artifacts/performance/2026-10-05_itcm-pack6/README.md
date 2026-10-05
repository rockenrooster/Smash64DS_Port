# 2026-10-05 Sixth ITCM pack

`linker/nds_hot_text.ld`: sixteen main-RAM / `.text.hot` functions admitted to
ITCM by name, from section D of a fresh official-gate census
(`artifacts/task37-census/gp5-collision`, frames 1,100-1,900; 1,200 B free),
ranked by non-mem stall a byte and taken only where the function is the same
size in the gate and shipping ELFs (`ndsMObjStepScaled`,
`gcParseMObjMatAnimJoint` and `ndsRendererAdapterNdlDispatchEffect` inline
into larger bodies in the shipping ROM). 384 B. All three targets link:
gate `.itcm` 31,920, published `smash64ds` 31,600, P1
`smash64ds-battle-playable-hwtri` <= 31,960 of 32,736.

Official gate, `build-gate-1005u` (before) against `build-gate-1005v`, two
runs each: paired median -1,536 / -1,600, P95 1,121,728 / 1,121,408 ->
1,120,320 / 1,121,024, two-VBlank presents 1,858 -> 1,860 / 1,858, replay
digest identical (`gate-v1`, `gate-v2`, `gate-u2`;
`../2026-10-05_collision-window5/gate-u`).

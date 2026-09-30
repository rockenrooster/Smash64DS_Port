# P2-2p8: the build-dependent replay digest was a stale loaded-file pointer (2026-09-30)

Solo, no subagents. Lab = the NDS_LAB_FOURCPU_SWEEP ROM (Donkey/Samus/Link/Kirby),
one full match per run, every new map-collision word off; run files are in
`artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/` under the arm names
below. Opened by `2026-09-30_p2-2p8-map-collision` ("the simulation depends on
the build's heap layout").

## Symptom

Two builds whose only difference was code in `reloc_backend_mp_collision.c`
(`cq0`, `cb0`; the new words off in both) gave different digests on Dream Land
(from frame 1370), Saffron (from 1137) and Sector Z (from 1259), while within
each build every word pair agreed. Heap shifts at the stage load (24-128 B),
a 64 B BSS pad and 20K/70K-tick storage read delays changed nothing; small
code changes flipped Dream Land and Saffron between the two digests.

## What misled

`-ftrivial-auto-var-init=zero` over the whole build made the two layouts
agree (`z1`, `z2`), and zero-init limited to the port units, then to the
`scene_backend` unity, did too (`zb1`, `zb2`); a 16-region pragma group test
(`zg0`-`zg3`) decoded "map collision, first half". Every region candidate
read clean on inspection, and `mpProcessUpdateMain`'s unset `result` (entered
with `is_coll_end` already TRUE) was never reached (`ce*`, counters 0). Zero
and pattern initialisation change code size, code size moves the heap, and
the heap sizes the animation cache arena -- which is what decided the bug.

## Mechanism (traced)

A per-component digest (`mk*`) put fighter link slot 1 (Samus) first at
Saffron 1135 B. A trace armed on the last common tick's digest (`tr*`-`tr4*`)
showed Samus entering EscapeB (back roll) in both builds, but in the new
layout its TransN joint had no script (`event32` NULL, `anim_wait` =
AOBJ_ANIM_NULL): the roll ended the tick it began. The install's raw figatree
words were `0x0001004c, 0x00020059, ...` -- the clip's relocation chain,
never fixed up -- so every joint resolved NULL.

`ndsRelocForceLoadFighterAObj16File`'s full-load path registers the clip
(`loaded`), byte-swaps it, stores the raw template in the animation cache,
then finalizes `loaded`. The store can wrap the cache's raw ring over another
fighter's pinned zero-copy clip; that pin's rescue copies the clip home and
calls `ndsRelocPrepareFighterAnimHeapOverwrite`, which memmoves
`sNdsRelocLoadedFiles`. `loaded` then named a different record: finalize
fixed that one up and left the new clip raw. Whether the ring reached a pin
depended on the arena's size, i.e. on the build.

## Fix (`src/port/reloc_backend_assets.c`)

- After the raw store, the full-load path looks its record up again (by
  asset, checked against the heap) before finalizing.
- `ndsRelocRegisterLoadedFileImpl` looks the record up again after it
  allocates an extern-id block (any allocation can take the elastic cache's
  bytes back, and a pin rescue can compact the table), and re-checks capacity.
- Residual, not observed: `ndsRelocFinalizeLoadedFile` holds `loaded` across
  external fixups, which can load dependency files; fighter clips carry no
  externs.

## Verification

- The fix in three layouts (`lf1` evictions + no init, `lf2` no evictions,
  `lf3` zero-init): IDENTICAL on all nine stages (`lf3` on four); the final
  source (`lf4`, both fixes) IDENTICAL to `lf2` on all nine. Every layout now
  gives the `cq0` digest on Dream Land and Saffron; the first fix fired once on
  Saffron in the layout that had diverged (`fx_n_g7`).
- Gate ROM (`b13_a`, `b13_b`): IDENTICAL to the previous commit's gate runs
  (`b12m1_a`); WORK-H P50/P95 914,112 / 1,226,560 against 913,472-913,984 /
  1,229,120-1,229,440 (drift), two-VBlank 87.0%, heap low-water 350,636, native
  failures 0. The two arms' rows are byte-identical.
- The deterministic path shows the owed native owner on Sector Z (119 failures,
  identity 393,215, status 224, reason 2; Saffron 3 of the same record, as
  before). The `cq0` layout already took it; the `cb0` layout diverged first.

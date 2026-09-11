# Four-slot pack-disabled skeleton rebaseline — 2026-09-10

This is the permanent memory-budget record for the shipping-configuration shell
skeleton requested by `docs/reviews/Review_Deriving_Fighter_Live_After_Setup_Set.md`.
The run falsified the historical exact-ceiling derivation before it could produce
a battle low-water, so this record preserves the failure and the safe bound it
does establish.

## Build identity

- integration HEAD before build: `de648aaae97ce8e432c2918975216ad3228fffd5`
- working-tree binary diff hash before build: `858618caf4cadba5d68f0e2477581e68c0b863c8`
- command: `make TARGET=smash64ds-p2-shell-hwtri BUILD=build-p2-skeleton NDS_R2_BATTLEPACK=0 NDS_P2_SHELL_ARGMAX_ROSTER=1`
- generated config: `NDS_R2_BATTLEPACK=0`, `NDS_P2_SHELL_ARGMAX_ROSTER=1`
- ROM SHA-256: `F40C983FDDE11E093C2F6B67338FF6BA9D040F44303DB262C0088DBBFDC25446`
- ELF SHA-256: `AE9D0A9F7AD41D9986735BAC45CF4768233CB38A9D276B469BB4934B85A77E30`
- ROM bytes: 51,124,224
- ELF bytes: 16,043,000
- linked ARM9 sections from `arm-none-eabi-size -A`: `.itcm` 31,936 B,
  `.dtcm` 8,736 B, `.dtcm.bss` 152 B, `.text.hot` 4,472 B,
  `.text.hot.draw` 5,936 B, `.main` 1,435,256 B, `.main.rw` 212,412 B,
  `.main.bss` 906,800 B. The present generated fighter relocation catalog is
  part of this baseline. A future semantic-pack binder must be differenced
  against this exact ELF/map; it is not valid to call today's entire catalog a
  new `D_binder` charge.

The build was isolated under `builds/build-p2-skeleton`; it did not publish a
root ROM. No other make or melonDS process was running when the build started.

## Dynamic result

The one-minute shell probe stopped before battle, at the fourth main-file tree:

```text
FOURKIND SETUP hit=1 kind=3 scene=22 free=418920
FOURKIND BIGALLOC size=83200  scene=22
FOURKIND SETUP hit=2 kind=1 scene=22 free=297820
FOURKIND BIGALLOC size=116944 scene=22
FOURKIND SETUP hit=3 kind=7 scene=22 free=162548
FOURKIND BIGALLOC size=100160 scene=22
FOURKIND SETUP hit=4 kind=2 scene=22 free=23732
FOURKIND BIGALLOC size=77360  scene=22
FOURKIND HALT count=1 arena_id=65536 request=77360 align=16 headroom=23732
FOURKIND ARENA chosen=1294336 allocfail=105 ... free=23732 overflow=1
FOURKIND ROSTER s0=0x030003 s1=0x010103 s2=0x070103 s3=0x020103
FOURKIND LOWWATER generalfreemin=4294967295 ...
```

The exact captures are
`artifacts/verification/2026-09-10_p2-skeleton-fourkind.txt` and
`artifacts/verification/2026-09-10_p2-skeleton-fourkind-census.txt`.
`generalfreemin` remains its sentinel because battle never starts; using it in
the old `D_other` equation would manufacture a result.

The >=16 KiB census names the large battle-scene consumers before the fighter
trees: 153,600 B scene/cache allocation, 208,672 B `scVSBattleSetupFiles`,
185,920 B collision, 82,976 B item manager, 33,152 B additional battle state,
52,736 B + 28,352 B effect manager, then the fighter trees above. Smaller
allocations are intentionally outside this threshold and remain charged by the
observed free-space witness.

## Safe current capacity statement

The first three raw fighter-tree requests total:

```text
83,200 + 116,944 + 100,160 = 300,304 B
```

Immediately before the fourth raw tree, free space is 23,732 B. Remove those
three already-resident raw trees for free and charge zero bytes for every cost
that still remains after this stop (the fourth fighter instance, later startup,
runtime variation, and future semantic binder). That deliberately impossible
best case has 324,036 B available. Preserving the required 32,768 B floor gives:

```text
W <= 23,732 + 300,304 - 32,768 = 291,268 B
```

This is a **relaxed upper bound**, not the exact pack ceiling. Every omitted
post-stop cost can only lower it. The current estimator worst set is 402,984 B,
therefore the pack is still provably RED even under this relaxed current-shell
bound:

```text
402,984 - 291,268 = 111,716 B minimum shortfall
```

The historical 175,604 B ceiling / 227,380 B shortfall came from a previous
binary and must not be presented as today's exact shell ceiling. It remains
useful historical context only. A new exact threshold needs a skeleton that can
reach the complete four-slot state while excluding replacement-pack bytes; this
raw-tree control cannot do so.

## Decision

Do not spend another cycle trying to re-pin the old equation from this control.
The current direct evidence already answers the go/no-go question: the 402,984 B
pack needs at least 111,716 B more structural reduction before an exact ceiling
could possibly make it fit, and the real required reduction is larger because
fighter 4 and later battle costs were given a zero-byte allowance above.

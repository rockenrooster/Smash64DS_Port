# P2-2p8 phase L — consume exact packet precheck before owner preflight

Date: 2026-09-15

## Removed steady-hit work

The adapter already performs `ndsRendererFighterPacketPrecheck()` before it
decides whether native material rows may be skipped. On an exact hit that check
proves the packet key, texture residency/generation and live texgen patch against
the same production roots passed immediately to the owner executor. The owner
executor nevertheless ran whole-owner preflight, rebuilt the packet key, matched
it again and recalculated texgen before replay.

The retained route consumes that exact precheck first. A prechecked hit submits
the packet before whole-owner preflight. A miss or unexpected handoff failure
falls through to the original preflight plus ordinary replay/record route. The
ordinary route still rebuilds its key and patches texgen. Captain HIGH remains
ineligible because its alpha-test state is outside the FIFO packet.

The streamed Kirby copy-hat lifetime was checked before enabling the shortcut.
BattleShip changes the copy model through `ftParamResetModelPartAll`; the DS shim
routes that topology mutation through `ndsFighterRendererInvalidateMaterialCachesForSlot`,
which invalidates that player's fighter packet. The shortcut therefore cannot
reuse a packet across a live copy-hat topology change.

## Host/static correctness

- camera/texgen + native failure tests: **8/8 PASS**;
- native owner wiring: **39 owners, 0 gaps**;
- fighter production manifest: **PASS**;
- final linked ITCM: **32,704 / 32,768 bytes**;
- no measurement route/counter remains in the retained source.

## Same-ROM timing A/B

Measurement ROM SHA-256:
`F121EE317606A1B543286BD69529A0AD2B0F26409EDD673010EDD1F6A0D075D5`.
Both arms use the same four-CPU target, input, renderer state and packet corpus;
only the one-time lab route selector differs. In the full match both arms record
**6,673 packet precheck hits = 6,673 packet replay hits**, **178 records** and
**0 declines**.

Frames 1400..1527 (128 matched samples):

| metric | control | candidate | paired result |
|---|---:|---:|---:|
| FTR P50 / P95 | 337,024 / 670,208 | 312,768 / 649,024 | 128/128 wins, median **-26,304** |
| WORK-H P50 / P95 | 1,750,528 / 2,445,184 | 1,733,120 / 2,467,520 | 119 wins / 9 losses, median **-26,208**, mean **-25,413** |

The short-window WORK-H P95 sign is noisy, while every fighter frame improves,
so the verdict uses the required whole-match extension rather than the isolated
128-frame P95.

Frames 2..1973 (1,972 matched samples):

| metric | control | candidate | paired result |
|---|---:|---:|---:|
| FTR P50 / P95 | 396,224 / 770,112 | 365,312 / 755,712 | 1,894 wins / 76 ties / 2 losses, median **-30,016**, mean **-25,934** |
| WORK-H P50 / P95 | 1,692,352 / 2,422,528 | 1,665,088 / 2,399,616 | 1,744 wins / 76 ties / 152 losses, median **-28,672**, mean **-25,748** |
| 5+ VBlank presents | 249 | 227 | **-22** |

The whole-match WORK-H P50/P95 cut is **27,264 / 22,912 ticks** and exceeds
the project's 14,080-tick cross-build floor. The work reduction is isolated to
the fighter hit path: SRC is essentially neutral in the paired population.

## Final hard-on and Boundary

The measurement selector/counter was removed before the final build. Final
no-probe stress ROM SHA-256:
`45869C2A60A37124738D1F5866CF84B69E4BA4F787E2DFD9E0A961B85AFCC203`.
The official four-CPU verifier exits **0** with:

- WORK-H P50/P95 **1,654,528 / 2,385,536**;
- FTR P50/P95 **362,944 / 753,792**;
- VBlank 2/3/4/5+ **103 / 818 / 817 / 235**, max 13;
- Donkey/Samus/Link/Kirby observed exactly as configured, draw mask `0xF`;
- native failures/direct rejects **0/0**;
- pose bind-full/track-overflow/wide-fallback **0/0/0**;
- BPS1 directory **9,320 B / 681 hits / 0 fallback / 0 failures**;
- BGM direct/fallback **331/0**, FGM direct/fallback/stdio **329/0/0**;
- graphics heap overflow/no-room **0/0**;
- general-heap low-water **108,096 B**;
- weapon pool **10 entries / 2 live high-water / 0 refusals**.

The complete `Boundary` profile then exits **0**: shell loop, realtime battle
and four-CPU stress all pass. The realtime arm also produced and accepted the
canonical screenshot pair
`artifacts/visibility/2026-09-15_canonical_fast_113824-9777518-p54700.png`
and `_next.png` under its automated image thresholds. Boundary's rebuilt stress
ROM reports WORK-H **1,653,952 / 2,389,376**, native **0/0**, and the same
108,096 B heap low-water.

P2-2p8 remains **RED** against the product target of approximately 1.12M WORK-H
P95 and >=95% two-VBlank presentation. This phase removes one complete class of
redundant steady packet-hit preparation; it does not close the remaining
whole-frame/SRC/tail gap.

# P2-2p8 phase J — pose running-joint mask KEEP

After the live-track mask, `ndsFtPoseUpdate` remained the largest movable
non-renderer body and added about 58.8K cycles/frame on every one of the seven
worst frames in the post-phase-I ARM9 census. The retained path tracks which
bound hierarchy entries can still run and skips entries once their
source-visible `anim_wait` reaches NULL. Track evaluation, scripts and stored
pose values are unchanged.

The mask is bounded to 64 walk entries. Wider future hierarchies use a cold
complete-scan fallback; `gNdsFtPoseRunMaskFallbacks` makes that visible and the
standing four-CPU verifier requires it to remain zero on the accepted roster.

## Correctness

Oracle ROM SHA-256:
`558F5CDCC9BAA089B37023AD7A1C286168806DFD72DE81B1F98D498821E623AE`.
The generic-player oracle completed **190,062 field comparisons** with **0
mismatches**, **0 pose mismatches**, zero track overflow and zero runaway.
Evidence: `pose-oracle.txt`.

## Same-ROM timing A/B

The first measurement route interleaved the historical scan and masked walk in
one hot loop; it saved mean CPU but perturbed P95. The control was moved to a
`cold,noinline` helper so the candidate hot path matches the intended shipping
layout. The resulting single ROM SHA-256 is
`E9453FF306CB20271AD98F879F3E71FF38B0F98C2ABDA39401BA07C66BC96468`.

Frames 1400..1527:

| metric | scan control | run mask | delta |
|---|---:|---:|---:|
| WORK-H P50 | 1,688,064 | 1,692,864 | +4,800 |
| WORK-H P95 | 2,438,336 | 2,411,392 | **-26,944** |
| SRC P95 | 1,063,808 | 1,059,200 | **-4,608** |
| SINT P95 | 520,064 | 511,808 | **-8,256** |

Whole one-minute four-CPU match, frames 2..1973:

| metric | scan control | run mask | delta |
|---|---:|---:|---:|
| WORK-H P50 | 1,631,808 | 1,631,296 | **-512** |
| WORK-H P95 | 2,366,528 | 2,357,120 | **-9,408** |
| SRC P95 | 1,066,880 | 1,063,168 | **-3,712** |
| SINT P95 | 605,312 | 599,936 | **-5,376** |

Paired whole-match WORK-H improves on **1,309/1,972** frames (median -1,600,
mean -1,758 ticks/frame); SRC improves on 1,486 and SINT on 1,599. Both arms
perform exactly 677 binds, 15,345 updates, 258,827 joint ticks, 137,269 joint
evaluations, 121,558 holds, 561,141 track evaluations and 58,534 script steps,
with zero track overflow. The A/B therefore prices traversal only.

## Final hard-on checkpoint

The measurement route was removed. Final four-CPU ROM SHA-256:
`B1C037FB1EF0BB5339EC7E52E0C09D595B49DCEF320C5E20921EE076F813312A`.
The standing one-minute verifier passes with:

- WORK-H P50/P95 **1,680,384 / 2,389,376**;
- SRC P95 **1,060,928**, SINT P95 **597,120**;
- pose binds/full/track-overflow/run-mask-fallback **677/0/0/0**;
- native failures/direct rejects **0/0**;
- BPS1 reads/misses/failures **676/5/0**, directory **9320/681/0/0**;
- BGM direct/fallback **335/0**, FGM direct/fallback/stdio **330/0/0**;
- general-heap low-water **108,096 B**, 82,496 B above the safety floor;
- VBlank 2/3/4/5+ histogram **103/749/881/240**.

Final shell ROM SHA-256
`021777C3A456A7B668661AC3A8B17DE547C0BA24D21EC5B858AEE2CE99E69931`
passes the natural Pupupu realtime smoke, native-only link checks, canonical
texture/detail assertions and published-ROM contract.

The same-ROM A/B is the performance verdict; the final hard-on run proves the
shipping layout and guards. P2-2p8 remains RED against the 1.12M-tick and
>=95% two-VBlank product targets.

# Handoff

Updated: 2026-08-08. **Whispy Route 7 is promoted; the campaign remains on
R2-07's performance gate and the board's gate lane G1→G2→G3→G4.**
Published pair: `smash64ds-battle-playable-hwtri.nds` `C045544B…`,
`smash64ds.nds` `AB9DE9E3…`; flag-identical tick-HUD sibling
`builds/build-tick-hud-buckets` `574F6F40…`.

## Read this first: every 128-frame measurement in the archive is unusable

**The 128-frame window reads the cheapest 6% of the match.** Same ROM, same
options, Boundary arm: frames 441–568 read `WORK-H` P95 1,156,992 at 8.7% over
gate, against 1,463,104 and 44.6% whole-match — P95 understated ~306,000, the
over-gate rate five times. It sits in stop0–stop1 (stop0 is 8 of 96 over gate,
stop2 is 97 of 97). `sample-tick-hud-buckets.ps1` takes repeated ring dumps as
of `58ca8723` (`-Samples` to 4096, `-RingStopStride` default 96, **ROM
byte-identical**). Never take a gate reading on 128 frames again.

## The two baselines — label every figure with its arm AND its coverage

Re-banked cycle 80 on the corrected seed; both arms now run the **same 60-second
match** (coverage 86.7%, clock 52 s → 0 s, logic:presented 2.000), and both
windows end 43 frames past the buzzer in GAME SET.

| arm | role | coverage | `WORK-H` P50 | P95 | over gate |
|---|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE (owner, 2026-08-05)** | 86.7% | 1,094,464 | **1,624,064** | 704/1600 (44.0%) |
| **Boundary** mode 163 | shipped configuration | 86.7% | 1,082,112 | 1,476,672 | 673/1600 (42.1%) |

Slips 0 in both. **Gap to the gate is 503,684** (Boundary trails at 356,292).
The old 485,060 came off a 12.6% window: the gate arm seeded `time_limit = 7`.
Fixed — the soak's long match is now `NDS_R2_SOAK_MATCH_MINUTES` and
`probe-match-window.ps1` reads the match timer out of the guest.
The owner's bar: the whole match under the P95 budget on the both-CPU config,
loading states excluded; the shipped ROM stays the Boundary hwtri pair.
`Makefile:305-308` still forbids reporting a both-CPU P95 as the Boundary
figure. Both-CPU is only ~10% worse at P95 — harder, not a different animal.

**The Boundary verifier is GREEN again** (cycle 80). It had been red since
`fcf93d00` — **35 commits, including the whole cycle-79 gate lane** — on a stale
`EXPECTED_CENSUS_SHA256` that aborted the profile in pre-flight. Bisected to six
`renderer_key_contract` constants; no texture corpus moved. Board carries the
detail and the retraction. **Re-pin in the commit that changes what it covers.**

## The target: effect DObj submits, and the denominator is the display list

`MISC` is the tail and it is a **cost, not a marker** — it passes the test that
killed the old 0.21-quads-per-frame claim (`MISC`-hot and `SRC`-hot are
near-disjoint; among `SRC`-normal frames `MISC`-hot is 79.9% over gate against
32.9%). Inside `MISC`, effect DObj submits are **99.3%** of the excursion:
359,717 ticks/frame on over-gate frames, **0** on clean ones.

Phase split — an **exact partition**, nine phases summing with delta 0, all four
nesting invariants holding. 160,627,648 ticks · 21,854 triangles · **1,360
display lists** · 1,436 nodes:

| phase | share | per list |
|---|---:|---:|
| **generic DL interpreter (Exec − Tex)** | **65.57%** | 77,440 |
| **texture resolve (Tex)** | **21.41%** | 25,289 |
| Matrix | 6.93% | 8,179 |
| everything else | ~5.8% | — |

**Recoverable is ~315,000, not 363,004** — `MISC` +365,136 against `WORK-H`
+317,136 means ~48,000 displaces `FTR` rather than adding to the frame.

## What is dead, so nobody re-derives it

- **Projectiles** — weapon DObj submit medians **44 ticks/frame**. Fox's laser
  and Mario's fireball are not the tail. Native projectile owners are
  architecture work, not gate work.
- **Particles** — flat ~47,000/frame, hot–cold delta 4,838, inside the floor.
  A P50 lever, never a gate lever. This retires SwitchPlan §7 option 2 (15 Hz
  round-robin) as a *gate* answer.
- **Texture thrash** — 1 upload per ~1,408 frames, 0 evictions, 0.0071% of the
  effect cost. `Tex` is entirely cache-**hit** key-build/hash/lookup.
- **`Find`** (0.44%) and **`Material`** (0.25%) — both named prime suspects,
  both refuted. `Material` also clears §3.11: it bump-allocates from
  `gSYTaskmanGraphicsHeap` with its caller saving/restoring the pointer and
  bounds-checking, so it cannot block the way `syMallocSet` can.
- **`FTR` as the gate** — flat only inside the bad window; across the match it
  drops −161,024 on `MISC`-hot frames and is **anti-correlated** with the tail.
- **Task 56 strips** — REVERT, and not for the recorded reason: 1,878 → 1,012
  vertices links in, but **the ROM hangs the present loop** (cannot reach
  presented frame 12 in 900 s; control does frames 10–13 in 30 s). Three
  attempts, two builds, three days. The `PERF_LEDGER` KILL row citing `FTR`
  +5,824 has no completed run behind it.
- **L7 fixed-point collision** — wired, priced at +534 won against 6,481 lost to
  its own text, removed. Collision is 2.9% of the over-gate premium.

## The interpreter is honestly generic — ANSWERED, so the packet path is correct

The cap-versus-end question is settled and the interpretation was fixed before
the number arrived:

| | |
|---|---:|
| lists | 1,360 |
| commands executed | 217,686 |
| **mean commands per list** | **160.1** |
| terminated at `G_ENDDL` | **1,360** |
| terminated at cap | **0** |
| other blockers | 0 (mask `0x0`) |

**Every list stops at its own terminator.** Nothing hits `max_commands`, nothing
ends on `BAD_BRANCH` / `TOO_DEEP` / `UNSUPPORTED` / `NO_END`. There is no
overrun to fix. **The honest per-command cost is 626 ticks** (136,334,848 exec
ticks over 217,686 commands) — the circular 12.54 was 50× too low, and 160
commands at 626 ticks is exactly the ~100,246 per-list constant.

So the **precompiled-packet path is the answer, not a workaround**, and it has
no defect-shaped alternative in front of it: build the GX packet at match load,
reserve patch offsets for the matrix and dynamic colour words, patch per frame,
submit. No re-parse, no per-list config rebuild, no per-command dispatch.

## The `Tex` memo is REFUTED, and the ROM has 96 BYTES of headroom

Built as approved and reverted: 10,336 consults, **471 hits (4.56%)**, 7,517
evictions of 7,525 fills, `Tex` ticks *up* (41,393,152 vs 34,394,304); working
set ~175 keys ≈ 6.3 KB, far past the headroom that exists.

**The binding finding is RAM, and it is 96 bytes, not 1.4 KB (corrected cycle
82).** "+1,408 boots" was a delta over a datum the tree kept outgrowing — 1,312
was already spent by cycle 80. Price it absolutely: highest `fake_heap_start`
proven to boot **`0x02294804`**, lowest proven to fail **`0x02294b24`**, current
control **`0x022947a4`**. **Text counts as much as bss.** Run
`scripts/check-boot-headroom.ps1 -Build <dir>` after every lab build (OK /
UNPROVEN / OVER CLIFF, exit 1); a failing arm never reaches presented frame 1 and
the harness reports a timeout that looks exactly like a hung emulator.

## Next single step — split SBAS, the measured owner of the gate arm's tail

**The load-frame exclusion is REFUTED; the gap is 503,684, not 237,956.** The
owner's "loading states excluded" bar must not be applied through the
`SRC > 2x median` rule: it thresholds on the bucket being attributed (circular
for SRC, 68.9% -> 52.3%), swings the gap **3.08x** across plausible thresholds,
and drops frames that are not loads — 100 of 122 are isolated singletons spread
evenly through play, `FTR` 1.01x / `STG` 0.99x / `MISC` 1.04x, only `SRC` up. It
moves the gate arm 3.08x but Boundary only 1.09x; no loading filter could do
that. Audit: `scripts/analyze-load-frame-exclusion.ps1`.

**The SRC instrument LANDED (cycle 85); hit detection is NOT the owner.** G2's
headroom paid for it (+1,152 bytes, boot PASS, arena at full request). Both arms,
whole match, identity error 0: `SBAS` (decomp sim path residual) owns **87.1%** of
SRC's excursion on the gate arm / **89.0%** on Boundary against `SHDT` 12.9%/11.0%,
`SWRM` inert; gate-arm upper bounds `SBAS` 315,456, `SHDT` 55,104. E35 is real but
minority — 79.2% of over-gate frames sit at the `SHDT` floor. **Split `SBAS` next.**

**Boundary for all of it.** Same geometry, same textures, same materials — the
effect models are a closed `BUGS.md` row the owner confirmed by eye and paid for
deliberately. Cheaper, never worse. A change that alters a visible pixel of the
shield, revival platform, impact wave or reflector needs the owner.

## Measurement rules this cycle established or re-proved

- **Per-bucket placement floor is ≥8,544**, not the ±5,376 that applies to
  `WORK-H` P95 — calibrated by an arm that only *removed* code and moved `FTR`
  +8,544. Judge on `WORK-H`; buckets locate, they never decide.
- **1.85 cycles of `FTR` mean per byte of added ARM text.** A change that adds
  text must beat its own footprint.
- **Verify a counter is live in the shipped configuration BEFORE the measuring
  run.** Six per-stop counters read 0 all match because they were proof-scoped;
  a zero is only reportable once the counter has been shown able to be non-zero.
- **Eliminate candidates with a liveness probe on an already-built ROM** before
  spending a measuring run. The GX flush (64–128 ticks) and the OAM path (0)
  came off the list that way.
- **`ALL` is VBlank-quantized** and hid a +52,928 that came straight out of the
  wait. Read `WORK-H`.
- **Do not multiply a number back by what you divided it by.** The "8192 × 12.54
  = 102,727 against 102,730" agreement was circular and was not evidence.

## Open and unowned

All parked items live on the board's **Parked** list (one place, not two).

## Restart surface

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Boundary contains only `battle_playable_realtime`, mode 163.

`docs/P1_EXECUTION_BOARD.md` is the only dynamic queue — rewritten cycle 79
from a 10,207-line log; history in
`docs/optimization/archive/P1_EXECUTION_BOARD_pre-cycle79.md`.
`docs/Smash64DS_Runtime2_SwitchPlan.md` is the charter. `docs/BUGS.md` carries
the owner's verdicts — they edit it directly, so preserve their wording.

A clean checkout must build through `build.ps1`, not bare `make`: four of six
generated `.inc` files are gitignored. For iteration, `make p1-tick` builds the
measuring ROM and `make p1` the published battle pair — bare `make` builds the
P2 ROM P1 does not ship. Never pass `-j`, never override
`MAKEFLAGS`, one build at a time. Never build a published target name for lab
work — those hardcode output to the project root whatever `BUILD=` says.
Preserve canonical mode 163, renderer mode 9, mip 0, static textures, source
countdown, Dream Land water at frame 0, Task 16 `1/1/1`. Do not edit `decomp/`.

Run `New-Smash64DSSnapshot.ps1` last, and nothing after it.

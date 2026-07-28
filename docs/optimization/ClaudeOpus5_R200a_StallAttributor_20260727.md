# R2-00a — The stall attributor, and the excursion that was never work

**Date:** 2026-07-27
**Phase:** R2-00a (`Smash64DS_Runtime2_SwitchPlan.md` §7).
**Status:** Built, validated, **gate met**. Emulator not yet adopted into the repo.
**Standing rules apply.**

**Instrument:** `melonDS-Accurate` branch `r2-stall-attributor`, commit `4a1abf61`;
binary `d81fc0bf318756fdb1d4f27f376cc7666947e1bf613210ef56019a2aba329059`.
**Validation windows:** presented frames 453–454 (the load-free `SRC` excursion)
and 455–456 (median), replayed against the surviving `builds/build-t108-w453`
and `build-t108-w455`.

---

## 1. Provenance, settled

The campaign's binary `de80e46b…` came from the *uncommitted* `melonDS-cache-profiler`
tree, while `melonDS-Accurate` holds a committed copy — so before building
anything the two had to be shown equivalent.

They are. Across the whole tree, 36 files differ and **35 differ only in line
endings; the sole real difference is `README.md`.** Both share root commit
`e0255fb9` and identical CMake caches (Release, `-O3 -DNDEBUG`, same UCRT64
compilers, `BUILD_STATIC=ON`, `ENABLE_JIT=ON`). The binary hash difference is
build nondeterminism, not source drift. Built on `melonDS-Accurate`.

## 2. What was added

Eleven CSV columns, additive, on both `arm9-profile.csv` and `.regions.csv`:
`issue, icache_fill, dcache_fill, write_buffer, bus_contention, dma_hold,
cart_spin, interlock, halt_wait, gx_paid, gx_blamed`. Metadata is `v3` and
carries `stall_partition_residual`, which **must be 0** — the classes through
`gx_paid` partition `total_cycles` exactly, so nothing can be silently
misattributed. Attribution is flush-on-change with save/restore, so nested calls
charge the innermost unit and uninstrumented time falls into a named residue
rather than onto the wrong class.

**The GX case needed care and is the reason a naive counter would have lied.** A
full FIFO does not block the store: `CmdFIFOWrite` diverts the entry to
`CmdStallQueue` and raises `CPUStop_GXStall`; the scheduler then skips
`ARM9.Execute()` and advances `DMA9Timestamp` while `ARM9Timestamp` stays
frozen. The debt is settled later and lazily by a DMA catch-up in the data
paths, landing on an arbitrary later load/store on the same line as genuine DMA
hold-off. `gx_paid` / `gx_blamed` split those apart with an exact debt counter.

## 3. The gate: it had to name the blocking unit on frames 453/454

R2-00b §4 set this gate deliberately, because 453/454 is the one case where the
old instrument provably reads zero. The attributor did not name a blocking unit.
**It showed there is no blocking unit.**

First, the build does not perturb timing: both windows reproduce the archived
Task 108 CSVs **bit-identically across all 27,058 and 27,485 rows** on the
original nine columns. `stall_partition_residual = 0` in both.

| class | 453–454 | 455–456 | delta |
|---|---|---|---|
| issue | 1,522,083 | 1,512,478 | +9,605 |
| icache_fill | 1,525,043 | 1,527,991 | −2,948 |
| dcache_fill | 1,311,977 | 1,308,600 | +3,377 |
| halt_wait | 1,592,488 | 1,603,750 | −11,262 |
| **gx_paid / gx_blamed** | **0** | **0** | **0** |

`gx_stall_events = 0`, and the counter trips even on a zero-cycle stall, so that
is a real zero rather than a dropped charge. `dma_hold` and `cart_spin` are also
zero. Explaining the tick HUD needs **+593,856** ticks; the largest class moves
9,605.

## 4. What is actually happening

The halt PC carries exactly two samples per frame, so emulator idle is directly
comparable to the HUD's `WAIT`:

| window | HUD `WAIT` | emulator idle | ratio |
|---|---|---|---|
| median (455–456) | 804,736 | 801,881 | **1.00** |
| excursion (453–454) | 210,752 | 796,250 | **0.26** |

**The CPU sits at `armWaitForIrq` for essentially the same time on both windows
— 796,250 against 801,881, 0.7% apart.** The HUD's `WAIT` shortfall on the
excursion is 588,353 ticks against its own claimed +593,856 of extra work:
**99.1% agreement.** Since the HUD computes `WORK = ALL − WAIT`, idle it fails
to count reappears as work that never happened.

**The load-free `SRC` excursion is a tick-HUD measurement artifact, not a
hardware stall.**

## 5. This was already implied by data in the tree

The finding does not rest on the new instrument alone, which is what makes it
safe to act on (Task 96: cross-check a new instrument against an old one).

Task 108 measured total cycles across the two windows as **identical to
0.006%** and instructions to 0.4%. If total cycles are equal and idle cycles are
equal, then working cycles are equal — the HUD's +297,000 ticks/frame of extra
work could not have been real. Task 108 had the arithmetic and drew the
available conclusion ("real, but not executed code"), because it had no way to
measure idle independently. It does now, and the honest reading is that the
excursion is not real at all.

## 6. Consequences, and they are large

1. **The E1 hunt for a GX / DMA / register stall is closed before it started.**
   Task 108 §6 named those three candidates. All three measure exactly zero.
   Nothing blocks, so there is no blocking unit to find.
2. **The next action is auditing the HUD's `WAIT` bracket**, which is accurate on
   median frames and under-counts genuine VBlank idle on excursion frames. The
   bracket lives in `ndsPlatformEndFrame` (`gNdsTickHudVBlankWaitTicks`) and the
   suspicion is idle entered from a path that bracket does not enclose.
3. **`WORK-H` P95 — the project's gate metric — is inflated on exactly the
   frames the gate is decided by.** `PROJECT_GOAL.md` gates on P95, the tail is
   where the artifact lives, and the campaign has been steering by it since Task
   66. How much of the 1.12M gap is phantom is now the most valuable open
   question in the performance lane.
4. **Task 75's ~103,488 preload estimate is derived from `WORK-H` P95 over
   load-free frames** and inherits the artifact. It should be re-derived before
   the preload is scoped as a subsystem.

Caveat, stated plainly: this holds under the emulator's model. But the HUD
readings came from that same emulator, so both sides are one machine — and only
one side has a closed ledger.

## 7. Adoption

**Not yet adopted.** R2-00b §4 warned that a new emulator binary would break
absolute comparability with the ledger, per `PERF_LEDGER.md`'s 2026-07-22
precedent. **That warning is now withdrawn on evidence:** the attributor build
reproduces the prior census bit-identically over 27,058 and 27,485 rows, so it
does not perturb timing and absolute comparability survives. Adoption is
therefore a low-risk drop-in — copy to `emulators/melonds/`, refresh slots with
`New-MelonDSRunnerSlots.ps1 -Force`, and `check-melonds-policy.ps1` fails closed
if any slot disagrees.

Two incidental findings in the fork, recorded, not acted on:

- the code-fetch DMA catch-up is commented out (`CP15.cpp:2211/2255/439`), so the
  ARM9 retires instructions for free after a GX stall;
- `GXFIFOStall`'s `if (CurCPU == 1) ARM9.Halt(2)` is unreachable for a
  CPU-driven stall under this fork's 0-based `CurCPU`.

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

## 6a. Follow-on: the idle moves into whichever phase is running

Measured here, from the 128-frame control capture taken for R2-01
(`artifacts/r2-01-ab/control.json`) — a different window (439–566) and a
different run from the one R2-00a validated against, so this is independent
corroboration rather than the same observation restated.

29 of 128 frames run `WAIT` below 60% of its median. On those frames:

| bucket | low-WAIT mean | normal mean | delta |
|---|---|---|---|
| `WAIT` | 70,365 | 384,805 | **−314,440** |
| `OTHR` | 86,583 | 401,080 | **−314,496** |
| `SRC` | 444,036 | 350,834 | +93,203 |
| `FTR` | 634,790 | 560,772 | +74,018 |
| `ALL` | 1,699,332 | 1,770,566 | −71,234 |

**`OTHR` tracks `WAIT` to within 56 ticks.** Task 65 already established that
`OTHR` is essentially the VBlank wait, so the two falling together says the idle
left the wait — it did not become less idle.

Per frame, it lands in **whichever phase happens to be running**, not in one
fixed place:

| frame | `WAIT` | shortfall | bucket that absorbs it |
|---|---|---|---|
| 547 | 3,392 | +355,584 | `FTR` +447,744 |
| 479 | 4,480 | +354,496 | `FTR` +448,192 |
| 481 | 20,928 | +338,048 | `FTR` +443,200 |
| 519 | 11,136 | +347,840 | `SRC` +268,416 |
| 542 | 11,264 | +347,712 | `SRC` +307,264 |
| 469 | 22,848 | +336,128 | `SRC` +363,136 |

`ALL` stays pinned at ~1,680,064 — three VBlanks — on every one of them.

This rules out a single mis-scoped bracket and points at the CPU blocking inside
a named phase, where the HUD has no wait accounting at all. It does **not** yet
say on what. Two readings remain open and they take opposite actions:

- If the CPU **halts** there (a library call that waits on card I/O, DMA, or a
  ring refill), the HUD is wrong and the fix is to account for idle at its
  source rather than only around `ndsPlatformWaitForScheduledVBlank`.
- If the CPU **stalls on a GX FIFO write**, charging it to `FTR`/`SRC` is
  arguably correct per `AGENTS.md` ("GX backpressure is distributed into the
  named buckets as memory stall on the write that could not retire"), and it is
  the *emulator idle* figure in §4 that needs re-reading.

`gx_stall_events = 0` on frames 453/454 argues for the first, but 453/454 are not
these frames. **Deciding this needs the attributor run on this window**, which is
exactly what it was built for.

### Blocked step, for the owner

Installing the attributor into `emulators/melonds/` was **denied by the sandbox**
(replacing an executable). I did not route around it. To finish this:

```powershell
Copy-Item emulators\melonds\melonDS.exe backups\melonds-pre-attributor.exe
Copy-Item D:\Stuff\DevFolder\melonDS-Accurate\build\melonDS.exe emulators\melonds\melonDS.exe -Force
.\scripts\New-MelonDSRunnerSlots.ps1 -Count 4 -Force
.\scripts\check-melonds-policy.ps1
```

Then re-run the census over a window containing frames 469–547 and read the
`gx_paid` / `gx_blamed` / `halt_wait` columns. Running it from its build path
instead was rejected on purpose: `AGENTS.md` requires repo-local scripted
melonDS, and a lab binary outside `emulators/` would violate the same rule that
keeps manual and sharded runs honest.

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

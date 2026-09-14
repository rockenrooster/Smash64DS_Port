# 2026-09-14 libc heap abort

Pre-fix four-CPU stress diagnosis, `build-p2-fidelity-02`, runner slot 11.
The one-stop probe broke at `0x021046a4`, immediately after libnds
`vramBlock__allocateBlock` called `malloc(28)` and received `NULL`.

- failed return: `0x00000000`; caller LR `0x0210786c`
- `fake_heap_start=0x02258d10`, `fake_heap_end=0x023f0000`
- program break `0x023f0000`: libc span/used `1,667,824 / 1,667,824 B`, `0 B` remaining
- exported `__malloc_current_mallinfo`: arena `1,667,824 B`; uordblks/fordblks/keepcost reported `0`
- taskman arena: chosen `1,412,864 B`, refine `3,840 B`, search allocation failures `77`
- BattleShip general-heap low-water at failure: `128,668 B`
- open newlib streams: `5` across two glue nodes
- stdio fds 0/1/2 plus fd 4 and fd 5; both extra streams have 1,024-byte FILE buffers
- fd 4 FILE/buffer: `0x0228f390 / 0x0228f5a0`; fd 5 FILE/buffer: `0x0228f408 / 0x023edd20`

The failure is therefore libc-heap exhaustion, not the taskman arena. The two
live non-stdio streams are the immediate lifetime suspect; fd 5's 1 KiB buffer
is still live near the top of the exhausted libc heap at the failing allocation.

## 12 KiB measurement arm

With a provisional `0x3000` libc reserve, the whole slot-11 stress match passes
all gates. Arena chosen/refine are `1,408,768 / 3,840 B`; BattleShip general-heap
low-water is `118,752 B`; weapon pool is `10` entries, live high-water `2`, zero
refusals; native failure is zero and Task36 capture outcome is `2` (READY).

The first libc counter reported `257,200 B`, but that value included libc blocks
already live before the taskman arena was chosen, so it is not a reserve sizing
measurement. The counter is being corrected to publish only live-byte growth
above the post-arena mallinfo baseline; the passing 12 KiB arm is retained only
as proof that a bounded reserve increase removes the abort with ample taskman
headroom.

After baselining mallinfo immediately after the arena shrink, the repeated
whole-match run reports `gNdsTaskmanLibcRuntimeHighWater=33,288 B`. The final
reserve is therefore `ceil((33,288 + 4,096) / 4,096) = 40,960 B (0xA000)`:
measured live-byte growth plus one allocator page of margin. The stress verifier
reads that counter and rejects a run whose high-water plus margin exceeds the
compiled reserve.

## Final acceptance

Final stress ROM/ELF SHA-256 are
`D5765269532946F30FED3C6FF92D20DE69871C09E55EF4B841FB37DCE8A9DD05` and
`620080D11527DB5ADFA3623F1EB46C8A3B3AAD341715ACFC7697EF85F5AB5F96`.
The final slot-11 timing run passes every standing stress gate with arena
chosen/refine `1,380,096 / 3,840 B`, libc runtime high-water `33,288 B`, and
BattleShip general-heap low-water `118,752 B`. Task36 outcome is `2` (READY),
graphics heap overflow/no-room are `0/0`, scene-file buffer alloc/decline are
`0/0`, anim-cache reserve failures are `0`, native failure count is `0`, and
the weapon pool is `10` entries / live high-water `2` / refusals `0` with
ITCommonData rejects `0`.

Shell ROM/ELF SHA-256 are
`30C39E33DE8F49AD835D34B71743737804382B2FDB639CA9D96CC845994E7410` and
`A5E4A87D8533B927CC5248A355F56003ACFC0BC82683417F9B3A28C1805E8B6A`;
both final build logs contain `NATIVE_ONLY_PASS`.

Host closure also passes: `python scripts/check-untracked-dependencies.py`,
`pwsh scripts/check-architecture.ps1`, `pwsh scripts/check-melonds-policy.ps1`,
and `pwsh scripts/check-docs.ps1`. Build logs are
`builds/codex-heap-stress-build.out` and `builds/codex-heap-shell-build.out`;
the final timing verifier log is `builds/codex-heap-stress.out`.

## 2026-09-14 review correction 1 — measure the top chunk

The live-byte metric was insufficient for the failure mode: fragmented free
bytes can remain live/free while no chunk fits libnds' next allocation. The
owner now records newlib `mallinfo().keepcost` immediately after the arena
shrink, keeps `gNdsTaskmanLibcTopChunkMin`, and publishes runtime high-water as
`initial_top - min_top`. The sbrk break is deliberately not part of this gate.

## 2026-09-14 review correction 2 — sample loader/menu lifetimes

`ndsTaskmanSampleLibcHeapNow()` is unconditional. The source-menu pump calls it
once per pumped frame, and `src/nds/nds_reloc_assets.c` calls it immediately
before all 31 `fclose` sites (current sample lines 839, 894, 900, 907, 919,
926, 977, 984, 995, 1002, 1006, 1064, 1080, 1089, 1101, 1112, 1119, 1123,
1499, 1517, 1525, 1529, 1593, 1600, 1604, 1693, 1736, 1745, 1756, 1763,
1767). `nds_renderer_assets.c` was not edited; its four owner/hat closes call
the shared `ndsRelocAssetStreamClose`, so they reach the sampled close seam.

## 2026-09-14 review corrections 3–4 — verifier and red test

The stress verifier now reads/prints the top-chunk minimum and fails if runtime
high-water is zero before applying the high-water + 4 KiB <= reserve gate.
`test_arena_capacity.py` parses `NDS_TASKMAN_LIBC_RUNTIME_RESERVE`, stubs only
the reset helper around its extracted chooser, includes the existing 256-byte
upward refinement in its expected chosen size, and asserts the realloc tail is
exactly the parsed reserve. With host GCC exposed from `C:/msys64/ucrt64/bin`,
the requested pytest is green: `1 passed, 8 subtests passed`.

## 2026-09-14 evidence correction — prior run identity and cost

The earlier `33,288 B` live-byte reading came from the shipped `0xA000` reserve
build in `builds/codex-heap-stress.out`, not from a 12 KiB-reserve arm. That run
also reported arena `1,380,096 B` and BattleShip general-heap low-water
`118,752 B`; no separate banked 12 KiB-arm figures are retained, so that claim
is dropped. Its per-frame mallinfo instrumentation had no measurable timing
movement: ALL P95 `3,918,656` vs `3,918,720` in `builds/orch-stress-run.out`,
and WORK P95 `3,648,064` vs `3,666,944`.

## 2026-09-14 rebuild attempt — shared generator blockers

The post-review stress build reached this package cleanly after removing an
over-broad startup-header include, but generation failed first in another owner:
`builds/codex-heap2-stress-build.out` reports Kirby high CopyLink root `0x1148`
inherits a bake preamble with no earlier explicit light colours to restore. The
independent shell build also failed in the shared generated-owner target;
`builds/codex-heap2-shell-build.out` reports M3_STAGE_FALSIFIER for unclassified
`sNdsNativeFighterActiveTables.runs` reads. Neither log contains
`NATIVE_ONLY_PASS`, so existing ROM/ELF files are stale and are not re-hashed as
post-review outputs. Shell/stress runtime is therefore intentionally not run
against them.

## 2026-09-14 post-review host checks

`check-untracked-dependencies.py`, `check-architecture.ps1`,
`check-melonds-policy.ps1`, and `check-docs.ps1` all pass. The architecture
check retains its two standing warnings (large-file split plan and local
generated outputs). `git diff --check` reports only existing CRLF conversion
notices on tracked files, with no whitespace error from this package.

## 2026-09-14 final rebuild/runtime disposition

The shared generator later advanced far enough for the stress target to reach
link. Its final `builds/codex-heap2-stress-build.out` failure is external to
this package: `.itcm` overflows by 40 bytes. Object inspection confirms this
package's relevant symbols remain ordinary text: `ndsTaskmanSampleLibcHeapNow`
is `.text.ndsTaskmanSampleLibcHeapNow`, `ndsTaskmanArenaBytes` is
`.text.ndsTaskmanArenaBytes`, and `syTaskmanRunTask` is `.text.syTaskmanRunTask`.
Therefore no fresh post-review stress ROM/ELF or stress timing run exists.

The final shell rebuild succeeds and contains `NATIVE_ONLY_PASS`. SHA-256:

- ROM `290B3019F2B8A37D23A2B44C3A91E93136F79F713647F4924B0F2F04024F6CAE`
- ELF `12489E9E52F98873641A4454E4AD30E224A429D1C85888C7C3948C6BDF604865`

The requested `probe-shell-four-kind.ps1` run cannot use isolated runner slot
12 because its current parameter contract is `[ValidateRange(1, 8)]` for
`RunnerSlot`. The task explicitly forbids substituting another slot, so no
shell runtime was launched. The script also does not currently expose the new
libc counters in its FOURKIND output. Those are named runtime gaps; no stale or
wrong-slot result is substituted.

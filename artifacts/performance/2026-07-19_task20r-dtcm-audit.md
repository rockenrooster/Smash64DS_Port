# Task 20R DTCM audit

## Verdict

`KEEP CHECKER + CENSUS SUPPORT / DO NOT MOVE SCRATCH / DO NOT MOVE STACK`.

Task 20R produces no profile-0 placement change. The current full 16 KiB
coroutine allocation cannot fit in the linker-visible DTCM/user-stack gap even
before real user-stack demand is counted, and shrinking it from an incomplete
lifecycle trace is forbidden. The one credible 2 KiB scratch object is safe to
identify but not safe to promote: DTCM/cache layout is retail-authoritative and
the user declined further retail repeats.

## Artifact identity

- source HEAD before this report: `0102a741d6faf8650396d9b101e4ecced2bc39df`
- profile-0 ROM: `smash64ds-battle-playable-hwtri.nds`, SHA-256
  `757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4`
- profile-0 ELF: `smash64ds-battle-playable-hwtri.elf`, SHA-256
  `5F77112C0487DFF4B053AAA4227279B2472D31F72F70D7CE7B57CF0617046639`
- current profile-1 census lab ROM / ELF:
  `BBDD605F2160263649BDE164E858DAB083197EBB517D34908FD5345A5D1D1100` /
  `3DFF511B9FA6705E0A3EA86FFF3D69C96E736F8577E843170C08038F3A146E38`

The profile-0 ELF contains zero `gNdsTask20*` or Task-20 helper symbols. The
compile-gated census remains profile-1-only.

## Current DTCM ownership and boundaries

The profile-0 ELF and current profile-1 lab agree exactly:

| Item | Address / range | Bytes | Ownership |
|---|---:|---:|---|
| `.dtcm` | `0x02ff0000` | 0 | no loaded application data |
| `__irq_table` | `0x02ff0000` | 128 | Calico IRQ table |
| `__sched_state` | `0x02ff0080` | 24 | Calico scheduler state |
| `.dtcm.bss` end | `0x02ff0098` | 152 total | exact two-owner closure |
| shared data/user-stack gap | `0x02ff0098..0x02ff3e80` | 15,848 | data grows up; user stack grows down |
| `__sp_usr` | `0x02ff3e80` | boundary | user stack top |
| `__sp_irq` | `0x02ff3f80` | 256 below top | IRQ reserve |
| `__sp_svc` | `0x02ff3fc0` | 64 below top | SVC reserve |
| BIOS variables | `0x02ff3fc0..0x02ff4000` | 64 | includes IRQ flags/vector |

Both named objects begin at addresses divisible by 32. Calico's `dtcm` linker
region ends at `__sp_usr`, so any static data collision already fails the link.
The new post-link checker additionally fails on any third DTCM object, boundary,
size, or alignment change before such an object can reach runtime.

## Stack capacity falsification

The production gameplay coroutine owns a 16,384-byte malloc-backed main-RAM
stack. Its unchanged full capacity plus the existing 64-byte guard requires
16,448 bytes, already 600 bytes more than the 15,848-byte shared gap while
assuming an impossible zero bytes for the live user stack. Therefore the
current stack cannot move intact.

Earlier Countdown/early/Whispy console observations reported gameplay/main
high-waters of 13,044/3,700 bytes. Replaying those values through the checker
gives:

- guarded lower bound: `13,044 + 3,700 + 64 = 16,808`, `NO_FIT` by 960;
- with two 1,024-byte margins: 18,856, `NO_FIT` by 3,008.

Those deeper values remain provisional because the legacy JSON omitted its
Task-20 rows. They are not needed for the intact-stack rejection: nominal
capacity alone fails. Three current lifecycle attempts also failed before ROM
execution because melonDS did not open its ARM9 GDB listener on ports 3373,
3383, or 4333. No runtime result is inferred from those transport failures.

## Scratch candidate evidence table

| Field | Evidence |
|---|---|
| Object | `sNdsRendererAdapterNativeOwnerModelviews` |
| Current address / size | `0x02217f80`, 2,048 bytes, 32 x 64-byte matrices |
| Producer / lifetime | overwritten for the selected Mario/Fox bindings each presentation; no cross-frame reuse contract |
| Consumer | synchronous production-input construction and immediate fighter execution in the same ARM9 call path |
| Access shape | up to 32 matrix writes and corresponding synchronous reads per presentation; no allocation |
| Current bus | ARM9 main RAM |
| Invalidation | complete live overwrite; no retained generation or dirty-range protocol |
| DMA / GX / IPC / ARM7 | no address passed to DMA, IPC, ARM7 audio, callback storage, or asynchronous GX ownership; GX matrix calls consume converted local matrices synchronously |
| Timing authority | Task 25R identifies M2 matrix work, but no retail DTCM A/B exists for this object |
| Decision | `NO PROMOTION`; cache/layout-sensitive performance cannot be inferred from melonDS |

The candidate's address is not visible to gameplay state. Its pointers remain
inside the immediate native-owner call chain. The separate renderer DMA paths
read texture staging buffers, not this matrix array; BGM/FGM ARM7 commands read
their resident ring/pack buffers. The coroutine stack pointer escapes only to
the heap-owned `PortCoroutine`, `OSThread.port_coroutine`, and the synchronous
CPU scheduler. No DMA/IPC/ARM7/audio owner of either candidate was found.

## Retained guardrail and verification

`scripts/check-task20-dtcm-layout.ps1` parses the actual linked ELF and prints
the named owners, section sizes, shared gap, user/IRQ/SVC/BIOS boundaries,
guard/margin arithmetic, and zero application-owner DMA candidates. It is now
run by the canonical battle verifier. An application DTCM object fails closed
with an explicit DMA/IPC/ARM7 audit requirement.

Focused results:

- production and current profile-1 lab ELF layout checks: pass;
- guarded 13,044/3,700 replay: `NO_FIT` by 960; two-margin replay:
  `NO_FIT` by 3,008;
- GBI decode fixtures, including the new verifier/checker contract: pass;
- Boundary rebuilt the production ROM byte-identically and passed fixtures,
  registry, Task-9/16 placement, renderer placement, and this DTCM gate. Its
  emulator phase returned 1 after those checks without producing a new
  emulator log or runtime artifact; treat that as launch transport, not game
  evidence, and do not spend another duplicate run;
- production ROM identity remains the already-green Task-27 ROM above.

No state, audio, lifecycle, geometry, material, texture, depth, pixel, reserve,
IRQ, DMA, or GX behavior changed because no DTCM placement was made.

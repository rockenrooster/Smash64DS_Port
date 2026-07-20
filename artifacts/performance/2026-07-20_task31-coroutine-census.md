# Task 31 coroutine-stack census

## Verdict

`STOP / KEEP PROFILE-ONLY CENSUS / DO NOT PLACE A DTCM STACK`.

The mandatory Step-0 census found five 16 KiB coroutines live concurrently in
ordinary combat and a peak of six. Task 31 permits DTCM placement only when
exactly one large gameplay-class coroutine exists. No stack was resized,
statically placed, or routed to DTCM, and no multi-stack scheme was attempted.

## Identity

- source HEAD: `8add112fa7f962a60c7b1294a1345770da53f6c6`
- mode/profile/window: `battle_playable_realtime` 163, profile 1, frames
  600–607, static textures 1, fast wallpaper affine 0
- ROM: `874E9CB9C1CAEFB89A88D405514BAD3C9FED6F260F55566433FF5EB229C4666F`
- ELF: `0834451F31AD98181C9A35AED489A5664442F881C5077E492EC8971C928C0040`
- exported JSON: `5F314D74F0039E25AC0736EF0B210D59D813DD157730537670DA27549E3D9CE6`

The linked profile build retained the current DTCM layout: `.dtcm=0`,
`.dtcm.bss=152`, and `15,848` bytes between `__dtcm_bss_end` and `__sp_usr`.

## Census

`libultra_os.c` currently requests 16,384 bytes for every thread ID below 100
and 4,096 bytes otherwise. All seven observed owners were below 100, so every
row requested and received the 16 KiB service-class size.

| Row | Owner ID | Stack | State | Sampled high-water |
|---:|---:|---:|---|---:|
| 0 | 90 | `0x02294A80` | destroyed | 72 |
| 1 | 1 | `0x02294A80` | live | 0 |
| 2 | 5 | `0x02298B20` | live | 268 |
| 3 | 3 | `0x0229CBC0` | live | 0 |
| 4 | 4 | `0x022A0C60` | live | 0 |
| 5 | 6 | `0x022A4D00` | live | 0 |
| 6 | 91 | `0x022A8DA0` | destroyed | 224 |

Summary: `count=7`, `overflow=0`, `live=5`, `peakLive=6`, `largeLive=5`,
`peakLargeLive=6`. Matching the existing gameplay-stack base identifies owner
ID 5 as the owner of `gNdsTask20GameplayStackHighWater`.

The small high-water values are the existing request-gated startup samples,
not full-lifecycle sizing evidence. Task 31 stops before Step 1 when several
large coroutines coexist, so they must not be used to justify a smaller stack.

## Retained diagnostic

The profile-only census records owner ID, requested/actual size, stack and
coroutine addresses, live/destroyed state, poison-derived high-water, total and
overflow counts, and current/peak total and large-live counts. The GDB exporter
checks row sizing, alignment, state, watermark bounds, conservation, overflow,
and the unique owner of the measured gameplay stack. Normal profile-0 builds
retain no census table or counters.

Verification passed:

- GBI/native-owner fixtures, including the census/export contract;
- `verify-dev-fast.ps1`, including the canonical capture;
- `verify-all.ps1 -Profile Latest`, covering shared runtime and Boundary;
- profile-0 ELF symbol audit: zero Task-20/31 census symbols;
- profile-0 DTCM remains `0/152` bytes with the 15,848-byte shared gap.

Verified profile-0 battle ROM / ELF:

- `1111D6AF2ADDAF07ABA3B1C16D6071181866E7D1445BF182B46E31B0B69593D8`
- `FDB5189633EE7EA13B5BCCD59A6A70EFCBE9EFE8BFCAB41D41B4C7BB1FAE0361`

Evidence:

- `artifacts/performance/2026-07-20_task31-census-early600.json`

# Task 28 ARMv5TE matrix-leaf audit

Decision: **REVERT candidate; retain no implementation.** The bounded 32-bit
matrix leaf passed its host arithmetic proof and produced the intended ARMv5TE
instructions, but it never reached the mandatory natural-owner, ARM9-golden,
state/pixel, or retail gates. Two exact control launches failed before sampling
because the scripted melonDS instance did not open its ARM9 GDB listener. No
speed or correctness result is inferred from that transport failure.

## Atomic unit and identity

- Branch/worktree: `master`, shared live tree.
- Source checkpoint: `d06c01497b673a679adb08b6ffd04e6271575880`.
- Owned surface: `src/nds/nds_renderer.c`, only
  `ndsRendererNativeMatrix3Mul20p12` and its two preflight call sites.
- The focused candidate diff changed the leaf from `void` to a Boolean bounded
  fast path, admitted all 18 matrix elements only in `[-16384, 16383]`, used
  signed-32 multiply/accumulate plus the existing sign-aware 20.12 rounding,
  and returned to the literal current fighter fallback before GX otherwise.
- The candidate diff is removed. The live renderer source is byte-identical to
  the checkpoint; only this report and queue/status documentation remain.
- Pre-existing changes to `AGENTS.md`, the JumpABC task file, playtesting notes,
  Python bytecode, the untracked Publish task file, and the Task-21 disassembly
  log were not owned or touched.

Toolchain: devkitARM `r67.1-1`, GCC `15.2.0`, GNU binutils `2.45.1`.
The effective renderer flags were the current mode-163 flags:

```text
-std=gnu11 -g -Wall -Wextra -O2 -ffunction-sections -fdata-sections
-march=armv5te -mtune=arm946e-s -mthumb ... -O2 -marm
```

The final `-marm` is the existing mode-163 renderer-TU override. Both static
A/B builds used `battle_playable_realtime`, mode 163, profile 1, detailed M2
ledger 1, generated Task-26 segment 0 enabled, and Task-16 compare/i2f/add-sub
`1/1/1`. Their generated `nds_build_config.h` and
`nds_scene_harness_config.h` files compare byte-for-byte.

### Production binding

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `smash64ds-battle-playable-hwtri.nds` | 14,681,088 | `757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4` |
| `smash64ds-battle-playable-hwtri.elf` | 9,053,136 | `5F77112C0487DFF4B053AAA4227279B2472D31F72F70D7CE7B57CF0617046639` |
| canonical profile-0 `.map` | 3,027,484 | `6B2ECA53322A98D564490E6F1EFC7FF8255F2DBDAAE2E2D73DA90CC84567FD20` |
| canonical profile-0 `nds_renderer.o` | 908,768 | `464C93083B69755C3B957D84F4A69693B0E07F63D9AA286C4D5098102EA4179E` |

In that ELF, `arm-none-eabi-nm -S --size-sort` and `readelf -sW` bind
`ndsRendererNativeMatrix3Mul20p12` to ARM main text at `0x0200B764`, size
`0x100` (256 bytes). The current packed lighting/color candidate was also
inspected: `ndsRendererHardwareLitShadeColorPrepared` is already ARM ITCM at
`0x01FF87C0`, size `0x1CC`, and its three-term dot is already one `MUL` plus
two `MLA`. It was not reopened.

### Exact profile-1 static pair

| Artifact | Control bytes / SHA-256 | Candidate bytes / SHA-256 |
|---|---|---|
| ROM | 14,696,448 / `CFFE57637E41455E24AFF3C6BBA683B2ED4ED8BD688C32BA0DE56548609D8904` | 14,696,448 / `DEFC9ABFA52C9E66E18A2AAC29EF528BFCE08682596756254703263C11CD4C7B` |
| ELF | 9,126,880 / `C2AFA2D4858764092E720D3CC68B92516C11767314B4D534D313272E7F4F77BA` | 9,126,688 / `18A13E4447E881B0648E7A09EA1F996FCA88604254989687EDDCD9ACE7FB744E` |
| map | 3,041,735 / `15ACB8622C63B7B03AAB925DD9E7C21FF22B29FD608586463D6E15C96919E127` | 3,041,775 / `6252ED5C003CD49C03BE621C33A9B7E1CA7E700E9ACD2A7A9E0273AB772694DE` |
| `nds_renderer.o` | 959,356 / `57073BC5FF77E03EEA2598DC530AC4C4DB60959AA7F165113CDA8D89A473469E` | 959,020 / `B719B1783B9C4BFED474E487F3E41D47B0019E6BCB7C087219305E476B51F89F` |

The control is `builds/build-task26-segment0-e1-p0-lab`; the candidate is
`builds/build-task28-matrix-candidate`. The same-config section comparison is:

| Section | Control | Candidate | Delta |
|---|---:|---:|---:|
| ITCM | 30,420 | 30,420 | 0 |
| update `.text.hot` | 5,016 | 5,016 | 0 |
| ARM9 `.main` | 787,048 | 787,088 | +40 |
| main BSS | 1,784,464 | 1,784,464 | 0 |
| DTCM / DTCM BSS | 0 / 152 | 0 / 152 | 0 |

The candidate leaf itself shrinks, but the admission and pre-GX fallback
plumbing makes the linked main image 40 bytes larger. Reserve was not promoted
or claimed because the runtime gate was never reached.

## Hot path and pre-code bound

Task 21R's retained compact representation proves 52 scheduled fighter joints
and 32 binding roots per presentation. The preflight therefore invokes this
3x3 multiply 84 times, or 756 three-term dots, per presentation. The current
same-window matrix owner was measured previously at 158,464/158,528/158,528
P50/P95/max ticks. That is an inclusive owner bound, not a leaf-only timing
claim. The call density and replacement of three multiword MAC operations per
dot clear the 2K theoretical-screening threshold, so one implementation was
justified; natural timing remained the decision gate.

## Operand and arithmetic proof

Current golden semantics for every output cell are:

1. Sign-extend all six operands to signed 64-bit.
2. Accumulate three signed products without overflow.
3. Round signed 20.12 to nearest with the existing away-from-zero half rule:
   positive `(sum + 2048) >> 12`; negative
   `-(((-sum) + 2048) >> 12)`.
4. Saturate to signed 32-bit.

The candidate admitted an entire matrix pair only when every element was in
`[-16384, 16383]`. For that domain, each product magnitude is at most
268,435,456 and a three-term sum magnitude is at most 805,306,368. Adding the
2,048 rounding bias and negating a negative sum both remain strictly inside
signed 32-bit, so the current saturation can never fire. Signed-32 product,
accumulation, rounding, and output are therefore identical to the signed-64
golden. Any value outside that finite partition returned false before GX and
selected the unchanged current owner.

A deterministic host falsifier covered 100,000,000 admitted-domain dot vectors
with seed `0x6D2B79F5`, plus directed range-boundary values including signed
limits, `-16385/-16384/-16383`, `-1/0/1`, and
`16382/16383/16384`. It reported zero mismatches. Observed random sums ranged
from `-629,507,010` to `604,751,636`; elapsed time was 9,194 ms. This supports
the mathematical partition but does not replace the required literal ARM9
golden run.

## Actual ARMv5TE code generation

The same-config control symbol is ARM main text at `0x0200CC14`, size 256,
with a 52-byte frame. Its loop body uses one `SMULL` and two `SMLAL` for each
dot. The candidate symbol is ARM main text at `0x0200C364`, size 248, with a
40-byte frame. Its loop body uses one `MUL` and two `MLA` for each dot. Counts
from the actual linked ELFs are:

| Shape | Control | Candidate |
|---|---:|---:|
| Static instructions | 64 | 62 |
| `SMULL` / `SMLAL` | 1 / 2 | 0 / 0 |
| `MUL` / `MLA` | 0 / 0 | 1 / 2 |
| Branch-with-link helper calls | 0 | 0 |
| Leaf bytes | 256 | 248 |
| Stack frame | 52 | 40 |

There is no divide, helper call, or veneer in either leaf. The candidate proves
the desired instruction sequence and reduces the leaf by 8 bytes and its frame
by 12 bytes, but it does not prove a natural-owner speedup.

## Runtime attempts and failed gates

Two exact control attempts used separate repo-local runner slots (7, then 2).
Both completed their static Task-9, renderer-ITCM, and Task-20 DTCM checks, but
melonDS never listened on `127.0.0.1:4333`. A direct diagnostic launch remained
alive and responsive but exposed no TCP listener or capturable top-level
window; stdout and stderr were empty. No control JSON was exported and no
candidate runtime was launched after the missing control.

Consequently the following mandatory KEEP evidence is absent:

- representative microbenchmark and natural matrix-owner P50/P95/max;
- candidate-versus-current literal ARM9 golden corpus;
- exact state, matrix/light, geometry, material, texture, audio, and lifecycle
  rows;
- synchronized `0/49,152` native pixels;
- runtime reserve/fallback/fence/conservation proof;
- retail scheduling, ARM/cache/layout, and pacing falsification.

The user declined more retail repeats. melonDS cannot referee ARM/cache/layout
performance even if its transport recovers. A host proof plus promising
disassembly is therefore insufficient to bank this working-set-sensitive ARM
change.

## Post-revert verification

- `git diff --exit-code -- src/nds/nds_renderer.c` passes.
- The GBI/native-stage fixture suite passes, including all 12 fail-closed
  manifest perturbations and the Task-26 generated-segment checks.
- Renderer ITCM placement passes at 28,820/32,768 bytes, with 3,948 free.
- Task-20 DTCM placement passes at 0/152 section bytes, 15,848 shared-gap
  bytes, and zero forbidden application/DMA owners.
- Task-9/16 placement passes with compare/i2f/add-sub `1/1/1`, stock helper
  bytes 1,952, zero ITCM fill, and the bound GCC-15.2.0 libgcc hash.
- The live production ROM and ELF remain byte-identical to the production
  binding above. Since the implementation diff is empty, the already-green
  checkpoint supplies the retained runtime/pixel evidence; it is not evidence
  for the removed candidate.

## Disposition

**REVERT BOUNDED MATRIX LEAF / NO RETAINED CODE.** Restore the exact 64-bit
golden and both direct call sites. Do not add a machine-code drift checker for
a rejected leaf. Do not open the already-optimal packed lighting dot or another
Task-28 family after this atomic candidate failed to reach its runtime gate.
Task 29 is next, beginning with its no-behavior GX census; any transport- or
retail-sensitive promotion remains fail-closed.

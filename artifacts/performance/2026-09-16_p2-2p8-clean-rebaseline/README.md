# P2-2p8 clean rebuild and re-baseline of the canonical four-CPU directory

Owner-approved 2026-09-16. This is a **build-hygiene fix, not a code change**:
no source was touched. It moves the four-CPU baseline, so every banked absolute
figure before it refers to the old one. Deltas measured *within* the old
directory remain valid; deltas measured *across* the boundary do not.

## Why

`NITRO_FILES := $(NITROFS_DIR)` hands the whole build directory to the packer, so
a build ships every file present in its `nitrofs/` rather than the set its own
flags produce. The Makefile records this for audio — "superseded BGM assets can
survive an incremental build-directory reuse and are otherwise silently repacked
by ndstool" — and `prune-obsolete-audio` and `prune-streamed-ftanim` are the only
two prunes that exist. Nothing prunes fighter images, CSS previews, shield poses
or reloc animations.

`builds/build-p2-fourcpu-tickhud` had therefore been accumulating payload since
at least 2026-09-11: **710 files / 29,325,885 B**, against the **365 files /
28,320,664 B** that three independent fresh builds of the same target each
produced byte-for-byte. The 345 extras were 328 Kirby reloc animations, 12 CSS
preview FPCs (gated behind `NDS_P2_1P_GAME`/`NDS_P2_MENU_SHELL` at
`Makefile:7668`, and this target has no menu shell), the Pikachu and Yoshi
fighter images, and one shield pose — 1,005,221 B the current configuration does
not ask for.

## Procedure

1. Recorded the old identity (below).
2. `Remove-Item -Recurse -Force builds/build-p2-fourcpu-tickhud`.
3. `make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=builds/build-p2-fourcpu-tickhud`
   — `NATIVE_ONLY_PASS: smash64ds-p2-fourcpu-tickhud-hwtri.elf, 245 actual link inputs`.
4. Verified the payload against the fresh-build reference: **365 files /
   28,320,664 B**, an exact match. `nds_build_config.h` hash unchanged, so this
   is the same configuration, and `.itcm` is `0x7f78`, unchanged.
5. `verify-p2-four-fighter-stress.ps1 -Build build-p2-fourcpu-tickhud -NoBuild`,
   1,972 samples.

## Identity

| | stale | clean |
|---|---|---|
| ROM SHA-256 | `94B60BEF…66353` | `FD1E5B15062DC05022E72FDC9B19C969D76771F97C9158B6C45309F19F4740ED` |
| ELF SHA-256 | `C8D863AC…26884` | `3D901227F7C8A6FFAE38FF6CB0B218E2113E57C2CB2E9B55485007D43EB61047` |
| config SHA-256 | `23314B62…5415F3` | `23314B62…5415F3` (unchanged) |
| NitroFS | 710 files / 29,325,885 B | 365 files / 28,320,664 B |
| ROM bytes | — | 30,164,992 |

Log: `builds/verify-p2p8-clean-baseline-stress.log`.

## Result

| Bucket | stale P50 | clean P50 | dP50 | stale P95 | clean P95 | dP95 |
|---|---|---|---|---|---|---|
| **WORK-H** | 1,639,808 | **1,589,376** | **-50,432** | 2,365,120 | **2,318,208** | **-46,912** |
| **STG** | 398,848 | **344,192** | **-54,656** | 442,048 | 388,096 | -53,952 |
| **ALL** | 2,237,504 | **1,677,952** | **-559,552** | 2,798,208 | 2,798,144 | -64 |
| WORK | 1,693,504 | 1,639,104 | -54,400 | 2,491,840 | 2,430,912 | -60,928 |
| FTR | 357,312 | 356,544 | -768 | 744,640 | 743,808 | -832 |
| SRC | 552,192 | 553,984 | +1,792 | 1,055,360 | 1,059,712 | +4,352 |
| GCRA | 546,688 | 548,480 | +1,792 | 1,047,168 | 1,054,208 | +7,040 |

WORK-H mean 1,690,473 -> **1,636,381** (-54,092).

**`ALL` P50 crossed a VBlank quantum** — 4 intervals to 3. The cadence histogram
moves with it: VBlank 2/3/4/5+ **104/845/809/215 -> 128/947/706/192**, max
interval **13 -> 12**, slips 0 in both. That is the gate metric moving, not a
bucket rearrangement.

-50,432 WORK-H P50 is **3.6x the 14,080-tick cross-build significance floor**,
and larger than every N04.0x code lever put together.

### The causal question is now answered

The batch artifact for N04.08 recorded a 52,480-tick gap between a fresh lab
directory and the stale canonical one and explicitly refused to claim that stale
payload *caused* it, on the grounds that an unopened file costs ROM size and FAT
chain length rather than stage time. This rebuild is the controlled experiment:
same source, same directory name, same config hash, same flags — only the
payload cleaned. **-50,432 WORK-H P50, of which -54,656 is STG.** The cause is
established.

The mechanism is the FAT chain, and the profiles agree. The 2026-09-15 profile,
taken on a stale-payload ROM, put `get_fat` at 1,422.6 calls/frame on its
costliest frames with `f_lseek` at 16,079 cycles a call. The 2026-09-16 profile,
taken on a clean-payload ROM, has no `f_lseek` at all and `get_fat` at 0.34%. A
filesystem 1,005,221 B larger has longer cluster chains, and every stage asset
read walks them. That also retires N04.09's first premise properly: the FAT lane
was real, it was a *symptom* of the stale payload, and cleaning the payload fixed
it without a line of code.

### Invariants

Workload identical, resources better:

| | stale | clean |
|---|---|---|
| Native failures / direct rejects | 0 / 0 | 0 / 0 |
| Fighter draw-plan build/hit/mismatch | 618 / 6,217 / 0 | 618 / 6,217 / 0 |
| Fighter kinds word | 151,389,187 | 151,389,187 |
| Draw-slot triangle mask | 15 | 15 |
| `gNdsMObjMatAnimStableSkipCount` | 7,892 | 7,892 |
| Heap low-water | 108,096 B | **111,680 B** |
| Arena chosen size | 1,347,328 B | **1,355,520 B** |
| Arena alloc failures | 85 | **83** |
| Slips | 0 | 0 |

Five `cpuGetTiming()` 2^22 overflow artifacts were corrected (frames 137, 517,
854, 1849, 1897), each landing back on the run median.

## What this changes

- **The four-CPU baseline is now WORK-H P50 1,589,376 / P95 2,318,208**, heap
  low-water 111,680 B. Every figure banked against the old 1,639,808 describes
  the stale directory.
- N04.08's own verdict is unaffected. Its same-ROM attribution was one binary in
  one directory, and its canonical re-bank compared two builds that were both
  stale-payload, so the -9,920 / -8,512 it banked is still a like-for-like
  comparison.
- **Nothing in the tree gates this.** `check-published-roms.ps1` is 42 lines and
  checks ROM filenames plus one rejected payload signature; it never looks at
  NitroFS membership. A payload manifest check would close the class.
- P2-2p8 remains RED. 1,589,376 against a target near 1.12m is still RED, and
  this bought no code.

---

## The published ROM carried the same stale payload

`builds/build`, the default `BUILD` that produces `smash64ds.nds`, held **955
files / 53,614,440 B with the oldest dated 2026-08-01** and 4 written on
2026-09-16. Rebuilt clean from the same source:

| | stale | clean |
|---|---|---|
| NitroFS | 955 files / 53,614,440 B | **660 files / 51,859,504 B** |
| `smash64ds.nds` | 55,712,768 B | **53,883,904 B** |
| `smash64ds.nds` SHA-256 | `E9AF96F8…7EA6` | `FE236E096EC0C2790B27C7B7E8E4E249FDAAC7A42C3DFD7D946F0DCA21847EA8` |
| `smash64ds.elf` SHA-256 | `97E342F4…59356` | `97E342F4…59356` (**identical**) |

**295 files and 1,754,936 B of payload removed; the ROM is 1,828,864 B smaller;
the ELF is byte-identical.** The code did not change — only the assets the packer
found lying in the directory. `make TARGET=smash64ds` passed `NATIVE_ONLY_PASS:
smash64ds.elf, 262 actual link inputs` and `check-published-roms.ps1` reports the
contract GREEN.

The published build keeps more than the four-CPU one (660 against 365) because it
is the full shell + 1P + VS configuration and legitimately produces CSS previews,
the menu kit and the wider roster.

**RUNTIME PROOF OWED.** The four-CPU arm is measured above and is GREEN with
native failures 0/0, but the republished `smash64ds.nds` has not been run since
the payload changed. The four-CPU target is not the shipping shell, and a clean
payload is exactly the change that could remove something a menu or 1P path
reaches at runtime but the build flags do not declare. Do not treat the smaller
ROM as accepted until the shell loop and realtime arms have run against clean
directories of their own — `build-p2-shell-loop` and `build-p2-shell` have not
been checked for the same accumulation.

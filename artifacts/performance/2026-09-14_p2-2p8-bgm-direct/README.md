# P2-2p8 direct NitroROM BGM refill

Date: 2026-09-14

Verdict: **KEEP. BGM refill tail cost falls materially; P2-2p8 remains RED.**

The BGM worker already owns one immutable NitroFS track and exact packet byte
offsets, but each live refill went through stdio/libfat. A 128-frame ARM9 census
showed all 11 `ndsAudioBgmReadPacket` frames carrying the FAT tail. The retained
change resolves the selected track to one NitroROM file id and reads the same
bounded ranges through Calico. The logical stream cursor, packet/loop state,
two-buffer playback and source-visible audio API remain unchanged. If a direct
transfer fails, the reader disables the route, re-seats the existing stdio file
at the authoritative logical cursor and continues through the old path.

## Synchronized eight-frame A/B

Frames 1400..1407 use identical four-CPU build configuration SHA-256
`23314B62FBBECCAC413A9D677555EB1233137F84C26D5D9DF0177F801F5415F3`.
Control ROM is
`2742FE722436EBC63D46A9B0C99EFDD35CE3C3174327A7A68D12157D7E6777E9`;
candidate is
`373595A3D2CB0859AC12F48599B426218431F874FCA7223D0EB4467E402A47A4`.

The short tick-HUD slice is deliberately not used as the keep decision: BGM
worker scheduling re-phases WAIT/OTHR between the two ROMs. It does prove the
runtime contract. Both arms end with BGM refill count 113 and zero BGM
read/play/packet/seam failures; the candidate records **235 direct reads and 0
fallbacks**.

## ARM9 refill attribution

The same 128-frame 1400..1527 window was profiled with one region per frame and
split on `ndsAudioBgmReadPacket`. Both arms contain 11 refill frames.

| measurement | control | candidate | delta |
|---|---:|---:|---:|
| refill-frame cycles/frame | 5,091,523 | **4,786,363** | **-305,160** |
| refill premium over non-refill frames | 878,955 | **516,356** | **-362,599** |
| refill `get_fat.isra.0` premium | 112,815 | **25,208** | **-87,607** |
| refill `f_lseek` premium | 69,288 | **15,612** | **-53,676** |

This isolates the intended effect: profile-wide FAT symbols remain because other
subsystems still use libfat, while the recurring BGM refill association largely
disappears.

## One-minute four-CPU stress

Configuration-exact Donkey/Samus/Link/Kirby Dream Land stress covers frames
2..1973 and source clock 60 -> 1 (59/60 seconds). Against the retained Link
live-texgen packet checkpoint:

| bucket | prior P50 / P95 | BGM direct P50 / P95 | delta P50 / P95 |
|---|---:|---:|---:|
| ALL | 2,237,824 / 2,798,464 | **2,237,760 / 2,798,400** | -64 / -64 |
| FTR | 394,304 / 761,728 | **394,176 / 768,640** | -128 / +6,912 |
| SRC | 565,056 / 1,254,976 | **561,536 / 1,247,040** | **-3,520 / -7,936** |
| GCRA | 559,360 / 1,249,280 | **555,648 / 1,241,024** | **-3,712 / -8,256** |
| SINT | 260,544 / 811,648 | **260,160 / 817,216** | -384 / +5,568 |
| AUD | 3,456 / 139,968 | **3,392 / 123,712** | -64 / **-16,256** |
| WORK-H | 1,670,976 / 2,539,584 | **1,650,688 / 2,505,280** | **-20,288 / -34,304** |

Cadence changes from 95/775/790/313 to **120/803/755/295** for
2/3/4/5+ VBlanks across 1,973 presents. Two-VBlank cadence therefore rises from
4.82% to **6.08%** and 5+ VBlank presents fall by 18. The product requirement is
still >=95% at two VBlanks, so this is a retained P2-2p8 slice rather than
closure.

Same-run guards are clean: BGM direct reads/fallbacks **335/0**, native
failures/direct rejects 0/0, animation-stream failures 0, graphics-heap
overflow/no-room 0/0, weapon refusals 0, SyMalloc overflow 0 and objman panic 0.
General-heap low-water remains 118,752 B, 93,152 B above the 25,600 B floor.

## Verification notes

`scripts/check-audio-bgm-derived-assets.ps1` passes the derived BGM corpus.
The existing `verify-audio-bgm-results.ps1` finite-track check could not reach
runtime because its current harness configuration fails to compile the owner
renderer overlay (`NDS_R2_FIGHTER_GX_COMPOSE` prerequisite plus missing
split/composed-matrix declarations). That separate build-closure defect is not
used as evidence for or against this battle-only checkpoint.

The Boundary realtime battle child was then run directly through `-P2ShellFlow
-FastIteration` on the final source. Its natural Dream Land route reports
`battle_playable Pupupu realtime pacing smoke passed` and publishes the canonical
two-frame capture. The verifier still emits its standing locked-30 warning,
consistent with P2-2p8 remaining open.

The widest `verify-all.ps1 -Profile Boundary` front gate passes toolchain, docs,
untracked-dependency, owner-wiring and the complete fast generator-staleness
sweep. The umbrella cannot enter its runtime plan because `check-architecture.ps1`
rejects the pre-existing owner input `?? decomp/alt_assets/` under the read-only
`decomp/` rule. That owner data was preserved. The configuration-exact four-CPU
child and realtime battle child above therefore carry the runtime evidence for
this checkpoint.

The raw 332 MB ARM9 profile is scratch and must not be committed. Permanent
evidence is the derived census/report plus the synchronized and whole-match
timing/coverage/memory artifacts in this directory.

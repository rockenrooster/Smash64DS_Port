# P2-2p8: 1P-only resident code joins the frontend overlay (2026-09-28)

Status: KEEP. Owner approval 2026-09-28 ("Free memory by moving 1-player-only
code out of main memory"). Linker layout only: no source, gameplay or asset
change. The VS asset loan grows by 39,584 B. In a four-fighter VS battle,
general-heap free at the first battle update rises by 35,408 B.

## What moved (linker/nds_frontend_overlay.ld)

Into `.ovl.frontend` (text/rodata of whole TUs):
- `battleship_mntraining.o`: Training CSS, 12,934 B.
- `battleship_sc1pbonusstage.o` + `sc1pbonusstagefiles.o`: bonus scenes, 4,124 B.
- `battleship_sc1pgameboss.o`: Final Destination wallpaper, 4,236 B.
- `battleship_ftboss.o` + `ftboss_status_1..4.o` + `wpbossbullet.o`: Master
  Hand, 8,012 B.
- `battleship_grbonus3.o`, `item_tarubomb.o`, `item_target.o`: Race to the
  Finish ground and its two ground items, 1,788 B.
- The 1P runtime's boss/death/camera closure that `.text.frontend_resident` kept
  resident, 1,608 B. Its shared callers are guarded (see the pairs below).

Read-only `.data` tables, 4,936 B. Every reader was checked in source and none
writes to them:
- boss status/wait tables, bullet and ground-item descriptors;
- boss wallpaper plans/anims/effects/colours;
- bonus target/bumper/platform/timer tables and Training file IDs;
- 1P `StageDesc`/`ComputerDesc`/zoom/Kirby-team/sleep tables;
- `SC1PManager*Overlay` descriptors (their consumer `syDmaLoadOverlay` is a no-op);
- stage-clear bonus data, Congratulations pictures;
- credits sprite info and textbox display list.

Into `.ovl.frontend.bss`: the Training and bonus-stage LBFileNode status and
force-status buffers, 1,872 B. This uses the Phase 3 rule: each scene installs
its own buffers with `lbRelocInitSetup`, and the dispatcher releases them
before a loan.

## Section sizes (arm-none-eabi-size -A)

Both arms are built from the same objects. The only difference is the
overlay linker fragment (`symsize_parity`: 0 size differences; the 57 moved
data symbols are re-typed D->T).

Arm A is the brief's twin: `smash64ds-p2-shell-hwtri NDS_P2_1P_GAME=1`, HEAD
`a8d7a13`. Its objects are identical except the git-revision string in
`nds_platform.o`.

| section | A ctl | A cand | delta |
|---|---:|---:|---:|
| .text.frontend_resident | 3,500 | 1,892 | -1,608 |
| .main | 1,552,240 | 1,521,080 | -31,160 |
| .main.rw | 246,868 | 241,932 | -4,936 |
| .main.bss | 825,240 | 823,368 | -1,872 |
| .ovl.frontend | 247,136 | 284,864 | +37,728 |
| .ovl.frontend.bss | 18,688 | 20,544 | +1,856 |
| VS loan (overlay_memory_bytes) | 265,824 | 305,408 | **+39,584** |
| image end / heap start | 0x022c6a00 | 0x022c6960 | -160 |

Arm B is the published configuration plus walk and argmax:
`smash64ds-p2-shell-freeplay-hwtri`, which is the same flag block as
`smash64ds` (COMPACT=1), plus `NDS_P2_MENU_WALK=1 NDS_P2_SHELL_ARGMAX_ROSTER=1`.
It is built from HEAD `f7917b5`.

Both B arms link against a snapshot of HEAD `nds_hot_text.ld` with
`ndsFighterDisplayContractProjectTarget` evicted from ITCM
(`NDS_HOT_TEXT_LINKER_SCRIPT=` override). Concurrent WIP had grown ITCM
24 B past its limit in both arms.

B shows the same deltas: `.main` 1,555,312 -> 1,524,152; loan 265,216 -> 304,800.
Hashes and addresses are in `overlay-1p-summary.json`.

## Gate

The ELF crossing gate passes. It has 21 new exact reviewed pairs and 0
unknown pairs, for 175 allowed in total (A and B candidates). Every pair has a
source-cited reason:

| Guard in source | Pairs (caller -> overlay target) |
|---|---|
| Fighter kind is Boss | `ftMainProcParams`, `ftManagerInitFighter` -> boss helpers; `dFTMainSpecialStatusDescs[fkind]` -> boss status table |
| Item kind is Target/TaruBomb (maker table) | `sNdsITManagerProcMakeList` -> maker functions |
| gkind is Bonus1/2/3 or Last | `grMainSetupMakeGround`, `grWallpaperMakeDecideKind` -> bonus/boss ground makers |
| `SCBATTLE_GAMERULE_1PGAME` or 1P game type | `ftCommonDead*`, `ifCommon1PGameInterfaceProcSet` -> 1P callbacks |
| Called only from boss scene code | `ifCommonBattleEndSetBossDefeat` |
| Non-VS setups and dispatch (existing pattern) | Training and bonus taskman setups; `ndsBaseSCManagerRunLoop` |

The VS SSS offers only Castle..Inishie, and the VS CSS cannot pick Boss or
the polygon team.

One old pair is now unused in the new layout but is kept so that pre-move
controls still gate: `ndsBaseSC1PBonusStageStartScene -> sc1PManagerCheckUnlockSoundTest`.

The gate itself was strengthened: an allowlist pair without a review reason is
now rejected. There is also a new test that the repository allowlist loads.
`test_frontend_overlay.py` and `test_frontend_overlay_pool.py` pass (17).
`NATIVE_ONLY_PASS` holds on every build.

A source grep for every newly overlaid symbol finds no other resident caller,
lab harnesses included. The only references are declarations, weak stubs and
the listed callers.

## Runtime (runner slots 9-12)

**VS battle, arm B** (`probe-shell-four-kind.ps1 -ThroughResults`, slot range
copy). The argmax roster is Captain/Link/Pikachu/Kirby, and the arena is
identical (962,304 B at 0x022f9c00).

| | ctl | cand | delta |
|---|---:|---:|---:|
| loan used / bytes | 265,152 / 265,216 | 300,560 / 304,800 | +35,408 used |
| spill to general heap | 99,192 | 74,208 | -24,984 |
| general free at 4th fighter make | 159,488 | 194,896 | +35,408 |
| **general free at first battle update** | **103,936** | **139,344** | **+35,408** |
| low-water at stop | 54,304 | 72,008 | +17,704 |

- Every other marker is identical: CSS commits, 12 makes, roster, 1,492 VS
  process events and 16 status events, reloc and storage failures 0, and
  allocfail 178 in both arms. ARM7 read 37,728 B more at boot: the larger
  overlay file.
- **Both arms then abort identically** in `glBindTexture(name=80)`, in the
  VS battle on this tree. This abort is not caused by this change and needs
  its own owner. The low-water is therefore not a full-match figure.

**1P entry, arm B** (`probe-p2-campaign.ps1`, slot range and COMPACT check
copy):
- Both arms walk Title -> 1P Mode -> 1P CSS -> Intro into the stage-1 battle
  and reach GO with identical counters (presents 197/204, go=8, same frees).
- Both report the same pre-existing `graphics-buffer-refusal`
  (dl_overflow=1218).
- The candidate's `sc1PGame` code and its moved tables run from the loaded
  overlay.
- The one differing value is an intermediate extern-patch counter read
  (10/11). The next stop agrees (12/18). This is a stale gdb read, not
  behaviour.

**Shell tour, arm A** (`probe-p2-shell.ps1`):
- Control and candidate both read CSS tour kind=fff drew=fff.
- Both hit the known BADSETUP in `mnMapsStartScene`.

Evidence is in this directory: `fourkind-*.txt`, `campaign-*.txt` and
`shell-probe-*.txt` (with raw-log hashes), and `overlay-1p-summary.json`.

## Left resident, and why

- Renderer tables for the polygon team and boss (`sNdsNative{N*,Boss}*` rodata,
  about 7.6 KB) and the growth of the `ndsRendererNativeBindOwnerImage`
  kind switch (+7.2 KB). They live in `nds_renderer.o`, which other lanes own
  and are editing concurrently. A future move would need kind-guard pairs on
  that code.
- The `main.o` merged string pool (+5.0 KB), `scene_backend.o` renderer and
  harness growth, and `ifcommon` out-of-line helpers. These are shared input
  sections or parts of functions, so they cannot be moved separately.
- Taskman and video setups, `ll*` offset words, and interface position tables
  (a resident pointer stores their address).
- `dSCStaffrollStaffRoleCharacters`, which is mutated.
- 1P battle-state and campaign BSS, whose persistence across scenes is
  required.

## Risks

- A VS path that reaches a moved symbol outside the listed guards would
  execute loaned memory. The static gate covers direct references only. Debug
  or lab direct-entry boots that bypass the dispatcher were already outside
  the gate's scope (Phase 3).
- Code addresses in `.main` shift by about 31 KB, and cache phase moves with
  them. Perf figures from before and after this change are different layouts.

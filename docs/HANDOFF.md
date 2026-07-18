# Handoff

Updated: 2026-07-18 16:38 Central
`P1_EXECUTION_BOARD.md` owns all current state. This is only the restart surface.

## Restart
Branch: `master`

Boundary: `battle_playable_realtime`, mode `163`.

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

`P1_EXECUTION_BOARD.md` owns exact artifact identity and retained evidence.
Preserve canonical mode 163, intrinsic renderer mode 9, mip 0, static residency,
source countdown, exact Dream Land water frame 0, and Task 16 compare/i2f/addsub
`1/1/1`; global Task 16 lab defaults remain zero. Retail hardware closes Task
12 as blanket-Thumb REVERT and renderer-hot REVERT WITH BASE. The renderer is
ARM again. Task 17 is retained only as the exact 11-function / 5,016-byte
update working set. Its ARM-base device pair passes by crossing LOOP from five
to four VBlanks and FPS 13.9→14.3; the actual UPD delta is only -2,560, so do
not carry the earlier -44K Thumb-base result into ARM claims. melonDS remains
correctness evidence, never its speed referee.

## Next Packet

`docs/optimization/tasks.md` remains authoritative. Preserve the user's
uncommitted `AGENTS.md` and 344-line
`docs/optimization/ClaudeFable5_JumpABC_Tasks_20260715_2326.md` changes; neither
belongs to this checkpoint.

Task 25R is complete as a report-only baseline at measured source HEAD
`f088db98de272e9788405c2181029ad4a4c353ba`. Its detailed/profile-0 ROM pair is
`6E90D414...` / `E685C034...`; synchronized frame 607 is exactly 0/49,152
changed pixels. Profile 0 completes 4,084 updates / 2,042 presentations and one
teardown with 166,672-byte reserve, but reaches only 18.6 presentations/s and
37.3 updates/s. Its `61/1547/396/38` interval histogram contains 1,981
intervals of three or more VBlanks and 2,457 excess VBlanks. The exact Mario-KO
source sequence is present but playback/generation failures are 1/1, and the
post-GO texture fence first trips at class+1/frame 10/1111. The strict
stable-30 gate correctly fails; normal verifier paths remain strict.

The same-ROM owner ranking selects **M3-first**: stage P95 reaches 468,480
ticks, versus 380,544 for the largest combined fighter pair. Task 23R Phase 0
is complete at measured HEAD `1d381c447f06deed04b7749bffe6d5bb1259b303`.
Its generated certificate binds 588 pointer-field accesses in 36 production
closures to 140 immutable-generation, 43 live-camera, 260 live-camera-
independent, and 145 callback-visible classifications. Every pointer base is
observed, so a newly named base cannot evade the unclassified-read gate. Eight
same-ROM eight-frame windows report
all eight prepared segment lanes and four material lanes at `7/0` adjacent
hits/changes. Frames 672–679 include the exact natural Whispy Wait→Open /
material-animation edge at 675→676, with source state and exact G2 tuples
exported; other cross-window boundaries remain explicitly unknown. Early frame
607 is exactly `0/49,152` pixels against Task 25R, and Boundary passes in 77.8
seconds. Evidence is under `artifacts/performance/2026-07-18_task23r-phase0-*`,
with the authoritative manifest at
`docs/optimization/NDS_NATIVE_STAGE_CONSUMED_FIELDS.generated.json`.

Start Task 26. The smallest first candidate is generated preflight/control for
segment 0 / `layer0` only: bindings 0–19, runs 0–25, 54 triangles, 22 epochs,
all projected-no-Z, no material event. Reuse the existing prepared storage,
commit loop, and GX emitters; keep matrices, near-plane work, texture/color/
alpha/UV selection, validation, and pre-GX fallback live. Do not add a second
topology cache or a residual Task 23R cache. `src/nds/nds_renderer.c` remains
a mandatory one-writer surface. Attempt Task 23R Phase 1 only after Task 26
stabilizes and remeasurement proves residual work remains.

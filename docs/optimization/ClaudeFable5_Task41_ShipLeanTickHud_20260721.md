# TASK 41 — True ship configuration + profile-0 per-owner tick HUD

**State: EXECUTE — implementation and static/build gates pass; Tyler/runtime
qualification remains.**

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.** Run only after the Task 36 session is fully closed (branch settled).

## Why (review-validated)

`docs/optimization/reviews/gpt56.txt` P0-2/P0-3 + P1-2/P1-4/P1-5/P1-6 and
`docs/optimization/reviews/gpt56_2.txt` §4/§7/§8/§11-P0: the shipping profile-0
frame still pays every frame for renderer telemetry reset/publication, EndFrame
GX status/usage register reads, native-OAM semantic hashing and timing,
lower-screen HUD state fingerprinting, repeated zeroing of large
`NDSRendererStats` objects, and 64-bit rate-marker math in
`ndsAudioBgmUpdateRateMarkers` (src/nds/nds_audio_bgm.c:308) on every update.
It ALSO pays at UPDATE cadence (twice per present — planner-verified 7/21) for:
- the Fox AI recorder: `gNdsFTComputerProcessCount++; ndsFTComputerRecord(fp);`
  unconditional at src/import/battleship_ftcomputer.c:165-166 (~130-line
  observer: float conversions, collision-entry loop, volatile stores);
- fighter status/anim-event diagnostic wrappers: the production
  `ftMainSetStatus` IS the wrapper (src/port/reloc_backend_diagnostic_recorders.c:4463)
  and `ftMainPlayAnimEventsAll` has an always-on recorder;
- battle-update wrapper proof counters and harness-mode dispatch around
  `scVSBattleFuncUpdate` (src/port/taskman_seam.c:4353; verify which dispatch
  chain actually executes per update in mode 163);
- taskman per-update proof bookkeeping and FGM proof counters.
None of it changes pixels. Tyler independently requested the same thing plus a
realtime per-owner tick HUD that does not require profile 1 (profile-1 ROMs
distort timing and cannot run affine without the arena OOM).

## Deliverable: two profile-0 variants

- **LEAN (published):** zero diagnostics except (a) the FPS/UP HUD, (b) the
  pacing interval histogram increments and shared engagement counters — these
  are the device-evidence backbone and MUST survive (they are cheap RAM adds).
- **TICKHUD (new target, e.g. `smash64ds-battle-playable-tickhud-hwtri`):** LEAN
  plus a lightweight per-owner tick HUD (below). Profile 1/2 remain unchanged
  for forensic work.

## Steps

1. **Inventory first.** Table of every profile-0-active diagnostic: symbol/site,
   what it writes/reads (volatile publications, GX status reads, hashes,
   memsets, 64-bit divides), and an estimated per-frame cost. Cite file:line for
   each. This table is the checklist for step 2 and the report's core.
2. **Gate them.** New `NDS_SHIP_TELEMETRY` (default 1 = today's behavior; LEAN
   builds set 0; gpt56_2 calls the same concept NDS_RUNTIME_PROOF_DIAGNOSTICS —
   ONE switch, use the NDS_SHIP_TELEMETRY name). Behind it: telemetry
   publication/reset, GX status/usage reads, OAM semantic hash+timing, HUD
   fingerprinting, stats zeroing beyond what the active profile consumes,
   BGM/FGM rate-marker publication, the Fox AI recorder, the fighter
   status/anim-event recorders, and battle-update proof counters.
   **SAFETY GATE for `ftMainSetStatus`:** its before-hook can SUPPRESS the
   original call — before bypassing the wrapper in LEAN, prove with one
   instrumented soak (counter on the suppress path) that the hook never fires
   in profile-0 gameplay; if it ever fires, keep the hook's functional branch
   and strip only the recording. Same proof discipline for any hook that can
   alter control flow. NOT behind it:
   pacing histogram, engagement counters, FPS sampler, freeze-diagnostics entry
   points that are already compile-time-off. Every verifier script that
   GDB-reads a gated counter must keep a build that still has it (diagnostic
   targets keep NDS_SHIP_TELEMETRY=1) — list which scripts read what.
3. **Tick HUD.** `NDS_TICK_HUD` flag (LEAN=0, TICKHUD=1): cpuGetTiming() reads
   at the existing phase boundaries, accumulated into Tyler's nine buckets —
   ALL, Mario+Fox, Stage, Background(wallpaper), Audio, HUD(OAM/interface),
   SourceUpd, MiscDraw(residual+gxFlush), Other(=ALL − sum, so nothing hides).
   Present as rows on the lower screen at the existing half-second HUD cadence
   (current | rolling mean, same format as the phase HUD rows). Measured
   overhead budget: ≤3K ticks/frame — measure and report it honestly.
4. **Build matrix + identity.** Published target becomes LEAN; add TICKHUD
   target to the standard build set. The published ROM bytes change, so update
   the publish identity expectations (DECOMP_PIN OUTPUT_* / README expected
   SHA-256) in the same change — otherwise stranger builds fail their identity
   gate.
5. **Measure the win.** melonDS synchronized A/B, LEAN vs current published
   config: whole-loop and typed owner deltas + calibration-predicted device
   delta. Queue the retail pair in `builds/device-queue/`.

## Gates

- Gameplay verifiers green; synchronized screenshots pixel-identical (stripping
  telemetry must not change rendering).
- Pacing histogram + engagement counters verified still functional in LEAN
  (GDB-read them in one melonDS run — this is a hard gate, not optional).
- TICKHUD bucket sum-check: named buckets + Other ≈ ALL within ~1%.
- verify chain green; full-match soak on both variants.

**Kill criterion:** if the inventory's realistic aggregate is < ~10K/frame,
report the table and stop before building the flag plumbing — the tick HUD
half still proceeds (it is wanted regardless of the telemetry win).

## 2026-07-21 execution

### Phase A census

These are estimates from the actual profile-0 call sites, not measured wins.
The conservative total is already above the 10K-tick kill threshold, chiefly
because Fox recording runs at update cadence and renderer reset/publication
runs at present cadence.

| Site | Profile-0 diagnostic work before this task | Estimated ticks/present | Disposition |
|---|---|---:|---|
| `src/port/taskman_seam.c:4566,4809`; `src/nds/nds_renderer.c:23013-23414` | Renderer frame reset/publication: volatile clears, fast-path arrays, runtime-summary clear and copy-out | 10K-25K | `NDS_SHIP_TELEMETRY` |
| `src/nds/nds_platform.c:2866-2869` | Polygon/vertex usage and GX status/control MMIO reads | 1K-4K | telemetry/profile only |
| `src/nds/nds_ifcommon_oam.c:2114,2294,2612-2750` | OAM timer reads/adds plus per-SObj semantic hashing | 2K-8K | `NDS_SHIP_TELEMETRY`; OAM behavior retained |
| `src/nds/nds_platform.c:2290-2312,2376-2395` | Lower-HUD 13-field FNV fingerprint and proof publications | 1K-3K | direct functional field comparison in LEAN |
| `src/nds/nds_audio_bgm.c:309-340,474-475,721-722,764-765` | 64-bit stream/playback rate and position marker math | 3K-10K | `NDS_SHIP_TELEMETRY`; playback retained |
| `src/import/battleship_ftcomputer.c:37-126,169-172` | Fox observer: float-to-milli conversion, collision scan, masks and volatile stores, twice per present | 8K-20K | observer gated; base level-3 AI always runs |
| `src/import/battleship_ftmain.c:97-102,106-118` | Animation/status diagnostic recorders at update/event cadence | 1K-6K | after-recorders gated; functional before-hook retained |
| `src/port/taskman_seam.c:4396-4407` | Per-update proof mirror, dispatch counter and result/mask publications | 0.5K-2K | `NDS_SHIP_TELEMETRY`; source update retained |
| `src/nds/nds_audio_fgm.c:800-857` | Handle duration/envelope service and event-only proof counters | no removable steady-state rate marker | functional service retained; ARM7 ACK diagnostics remain separately default-off |

Retained in LEAN: FPS/UP sampling, the 2/3/4/5+ pacing histogram, shared
engagement counters, functional HUD change detection, and the compile-time-off
freeze entry points. `ftMainSetStatus` keeps its before-hook because it can
suppress the source call; only its after-recorder is stripped.

### Implementation and build matrix

- `NDS_SHIP_TELEMETRY` defaults to 1; the published target forces 0.
- `NDS_TICK_HUD` defaults to 0. The new
  `smash64ds-battle-playable-tickhud-hwtri` target is profile 0, telemetry 0,
  tick HUD 1.
- `smash64ds-battle-playable-proof-hwtri` is release-equivalent profile 0 with
  telemetry 1 and tick HUD 0. Metric-reading GDB verifiers use it; only the
  explicitly named observer-free Down-Air snapshot uses LEAN.
- The tick HUD publishes current and running-mean rows every 0.5 seconds for
  ALL, Mario+Fox, Stage, Background, Audio, HUD, SourceUpd, MiscDraw and Other.
  `Other = max(ALL - named, 0)`; flush is included in MiscDraw.
- Fox AI source default is 1. Audit/capture scripts may disable it explicitly;
  published and proof targets never do so implicitly.

Build-only checkpoint identities (these will change after Tasks 38/42 and are
not final publish pins):

| Variant | Bytes | SHA-256 |
|---|---:|---|
| LEAN `smash64ds-battle-playable-hwtri.nds` | 11,141,120 | `0418EB71119F99108D85757566F5CDFE8FF1EB7AE4D3C1682B207CE819C2477E` |
| TICKHUD `builds/build-task41-tickhud-current/smash64ds-battle-playable-tickhud-hwtri.nds` | 11,143,168 | `C51178A4372E82E89C45AA954D30EAD424BC004345C47929FFE8DC014203BDA6` |
| PROOF `builds/build-task41-proof/smash64ds-battle-playable-proof-hwtri.nds` | 11,145,216 | `9AD548D8A674121FA27B739A64B50C039C1E9153DC247E19B5A82217D93BD758` |

Static/build gates: all three variants compile; generated configurations are
`0/0/0`, `0/0/1`, and `0/1/0` for profile/telemetry/tick HUD respectively;
`check-gbi-decode-fixtures.ps1` and `check-harness-registry.ps1` pass.

Runtime gates still open and must not be inferred from compilation: Tyler's
pixel check, LEAN pacing/engagement GDB read, TICKHUD <=3K overhead and ~1% sum
conservation, synchronized LEAN/control timing, and full-match soaks. No
emulator rerun was made while recovering from the prior hang loop.

The retained Boundary run passed on 2026-07-21 with Fox CPU on and the LEAN
canonical screenshot/detail contract green. It does not replace the remaining
LEAN-counter, TICKHUD conservation/overhead, or full-match gates.

The focused TICKHUD profile-0 probe reached exact frame 600 with Fox CPU `1`.
Its buckets were ALL 70,016; Fighters 25,344; Stage 16,320; Background 128;
Audio 64; HUD 64; SourceUpd 13,632; MiscDraw 1,152; Other 13,312. Named 56,704
+ Other 13,312 equals ALL exactly (0% conservation error). The raw interval
buckets were `0/0/519/73/7`, max 17, BGM seams 31, and the Task 39 engagement
mask was nonzero. The <=3K instrumentation-overhead A/B and full-match soak
remain open.

Tyler then found that the post-Task-42 LEAN and TICKHUD FPS/UP values were wrong
and updated infrequently, while the pre-Task-42 TICKHUD ROM was correct. Root
cause: Task 42 assigned its seam IRQ to ARM9 timer 2, but current Calico
`tickInit` owns timers 2/3 and `cpuGetTiming()` reads that system tick. The BGM
seam IRQ now uses free timer 0; the asset checker fails if it regresses to 2/3.
The rebuilt hashes above contain that fix. Tyler's confirmation remains open.

Metric readers and their diagnostic configurations:

- `verify-battle-mariofox-gcrunall-loop-harness.ps1`: renderer/GX/OAM/Fox/BGM
  publications; the profile-0 wrapper routes it to PROOF.
- `verify-battle-playable-camera-containment.ps1` and
  `verify-battle-playable-fox-recovery.ps1`: renderer/GX and Fox-AI counters;
  both route to PROOF.
- `verify-battle-playable-down-air-stall.ps1`: update/Fox counters route to
  PROOF, except its explicit observer-free LEAN snapshot.
- `verify-ifcommon-native-oam.ps1`: OAM timing/hash counters; its default
  profile-1 coarse target keeps telemetry 1.
- `capture-cut-g-exact-frames.ps1`: reads its caller-supplied ELF and therefore
  inherits the caller's LEAN/PROOF choice.

# Scene transitions — investigation, 2026-09-30

**INVESTIGATION COMPLETE. Both owner-reported failures reproduced. No tracked file changed, no fix applied, no ROM built or published.**

The old main BG0 polygon list survives scene teardown. Results submits no replacement geometry until its fighter reveal, and CSS performs its preview loads before its existing layer clear. New BG2 artwork is written directly into visible VRAM during those loads. This produces the old Castle stage over Results wallpaper and the large Results fighters over CSS art. A register-only probe independently proves the BG0/BG2/OBJ attribution. Four runs supplied **885 consecutive both-screen baseline scanouts**, plus 28 layer-probe captures.

Owner report: "some transitions (match to results, results to CSS) don't transition cleanly (we see leftover fighters while CSS is loading, or stage is still present while results background is present and loading the rest of results), transitions should be more 'separated' or clear out previous screen before loading next screen. TLDR need cleaner loading transitions."

## 1. Frozen identity and exact recipe

| Input | Identity |
|---|---|
| ROM | `builds/p2p8-playtest-r58/smash64ds.nds`, 63,177,728 bytes |
| ROM SHA-256 | `2C22DE8E17D5D016459F868D4D56D261AE64929985E4C3A504DC326F39C56439` |
| ELF | matching `smash64ds.elf`, 27,196,248 bytes |
| ELF SHA-256 | `99F2775E82DBA54AF06A04411DCC5169067F6003AA5AF8DB35D575DCB6438C48` |
| Declared build/source revision | `9fdc936adeb68aa201b58fab073befda2d3317b5` |
| Frozen investigator copies | `builds/codex-transitions/smash64ds.{nds,elf}`; final hashes still match |
| Accurate emulator | `emulators/melonds-runners/slot2/melonDS.exe`, SHA-256 `DE80E46BDCF1FD986162DE6AFFD9EE1148F8C40565DBAF667B9C2B3EF5475715` |
| Original/final slot2 TOML SHA-256 | `00E57544867E06FB1A45D8BE2FBB3B5F1016C3C55AA32A11B3BAF419949052B3` |
| Debugger | devkitARM `arm-none-eabi-gdb`, GNU GDB 14.1, MI2, ARM9 port **3320** |
| Execution | Interpreter from boot, `[JIT] Enable=false`; software accurate 3D; no GL upscale; direct boot; no cheats |
| Storage | Each run copies slot2 DLDI + index into its own scratch storage; read-only DLDI, folder sync off, separate fresh saves/states |

The shared HEAD was `365cf5688531d50de8084f1c7bf89f9fb8bc7ae2` at startup and other people continue editing it. **Port file:line citations and the proposed diff below refer to frozen r58/`9fdc936adeb`, not the changing working tree.** Relevant baseline source copies live in `builds/codex-transitions/baseline-source/`. The scoped read-only comparison subsequently found 33 integrator audio additions in `nds_menu_shell_router.c`; these are preserved and must be accounted for when adapting the proposal. The r58 archive contains only ROM/ELF, no matching generated configuration header; expanded build flags were not independently recovered.

Run from repository root using PowerShell 7:

```powershell
# Complete baseline recipe, with the corrected Results START feed.
$job = Start-Process pwsh -ArgumentList '-NoProfile','-File',
    'builds\codex-transitions\run-transitions.ps1','-Tag','<fresh-tag>' `
    -WindowStyle Hidden `
    -RedirectStandardOutput 'builds\codex-transitions\<fresh-tag>.launch.out' `
    -RedirectStandardError 'builds\codex-transitions\<fresh-tag>.launch.err' -PassThru

# One status line, no growing log. Poll no more frequently than every two minutes.
Get-Content 'builds\codex-transitions\<fresh-tag>.status' -Tail 1
```

The copied driver (`run.ps1`, `setup.gdb`, `collect-text.py`, `capture.ps1`, config template) was extended by `run-transitions.ps1`, `collect-transitions.py`, and `capture-win32.py`. The launcher refuses a busy slot2, saves the original TOML before changing it, and restores its exact bytes in `finally`. Every run's final launcher status confirms `CONFIG_RESTORED=True` with the original hash. Slot2 is idle at handoff.

Input travels the game's ordinary keypad-return path: Title START at reads 140/141; mode menu Down at 12 and A at 24/25; VS A at 24/25; first CSS START at 140/141; first SSS A at 60/61. At first SSS, four **aligned whole-word** writes seed Mario/Kirby/Fox/Yoshi (`0,8,1,6`), one human and three level-9 CPUs, and a whole-word read/modify/write sets one-minute Time. Battle-start registers/telemetry positively confirm `stage=0 time=1 rules=1 players=1 cpus=3 roster=0,8,1,6 pkind=0,1,1,1`. Stage 0 is **Peach's Castle**. A real one-minute match reaches Time Up; no timer shortening, fast logic, result injection, load rebudgeting, source fade modification, GDB calls, or byte pokes. Results START is held consistently across all three keypad reads in source tics congruent to 12/13 mod 30 once exit is allowed; accepted on tic 433.

Each clip stops at **`ndsPlatformVBlankInterrupt+8`** (`0x0201014c` in r58), after the IRQ increments the VBlank count. Register **r3** supplies the authoritative counter; all four clip sequences were checked for increments of exactly one. PrintWindow captures the specific slot2 HWND under the existing `Global\Smash64DS_MelonDSCapture` mutex; both screens are stacked, 416×664 window, native aspect, nearest sampling. Per-frame JSON/PNG is persisted after every capture. Additional anchors stop at EndFrame entry or Time Up entry.

**Interpretation:** a VBlank screenshot is the last completed scanout; the source tic/register snapshot is the tic currently in progress and its OAM commit may still be ahead. The first ~130 Results tics are covered by capture through tic 131. `present` is the port's `sTicks`, not an assertion of new scene content: fallback presents also increment it. Frozen HUD FPS values (often 0) are ignored. Loading lengths below use guest VBlanks, not host pause/capture time.

| Run / launch CDT | Exit and retained proof |
|---|---|
| `r58-sequence01`, 15:18:54 | EXIT 1: valid CSS→SSS and SSS→battle clips. Collector rejected Time Up's correct PC because inline DWARF named the frame `ifCommonBattleSetInterface`. Address-based validation repaired; guest did not fail. |
| `r58-results02`, 15:28:00 | EXIT 143: complete battle→Results clip. Read-count-based START pulse was lost among three Results keypad reads; guest continued normally. Sole Python child of its verified launcher was stopped, wrapper restored TOML. This partial run's Results→CSS tail is excluded. |
| `r58-results03`, 15:35:52 | EXIT 0: corrected tic-consistent input reaches CSS naturally; complete Results→CSS clip. Early Results clip repeated as the corrected-input control; primary early Results evidence remains run 2. |
| `r58-layer04`, 15:45:42 | EXIT 0: causal layer probe at Results tic 100; original DISPCNT restored. |

Helper limitations: WMI process enumeration denied; used Get-Process and Toolhelp parent PID instead. Winget `rg.exe` inaccessible; CodeGraph was used first, then bounded PowerShell reads/searches. `psutil` unavailable; no installation attempted. No build was needed. Scratch driver Python/PowerShell syntax checks pass. `check-melonds-policy.ps1 -SkipLocalConfigs` passes, runner identities 13/13; it did not audit or change another slot's live configuration. Final evidence QA validates all 885 PNG hashes, frame-table/report links and included diff. Raw MI, emulator and launcher transcripts/configs belong to `builds/codex-transitions/`; decoded event transcripts left alongside captures are raw diagnostics and must not be committed with the curated evidence.

## 2. Every captured frame, with both-screen links

[All classified rows, CSV](classified-frames.csv) · [All metadata and capture SHA-256, JSON](classified-frames.json).

The four linked full tables below contain **one row and PNG link for every scanout**, with flags for retained 3D, old content during loading, mixed content, and incomplete destination drawing. Mixed-content flags explicitly include old SUB telemetry when MAIN already changed; that debug-only case is distinguished from the owner's MAIN product-art overlap. Tables here group identical findings without omitting frames.

### Battle Time Up → Results, tics 0–130

**411 consecutive scanouts, VBlanks 5097–5507.** [Every-frame table](battle-results-frames.md) · [Contact sheet](battle-results-detail-contact.png).

| Frame indices | Source tic in progress / VBlank | MAIN | SUB | Classification / captures |
|---|---|---|---|---|
| 0000–0095 | Time Up hold / 5097–5192 | Battle stage/held fight | Battle HUD | Authored end-of-battle hold, not counted as loading. [First](r58-results02/battle-results-0000.png), [last](r58-results02/battle-results-0095.png) |
| 0096–0247 | Destination selected, Results tic 0 / 5193–5344 | Old Castle geometry + old wallpaper | Old clock/portraits/damage | **Old scene visible during entry loads: 152 scanouts, ~2.54 guest seconds.** Main-loop fallback presents occur, but no new 3D list. [Start](r58-results02/battle-results-0096.png), [end](r58-results02/battle-results-0247.png) |
| 0248–0249 | Tics 1–2 / 5345–5346 | Castle still visible | HUD cleared | Source scene is running, old 3D remains. [0248](r58-results02/battle-results-0248.png) |
| 0250–0327 | Tics 3–80 / 5347–5424 | Old stage shaded then uncovered by Results startup OBJ tint | Cleared | **New Results tint over stale battle list.** No 3D flush. [0257](r58-results02/battle-results-0257.png), [0327](r58-results02/battle-results-0327.png) |
| 0328–0332 | Tic 80 / 5425–5429 | Old stage over progressively written Results wallpaper | Cleared | **Mixed MAIN scenes + exposed in-scene wallpaper load.** [0328](r58-results02/battle-results-0328.png), [0332](r58-results02/battle-results-0332.png) |
| 0333–0371 | Tics 81–119 / 5430–5468 | Castle stage/battle fighters over complete Results wallpaper | Cleared | **Mixed MAIN scenes.** [0333](r58-results02/battle-results-0333.png), [0371](r58-results02/battle-results-0371.png) |
| 0372–0399 | Tics 120–121 / 5469–5496 | Same old stage over Results wallpaper during fighter construction and first GX handoff | Cleared | **Mixed MAIN scenes + late Results load.** New 3D flush appears only near the end. [0372](r58-results02/battle-results-0372.png), [0399](r58-results02/battle-results-0399.png) |
| 0400–0410 | Tics 121–131 / 5497–5507 | Results fighters/YOSHI WINS/wallpaper; old stage gone | Cleared | First destination 3D scanout and following source reveals. [0400](r58-results02/battle-results-0400.png), [0410](r58-results02/battle-results-0410.png) |

Retained stage is visible in **304 successive scanouts after Results is selected** (0096–0399); new Results wallpaper and old stage overlap in 0328–0399 (72 scanouts). `gNdsHardwareRendererFlushCount` is **2125** throughout tics 1–119 and almost all tic-120 loading; the next count appears at 0398. This is a retained native 3D list, not a source photo being animated by fresh geometry.

### Results START → CSS

**114 consecutive scanouts, VBlanks 5845–5958.** [Every-frame table](results-css-frames.md) · [Exit/loading detail](results-css-ending-0.png) · [CSS overlap/removal detail](results-css-ending-1.png).

| Frame indices | VBlank / state | MAIN | SUB | Classification / captures |
|---|---|---|---|---|
| 0000–0041 | 5845–5886; tics 405–432 | Complete Results | Results telemetry | Normal wait/input; no load. [0000](r58-results03/results-css-0000.png) |
| 0042 | 5887; tic 433, CSS requested | Last complete Results | Same | Exit accepted; last old scanout. [0042](r58-results03/results-css-0042.png) |
| 0043 | 5888 | Text gone; **large Results fighters/confetti still over Results wallpaper** | Same | OAM tenant exited, 3D retained. [0043](r58-results03/results-css-0043.png) |
| 0044–0083 | 5889–5928; CSS preview init/load | Large Results fighters/confetti over black then blue field | Results telemetry | **Old 3D visible throughout CSS loading.** Main product picture is byte-identical from 0045–0083. [0045](r58-results03/results-css-0045.png), [0083](r58-results03/results-css-0083.png) |
| 0084–0087 | 5929–5932 | Large Results fighters/confetti over cleared main field | Same | Old 3D still enabled; menu layers have finally been cleared. [0084](r58-results03/results-css-0084.png) |
| 0088–0101 | 5933–5946 | CSS blue plate, portraits, panels progressively appear **under large Results fighters/confetti** | Same | **Mixed MAIN scenes during CSS art loading.** [0090](r58-results03/results-css-0090.png), [0101](r58-results03/results-css-0101.png) |
| 0102 | 5947; first CSS draw approaching | CSS art still under old large fighters | Same | **Final captured mixed scanout.** BG0 already reads disabled at this IRQ; the screenshot completed just before that state took effect. [0102](r58-results03/results-css-0102.png) |
| 0103–0113 | 5948–5958 | CSS panels and own slot silhouettes; large Results 3D gone | CSS telemetry | Previous scene removed. This short tail does not prove all live preview models have finished loading. [0103](r58-results03/results-css-0103.png), [0113](r58-results03/results-css-0113.png) |

Sixty scanouts after the last full Results frame contain its old fighters (0043–0102); **15 show them directly over new CSS art/plate** (0088–0102). No old Results text OBJ survives: it is cleared at 0043. Confetti follows the retained 3D list and disappears with it, rather than behaving like leftover menu OBJ.

### CSS → SSS

**57 consecutive scanouts, VBlanks 563–619.** [Every-frame table](css-sss-frames.md) · [Consecutive detail](css-sss-detail-contact.png).

| Frame indices | MAIN | SUB | Finding / captures |
|---|---|---|---|
| 0000–0036 | CSS, including 30-tic accepted START delay | CSS telemetry | Normal source-authored wait. [0036](r58-sequence01/css-sss-0036.png) |
| 0037–0040 | Last CSS preview frame at 0037; old CSS art without previews at 0038–0040 | CSS telemetry | SSS requested while old screen is shown. Existing CSS exit hides BG0; **no CSS fighter over new SSS art**. [0037](r58-sequence01/css-sss-0037.png), [0040](r58-sequence01/css-sss-0040.png) |
| 0041–0044 | Cleared main field | CSS telemetry | Loading exposed; old SUB debug content remains. [0041](r58-sequence01/css-sss-0041.png) |
| 0045–0051 | Blue and partial SSS art | CSS telemetry | **Partial destination construction**, mixed only through old SUB telemetry. [0045](r58-sequence01/css-sss-0045.png), [0051](r58-sequence01/css-sss-0051.png) |
| 0052–0053 | Full plate and selected preview; red selection border not yet visible | CSS then SSS telemetry | Destination entry finishing. [0052](r58-sequence01/css-sss-0052.png), [0053](r58-sequence01/css-sss-0053.png) |
| 0054–0056 | SSS with selected Castle preview/border | SSS telemetry | Complete captured SSS state. [0054](r58-sequence01/css-sss-0054.png), [0056](r58-sequence01/css-sss-0056.png) |

### SSS → battle

**303 consecutive scanouts, VBlanks 668–970.** [Every-frame table](sss-battle-frames.md) · [Loading rows](sss-battle-loading-0.png) · [First 3D handoff](sss-battle-loading-1.png).

| Frame indices | VBlank | MAIN | SUB | Finding / captures |
|---|---:|---|---|---|
| 0000–0005 | 668–673 | SSS | SSS telemetry | Before A acceptance. [0000](r58-sequence01/sss-battle-0000.png) |
| 0006–0271 | 674–939 | Old SSS retained while battle is selected/loaded | SSS telemetry | **266 old-main-picture scanouts after request (~4.45 seconds).** Main content identical from 0007–0271. [0006](r58-sequence01/sss-battle-0006.png), [0271](r58-sequence01/sss-battle-0271.png) |
| 0272–0277 | 940–945 | Partially copied Castle wallpaper over cleared field | SSS telemetry | Exposed VRAM construction; **no old SSS MAIN art in these partial rows**, but old SUB remains. [0272](r58-sequence01/sss-battle-0272.png), [0277](r58-sequence01/sss-battle-0277.png) |
| 0278–0292 | 946–960 | Castle wallpaper only | SSS telemetry | Destination still incomplete; no battle geometry. [0278](r58-sequence01/sss-battle-0278.png), [0292](r58-sequence01/sss-battle-0292.png) |
| 0293 | 961 | Castle wallpaper; BG0 being enabled | Battle telemetry starts | First GX swap bracket, still incomplete scanout. [0293](r58-sequence01/sss-battle-0293.png) |
| 0294 | 962 | **Retained two CSS preview fighters over Castle wallpaper** | Battle HUD starts | **One MAIN mixed-scene frame.** Before-swap BG0 enable exposes the previous list; next frame has the battle geometry. [0294](r58-sequence01/sss-battle-0294.png) |
| 0295–0302 | 963–970 | Castle battle geometry under source entry fade | Battle HUD | New scene's own output. [0295](r58-sequence01/sss-battle-0295.png), [0302](r58-sequence01/sss-battle-0302.png) |

`present=438` remains fixed from VBlank 677 through 961: **285 scanouts with no new completed present**, ~4.76 guest seconds. The deferred enable already prevents CSS fighters during almost all loading, but its enable happens before the flush-associated retrace and fails for the first scanout. Cleared geometry plus an enable only after the new list can be displayed resolves that remaining sibling.

## 3. What the source authors at each boundary

These are contracts established from read-only BattleShip source, **not an N64 emulator capture**. The cut means the first new composed framebuffer replaces the old one; there is no authored arbitrary black-frame dwell in the CSS/SSS/Results exit paths.

| Boundary | Source contract and exact references | Port divergence |
|---|---|---|
| Time Up → Results | `decomp/BattleShip-main/decomp/src/if/ifcommon.c:3342–3345` installs Time Up with restore wait 90; `:2974–2980` hides interface and enters Set; `:3144–3152` requests scene load after the Set wait. `sc/sccommon/scvsbattle.c:540–560` handles Sudden Death then selects Results. Normal Results **animates a photo of the last battle framebuffer**, not an unchanging live stage: `mn/mnvsmode/mnvsresults.c:3360–3364`; `lb/lbtransition.c:152–170` draws/updates the animated transition; `:205–238` copies the photo. Black startup tint begins alpha 255 and decrements 5 per draw (`mnvsresults.c:1699–1725`); wallpaper tic 80 and results/fighters tic 120 (`:2799–2845`, `:3233–3268`). | No scene-exit retirement; old BG0 list rerenders during loads and early Results. Port photo aliases uniform CPU framebuffer storage (`src/import/battleship_lbtransition.c:21–35`), so the stale Castle is **not** the source's animated captured photo. Startup tint exposes it, and new wallpaper at 80 is composited behind it until replacement GX geometry at 120. |
| Results → CSS | Exit gate 410 for Time (`mnvsresults.c:2820`), START check (`:266–282`), immediate scene selection/stop BGM/load (`:3277–3316`). **Cut; no exit fade actor.** CSS constructs its own full display (`mn/mnplayers/mnplayersvs.c:4725–4798`), with its own setup (`:4850` onward). | `ndsMNVSResultsSetLoadScene:310–312` exits main Results OAM but does not retire BG0. CSS wrapper `nds_menu_shell_router.c:642–648` loads previews before the clear in `ndsMenuShellRun:352–411`; requesting BG0 enable does not disable an already visible old list. |
| CSS → SSS | Accepted START after one second, ready check, pause slot processes, **30-tic proceed wait** (`mnplayersvs.c:4485–4525`), then Maps selection/load (`:4491–4512`). **Cut; no fade.** SSS constructs its cameras/art in `mn/mnmaps/mnmaps.c:1591–1660`, setup `:1712` onward. | Existing CSS exit disables BG0 (`nds_menu_shell_router.c:475–481`), which works. BG2/OBJ art construction remains visible before first SSS present; SUB telemetry updates late. |
| SSS → battle | A/START selects battle immediately (`mnmaps.c:1478–1493`), **no SSS exit fade**. Battle constructs its stage/fighters/HUD, then requests **12-tic black lbFade** (`sc/sccommon/scvsbattle.c:218–224`; same for Sudden Death `:495–501`). | SSS plate remains during loads, visible BG2 copy exposes partial wallpaper, and first BG0 enable briefly shows cached CSS geometry. Then the existing source lbFade runs. |

The source-faithful loading cover must end when the destination frame is ready; **do not wait until tic 80/120 or change the 12-tic battle fade, startup/wallpaper tint alpha progression, exit gates, or 30-tic CSS proceed wait.** Real source photo/emblem delivery remains separate required fidelity: a generic blackout must not be treated as implementing the missing photo producer or accepting its uniform substitute. If a native last-frame photo is added, capture/preserve it at the final battle present **before** the old display is cleared.

## 4. Layer proof and owning causes

[Layer probe contact sheet](layer-probe-contact.png) · [Probe register/mask recipe](r58-layer04-probe.json) · [Each probe frame](r58-layer04-frames.json).

| Register-only state | Observed pixels | Conclusion |
|---|---|---|
| Original DISPCNT `0x0021155d` | Old Castle stage/battle fighters over Results wallpaper | Baseline |
| Clear BG0 bit `0x100` → `0x0021145d` | Stage and old fighters disappear; Results wallpaper remains | **Retained 3D list is the leftover owner.** |
| Restore original BG0 | Castle reappears with no new flush | Same saved list, not new source stage submissions |
| Clear BG2 bit `0x400` | Results wallpaper disappears; old stage remains over backdrop | BG2 is the new wallpaper owner |
| Clear OBJ bit `0x1000` | Tint disappears; geometry remains, brighter | OAM owns the source black tint, not the stage |
| Clear BG3 bit `0x800` | No change (BG3 already off at anchor) | BG3 is not carrying this stage symptom |

Every state uses aligned 32-bit DISPCNT writes, three consecutive scanouts to remove capture/pipeline ambiguity, and restoration between tests. The 3D flush counter remains **2125 across every probe capture**, including hide→restore. No timing/input/result state changed by the probe.

Owning seams in r58:

1. **No display retirement at exit.** `src/port/nds_scene_manager.c:373–393` records accounting and releases Hyrule textures, with no blackout, empty swap, BG0 disable, or general display clear. Object/arena teardown does not retire DS polygon RAM.
2. **A zero-submit destination keeps the old 3D frame.** `src/nds/nds_platform.c:3724–3788` skips glFlush unless real geometry or an overlay transform is pending. Results does not replace the list until its fighter reveal; measured 2125 proves that branch. Results is explicitly exempted from the source-menu entry clears (`src/port/taskman_seam_harness.c:66–76`).
3. **Entry clears run after blocking CSS loads.** `src/nds/nds_menu_shell_router.c:642–648` initializes/synchronizes previews first. Clear/3D ownership is inside the later `ndsMenuShellRun:352–411`. Source Results exit (`src/import/battleship_mnvsresults.c:310–312`) clears OAM only, while `nds_results_oam.c:1351–1360` does exactly that hardware operation. This explains why text goes first, while fighters/confetti stay.
4. **Current 3D enable is a queued request, not an old-owner disable.** `nds_platform.c:1443–1466` sets `s3dLayerEnableOnNextPresent` on TRUE without changing an already active BG0. The actual enable is before the VBlank wait (`:3796–3806` vs `:3833`), so a swapped list is mistaken for a displayed list on the initial handoff.
5. **BG2/OBJ writes are live before a complete present.** Existing shell clears/backdrop/blits are immediate (`nds_menu_shell_router.c:378–411`); the captured progressive SSS/CSS/wallpaper rows prove their visibility. New Results wallpaper is BG2; it cannot remove stage geometry in BG0.
6. **Blackout requires a loading latch independent of source setup.** `video_blackout.c:53–57` only marks dirty; hardware writes occur in its sole `ndsVideoBlackoutCommit:104–118`, currently called by EndFrame after VBlank. `battleship_sys_video.c:47–71` and `ndsMenuShellEnterBackdrop:94–98` honor destination NOBLACKOUT. Reusing that source latch alone would let setup undo a loading cover.
7. **Any-EndFrame release is premature.** `src/nds/main.c:162–178` presents if the scene coroutine yielded without a scene-owned present. Captures show several `present` increments while destination tic is zero. Those service/loading frames must not release a transition.

The DS references were inspected read-only: `decomp/sm64-nds/src/nds/nds_renderer.c:1250–1284` flushes and brackets with retraces; `decomp/sm64ds-decomp/src/_ZN3OAM5FlushEv.cpp:5–24` keeps separate main/sub DMA staging. They support retiring native hardware owners and committing at a display boundary; no interpreter, compositor, speculative scene framework or copied architecture is needed.

## 5. Proposed smallest general repair — not applied or built

Use one independent loading-cover latch in the existing sole brightness writer. At the shared scene exit, drain asynchronous GX submission, swap the now-empty build list once, commit black on both screens at VBlank, hide old BG0, and clear old overlay/HUD contents through their existing ownership guards. Keep source BLACKOUT/NOBLACKOUT and lbFade untouched. Release the loading latch only at the **actual scene draw producers**, immediately before their complete-frame EndFrame call; main-loop fallback frames never release it.

Results must explicitly request its own BG0 now that it cannot inherit it. Keep BG0 hidden until a submitted new list has passed the swap/raster/display bracket; repeated TRUE requests while BG0 is already active must not pay another wait. Move brightness commit after native OAM/BG commits so a release sees the complete new frame.

This draft changes one latch, one shared exit helper, and the existing presentation call sites. It allocates no framebuffer/mirror, adds no source update or extra gameplay present, and applies no source fade resets or tick offsets. It intends to spend retraces **only at scene/3D ownership handoffs**. The exact first-GX visible timing, empty-list precondition, bank-D loan safety, current pacing integration and ordinary-frame cost still require the integrator's frozen candidate proof. The 12-tic source fade and every gameplay logic/digest row must remain unchanged.

**Unified diff against r58 (`9fdc936adeb`), included here for review only:**

```diff
--- a/include/sys/video.h
+++ b/include/sys/video.h
@@ -186,6 +186,9 @@
 
 void ndsVideoSetBlackout(sb32 black);
 sb32 ndsVideoGetBlackout(void);
+/* Port loading cover, independent of source BLACKOUT/NOBLACKOUT and lbFade.
+ * Release only after a scene-owned complete draw, never a loading present. */
+void ndsVideoSetTransitionBlackout(sb32 black);
 /* Frame-boundary apply of the blackout latch; call after VBlank (see
  * src/port/video_blackout.c). Cheap no-op when the latch is clean. */
 void ndsVideoBlackoutCommit(void);

--- a/include/nds/nds_platform.h
+++ b/include/nds/nds_platform.h
@@ -58,6 +58,8 @@
  * own no 3D content hide it so a retained previous 3D frame cannot bleed
  * through; CSS/battle/Results show it when they own 3D again. */
 void ndsPlatformSet3DLayerEnabled(s32 is_enabled);
+/* Retire the old display before destination asset construction. */
+void ndsPlatformBeginSceneTransition(void);
 /* Map the source engine's 320x240 viewport edge coordinates to the DS GX
  * viewport. CSS uses the source PlayersVS camera's (10,10)-(310,230) window;
  * restoring the full viewport afterwards prevents that menu camera from

--- a/src/port/video_blackout.c
+++ b/src/port/video_blackout.c
@@ -47,6 +47,7 @@
 #endif
 
 static sb32 sNdsVideoBlackout = FALSE;
+static sb32 sNdsVideoTransitionBlackout = FALSE;
 static u32 sNdsVideoSourceFadeLevel;
 static sb32 sNdsVideoBrightnessDirty = FALSE;
 
@@ -54,6 +55,17 @@
 {
     sNdsVideoBlackout = (black != FALSE) ? TRUE : FALSE;
     sNdsVideoBrightnessDirty = TRUE;
+}
+
+void ndsVideoSetTransitionBlackout(sb32 black)
+{
+    sb32 next = (black != FALSE) ? TRUE : FALSE;
+
+    if (next != sNdsVideoTransitionBlackout)
+    {
+        sNdsVideoTransitionBlackout = next;
+        sNdsVideoBrightnessDirty = TRUE;
+    }
 }
 
 sb32 ndsVideoGetBlackout(void)
@@ -111,7 +123,8 @@
 #ifdef ARM9
     {
         const u16 value = ndsVideoResolveBrightnessValue(
-            (u32)sNdsVideoBlackout, sNdsVideoSourceFadeLevel);
+            (u32)(sNdsVideoBlackout || sNdsVideoTransitionBlackout),
+            sNdsVideoSourceFadeLevel);
 
         REG_MASTER_BRIGHT = value;
         REG_MASTER_BRIGHT_SUB = value;

--- a/src/port/nds_scene_manager.c
+++ b/src/port/nds_scene_manager.c
@@ -1,6 +1,7 @@
 /* P2-1b -- the port-owned scene seam. Contract and reasoning: include/nds/nds_scene_manager.h. */
 
 #include <nds/nds_os.h>
+#include <nds/nds_platform.h>
 #include <nds/nds_scene_manager.h>
 #include <nds/nds_frontend_overlay.h>
 #include <nds/nds_particle_runtime.h>
@@ -377,6 +378,8 @@
     u32 freed = (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
                       (uintptr_t)gSYTaskmanGeneralHeap.ptr);
 
+    ndsPlatformBeginSceneTransition();
+
 #if NDS_P2_STAGE_HYRULE && NDS_RENDERER_HW_TRIANGLES
     /* Taskman finished the frame/VBlank bracket before returning here. Drain
      * renderer-owned DMA before deleting the stage's names/palette references. */

--- a/src/nds/nds_platform.c
+++ b/src/nds/nds_platform.c
@@ -1459,7 +1459,10 @@
      * layer hidden, which is also the safer failure. */
     if (is_enabled != FALSE)
     {
-        s3dLayerEnableOnNextPresent = TRUE;
+        if ((REG_DISPCNT & DISPLAY_BG0_ACTIVE) == 0u)
+        {
+            s3dLayerEnableOnNextPresent = TRUE;
+        }
         return;
     }
     s3dLayerEnableOnNextPresent = FALSE;
@@ -1467,6 +1470,28 @@
 #else
     (void)is_enabled;
 #endif
+}
+
+void ndsPlatformBeginSceneTransition(void)
+{
+    ndsVideoSetTransitionBlackout(TRUE);
+#if NDS_RENDERER_HW_TRIANGLES
+    /* The last scene frame has already swapped; its build list is empty.
+     * Drain asynchronous submission before retiring that retained GX list. */
+    ndsRendererFighterPacketDmaWait();
+#if NDS_TASK29_GX_CENSUS
+    ndsRendererTask29GXRecordFlush(GL_TRANS_MANUALSORT);
+#endif
+    glFlush(GL_TRANS_MANUALSORT);
+#endif
+    /* A scene-boundary wait, with no extra logic tick or gameplay present. */
+    swiWaitForVBlank();
+    ndsVideoBlackoutCommit();
+    ndsPlatformSet3DLayerEnabled(FALSE);
+    ndsPlatformClearOriginalSpriteOverlayLayer(FALSE);
+    /* Keep the existing bank-D loan guard; a lent bank is not an overlay. */
+    ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
+    ndsPlatformClearBattleTextHud();
 }
 
 #if NDS_RENDERER_HW_TRIANGLES
@@ -3796,14 +3821,6 @@
         if (submitted != 0u)
         {
             gNdsHardwareRendererFlushCount++;
-            /* Commit a deferred 3D-layer enable only now, with this scene's
-             * own frame completed: the retained-image hazard this guards is
-             * documented at s3dLayerEnableOnNextPresent. */
-            if (s3dLayerEnableOnNextPresent != FALSE)
-            {
-                REG_DISPCNT |= DISPLAY_BG0_ACTIVE;
-                s3dLayerEnableOnNextPresent = FALSE;
-            }
         }
         else
         {
@@ -3838,6 +3855,15 @@
     gNdsRendererProfileVBlankWaitTicks = cpuGetTiming() - profile_start;
     profile_start = cpuGetTiming();
 #endif
+    if ((submitted != 0u) && (s3dLayerEnableOnNextPresent != FALSE))
+    {
+        /* The swap retrace starts rasterizing the new GX list. Keep BG0
+         * hidden until the following retrace can display that completed list.
+         * Only a hidden -> visible ownership change pays this wait. */
+        swiWaitForVBlank();
+        REG_DISPCNT |= DISPLAY_BG0_ACTIVE;
+        s3dLayerEnableOnNextPresent = FALSE;
+    }
     /* P2-2p8 Phase 1 slice 2b: a battle's pending bank D return lands here,
      * after the VBlank that made the next scene's first 3D frame the displayed
      * one -- from now on no displayed geometry reads D. */
@@ -3879,7 +3905,6 @@
      * Covers 3D, both staging layers, and fade-only frames with no staging
      * commit; blackout precedence resolves inside the sole register owner. */
     ndsLBFadePushHardwareFrame();
-    ndsVideoBlackoutCommit();
 #if NDS_SCENE_MIP_CACHE_LAB
     ndsPlatformSceneWallpaperCommitAffine();
 #endif
@@ -3903,6 +3928,8 @@
         ndsPlatformSceneWallpaperCommitAffine();
     }
 #endif
+    /* Release brightness only after this frame's BG/OAM owners committed. */
+    ndsVideoBlackoutCommit();
     sTicks++;
 #if NDS_RENDERER_PROFILE_LEVEL >= 1
     gNdsRendererProfilePostVBlankTicks +=

--- a/src/nds/nds_menu_shell_router.c
+++ b/src/nds/nds_menu_shell_router.c
@@ -4,6 +4,7 @@
  * the Option screen. It lives in its own fragment like every other screen;
  * including it here rather than in the nds_menu_shell.c aggregator keeps the
  * one-TU build while this screen's own TU boundary is still the router. */
+#include <sys/video.h>
 #include "nds_menu_shell_data.c"
 #include "nds_menu_shell_characters.c"
 #include "nds_menu_shell_vsrecord.c"
@@ -455,6 +456,7 @@
 
         ndsPlatformRenderDebugHud();
         ndsMenuShellRecordFrame();
+        ndsVideoSetTransitionBlackout(FALSE);
         ndsPlatformEndFrame();
         ndsMenuShellRecordPresent();
     }

--- a/src/port/taskman_seam_harness.c
+++ b/src/port/taskman_seam_harness.c
@@ -156,6 +156,7 @@
              * redundant main-loop present, so removing that present would
              * otherwise take the HUD with it. */
             ndsPlatformRenderDebugHud();
+            ndsVideoSetTransitionBlackout(FALSE);
             ndsPlatformEndFrame();
         }
     }

--- a/src/port/taskman_seam_battle_host.c
+++ b/src/port/taskman_seam_battle_host.c
@@ -1,5 +1,6 @@
 #include <nds/nds_native_wallpaper.h>
 #include <nds/nds_r2_hwmath_unit.h>
+#include <sys/video.h>
 
 /* Effect-instance pool free count (efmanager.c:1720), sampled per presented
  * frame for the NDS_R2_EFFECT_POOL low-water. See include/nds/nds_effects.h. */
@@ -775,6 +776,7 @@
     (NDS_RENDERER_PROFILE_LEVEL >= 1)
     gNdsRendererProfileHudTicks = cpuGetTiming() - hud_start;
 #endif
+    ndsVideoSetTransitionBlackout(FALSE);
     ndsPlatformEndFrame();
 #if NDS_RENDERER_PROFILE_LEVEL >= 1
     phase_start = cpuGetTiming();

--- a/src/port/taskman_seam_lifecycle.c
+++ b/src/port/taskman_seam_lifecycle.c
@@ -48,6 +48,7 @@
     (void)ndsPlatformReadInput();
     ndsPlatformBeginFrame();
     ndsPlatformRenderDebugHud();
+    ndsVideoSetTransitionBlackout(FALSE);
     ndsPlatformEndFrame();
     gNdsFrameCounter++;
     gNdsOpeningMoviePresentFrameCount++;

--- a/src/import/battleship_mnvsresults.c
+++ b/src/import/battleship_mnvsresults.c
@@ -765,6 +765,7 @@
      * source republishes it on the new emblem's first update. */
     sNdsVSResultsEmblemGObj = NULL;
     gNdsVSResultsStartCount++;
+    ndsPlatformSet3DLayerEnabled(TRUE);
     ndsResultsOamEnter();
     ndsBaseMNVSResultsStartScene();
 }
```

The draft's empty flush is a boundary retirement, not positive rendering engagement. Confirm the previous EndFrame consumed the old build list; keep packet DMA drained and the bank-D guard intact. Current Results photo storage is the existing uniform alias, so this does not implement the required real source photo. Do not label the whole transition source-faithful or close that content obligation merely because stale 3D is gone.

## 6. Integrator verification and remaining gates

1. **Adapt the diff to the actual integration head**, preserving audio prefetch/settle edits and the Results pacing investigator's cadence change. Review all four scene-owned presentation release sites and any additional producer introduced since r58. Do not release in `main.c` fallback EndFrame or at syVideoInit/NOBLACKOUT. Qualify one frozen natural-input native candidate; do not publish an investigation/lab ROM.
2. **Capture the same full boundary loop on slot2** using the extended driver. It now accepts both `-RomPath '<candidate.nds>' -ElfPath '<candidate.elf>'`, makes tag-specific frozen copies and preserves the r58 baseline. Start detached with a fresh tag and redirected logs; require EXIT 0 and exact TOML restoration. No register layer masks in the candidate verification. Check the new ELF's `ndsPlatformVBlankInterrupt+8` and `keysHeld+4` instruction anchors before running.
3. **Required changed captures:** all old/partial loading pictures in the frame tables become a clean black loading cover on both screens until the first complete destination frame. No Castle stage in early Results or over its tic-80 wallpaper; no large Results fighters/confetti during CSS loads or over CSS portraits/panels; no two CSS fighters in the first Castle battle scanout; no partial SSS/CSS/wallpaper VRAM rows exposed. Main-loop fallback presents must keep master brightness `0x8010`, even after source NOBLACKOUT setup. When source fade is nonzero, releasing the cover must reveal its exact current level, not forced brightness zero.
4. **Required unchanged output/state:** entire active match input/logic/digest, one-minute clock, Time Up hold/interface behavior, roster/stage/rankings/scores, CSS's 30-tic proceed wait, Results gates 410/370/200, reveal tics 80/120 (No Contest 1), source tint alpha progression, 12-tic battle/Sudden Death lbFade, audio cue order, final Results layout/models/text at equal source tics, and complete CSS/SSS selections. Captures at Results 122/130 and later settled CSS must retain all required content. Missing photo/emblem or failed live previews remain content failures, not a transition PASS.
5. **Sibling/lifecycle proof still owed:** Stock/Game Set; No Contest (distinct tic-1 reveal); Sudden Death (same scene kind re-entry); a second match→Results→CSS loop; CSS Back and SSS Back/cancel; non-VS Title/ending recovery because the latch is general. Check native resources/DMA/GX errors, bank-D returns/refused writes, OAM tenant ownership and no texture corruption. The one-minute Castle FFA baseline cannot stand for other roster-stage cases.
6. **Run the widest relevant integrated profile once: Latest** for this shared startup/platform change per `docs/VERIFYING.md`; reuse eligible existing checks instead of stacking Boundary/Latest. In a separate uninterrupted interval with other emulators/host-heavy work stopped, report ticks, P50/P95, FPS, engagement, all-present VBlank histogram and artifact identity. Confirm no ownership wait recurs on ordinary active frames. Short probes and a zero old-polygon counter do not prove performance acceptance or required output. No routine retail/hardware test is a prerequisite.

**What is delivered:** causes measured and isolated; four complete baseline boundary clips; every-frame classification and hashes; source contracts; reviewable unbuilt fix; exact before/after checks. **Not delivered/claimed:** applied candidate, build, natural candidate runtime, performance gate, all roster-stage/lifecycle coverage, source photo restoration, owner visual acceptance or bug closure. Investigator began 15:11 CDT, used four emulator runs, stayed inside the 2.5-hour/approximately-12-run box, preserved tracked files and P1, and restored slot2 after every run.

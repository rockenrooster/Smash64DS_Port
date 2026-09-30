# CSS selection intros and Pikachu/Ness contortion — investigation

Investigator: Codex. Started 2026-09-30 14:00 UTC.
Status: DIAGNOSED — FIX PROPOSED, NOT APPLIED / NOT ACCEPTED.

**r55 regression confirmed for both Pikachu and Ness.** The CSS Selected loader
copies the correct joint table, then returns without replacing the preceding
zero-copy clip's authoritative binding or releasing its pin. The fighter plays
its preceding submotion-0 clip under its new Selected status. The owning-seam
debugger correction restores both Selected animations and final poses with all
optimizations on; the unified diff below is for the integrator to apply.
Scope: read-only investigation; integrator owns implementation/build/verification.
No tracked files are changed. Scratch and evidence are restricted to this directory
and `builds/codex-css-intro-contortion*`. Emulator: accurate melonDS slot 2,
GDB 3320, interpreter/JIT off. No other slots or builds are authorized here.

## Owner report and fixed inputs

- r55: `builds/p2p8-playtest-r55/smash64ds.nds` (and adjacent ELF),
  owner states built from `1246ddfe5a1` today.
- r54: `builds/remaining-bugs-playtest-r54/smash64ds.nds` (and adjacent ELF),
  accepted by owner 2026-09-22.
- Reported: VS CSS selection intros end early; selected Pikachu and Ness have
  incorrect joint poses. Owner suspects a new regression.
- Ignore the integrator's uncommitted `src/port/nds_p2_hurtbox_reject.c` edit;
  it is absent from r55.
- `.codegraph/` is present; use CodeGraph before locating/reading code.

## Reproduction and regression status

**Pikachu contortion reproduced and is a regression.** r55's selected Pikachu
at CSS tic 270 is visibly distorted; the matched r54 selected pose is normal.
Both report identical selected status (65537), motion ID (1), and cumulative
P0 triangle count (33550). Ness also reproduces as a regression in the extended
trace/capture comparison. The source Selected clips are never consumed on the
failing r55 arm; nominal status and advancing clocks mask the stale binding.
Runs 07/08 complete the matched 120-frame selection and final-pose coverage.
See `r55-baseline-tic-270.png` versus `r54-baseline-tic-270.png` in this directory.
Ness comparison: `r55-trace-ness-440.png` versus `r54-trace-ness-440.png`.

The intro symptom is a wrong clip, not a measured shortening of the correct
clip's duration: the failing arm never binds the source Selected scripts. The
rotation/status transition still runs, and the animation clock continues, which
can appear to be a selection intro that stops or changes pose too soon.

### Exact reproduction recipe

Use the frozen ROM/ELF pairs above, accurate slot-2 executable identity below,
ARM9 GDB 3320, interpreter from boot (`[JIT] Enable=false`), no fast logic.
The scratch runner copies custom input ROMs before boot so original save/ROM
directories stay untouched. The natural button path is Title -> Mode Select ->
VS -> CSS; only `keysHeld`'s return register is used to supply pad input.

| Scene | Source update tics / pad input |
| --- | --- |
| Title | START at 140–141 |
| Mode Select | DOWN at 12; A at 24–25 |
| VS | A at 24–25 |
| CSS, first three selections | UP 1–30; RIGHT 31–39; A 40; RIGHT 42–61; A 62 (Link); A 110; LEFT 111–130; DOWN 131–141; A 142 (Yoshi); A 190; RIGHT 191–220; A 221 (Pikachu). |
| CSS, short Ness recipe (runs 05/06) | Wait through 320; A 321; LEFT 322–361; A 362 (Ness); wait through 500. |
| CSS, full duration recipe (runs 07/08) | Wait through 460; A 461; LEFT 462–501; A 502 (Ness); wait through 700. |

Pikachu's cursor is `(196,110)`, Ness's `(36,110)`. Cold loading and rotation
settling are left intact: measured Selected script attachment is tic 243 for
Pikachu, 385 for short Ness, 525 for full duration Ness. Do not confuse the
puck drop with attachment. No script directly changes fighter kind or status.

To repeat the original failing trace after this investigator has released slot 2:

```powershell
Start-Process pwsh -ArgumentList '-NoProfile','-File',
  'builds\codex-css-intro-contortion\run-css.ps1',
  '-Rom','r55','-Tag','repeat-r55',
  '-Feed','css-trace-feed.gdb' -WindowStyle Hidden
```

Do not pass `-Repair`. Reproduce r54 with `-Rom r54` and
`-Feed css-r54-trace-feed.gdb`, with a fresh tag. Poll only the tag's `.status`
last line, no more often than once every two minutes. A completed `.status`
plus `CSS_DONE` and the complete captures/parsed JSON establish completion.
All raw transcripts remain under scratch; structured state evidence is here.

## Evidence ledger

| Run / evidence | ROM / A-B state | Recipe / result |
| --- | --- | --- |
| Environment, 14:00 UTC | PowerShell 7.6.6 | Repository root confirmed; `.codegraph/` exists. |
| Fixed inputs | r55 NDS SHA256 `AB60DFA8ABF50E04BECFBF4FCA2F08FC6A14B2B6064C0C9F15DEFEB7C9162CE8`; ELF `5E06F75C5FDF5060137AEFD47851FE782BCD3B09517B997545C2D9B070AEEB57` | Original ROM/ELF retained. |
| Fixed inputs | r54 NDS SHA256 `C8FC02AA2DF0BB6E84C2E0AF4EF2BF6F8C731A95EA1E7D8F129BABFC8B863121`; ELF `F5BBD5F8077CD62FAAADB62B2E0A330FBCB9F1405AFC84926BE5EE5B98170BDB` | Original ROM/ELF retained. |
| Probe preparation | scratch `builds/codex-css-intro-contortion/run-css.ps1` | Copied original probe/feed and capture helper; adapted all writes to permitted roots; executable remains slot 2, private config/current directory and copied ROM/storage under scratch. JIT=false; ARM9 3320. |
| Infrastructure | winget-linked `rg.exe` unavailable; CIM process query denied | Used PowerShell Select-String/Get-Process; CodeGraph shell works. Other slots active: behavioral diagnostics only, no timing/visual acceptance claim. |
| Run 01: r55-baseline, EXIT 0 | Defaults | Original natural key feed, Title -> VS -> CSS -> Link/Yoshi/Pikachu, captures at CSS tics 104/185/270. Pikachu visibly contorted at tic 270; selected=1, status=65537 (demo Win1), motion=1, cumulative P0 triangles=33550. Native output engaged. |
| Infrastructure correction | slot 2 binary | melonDS chooses config beside its executable despite scratch working directory; the initial run automatically updated untracked slot 2 config. Hard-link creation failed (`request is not supported`); initial run-02 launch therefore failed before emulation. Further runs use an exact copy of the slot 2 executable in permitted scratch (SHA256 `DE80E46BDCF1FD986162DE6AFFD9EE1148F8C40565DBAF667B9C2B3EF5475715`, same 3320/3321 ports, private copy of its interpreter config), so config/save/storage writes stay in scratch. No other numbered slot is used. |
| Run 02: r54-baseline, EXIT 0 | r54 defaults | Same feed and capture tics; private interpreter config/storage. Pikachu pose normal. Matching status/motion/triangle counts at all three samples; stale runner config is not reused for writes. |
| Run 03: r55-owner-four-off, EXIT 0, INVALID A/B | Intended four owner words off, NOT ENGAGED | No arm readback appeared: duplicate Title breakpoints shared an address and the first command list continued before the second ran. Capture remains contorted but does NOT refute those controls. Harness corrected to inject writes inside the original Title command list; no ROM conclusion from this A/B. |
| Run 04: r55-zero-copy-off, EXIT 0 | `gNdsR2AnimZeroCopy=0`, readback=0 | Corrected Title-entry word write. Pikachu pose restored and visually matches r54; status/motion/triangle counts still match. Other r55 optimizations remain at defaults. The regression is dependent on zero-copy, introduced by `c7f4fc13f49` (2026-09-28). |
| Run 05: r55-trace, EXIT 0 | r55 defaults, all five relevant words=1 | Extended natural feed through Pikachu and Ness; no state intervention. Exact stale mappings captured at Selected fetch (Pikachu tic 243; Ness tic 385). Selected tables are copied correctly but `fp->figatree` remains the old pinned payload. Ness visibly contorted at tics 400/440. Zero-copy hits=16, rescues=0, drops=14; directreads=0, CLOCK skips=0, prefetch issued=0, fighter-load fallback=0. See `r55-trace-state.json`. |
| Run 06: r54-trace, EXIT 0 | r54 defaults | Same extended natural feed and snapshot/capture anchors. Ness pose normal; both selected fighters have `fp->figatree == fp->figatree_heap` and the first pointer in each equals the expected Selected script. GObj animation clocks match r55 at the same tics; advancing clocks alone conceal the wrong binding. See `r54-trace-state.json`. |
| Run 07: r55-seam-repair, EXIT 0 | all optimizations ON; scoped debugger seam correction | At each successful CSS Selected fetch, wrote only matching force-result `file=returned heap`, generation, and prior pin state=FREE. Both selected fighters now bind their correct heap/script; both poses normal in captures. Pikachu clock reaches 116 at CSS tic 360 then 0 at tic 364 after the 120-frame source end, and remains 0 through tic 460; Ness selection starts tic 525 and is held through tic 700. All five A/B words remain 1; hits=16, rescues=0, drops=10, directreads/skips/issued/fallback=0. See `r55-seam-repair-state.json`. |
| Run 08: r54-long-trace, EXIT 0 | r54 defaults | Same full-duration feed as run 07, no debugger repairs. Normal poses and matching 120-frame endpoints. All 99 Selected-state samples match repaired r55 in kind, selected state, cursor, status, motion and GObj clock; both arms bind their own correct heap/Selected table. See `comparison.json`. |

## Root cause

**Root cause proved by live pointer trace and same-ROM seam correction.**
`lbRelocGetForceExternHeapFile` returns from the compiled CSS Selected branch
(`src/port/reloc_backend_assets.c:17033-17037`) before dropping any prior zero-copy
pin or updating the authoritative heap-to-clip record. The preceding demo-null
clip replaces the freshly copied Selected joint table at attachment in
`src/port/reloc_backend_compat_shims.c:9050` and is stored into `fp->figatree` by
`src/import/battleship_ftmain.c:408`. All citations refer to r55's committed
`1246ddfe5a1` source, saved as read-only snapshots in scratch; they do not rely
on concurrent integrator edits.

The required source chain is:

1. `decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayersvs.c:1553` maps
   Pikachu to demo Win1 and Ness to Win2. Its `mnPlayersVSFighterProcUpdate`
   at `:1582` installs that status once rotation settles (`:1593`).
2. `decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatapikachu.c:66`
   names Idle for submotion 0; `:67` names Selected for submotion 1. Ness's
   `scsubsysdataness.c:91` names EggLay for row 0 and `:93` Selected for row 2.
3. `decomp/BattleShip-main/decomp/src/ft/ftmain.c:4623` force-loads the chosen
   file, discards its return and assigns `fp->figatree_heap` to `fp->figatree`
   at `:4624`; `:4704` attaches that table. The port's authoritative-result
   resolver exists to support this unchanged source behavior.
4. `src/import/battleship_scsubsysdata_ft.c:248` selects the resident source
   tables (Pikachu file 476 / Ness file 437), copies their native pointer array
   into the heap at `:341`, and returns it. This branch neither streams nor
   evicts a clip. Its NULL-heap preparation query returns the source table.
5. `src/port/reloc_backend_assets.c:17033` calls that loader and the early
   return at `:17036` omits the normal force path's result publication at
   `:17102`. `ndsRelocResolveAuthoritativeForceFile` at `:16956` subsequently
   replaces the heap with the earlier same-generation record's file pointer.
6. Zero-copy `c7f4fc13f49` made the preceding record point away from the heap:
   cache clip pinning (`ndsR2AnimPinTake`, `:14112`) returns a resident raw
   payload, while normal subsequent force loads drop the old heap pin at
   `:16570`. The CSS Selected early path missed that lifetime and binding seam.
   r54 uses a copied prior clip (`file == heap`), so copying Selected into that
   same heap happened to satisfy the stale record. This is a latent early-return
   defect exposed by the zero-copy optimization, not an animation-data change.

Run 05 confirms the actual failure (decimal pointers are also preserved in JSON):

| Selection | Fresh selected heap | Stale force result / live `fp->figatree` | Old pin asset / size | Correct heap first joint |
| --- | --- | --- | --- | --- |
| Pikachu, tic 243 | `0x023aee40` | `0x023a7320` | 1957 / 6448 B, BOUND | `0x021867b0`, `dFTPikachuAnimSelected_joint1` |
| Ness, tic 385 | `0x023aee40` | `0x023a8c50` | 1664 / 3632 B, BOUND | `0x02183f38`, `dFTNessAnimSelected_joint1` |

The stale clips are `FTPikachuAnim000` (asset `0x7a5`, token
`llFTPikachuAnimIdleFileID`) and `FTNessAnim000` (asset `0x680`, token
`llFTNessAnimEggLayFileID`). These are the preceding submotion-0 clips, **not**
the Selected clips (source files 476 Pikachu / 437 Ness).
Both stale payloads begin with offset `0x68` (stream format), while the Selected
heap begins with the correct compiled native script pointer. At Pikachu tics
280/320 and Ness tics 400/440/500, `fp->figatree` still equals the stale force
result, despite Selected demo status/motion being installed. This is the first
measured divergence. No eviction or wrong storage asset is needed: no direct
reads, CLOCK skips or prefetch issues occur in this run. The zero-copy pin is
still BOUND at the Selected early return, and no rescue occurs during the run.

Run 07 performs the proposed two state changes immediately after the Selected
copy, at `0x02094ae0` (`lbRelocGetForceExternHeapFile+0x28`, before r55's
early return): record `file=heap` and release the prior matching pin. It uses
registers `r0/r5` for the returned pointer/heap and whole aligned words for
updates, never `gdb call`. Correct binding is independently observed many
subsequent frames later, avoiding the debugger's just-written-line read trap.
All five words remain 1. The correction also handles the initial Mario/Fox
selections and subsequent Link/Yoshi selections in this feed.

Matched repaired-r55/r54 endpoints (`comparison.json`): Pikachu's clock is 116
at tic 360, then finishes to 0 at the next four-tic sample 364 and stays 0
through 460; Ness is 118 at 644, finishes by 648 and stays 0 through 700.
The source root scripts specify 120 frames
(`decomp/BattleShip-main/decomp/src/relocData/476_FTPikachuAnimSelected.c:60`,
block durations sum to 120; `437_FTNessAnimSelected.c:61`, explicit 120 block).
The 99 selected samples have zero comparison errors in the observed fields.

Visual scope: six P0 preview crops, rectangle `(38,235,122,300)` in 416x664
window captures, differ by 8/0/6/58/4/23 of 5,460 pixels at Pikachu tics
240/280/440 and Ness 540/580/680 respectively. Poses are visibly restored;
cross-build pixels are not universally identical. The small residual pixel
differences are unclassified and exact visual acceptance remains owed. P1 and
HUD are outside that crop and this pixel comparison's scope.

## Minimal proposed fix (not applied)

Candidate owning seam (NOT APPLIED; pointer divergence and scoped debugger repair
proved): when the CSS Selected loader succeeds, drop the prior
zero-copy pin for this heap and record the new authoritative force result before
the early return. Preserve zero-copy for normal cache clips.

```diff
--- a/src/port/reloc_backend_assets.c
+++ b/src/port/reloc_backend_assets.c
@@ -17033,5 +17033,12 @@ void *lbRelocGetForceExternHeapFile(const void *file_id, void *heap)
     file = ndsBattleShipLoadCSSSelectedFigatree(file_id, heap);
     if (file != NULL)
     {
+#if NDS_R2_ANIM_ZERO_COPY
+        /* Selected replaces the previous clip just like a regular force load. */
+        ndsR2AnimPinsDropHeap(heap);
+#endif
+#if NDS_R2_BATTLEPACK
+        ndsRelocRecordAuthoritativeForceFile(heap, file);
+#endif
         return file;
     }
```

This clears the obsolete lifetime claim and replaces the authoritative binding
at its producer. Correcting only rendering, global disablement, or a special pose
offset would leave the wrong animation active. `heap == NULL` remains a read-only
residency query (`ndsRelocRecordAuthoritativeForceFile` ignores NULL heaps).

## Integrator verification

1. Apply the above diff at the force-loader seam, keeping production zero-copy,
   direct-read, CLOCK, prefetch and event-32 behavior on. The diff's hunk counts
   and unique context were checked against the r55 snapshot, in memory;
   `comparison.json` records 5 old / 12 new lines and one context occurrence.
   No tracked code has been edited and no rebuild has been run by this worker.
2. Build/freeze the integrated candidate under the integrator's normal queue
   rules. Use the scratch runner with its **symbol-only reference feed**
   `css-r54-long-trace-feed.gdb`, which contains no r55 fixed-address hook,
   no seam repair and no A/B writes. Example detached invocation (use actual
   lab ROM/ELF paths; custom-ROM parameters were parsed, not runtime-tested):

   ```powershell
   Start-Process pwsh -ArgumentList '-NoProfile','-File',
     'builds\codex-css-intro-contortion\run-css.ps1',
     '-Tag','integrated-natural',
     '-RomPath','<candidate.nds>','-ElfPath','<matching.elf>',
     '-Feed','css-r54-long-trace-feed.gdb' -WindowStyle Hidden
   ```

   This uses the released slot-2 ports and private scratch configuration; adapt
   a scratch copy if using an integrator-owned slot. Do not reuse the hardcoded
   `0x02094ae0` hook against another ELF and do not use `-Repair` as fix proof.
3. Require Pikachu and Ness to bind their heap's compiled Selected script,
   remain Selected through all 120 source frames, reach the source endpoint
   and keep normal final poses. Compare the candidate's guest-anchored captures
   with r54's retained series, and resolve residual pixel differences before
   exact acceptance. Read pointer/clip binding as well as status/clock.
4. Cover the shared seam's siblings: all 12 CSS selection intros, repeated
   deselect/reselect, costume changes, four simultaneous preview slots, scene
   exit/reentry and Results -> CSS. Check ordinary zero-copy battle transitions
   and any pin rescue path after a CSS Selected binding; the old pin must no
   longer claim or later overwrite the selected heap. The failed four-word A/B
   is not reused as evidence; its unengaged controls establish nothing.
5. Qualify the frozen integrated frontend/shared-startup batch once with the
   widest relevant **Latest** coverage per `docs/VERIFYING.md`. Final shipping
   native-only ROM, resource/lifecycle checks, isolated tick/FPS/P50/P95 proof,
   accurate-interpreter visual proof and required owner acceptance are owed.
   These concurrent diagnostic runs establish a cause and proposed repair;
   they do not establish performance or publication acceptance.

## Next action / checks owed

Investigation complete in eight emulator runs (one invalid A/B plus one
pre-emulation helper launch failure), within the 2.5-hour/approximately-12-run
box. No owned emulator or build remains running. Original r54/r55 ROM hashes
were rechecked unchanged. Next owner: integrator applies the report diff,
builds and verifies the natural candidate. No acceptance gate is claimed passed
by this investigation beyond the scoped observations above.

Evidence is retained here as PNGs and structured JSON; reproducible scripts,
committed-source snapshots, ROM/storage/executable copies and raw stdout/stderr
are only in `builds/codex-css-intro-contortion*`. The initial melonDS automatic
slot-config write and the helper failures are recorded above. No tracked-file,
Git-index, worktree, published-ROM or decomp edits were made; no subagents used.

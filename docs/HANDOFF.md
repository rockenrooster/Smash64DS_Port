# Handoff

Updated: 2026-07-20 (Task 32 retail packet ready; device decision next)
`P1_EXECUTION_BOARD.md` owns all current state. This is only the restart surface.

## Restart
Branch: `codex/task30-bgm-slices-device-gate`

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

`docs/optimization/ClaudeFable5_Perf_Tasks_20260720.md` is the active queue.
Preserve that untracked user file and `Smash64DS_Port.zip`; neither belongs to
implementation commits.

**NEXT DECISION — Task 32 retail A/B.** The default-off 13-function draw-hot
candidate is 8,168 bytes in profile 1 / 8,060 in profile 0, keeps Task 17 at
5,016 bytes, and passes exact semantic/GX/0-of-49,152 pixel gates. melonDS
regresses M3/DRW about 4.3K ticks and cannot referee instruction-cache locality.
Run `builds/task32-draw-hot-device-pair/README.txt`: control `28CCE187...`, then
candidate `69B0050E...`, and report DHT, DRW latest/mean, 2/3/4/5+, and maximum.
A retail win keeps and promotes the section; flat/worse hardware removes it.
Evidence: `artifacts/performance/2026-07-20_task32-draw-hot-text.md`.
Focused fixtures, DevFast, and Boundary pass with the published default still
off.

Task 30 remains device-gated, but its human listen check passed: Tyler reported
that the sliced profile-1 ROM audio sounds good. It must still show `BGM slices`
engagement on retail and both ROMs need 2/3/4/5+ interval histograms and maximum.
Do not start deferred Task 33
unless Task 30 ships and fresh audio-shell P95 remains above about 40K.

---

**BG-0 is ENABLED in the published target.** The Makefile forces
`NDS_FAST_WALLPAPER_AFFINE := 1` in both the published
`smash64ds-battle-playable-hwtri` block and the freeze-diagnostic block.

**Device evidence:** profile-0 affine-ON ROM `C653C0AD...` boots to GO on both
melonDS and retail hardware with no OOM. The GDB-traced arena degradation was a
melonDS-specific artifact — device heap topology handles the Fox file allocation
without overflow. The prior disengagement (2026-07-19) was reversed based on
this device evidence. The engagement rule in AGENTS.md ("features must prove
retail engagement on hardware") is satisfied.

**BGM dip investigation closed; Task-30 spike smoothing authorized
2026-07-20.** The 5-VBlank dips are structural, not periodic.
Histogram evidence (503/503 samples in 5+ bucket on both BGM-on and BGM-off
ROMs) cleared synchronous BGM I/O as the dip source. The tail lives in
steady-state stage/fighter main-RAM streaming (Task 10 1.50x calibration).

The new 8 KiB sliced-refill candidate passes DevFast, Boundary, the full
one-minute lifecycle, and focused Results audio. Its profile-1 early window
reduces audio-shell P95 268,672 -> 156,160 and caps refill slices at 76,224
ticks with 0/0/0 unsafe/overrun/read failures; frame 607 is 0/49,152 changed
pixels. Retail engagement and a human track-start/loop-seam listen check remain
before KEEP. Packet and checklist:
`artifacts/performance/2026-07-20_task30-bgm-refill-slicing.md`.

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

Task 26 segment 0 is retained and broad generated-program expansion stops. The
fixed program covers 21 DObjs, bindings 0–19, runs 0–25, 54 triangles, and 22
epochs while all live matrices, clipping, material/texture/color/alpha/UV
selection, validation, commit, GX emission, and pre-GX fallback remain current.
The current/generated trace matches all 2,775 words across frames 438–445,
live mutation fails/revalidates before GX at `1/1/1`, five same-control phases
save 3,424–3,616 stage P50 ticks, and synchronized pixels are exactly
`0/49,152`. The current hardware-style melonDS pair saves 10,240/10,368 stage
P50/P95 and 11,232/11,200 draw ticks. The user's one retail A/B saves 21,568
DRW ticks, changes ACT +4,288, and leaves LOOP in the same VBlank bucket. It is
single-sample device evidence only; do not claim repeatability and do not ask
for another retail run.

The same checkpoint closes the post-GO texture fence with an exact 24-key /
23-output / 136,192-prepared-byte corpus including Whispy asset 152 and late
Fox asset 313. It also fixes the KO playback/generation failure by retiring an
exactly completed Calico channel owner before reuse. The refreshed lifecycle
passes exact `439/292/154` KO audio, all ten texture-fence counters, one
teardown, and 166,672-byte net reserve; stable-30 pacing remains red.

Task 23R Phase 1 is closed: keep the compile-gated residual census and **do not
restore its cache**. Five passing windows produced 40/40 complete-key hits and
an 832–896-tick key against an 85.4–86.3K-tick avoided upper bound. The narrow
same-ROM candidate saved about 12K active/draw ticks with exactly 0/49,152
changed pixels, but did not change the VBlank histogram and cannot clear its
working-set-sensitive retail gate. It also lacked the required profile-2
sampled recompute. All candidate code/data/selectors are removed; retained
production footprint is zero. Evidence is
`artifacts/performance/2026-07-18_task23r-phase1.md`.

Task 27 is closed at its expansion gate. Preserve the deterministic Mario
Phase-A certificate (25/14/18/30 events, 320 triangles, 62 immutable effects),
but do not restore its runtime executor: matrix improved about 3.1K while Mario
and complete draw regressed, missing the 8K continuation gate by 8,128 ticks.
Fox was not attempted. Evidence:
`artifacts/performance/2026-07-19_task27-generated-m2.md`.

Its closeout restores the exact already-green Boundary ROM pair: battle
`757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4`, public
`D06323485C866D74BA5D82F87B58182C82A3D7FBE5E9AAC08B83807583171A9E`.
Focused gates, canonical lifecycle smoke, and publication pass. The full
wrapper's only miss is the duplicate unattended-host capture-window failure;
binary identity proves the retained certificate has no production/pixel
effect, so do not spend another capture.

Task 20R is closed without a placement. Preserve its default-off census and
`scripts/check-task20-dtcm-layout.ps1`: the intact 16 KiB stack is statically
too large for the 15,848-byte shared gap, and the plausible 2 KiB matrix
scratch lacks the declined retail A/B. Do not restore either candidate.

Task 22R is closed. Preserve the production complete final key, neutral
span/store census, and exact oracle. Do not restore the threshold-4 writer: it
regresses natural-KO wallpaper P95 by 4,160 ticks. Phase D is ineligible
without a retained CPU writer and retail proof; the user declined repeats.

Task 28 is closed without retained code. Its one bounded signed-32 3x3-matrix
candidate passed the mathematical domain proof, 100,000,000 deterministic host
vectors, and the actual ARMv5TE shape gate: 256 -> 248 leaf bytes, 52 -> 40
stack bytes, and the hot dot changed from one `SMULL` plus two `SMLAL` to one
`MUL` plus two `MLA`. The exact profile-1 control failed twice before sampling
because repo-local melonDS never opened the ARM9 GDB listener. Therefore there
is no natural-owner, literal ARM9, state/pixel, reserve, or retail evidence and
the source/call sites are restored exactly. Do not promote the candidate from
host proof or disassembly and do not request another retail run. Evidence:
`artifacts/performance/2026-07-19_task28-armv5te-matrix.md`.

Task 29 is closed with diagnostic infrastructure only. Its profile-1,
real-GX-only census conserves exact commands, words, owner/reset/flush
boundaries, actual triangles, and semantic hashes across early, Whispy, and
natural-KO windows with zero faults. Paired frame 445 is exactly `0/49,152`
changed pixels and the independent profile-2 oracle passes. Control and alpha
remain unpromoted opportunities because retail proof is mandatory and the user
declined repeats; existing texture/poly shadows already remove safe repeats,
matrix mode has none, and prior color/texcoord suppression regressed. Task-26
segment 0 is not an immutable Task-29 run and misses the 5K actual-run gate.
Do not add a suppression or stream template. Evidence:
`artifacts/performance/2026-07-19_task29-gx-census.md`.
Final profile-0 Boundary ROM/ELF are `21D789F3...` / `89C83C40...`; the ELF
contains zero Task-29 symbols and the public sibling remains `D0632348...`.

Task 24R is complete. Its quiet-slot batch removes 7,929 files /
3,746,285,595 bytes from 14 closed/reverted lab directories and three telemetry
roots. Protected performance/visibility roots are byte-identical across the
deletion, 1,745 immutable migration destinations rehash with zero failures,
and post-delete Boundary passes. All 15 held worktrees, all 24 branches,
current generated A/Bs, hardware pairs, logs, and user-owned files remain.
Recovery snapshot:
`C:\Users\Tyler\Desktop\Snapshots\Smash64DS_Port_Lean_20260719_053053.zip`.
Evidence: `artifacts/performance/2026-07-19_task24r-quiet-slot-cleanup.md`.

The attack/hit audio-visual audit is complete. Preserve all seven
`five-effects-*` branches and the three patch-unique attack/Fox/hit worktrees.
The 128,196-byte / 27-entry pack adds exact Mario down-bounce ID 303 and exact
Fox Smash2/Smash3 IDs 373/374; the deterministic focused window improves from
22 supported + 19 unsupported requests to 26 + 15. It has 2,876 bytes pack
headroom and the focused runtime retains 183,056 bytes. Fighter voices
375/429/431/435/440, nonrepresentable activation programs, and composite hit
cues remain explicitly fail-closed. Do not replace them with approximate or
unrelated samples.

Every one of the 178 Mario/Fox motion-effect calls across 17 source kinds now
has a Boundary-guarded bounded DS route, as do Fox reflector, Fox blaster glow,
and the Mario fireball dust/fire/sparkle paths. This is source-derived P1
presentation, not original common-particle-bank texture/script exactness.
Focused fireball and natural hit routes pass, then final Boundary passes on
the 14,688,256-byte battle ROM `C344CA8...`; its fixed-frame meaningful delta
is 786/49,152 (1.599%). Evidence:
`artifacts/performance/2026-07-19_attack-hit-av-asset-audit.md`.

Task 30 is closed at its entry precondition; **stable-30 is not qualified**.
The strict Task 25R checker reproduces 1,981 intervals of three or more VBlanks
and 2,457 slips at 18.6 presentations/s / 37.3 updates/s; six of seven phase
P95/maximum rows miss the two-VBlank deadline. Task 26 repaired the historical
KO-audio and texture-fence gates, but its exact segment-0 gain does not change
the pacing conclusion and the single retail loop remains near 2.24 million
ticks. No final same-HEAD pair or retail qualification packet was designated,
and no retail repeat was requested. Evidence:
`artifacts/performance/2026-07-19_task30-stable30-precondition.md`.

The current `C344CA8B...` battle ROM is a verifier-covered candidate, not a
stable-30-qualified release. Publish text must state the observed 13.5–15 FPS
heavy-combat range on real hardware and must not turn the exact two-updates-per-
presentation semantic rule into a 30 FPS claim.

Publish Tasks P1 through P4 are complete. P1 audited the exact 906-file build-input
closure and stopped on real decomp divergence. Tyler selected the recommended
closest-base-plus-patch path. P2 commits `369b6ea0b26`, `84dc33dbf49`, and the
current documentation checkpoint add publish-safe O2R/relocData/fighter
regeneration, the pinned five-file source delivery, the one-command
`build.ps1`, and the verified 272-file P3 input allowlist. G1 has zero
differences across 2,159 O2R files,
3,130 relocData files, and 16 derived outputs; G2/G3 reproduce the exact
14,688,256-byte `C344CA8B...` ROM, including a complete idempotent rerun. The
requested DevFast sanity passes.

P3 leaves the one-commit public staging repository at
`D:\Stuff\DevFolder\smash64ds-publish`, branch `master`, HEAD
`0199d8f665da1c319dddd7e66364b651d5f63f27`. Its 272 manifest rows plus three
public templates total 275 files / 8,391,073 bytes outside `.git`; all leak
counters are zero. A real-network clean-room build and final exact-head repeat
both reproduce the 14,688,256-byte `C344CA8B...` ROM. Missing/wrong-ROM cases
exit 1, the external-decomp junction is removed, and the final clone is clean.
Evidence: `docs/publish/PUBLISH_VERIFICATION.md`.

P4 is live at `https://github.com/rockenrooster/Smash64DS_Port`: visibility
remains public; default/only branch `master` points at the one-commit staging
HEAD `0199d8f665da1c319dddd7e66364b651d5f63f27`; its 275-blob remote tree equals
the local staging tree and has zero forbidden paths. The old `main` README is
archived in `docs/publish/PUBLISH_LOG.md` before deletion. The development repo
still has no remote.

Local branches are `master` only. Eleven clean worktrees were removed after all
50 worktree evidence files revalidated through the 1,745-destination Task-24
manifest. Keep `.tura/control-task8-cut-e` and the three dirty detached `%TEMP%`
Task-16 worktrees exactly as listed in the publication log. The complete
recovery bundle is
`D:\Stuff\DevFolder\_backups\smash64ds-full-20260718.bundle`, SHA-256
`435000D2503DE30457BA2A35DA1C9AA230C7BA372835B936E1413B8630B96593`.
No publish-lane task remains; Tyler's file-list/README/NOTICE/license review
checklist is the final handoff.

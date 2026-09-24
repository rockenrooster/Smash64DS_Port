# P2-2p8 Phase 1 slice 7: default lean renderer

Status: **IMPLEMENTED_NOT_ACCEPTED**, committed as `98ebd1e2e51` after resuming
2026-09-24 on `c017e6b450b`. The implementation enables route 1 / admission 2 at boot,
adds Ness's yo-yo and forward-smash programs, and repairs fighter tint lifetime
when bank D returns from battle textures to the menu's BG3.

## Resume identity and reusable proof

No build or emulator job was live at resume. Source changes predate the final
`build-s7r-*-b.log` builds. Current ROM hashes match their successful build
receipts, which also record native-only enforcement:

| Arm | Build directory | SHA-256 |
|---|---|---|
| DK/Samus/Link/Kirby | `builds/build-p2p8-s7` | `50B8AF5804D12C984E09CB276212E297E3CD1944519274E22B45831CE2A509C8` |
| Captain/Pikachu/Purin/Ness | `builds/build-p2p8-s7-cppn` | `F2084476DBE0C8C4DFB459388D44BADAA97680F8EADC9165DE6851296B7DDFCA` |
| Purin/Ness HIGH | `builds/build-p2p8-s7-hi-puns` | `E1897B1B9D6B0D38C9B955C61E711C38B5EFD2D10241E959C8130418C8060A5C` |
| All-content natural-input shell | `builds/build-p2p8-s7-freeplay` | `FEE157D738B07646A1F409C18A30F922912FB18A3EEEA59B395F9EFA3A7921BC` |

The earlier `s7-*.json` and `s7-cppn-*.json` measurements describe older ROMs
(`13D44D67...` / `B62E3883...`), before the final exit fixes. They remain historical
evidence, not final-build timing. New runs use `s7r-*` names; no prior run is
overwritten. The changed linked layout is the timing invalidator.

`results-nowalk-default.log` and `results-nowalk-old.log` both finish
`PROBE_DONE` after natural-input match -> Results -> CSS. The default arm takes
and returns bank D once; native failures, admission failures and tint failures
are zero in both arms. Both report general-heap low-water 121,580 B in this
two-fighter case. The pre-existing arena allocation-failure counter is 199 in
both; this is not claimed as a zero-allocation-failure proof.

Inspected the matching `nowalk-{default,old}-results-t200.png` and
`nowalk-{default,old}-css-p300.png` under
`artifacts/visibility/2026-09-24_p2-2p8-phase1-slice7-results/`: Mario and Fox
retain their tinted body parts in both scenes. This is scoped visual evidence;
owner acceptance and full scene/roster coverage remain due. `css-post.log`
shows 188,368 B at the all-content animation reservation, 5,296 B above its
183,072 B requirement, with engaged native previews.

## Work and checks owed

- Collect final-ROM same-image default/control/oracle stress evidence and Ness
  LOW/HIGH coverage; use existing `tools/run-s7.ps1`, runner 9, sequentially.
- Fixed `check-p2-falcon-efdesc-native.ps1`: it follows the generated root
  indices, verifies source offsets/counts and bounds/distinctness of texture
  slots. It also accepts the entry call's existing NULL-result check. The
  checker passes; in-memory wrong-root, out-of-range-texture and missing-frame
  mutations all fail (`falcon-check-resume.log`). Prior `static-final-py.txt`
  records all four corrected host checks
  passing after the initial log-directory wrapper failure.
- Complete D8 coverage for CSS, Results, autodemo and 1P fighter scenes before
  deleting packet production/replay everywhere. Existing old/new routes remain
  temporary; slice 7 is not the phase's retirement gate.
- Qualify the integrated batch with the widest applicable profile. Root ROM is
  still r54 (`C8FC02AA2DF0BB6E84C2E0AF4EF2BF6F8C731A95EA1E7D8F129BABFC8B863121`);
  do not publish the lab candidate as accepted.
- Shipping's heaviest-roster load remains unproved after the earlier ~130 KB
  shortfall. Report shipping arena/margin separately from the larger lab arena.

Owner inputs preserved: deleted `CLAUDE.md`, deleted old diagnosis document,
and untracked `docs/N64_vs_Nintendo_DS_Hardware_Reference.md` are outside this
candidate. D9 continues to govern phase shortfalls: bank actual gains and
continue, without reopening a re-plan stop.

Completed session **37663**: emulator policy passed (13/13 local runner images
match), default main run exited 0 with 1,972 rows and the expected ROM hash.
The collector reported explained ring-stop seams and two timer-overflow
corrections; retain those warnings when interpreting frame labels.

Completed session **53658**: sequential `tools/run-s7.ps1 -Arm s7r-old
-Route 0 -Admit 0`, then `-Arm s7r-oracle -Route 2`, then `tools/summary7.py s7r`.
Runner 9 / GDB 3423; logs `s7r-{old,oracle}-run.log`, expected `RUN_EXIT=0`,
1,972 samples on the main hash above, zero oracle mismatches. Summary:
`summary-s7r-main.txt`. ROM/ELF/config and generated inputs stay frozen.
Completed job: exec-command session **13155**, sequential final-ROM
`tools/run-s7.ps1` runs `s7r-cppn-{default,oracle}` then
`s7r-hi-puns-{default,oracle}` on the build directories/hashes above. Each arm
must exit 0 with 1,972 rows; oracle mismatch counters must be zero. Each pair
then passes `compare-replay-digest.py --sequence`; final marker
`NESS_FINAL_RUNS_DONE`. Route 2 supplies the same-ROM live reference for these
coverage runs; another disabled-admission timing control is unnecessary.
All four runs exited 0 and printed `NESS_FINAL_RUNS_DONE`; both digest checks
passed. No jobs remain. Next: checkpoint slice 7, then remaining scene coverage.

Owner 2026-09-24: **no subagents**. The one read-only helper was interrupted;
all further work is serial. The hardware-reference brief remains supporting
material for later DS architecture work, not a replacement plan.

## Final main stress result

Sessions 37663 / 53658 completed with exit 0. Same ROM, default route 1/admit 2
versus control route 0/admit 0, 1,972 chronological samples, DK/Samus/Link/Kirby.
Percentiles below use linear interpolation over those samples (the retained
`summary7.py` convention), timer ticks at 33.513982 MHz:

| Bucket | Control P50 / P95 / P99 | Default P50 / P95 / P99 |
|---|---:|---:|
| WORK-H | 1,611,040 / 4,485,354 / 5,138,733 | 1,393,664 / 2,021,027 / 2,665,534 |
| FTR | 349,568 / 2,961,670 / 3,227,467 | 184,448 / 269,651 / 767,704 |
| STG | 324,544 / 368,861 / 375,763 | 324,288 / 368,925 / 374,611 |

Default VBlanks 2/3/4/5+: **209/1388/322/54** over 1,973 presented frames
(10.59% two-VBlank; max 9). Control: 146/953/422/452, max 26. Timer rate divided
by mean ALL gives 19.21 FPS default / 14.42 FPS control. These are combined
lean/admission gains against the disabled control, not a new gain over slice 6.
The 30 FPS gate remains RED. General-heap low-water is 122,412 B in all arms;
libc top minimum improves 11,800 -> 19,320 B. All four kinds draw lean on every
attempt (6,862 total), with no declines, recordings or post-GO fighter texture
uploads. Native failures are **293 -> 39**, direct rejects **254 -> 0**;
the remaining 39 keep native acceptance open. Route 2 compares 6,606 lists /
13,620,081 words with zero mismatches; clip-row maximum difference is 1 LSB.

The frame-key digest comparator reports differences because it keys on inferred
present labels at ring-drain stops. The collector itself records that skew and
rejects stale/wrapped data. **Every one of the 1,972 DGSA/DGSB pairs is identical
at the same recorded ordinal**, in both candidate and oracle, with no skipped
rows or resynchronization. `compare-replay-digest.py --sequence` now expresses
that strict equal-length comparison; the frame-key mode remains unchanged.
`digest-s7r-{default,oracle}.json` retain the passing strict results.
`test_replay_digest_compare.py` passes mutation, gap, no-resync and zero-digest
negative cases. The slice-7 summary no longer calls the permissive historical
look-ahead comparator.

Final shipping static delta is **+104 B**, not the pre-exit-fix -152 B:
`.main` +904 B, `.main.bss` -800 B (`ram-delta-final-shipping.txt`). The matching
final CSS reservation proof remains 188,368 B. Natural Results/CSS crops differ
from control at 76/120,000 and 44/120,000 pixels respectively; the inspected
differences are confined to fighter edges, with all body parts retained. The
comparison crop is (8,54)-(408,354) in the retained 416x664 window captures.

Final Ness LOW default has 1,739/1,739 lean draws, including 158 yo-yo draws;
F-smash is unengaged in that roster. The LOW oracle
completed with zero mismatches, and all 1,972 digest pairs match strictly in
recorded order. General heap low-water is 112,032 B; native failures remain
166, so this is fighter-program coverage, not full native acceptance.

Final HIGH Purin/Ness: Ness draws lean on 1,771/1,771 attempts, including 51
yo-yo and six bat draws. Native failures and packet recordings are zero; general
heap low-water 144,464 B (this two-fighter lab has extra oracle storage, not a
shipping memory figure). Route 2 compares 3,399 lists / 7,769,346 words with
zero mismatches; all 1,972 replay pairs match. LOW's oracle compares 6,515 lists /
12,160,341 words, including 1,633 Ness lists, with zero mismatches.

`check-docs.ps1` and focused diff checks pass. The owner-provided hardware brief
was indexed in the local docs README; both remain outside the slice-7 commit.
Root ROM was rehashed after these tests and still matches r54. No new build or
publication was performed during this qualification.

The follow-up live-Intro experiment was withdrawn on the owner's 09-24
clarification: >=95% four-concurrent VS work, campaign later, static 1P intros.
Its source edits were reverted; the recoverable patch and findings are under
`artifacts/performance/2026-09-24_p2-2p8-phase1-slice8/`. Continue with Phase 2.
Slice 7 is pushed to origin/master; its completed proofs remain reusable.

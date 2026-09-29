# P2-2p8: what a fighter status change costs (2026-09-28)

The 95th-percentile frames carry 1.2-1.6 fighter status changes against 0.2-0.4
in the median band (`2026-09-28_p2-2p8-tail-census`). This receipt prices one
change and banks the first cut.

## Instrument (lab builds only)

`src/import/battleship_ftmain.c`, under `NDS_LAB_FOURCPU_SWEEP && NDS_TICK_HUD
&& !NDS_TICK_HUD_SRC_SPLIT`, times the port's `ftMainSetStatus` wrapper (SCPU,
outer call only) and, only while a setter runs, the calls the imported setter
makes: the motion fetch `lbRelocGetForceExternHeapFile` (SHDT), the figatree
install `lbCommonAddFighterPartsFigatree` (SPRM), the new clip's first play
`ftParamUpdateAnimKeys` + `ftParamsUpdateFighterPartsTransform` (MCAP) and the
part/effect/hit-status resets (MPRO). MCAP/MPRO are the cumulative MISC
counters, so MCAM reads low in these rows. Column SPHC counts status changes
and SPHD motion reads per frame (commit `9800a6a1eca`).

## Price per status change (ticks), full-content lab ROM, R1

Runs `su_*` in `artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/`.
"setter x0" is the WORK-H P95 counterfactual with every setter removed.

| Roster / stage | changes | setter | fetch | install | first play | resets | events + rest | setter x0 P95 |
|---|---|---|---|---|---|---|---|---|
| DK/Samus/Link/Kirby, Dream Land | 722 | 76,479 | 29,341 | 12,423 | 23,574 | 1,142 | 9,999 | -108,864 |
| Fox/Pikachu/Ness/Samus, Saffron | 825 | 79,919 | 33,151 | 13,207 | 23,912 | 522 | 9,127 | -89,152 |
| Luigi/Kirby/Ness/Purin, Yoshi's Island | 734 | 84,033 | 36,873 | 13,284 | 24,244 | 436 | 9,196 | -83,968 |
| Captain/Yoshi/Kirby/DK, Jungle | 713 | 83,563 | 34,579 | 12,670 | 24,365 | 1,639 | 10,310 | -78,464 |
| Kirby x4, Dream Land | 803 | 60,268 | 16,007 | 10,943 | 23,517 | 711 | 9,089 | -59,008 |
| Pikachu x4, Zebes | 764 | 95,665 | 31,051 | 26,742 | 27,624 | 437 | 9,812 | -83,328 |
| Fox/Pikachu/Ness/Samus, Dream Land | 777 | 82,084 | 34,354 | 12,205 | 24,964 | 596 | 9,964 | -96,320 |
| Luigi/Kirby/Ness/Purin, Castle | 670 | 76,356 | 30,257 | 11,582 | 24,543 | 652 | 9,321 | -83,904 |
| Captain/Yoshi/Kirby/DK, Mushroom | 716 | 86,784 | 36,859 | 13,409 | 24,864 | 1,282 | 10,370 | -79,104 |

Cache hits alone (frames with a change and no read): fetch 12-17K on Dream
Land/Kirby x4, a frame with a read ~61K. The first play is the pose engine
parsing and evaluating every joint of the new clip from frame 0 (profile: pose
parse and play ~61K cycles per status-change frame). Counterfactual stack on
Dream Land / Kirby x4: no reads -8K/-5K; plus a 5K hit fetch -26K/-12K; plus
first play halved -50K/-25K; plus install halved -58K/-30K.

Profile split (`profile-split-setstatus-dl.txt`, older DL profile): the hit
fetch is table bookkeeping -- loaded-file table scans (56 B entries, up to 96),
the raw cache's linear find (up to 256 entries), path formatting, status-buffer
alias strips -- plus the clip memcpy (~12K cycles per status-change frame).

## Banked: cheaper cache-hit fetch (`src/port/reloc_backend_assets.c`)

All exact; no gameplay-visible change.
- The raw cache's find checks a per-asset-byte hint first (entries are unique
  per asset, the hint is validated against the entry).
- `ndsRelocPrepareFighterAnimHeapOverwrite` records, during its full scan,
  where the incoming asset's entry is; the registration that follows uses it
  instead of scanning again (epoch- and count-checked, one-shot).
- A clip the raw cache holds skips the path-table existence check, which
  formats the path string for a generated fighter clip.
- The tick-HUD "AnimForceResident" census (a whole-table scan per change) is
  lab-only now; the gate ROM no longer pays the instrument's cost.

| Run | P50 | P95 | P99 | replay |
|---|---|---|---|---|
| Gate `t3r_gate` -> `f1_gate` | 934,592 -> 934,272 | 1,289,280 -> **1,284,864** | 1,461,568 -> 1,464,960 | IDENTICAL |

Lab, nine rosters (`su_*` -> `sf_*`, census still on): replay IDENTICAL on all
nine; hit fetch per change -1.7K to -2.2K (e.g. DL 16,543 -> 14,500); P95
within -2.6K..+4.2K.

## Next

The hit fetch is still 10-26K and the install 11-27K (Pikachu x4 is the
outlier at 26.7K); the first play is ~24K on every roster.

## Refuted: a whole-victim hurtbox bound (A5)

Hurtbox-test frames (13% of frames) carry ~137K ticks over the rest; the reject
kernel is ~56K of it (`split_hb` on `builds/p2p8-prof-census`). Tried: once per
victim per latch epoch, the union of every damage box's per-box interval (same
rounding, largest `sum_abs` and `inv_smin`), keyed on the epoch and a snapshot
of the boxes, so one comparison stands for up to eleven per-box tests. Exact by
construction and by measurement (same ROM, `gNdsP2HurtboxVictimBound` 0/1):
rejects 13,499 = 13,499, passes 654 = 654, shadow-mode flips 0, replay
IDENTICAL; 12,013 of the rejects came from the bound. But WORK-H P95
1,287,616 -> 1,316,736 and P50 +3.8K (runs `vb0_gate`/`vb1_gate`/`vbs_gate`):
the per-box cost is the once-per-epoch world composition, which the bound must
still do for every box; the per-box test after it was already cheap, and the
snapshot compare added to every call. Reverted. Same finding as the 09-27
per-box cache and per-fighter epochs (`2026-09-27_p2-2p8-fast-mem` section 23).

## Lean model rebuild, measured parts (lab, `NDS_FTR_LEAN_PHASE_TICKS=1`)

Per materialization, Dream Land / Saffron / Jungle: 394K / 393K / 302K ticks;
texture resolve + bind 86K / 77K / 36K, spans + materials 71K / 65K / 43K,
corners 58K / 52K / 47K, run prepares 26K, root binds 25K, shade 24K, tint +
readback 20K, the rest outside the list walk ~80K. Keys: Dream Land 39
rebuilds of 23 distinct keys (16 repeats), Saffron 35 of 27. Removing every
rebuild prices at -35K P95 on Dream Land, so a 20% cheaper rebuild is ~-6K.

## Owed: six objects with no native owner (lab rosters)

Six of nine full-content lab rosters record native failures (fail-closed, the
object is not drawn); the gate roster records none. `gNdsNativeFailureLab`
(runs `nf_*`), identity = GObj kind << 16 | asset id:

| Object (asset, root) | Kind | Failures per match |
|---|---|---|
| Ness PK Fire pillar (NessSpecial3 336, root 0x870) | Item | DL 262, Castle 102, Yoshi's Island 36 |
| Goldeen / Tosakinto (ITCommonObject 86, root 0xB618) | Item | Jungle 197 |
| Bob-omb / BombHei (ITCommonObject 86, root 0x34C0) | Item | Zebes 108 |
| Saffron stage weapon (StageYamabukiFile3 159, root 0x28A8) | Weapon | Saffron 76 |
| NessSpecial2 (352, root 0x8E0) | Effect | Castle 12 |
| SamusSpecial2 (349, root 0x930) | Effect | DL 3 |

Each needs an owner in the pattern of `scripts/stages/generate_nds_native_*.py`
+ `src/nds/nds_native_*.exec.inc` + recognition in
`src/port/renderer_adapter_stage.c`. Until then those frames also under-count
draw cost.

## Banked: zero-copy motion cache hits

A stream clip (BPS1: relative offsets, no fixups) that the raw motion cache
holds is handed to the fighter where it lies -- as the resident battlepack
already hands out Mario's and Fox's clips -- instead of being copied into the
fighter's figatree heap and registered there. `src/port/reloc_backend_assets.c`
(`sNdsR2AnimPins`): each fighter's clip is pinned (pending from the fetch until
`lbCommonAddFighterPartsFigatree` binds it, then bound until the fighter's next
fetch). Before the cache reuses a pinned range (ring overwrite, elastic yield,
drop), the pin is rescued: the clip is copied into the fighter's heap and
registered exactly as the copy path does, and every pointer into it moves by
the same delta -- the authoritative force-file record, `fp->figatree` (Kirby's
capture re-binds it) and the joints' script cursors and TraI descriptors
(`ndsFtPoseRelocateScripts` in `src/nds/nds_ft_pose.c`). A fetch drops the
fighter's own pins first; a destroyed fighter's pins are dropped in
`ndsFtPoseRelease`. The pointer resolver answers a pinned clip's offsets.
Same-ROM A/B word `gNdsR2AnimZeroCopy` (default 1).

| Run (same ROM, word 0 -> 1) | P50 | P95 | P99 | zero-copy hits / rescues | replay |
|---|---|---|---|---|---|
| Gate `zc0/zc1_gate` | 936,768 -> 934,208 | 1,291,136 -> **1,282,944** | 1,468,928 -> 1,462,976 | 481 / 2 | IDENTICAL |
| DL lab `zn/zl_def` | 952,832 -> 951,808 | 1,318,208 -> 1,317,120 | | 426 / 4 | IDENTICAL |
| Jungle `zn/zl_cy_g2` | 983,680 -> 982,016 | 1,333,376 -> 1,333,376 | | 353 / 6 | IDENTICAL |
| Yoshi's Island `zn/zl_lk_g5` | 1,097,856 -> 1,091,712 | 1,473,600 -> **1,464,064** | | 429 / 6 | IDENTICAL |
| Saffron `zn/zl_fp_g7` | 1,160,640 -> 1,158,208 | 1,578,368 -> **1,569,984** | | 483 / 3 | IDENTICAL |

All nine lab rosters (`zl_*` against the copy-path build `sf_*`): replay
IDENTICAL, 0-6 rescues a match, 0 drops, 0 misaligned resolves; native failures
unchanged (the six owed owners above).

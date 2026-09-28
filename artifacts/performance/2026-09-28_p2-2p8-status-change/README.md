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

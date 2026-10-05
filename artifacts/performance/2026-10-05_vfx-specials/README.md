# 2026-10-05 Specials' effects: Pikachu's Down B, Ness's Up B

Owner (BUGS.md): "Pikachu: Down B VFX causes P95 slowdown", "Ness: Up B VFX
causes P95 slowdown", "Check all fighter attacks and VFX for P95 slowdowns".

Clean four-CPU lab ROMs (`NDS_LAB_FOURCPU_WORDS=1`), Dream Land, items on,
ring-dump sampler (1,960 presented frames). Forced runs use the lab input knob
(`gNdsLabForceInputSlots` / `gNdsLabForceInput` / `gNdsLabForceInputPeriod`,
`src/import/battleship_ftcomputer.c`): every CPU makes the special together on a
fixed period -- a worst case well beyond natural play.

| Run | ROM | Roster / input | P50 | P95 | over 1.12M |
|---|---|---|---|---|---|
| `pkn-w` | `build-lab-clean1005w` | 4 x Pikachu, natural | 827,648 | 1,166,464 | 143 |
| `pkn-x` | `build-lab-clean1005x` (+ token memo, knob) | 4 x Pikachu, natural | 826,944 | 1,166,784 | 142 |
| `nsn-w` | `build-lab-clean1005w` | 4 x Ness, natural | 801,152 | 1,161,920 | 131 |
| `nsn-x` | `build-lab-clean1005x` | 4 x Ness, natural | 800,128 | 1,160,960 | 130 |
| `pkdb-x` | `build-lab-clean1005x` | 4 x Pikachu, Down B every 120 ticks | 759,872 | 1,621,952 | 379 |
| `nsub-x` | `build-lab-clean1005x` | 4 x Ness, Up B every 200 ticks | 1,012,736 | 1,419,520 | 792 |

Token memo (`src/port/reloc_backend_assets.c`): the asset id for a relocation
token is a pure function of the token, and the chain that answers it walks
several hundred compares; effect makers ask on every effect they create. A
64-line direct-mapped memo: digest identical on both rosters, paired median
-512 (Pikachu) / -320 (Ness).

Forced Thunder's top 5% frames: SRC 810K (median 325K), MISC 1,031K (median
409K). Each Thunder spawns a trail weapon every tick (10-tick life, then a
6-tick trail effect): about a dozen lists a frame per Pikachu, each drawn
through the stage DL body. PK Thunder's ball and trail keep MISC's median at
593K for as long as they fly.

## Thunder and PK Thunder on the fast lane

Both specials' lists drew through the stage DL body on every frame they live:
the entry-model scan, the loaded-file lookup and ~60 candidate tests before the
owner, for each of a dozen quads a frame per Pikachu. Two fast-lane routes
(`src/port/renderer_adapter_stage.c`): `NDS_SDL_ROUTE_PIKACHU_THUNDER` (Thunder's
head and trail weapons, its trail effect and the shock roots; the adapter
re-proves the GObj kind, weapon kind, root and MObj snapshot on every draw) and
`NDS_SDL_ROUTE_NESS_PKTHUNDER` (PK Thunder's head and trail weapons; the weapon
kind, root, trail id and snapshot effects re-proved). An owner that declines
hands the list back to the body. `build-lab-clean1005y` = 1005x + the routes.

| Run | P50 | P95 | P99 | over | paired median vs 1005x | digest |
|---|---|---|---|---|---|---|
| `pkdb-y` (forced Thunder) | 761,984 | 1,433,536 (-188,416) | 1,848,000 (-183,168) | 337 (-42) | +2,624 | identical |
| `nsub-y` (forced PK Thunder) | 957,888 (-54,848) | 1,299,328 (-120,192) | 1,389,632 | 675 (-117) | -60,416 | identical |
| `pkn-y` (natural 4 x Pikachu) | 829,888 | 1,151,552 (-15,232) | 1,450,752 (-67,968) | 129 (-13) | +3,584 | identical |
| `nsn-y` (natural 4 x Ness) | 803,840 | 1,162,752 (+1,792) | 1,406,016 | 129 (-1) | +2,944 | identical |

The natural rosters' +3K paired medians are layout (both, alike; no routed list
runs on most of their frames).

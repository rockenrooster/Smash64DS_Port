# More stage-DL fast-lane routes: Arwing laser, Saffron's Pokemon, damage-fly dust, 2026-10-05

`src/port/renderer_adapter_stage.c`, same-ROM A/B word `gNdsStageDLFastMore`.
A body-submit census (clean sweep ROM, frames 100-1,900; scratchpad
`bodycensus.ps1`) listed the lists still reaching the native owners through the
whole stage-DL body every frame they live. Each now gets a fast-lane route the
body records when its owner draws the list, with the body's own admission
tested again every draw:

- Sector Z's Arwing laser (`NDS_SDL_ROUTE_SECTOR_LASER`): a weapon, no MObj;
  its texture file is found from the pointer the list carries and both
  relocated words are proved against it, as the body proves them.
- Saffron's Marumine, GLucky and Porygon: MObj-less items, ordinary item routes.
- Saffron's Hitokage and Fushigibana (`NDS_SDL_ROUTE_HITOKAGE` /
  `_FUSHIGIBANA`): one CURRENT_IMAGE snapshot taken live.
- The damage-fly dust (`NDS_SDL_ROUTE_DAMAGE_FLY_MDUST`): an effect; the
  effect layer's seeds and othermode witnesses, then the body's own helper
  (which snapshots the MObj under an effect owner), settled either way.

Clean sweep ROM `build-lab-clean1005g`, word 0 (`mr0`) -> 1 (`mr1`), digest
identical on both stages:

| stage | P95 | over 1.12M | affected frames (census) | paired median there |
|---|---|---|---|---:|
| Sector Z | 1,305,856 -> 1,307,136 | 291 -> 274 | laser 93 / dust 49 | -22.6K / -29.6K |
| Saffron | 1,269,312 -> 1,261,504 | 237 -> 231 | Pokemon 199 / dust 32 | -7.9K / -25.6K |

Official gate (`build-gate-1005s`, with the halo and N Bumper changes):
`gate-s1`, 1,112,960 P95, 1,860 two-VBlank presents, digest identical (neutral).

Then Mushroom Kingdom's two Pakkun (file 155 root 0x0B40, drawn through the
body on every frame: one MObj of flags 0x0001 whose CURRENT_IMAGE lies in the
same file, the palette in file 107) and its POW block (0x10D0: no MObj, TLUT
in file 107 found from the list's relocated word, both images in file 155):
`NDS_SDL_ROUTE_INISHIE_PAKKUN` / `_POWBLOCK`, same word. Clean ROM
`build-lab-clean1005h`, `mk0-g8` -> `mk1-g8`: P50 941,760 -> 916,416, P95
1,224,256 -> 1,197,056, over 209 -> 166, paired median -24.6K, digest
identical. The gate ROM does not compile the stage.

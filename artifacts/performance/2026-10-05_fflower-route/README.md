# Fire Flower lists take the stage DL fast lane, 2026-10-05

`src/port/renderer_adapter_stage.c` (word `gNdsStageDLFastFFlower`). The Fire
Flower's two lists (ITCommonObject 0x4520, the branch root with no MObj, and
0x4608, the live root with one palette-image MObj) were drawn by their native
owner but reached it through the whole stage-DL body on every draw: no route
was recorded. The body now records one when the owner draws a list -- the
branch root as an ordinary item route (`nNDSStageDLItemFFlower`), the live root
as `NDS_SDL_ROUTE_FFLOWER_LIVE`, whose admission is the body's (Item GObj of
kind FFlower under the item submit, a single MObj with the owner's flags, a
palette-image snapshot taken live each draw) -- and later draws go straight to
`ndsRendererSubmitNativeItemFFlower` with the body's inputs (persistent stats,
the item head's colours). The item replay never records a flower draw: its key
requires every root to be a replayable item route.

Clean sweep ROM `build-lab-clean1005d`, Congo Jungle (the sweep match has a
flower out for most of the match), same ROM, word 0 (`ff0`) -> 1 (`ff1`):
P50 926,272 -> 901,632, P95 1,310,272 -> 1,277,376, over 293 -> 258, paired
median -27,328, replay digest identical. Official gate (`build-gate-1005r`,
with the cloud changes, which the gate does not compile): 809,216 / 1,111,168,
1,861 two-VBlank presents, digest identical to `gate-q1` (no flower in that
match; neutral).

Also measured here (`szwhy-g1`, lab-only counters `gNdsLabStageGxFastWhy`):
Sector Z's stage segment 0 declines the GX fast commit on every frame for
reason 2, a hidden binding -- the wing-platform collision proxy the owner
ruled undrawn (r58, `ndsGRSectorPlatformDObj`), not the near box.

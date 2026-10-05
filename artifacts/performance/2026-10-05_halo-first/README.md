# Rebirth halo ahead of the body; N Bumper fast-lane route, 2026-10-05

Clean sweep ROM `build-lab-clean1005f` (`NDS_LAB_FOURCPU_WORDS=1`), 1,960
frames, same ROM A/B words; render only, replay digest identical in every
pair.

1. `gNdsStageDLHaloFirst` (`src/port/renderer_adapter_stage.c`,
   `ndsRendererAdapterTryRebirthHalo`). The RebirthHalo arm sat at the top of
   the stage-DL body, so its three lists (0x2378 and 0x2a88 on the child DObj,
   0x27e8 on the rotating grandchild) each paid the entry-effect probe and the
   body's prologue before reaching the native owner. The same statements now
   run from the dispatcher right after the fast lane; the body's copy stands
   down while the word is set. Whole-match paired median 0 (Yoshi's Island
   `hf0-g5` -> `hf1-g5`, Dream Land `hf0-g6` -> `hf1-g6`); on the 121 Yoshi's
   Island frames that draw a halo (body census frames) paired median -5.4K,
   every other frame 0.
2. `NDS_SDL_ROUTE_NBUMPER` (word `gNdsStageDLFastFFlower`, shared with the
   Fire Flower routes). The N Bumper's quad (ITCommonObject 0x7558, the list the
   castle bumper also draws) went through the body on 238 of Sector Z's
   frames; the body now records a route whose admission is its own (item
   submit, Item GObj of kind NBumper, one MObj of the owner's flags, a
   palette-image snapshot taken live). Sector Z `nb0-g1` -> `nb1-g1`: P95
   1,312,000 -> 1,305,920; on the bumper frames paired median -15.9K.

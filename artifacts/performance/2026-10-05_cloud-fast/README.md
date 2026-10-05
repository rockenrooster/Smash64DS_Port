# Yoshi's Island clouds: matrices without the generic builders; corner words kept, 2026-10-05

Clean sweep ROMs (`NDS_LAB_FOURCPU_WORDS=1`), Yoshi's Island (gkind 5),
1,960 frames, render only: the replay digest is identical in every pair.

1. `src/port/renderer_adapter_matrix.c`, `ndsRendererAdapterSubmitNativeYosterCloud`
   (word `gNdsYosterCloudFast`). Every cloud local is a Tra (the root and mids;
   a drawable's kind-48 recalc is skipped by the local build), so each is the
   identity with row 3 = RoundShift20p12(FTOFIX32(translate)), computed from
   the float bits (`ndsRendererAdapterFloatPow2ToS32`), and a drawable's
   chain is the identity with the three translations summed. The kind-48
   recalc keeps only row 3 of chain x camera modelview, formed with
   `ndsRendererMtxMulRow3_20p12`. 7 local builds and 9 full multiplies a cloud
   become 21 integer conversions and 3 row products; the bank-154 view and the
   drawables' file lookup are asked once a cloud. Translations at or past 2^14
   units take the old path. Same ROM (`build-lab-clean1005c`), word 0 (`yc0`)
   -> 1 (`yc1`): P50 952,192 -> 936,576, P95 1,275,136 -> 1,260,096, paired
   median -15,296.
2. `src/nds/nds_native_actor_yoster_cloud.exec.inc` (word
   `gNdsYosterCloudCornerMemo`). The three drawables emit the same shared
   corners; unlit, a corner's words depend only on the material RGB, the use
   flags, the bound tile's scale/origin and the filter offset, so they are kept
   with those inputs and replayed (GFX_COLOR, GFX_TEX_COORD, two GFX_VERTEX16
   writes, in glColor/glTexCoord2t16/glVertex3v16 order). A lit material
   builds them as before. Same ROM (`build-lab-clean1005e`), word 0 (`cm0`)
   -> 1 (`cm1`): P95 1,263,488 -> 1,257,728, paired median -5,312.

Captures (`artifacts/visibility/2026-10-05_cloud-ab`, local only): frames 300
and 1,200 with both words on vs off differ only in the window title's FPS
digits; frame 700's pair caught different presented frames (the window lags
the core), as noted in the capture method.

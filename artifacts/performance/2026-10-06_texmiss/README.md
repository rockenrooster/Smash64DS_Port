# 2026-10-06 Battle texture-miss census (Sector Z)

Question: the Sector Z over-gate profile (`artifacts/task37-census/sz-szprof09`)
put `ndsRendererHardwareResolveOrBindTexture` at +71K cycles a marked frame
(18 of 34), its hot code in the miss path (PAL16 packing, the texel0/texel1
blend loop). Is the battle re-baking textures?

Instrument: `NDS_LAB_TEXMISS=1` (Makefile, LAB ONLY; compiled out otherwise)
records every converted miss at the battle fallback in
`ndsRendererHardwareResolveOrBindTexture` (`src/nds/nds_renderer_textures_effects.c`):
frame, image, TLUT, format|size|flags, width|height, combine words, key hash
and texel1 fraction, 10 words a record, 1,024 records. Dump: gdb at presented
frame 1,950 (`sz-texmiss.bin`, little-endian u32 records).

Result (clean lab ROM, Sector Z, 4 CPUs): 155 conversions in 1,950 frames,
148 of them at setup (frames 0-1). In the match: frames 216 (one 64x64 CI4
with texel1), 225 (two 32x32), 533, 796 and 1,420 -- first uses of textures the
pre-GO manifest did not hold, no re-bakes. Frame 216's one conversion costs
~1.0M ARM9 cycles (614K PAL16 packing, 251K texel blend); the other marked
frames' resolve cost is not conversion. Not a P95 lever (a handful of frames a
match), but a hitch: the PAL16 packer and the manifest's coverage are the
places to look if one is reported.

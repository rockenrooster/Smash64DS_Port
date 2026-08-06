# Sector Z Arwing Spline Descriptor Byteswap — 2026-04-28

## Symptom

Sector Z's patrol Arwing flew through the Great Fox / main stage instead of following its original path around the stage.

## Root Cause

Sector Z's Arwing path is driven by `SetInterp` AObj data. The AObj event points to a `SYInterpDesc`, which in turn points to `Vec3f points`, `f32 keyframes`, and `f32 quartics` tables.

The 2026-04-25 Samus dodge fix adjusted the little-endian `SYInterpDesc` field order so fighter figatree data could read correctly after the figatree-specific u16 halfswap pass:

```
byte 0 = pad
byte 1 = kind
bytes 2..3 = points_num
```

That layout is correct for fighter figatrees, but Sector Z's stage/movie spline descriptors do not go through the figatree halfswap pass. They only get the normal reloc-file pass1 `BSWAP32`, which leaves the first word as:

```
byte 0 = points_num_lo
byte 1 = points_num_hi
byte 2 = pad
byte 3 = kind
```

So non-figatree spline descriptors decoded the wrong `kind` and `points_num`. Pointer tokens were fine; the failure was the packed subword metadata in descriptor word 0. The Arwing then evaluated the wrong spline shape and appeared to pass through the stage.

## Fix

`src/sys/interp.c` now normalizes `SYInterpDesc` headers for both resource classes before any field reads:

- For normal non-figatree descriptors, call `portFixupStructU16(desc, 0, 1)` to rotate descriptor word 0 into the byte order expected by the PORT `SYInterpDesc` struct. The full-width f32 fields and pointed-to f32 arrays are already native after pass1, so they are not touched.
- For fighter figatree descriptors, keep the existing halfswapped first word, but un-halfswap full-width `unk04` and `length` once on first access. The existing gated data-block un-halfswap remains figatree-only.

This preserves the Samus dodge/figatree fix while restoring stage and movie spline descriptors such as Sector Z's Arwing path.

## Files Touched

- `src/sys/interp.c`
- `src/sys/interp.h`


# Handoff

Current: ACTIVE. Native-only contract owns every ROM; 1P stays paused.
Zero native failures met 09-09 on shell ROM `5203F631` (both waves,
1,200 presents, one ROM per summary `rom_sha256`). Zero failures is not
zero owner bugs: Castle roof, Inishie bricks stay open on owner pictures.
Root `smash64ds.nds` NOT rebuilt this session -- no current hash pin.
`master` clean checkout broke 7 ways 09-06..09-09 (fixed as found);
an incremental build proves nothing, so verify a clean checkout before
publishing, and expect the next one to find more.

## Next readings (in order)

- Castle roof reproduces as red EDGES with sky through (witness3 shot).
  Runs emit vertex alpha 0xff, so candidate is POLY_ALPHA 0 (wireframe)
  from `ndsRendererHardwareAlpha`, or zero-area/depth fill loss. Read the
  per-run emitted-vs-declared witness (`emitted_triangles` vs
  `run->triangle_count`), polyfmt/cull bits, submitted winding.
- Yoster floor DRAWS (fighters stand on it); platforms unestablished, not
  missing-floor. Inishie side bricks still missing (pipe floats); earlier
  "grey tower" reading retracted same day. Do not re-probe generation or
  static gates: declared counts pass every gate already.
- Shell loop floor is 1,968 B vs 32,768 minimum: fails on its own
  assertion. Peak is character select (+45,940 B); arena lost 7 pages, so
  ~28 KB libc-heap pressure. Read `gNdsTaskmanArenaChosenSize` /
  `gNdsTaskmanArenaAllocFailCount` on a booted ROM.
- Yoster BGM garble is out of candidates (per-instrument, loop seam,
  MAX_RATIO, headroom, resampler all dead). Needs owner mute/solo listen
  to name the instrument.

## Landed today (acceptance owed, not closed)

Zebes acid TEXEL1 bind + alpha subdivision; shield depth path (stripped
Z bit); Sector Z Arwing matrix case; BGM pitch bends restored (Congo 89,
Saffron 88, Castle 5, Dream Land 3); Saffron Pokemon draw owners;
fighter item-pickup FileIDs (were all `0u`); the Saffron owners' missing
build wiring (includes, Makefile rules, object prerequisites). A further
item-owner batch is in flight, not landed.

## Known state, do not re-derive

Three 09-09 retractions: SUBMITTED table was DECLARED counts, not
emitted; Saffron "silent empty draw" (`DIAG_OWNERTRI` indexed by profile
owner, slot 3 = Luigi absent); item row is 45 kinds in code, ~40 with no
draw owner (22 bake / 13 live-material / 4 branch / 1 unlocated).
Marumine is bake-everything, not live-material. Shield alpha never flat
(11 alpha x 13 intensity levels); symptom is scrambled slices (texel
order), not banding. Roster does not fit: three packs exceed floor 21x
combined; VS blocker is eager closure load, not 1P packs. Cadence:
Yoster 19.9, Inishie 21.7, Saffron 19.6 FPS in 2-fighter matches -- a P2
gate problem, not geometry. Boundary: `p2_shell_loop` (floor red),
`p2_battle_realtime` mode 163 (Mario vs Lv3 Fox, Dream Land, 1-min,
items off), `p2_fourcpu_stress` (WORK-H P95 red, SRC lane 2.47x).

## Rules

CodeGraph first; other docs lookup-only. Bank verbose output, bounded
UTF-8 log reads. One build at a time, never `make` from a writer, no
-j/MAKEFLAGS. Owner symptoms: `docs/BUGS.md`; evidence: board +
`docs/p2/BUG_NOTES.md`.

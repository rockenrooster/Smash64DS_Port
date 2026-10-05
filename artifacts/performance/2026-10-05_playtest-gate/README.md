# 2026-10-05 Official gate after the playtest fixes

`build-gate-1005w`: the committed tree (`build-gate-1005v`, `d394d7bd177`)
plus the playtest fixes -- the Yoshi's Island floor projection
(`src/port/reloc_backend_mp_collision.c`), the costume accessory alias root
(`src/port/renderer_fighter_lean.c`), the unlit GO lettering
(`src/nds/nds_ifcommon_oam.c`), Ness's PSI Magnet toward the eye
(`src/import/battleship_efmanager.c`, `src/port/renderer_adapter_matrix.c`) and
the lab costume / Results podium words. `.itcm` 31,920 B, unchanged.

| Run | P50 | P95 | over | two-VBlank presents | paired median | digest |
|---|---|---|---|---|---|---|
| `gate-w1` vs `../2026-10-05_itcm-pack6/gate-v1` | 816,832 -> 814,208 | 1,120,320 -> 1,117,568 | 96 -> 93 | 1,860 | -2,176 | identical |
| `gate-w2` vs `../2026-10-05_itcm-pack6/gate-v2` | 816,832 -> 814,912 | 1,121,024 -> 1,117,312 | 96 -> 91 | 1,859 | -2,176 | identical |

None of the changes runs on the gate roster's frames except the floor
projection, which answers as before on Dream Land (no moving platform); the
-2K is layout.

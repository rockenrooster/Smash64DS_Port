### Summary

| stage | tree alloc | T1 (high) | T1+T2 (T2 medium) | T1..T4 (all tiers) | tree after T1 |
|---|---:|---:|---:|---:|---:|
| Peach's Castle | 176,880 | 158,144 | 165,888 | 165,888 | 18,736 |
| Sector Z | 226,192 | 158,144 | 171,312 | 171,312 | 68,048 |
| Kongo Jungle | 225,392 | 158,144 | 168,276 | 196,068 | 67,248 |
| Planet Zebes | 219,872 | 158,144 | 166,452 | 166,452 | 61,728 |
| Hyrule Castle | 185,920 | 158,144 | 169,644 | 169,644 | 27,776 |
| Yoshi's Island | 229,280 | 158,144 | 166,548 | 166,548 | 71,136 |
| Dream Land | 202,816 | 158,144 | 167,972 | 182,044 | 44,672 |
| Saffron City | 239,600 | 158,144 | 172,848 | 172,848 | 81,456 |
| Mushroom Kingdom | 192,224 | 158,144 | 166,696 | 166,696 | 34,080 |

### Table 1

| stage (gkind) | map | resident O2R files: id name payload B | tree alloc B | extra preload | blob body | .gxp body |
|---|---|---|---:|---|---:|---:|
| Peach's Castle (0) | 259 | 259 GRCastleMap 192; 106 ExternDataBank106 17,696; 90 MVOpeningRoomWallpaper 158,928; 156 MiscDataBank156 64 | 176,880 | - | 9,680 | 17,760 |
| Sector Z (1) | 262 | 262 GRSectorMap 304; 109 ExternDataBank109 47,120; 99 StageSector 158,928; 153 MiscDataBank153 7,680; 161 FoxSpecial3 12,160 | 226,192 | - | 17,599 | 30,068 |
| Kongo Jungle (2) | 261 | 261 GRJungleMap 224; 108 ExternDataBank108 62,944; 92 StageJungle 158,928; 158 MiscDataBank158 3,296 | 225,392 | asset 107: 27,792 | 14,516 | 28,060 |
| Planet Zebes (3) | 257 | 257 GRZebesMap 224; 105 ExternDataBank105 57,184; 89 StageZebes 158,928; 157 MiscDataBank157 3,536 | 219,872 | - | 13,842 | 19,840 |
| Hyrule Castle (4) | 265 | 265 GRHyruleMap 224; 113 ExternDataBank113 26,768; 95 StageCastle 158,928 | 185,920 | - | 14,131 | 30,204 |
| Yoshi's Island (5) | 263 | 263 GRYosterMap 192; 111 ExternDataBank111 47,408; 110 ExternDataBank110 21,040; 93 StageYoshi 158,928; 154 MiscDataBank154 1,712 | 229,280 | - | 12,524 | 29,452 |
| Dream Land (6) | 255 | 255 GRPupupuMap 192; 104 ExternDataBank104 17,392; 103 ExternDataBank103 12,224; 88 StageDreamLand 158,928; 152 MiscDataBank152 14,080 | 202,816 | - | linked (0) | 32,140 |
| Saffron City (7) | 264 | 264 GRYamabukiMap 832; 112 ExternDataBank112 66,160; 94 StagePokemon 158,928; 160 MiscDataBank160 2,704; 159 MiscDataBank159 10,976 | 239,600 | - | 18,209 | 36,296 |
| Mushroom Kingdom (8) | 260 | 260 GRInishieMap 368; 107 ExternDataBank107 27,792; 91 StageHyruleWallpaper 158,928; 155 MiscDataBank155 5,136 | 192,224 | - | 14,088 | 24,188 |

### Table 2

| stage | wallpaper pixels | wallpaper Sprite+Bitmap | Gfx | Vtx | texels | TLUT | DObjDesc+DLLink | anim scripts/tables | MObjSub+ptr lists | collision | header+attr | pad+u16 other | total |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Peach's Castle | 158,152 | 776 | 5,144 | 4,192 | 4,840 | 416 | 1,144 | 1,096 | 0 | 530 | 188 | 402 | 176,880 |
| Sector Z | 158,152 | 776 | 9,816 | 12,976 | 19,820 | 1,152 | 2,412 | 19,476 | 128 | 520 | 304 | 660 | 226,192 |
| Kongo Jungle | 158,152 | 776 | 7,272 | 5,360 | 38,884 | 832 | 1,936 | 10,992 | 152 | 408 | 224 | 404 | 225,392 |
| Planet Zebes | 158,152 | 776 | 5,456 | 5,248 | 17,316 | 1,320 | 2,088 | 25,540 | 2,788 | 396 | 224 | 568 | 219,872 |
| Hyrule Castle | 158,152 | 776 | 6,264 | 6,496 | 11,904 | 416 | 980 | 0 | 0 | 406 | 224 | 302 | 185,920 |
| Yoshi's Island | 158,152 | 776 | 6,616 | 5,280 | 34,976 | 896 | 2,420 | 17,676 | 1,280 | 494 | 188 | 526 | 229,280 |
| Dream Land | 158,152 | 776 | 7,360 | 4,880 | 15,116 | 2,864 | 3,124 | 8,616 | 760 | 488 | 188 | 492 | 202,816 |
| Saffron City | 158,152 | 776 | 11,608 | 8,288 | 44,556 | 1,728 | 2,800 | 8,316 | 1,108 | 554 | 832 | 882 | 239,600 |
| Mushroom Kingdom | 158,152 | 776 | 6,360 | 4,928 | 14,992 | 288 | 1,752 | 3,068 | 704 | 530 | 368 | 306 | 192,224 |

### Table 3

| stage | A: never read after load | L: read at load / first use only | F: read during frames | T1 wallpaper | T2 packet-only Gfx+Vtx | T3 static-corpus textures | T4 phantom bank | T1..T4 | tree alloc after T1 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Peach's Castle | 167,114 | 4,944 | 4,822 | 158,144 | 7,744 | 0 | 0 | 165,888 | 18,736 |
| Sector Z | 173,308 | 16,692 | 36,192 | 158,144 | 13,168 | 0 | 0 | 171,312 | 68,048 |
| Kongo Jungle | 170,228 | 32,208 | 22,956 | 158,144 | 10,132 | 0 | 27,792 | 196,068 | 67,248 |
| Planet Zebes | 168,108 | 12,272 | 39,492 | 158,144 | 8,308 | 0 | 0 | 166,452 | 61,728 |
| Hyrule Castle | 171,146 | 14,020 | 754 | 158,144 | 11,500 | 0 | 0 | 169,644 | 27,776 |
| Yoshi's Island | 168,146 | 22,528 | 38,606 | 158,144 | 8,404 | 0 | 0 | 166,548 | 71,136 |
| Dream Land | 184,668 | 3,636 | 14,512 | 158,144 | 9,828 | 14,072 | 0 | 182,044 | 44,672 |
| Saffron City | 175,326 | 13,848 | 50,426 | 158,144 | 14,704 | 0 | 0 | 172,848 | 81,456 |
| Mushroom Kingdom | 168,182 | 13,488 | 10,554 | 158,144 | 8,552 | 0 | 0 | 166,696 | 34,080 |

### Table 4

| stage | tree alloc | wallpaper container share of tree | after T1 | packet-referenced texels+TLUT (T5 candidate, not claimed) | packet-only Gfx+Vtx gross | packet DL roots |
|---|---:|---:|---:|---:|---:|---:|
| Peach's Castle | 176,880 | 89.9% | 18,736 | 3,288 | 8,640 | 12 |
| Sector Z | 226,192 | 70.3% | 68,048 | 14,816 | 14,584 | 19 |
| Kongo Jungle | 225,392 | 70.5% | 67,248 | 29,760 | 11,712 | 30 |
| Planet Zebes | 219,872 | 72.3% | 61,728 | 10,088 | 9,568 | 26 |
| Hyrule Castle | 185,920 | 85.5% | 27,776 | 12,320 | 12,760 | 15 |
| Yoshi's Island | 229,280 | 69.3% | 71,136 | 20,344 | 9,520 | 19 |
| Dream Land | 202,816 | 78.4% | 44,672 | 0 | 11,632 | 42 |
| Saffron City | 239,600 | 66.3% | 81,456 | 11,840 | 16,376 | 21 |
| Mushroom Kingdom | 192,224 | 82.7% | 34,080 | 11,392 | 9,800 | 23 |

### Table 5

| stage | resident files (ids) | resident B | A never read | L load/first use | F read in frames | T1 | T2 | T3 | T4 | confidence |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| Peach's Castle | 259, 106, 90, 156 | 176,880 | 167,114 | 4,944 | 4,822 | 158,144 | 7,744 | 0 | 0 | T1 high; T2 medium |
| Sector Z | 262, 109, 99, 153, 161 | 226,192 | 173,308 | 16,692 | 36,192 | 158,144 | 13,168 | 0 | 0 | T1 high; T2 medium (FoxSpecial3 12,160 B kept: no packet owner) |
| Kongo Jungle | 261, 108, 92, 158 (+107 preload) | 253,184 | 170,228 | 32,208 | 22,956 | 158,144 | 10,132 | 0 | 27,792 | T1 high; T2 medium; T4 medium (heap vs overlay UNPROVEN) |
| Planet Zebes | 257, 105, 89, 157 | 219,872 | 168,108 | 12,272 | 39,492 | 158,144 | 8,308 | 0 | 0 | T1 high; T2 medium |
| Hyrule Castle | 265, 113, 95 | 185,920 | 171,146 | 14,020 | 754 | 158,144 | 11,500 | 0 | 0 | T1 high; T2 medium (100% of its Gfx/Vtx is packet-only) |
| Yoshi's Island | 263, 111, 110, 93, 154 | 229,280 | 168,146 | 22,528 | 38,606 | 158,144 | 8,404 | 0 | 0 | T1 high; T2 medium |
| Dream Land | 255, 104, 103, 88, 152 | 202,816 | 184,668 | 3,636 | 14,512 | 158,144 | 9,828 | 14,072 | 0 | T1 high (frozen P1 pins); T2 medium; T3 low-medium (corpus engagement UNPROVEN) |
| Saffron City | 264, 112, 94, 160, 159 | 239,600 | 175,326 | 13,848 | 50,426 | 158,144 | 14,704 | 0 | 0 | T1 high; T2 medium |
| Mushroom Kingdom | 260, 107, 91, 155 | 192,224 | 168,182 | 13,488 | 10,554 | 158,144 | 8,552 | 0 | 0 | T1 high; T2 medium |

### Appendix

| stage | file | role | payload | Gfx (pkt-only) | Vtx (pkt-only) | texels | TLUT | DObjDesc+DLLink | anim | MObjSub+ptr | coll | hdr+attr | pad+o16 | wallpaper pix / sprite |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Peach's Castle | 259 GRCastleMap | map | 192 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 188 | 4 | 0 / 0 |
| Peach's Castle | 106 ExternDataBank106 | typed | 17,696 | 5,144 (4,640) | 4,192 (4,000) | 4,840 | 416 | 1,144 | 1,044 | 0 | 530 | 0 | 386 | 0 / 0 |
| Peach's Castle | 90 MVOpeningRoomWallpaper | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Peach's Castle | 156 MiscDataBank156 | typed | 64 | 0 (0) | 0 (0) | 0 | 0 | 0 | 52 | 0 | 0 | 0 | 12 | 0 / 0 |
| Sector Z | 262 GRSectorMap | map | 304 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 304 | 0 | 0 / 0 |
| Sector Z | 109 ExternDataBank109 | typed | 47,120 | 6,512 (5,448) | 9,360 (9,136) | 15,980 | 640 | 1,712 | 11,840 | 128 | 520 | 0 | 428 | 0 / 0 |
| Sector Z | 99 StageSector | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Sector Z | 153 MiscDataBank153 | typed | 7,680 | 216 (0) | 96 (0) | 0 | 0 | 0 | 7,368 | 0 | 0 | 0 | 0 | 0 / 0 |
| Sector Z | 161 FoxSpecial3 | typed | 12,160 | 3,088 (0) | 3,520 (0) | 3,840 | 512 | 700 | 268 | 0 | 0 | 0 | 232 | 0 / 0 |
| Kongo Jungle | 261 GRJungleMap | map | 224 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 224 | 0 | 0 / 0 |
| Kongo Jungle | 108 ExternDataBank108 | typed | 62,944 | 6,880 (6,640) | 5,136 (5,072) | 36,836 | 800 | 1,804 | 10,544 | 152 | 408 | 0 | 384 | 0 / 0 |
| Kongo Jungle | 92 StageJungle | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Kongo Jungle | 158 MiscDataBank158 | typed | 3,296 | 392 (0) | 224 (0) | 2,048 | 32 | 132 | 448 | 0 | 0 | 0 | 20 | 0 / 0 |
| Planet Zebes | 257 GRZebesMap | map | 224 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 224 | 0 | 0 / 0 |
| Planet Zebes | 105 ExternDataBank105 | typed | 57,184 | 5,168 (4,256) | 5,120 (4,896) | 15,268 | 1,192 | 1,940 | 24,968 | 2,640 | 396 | 0 | 492 | 0 / 0 |
| Planet Zebes | 89 StageZebes | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Planet Zebes | 157 MiscDataBank157 | typed | 3,536 | 288 (288) | 128 (128) | 2,048 | 128 | 148 | 572 | 148 | 0 | 0 | 76 | 0 / 0 |
| Hyrule Castle | 265 GRHyruleMap | map | 224 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 224 | 0 | 0 / 0 |
| Hyrule Castle | 113 ExternDataBank113 | typed | 26,768 | 6,264 (6,264) | 6,496 (6,496) | 11,904 | 416 | 980 | 0 | 0 | 406 | 0 | 302 | 0 / 0 |
| Hyrule Castle | 95 StageCastle | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Yoshi's Island | 263 GRYosterMap | map | 192 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 188 | 4 | 0 / 0 |
| Yoshi's Island | 111 ExternDataBank111 | typed | 47,408 | 6,128 (4,688) | 5,216 (4,832) | 13,928 | 576 | 2,200 | 17,404 | 1,144 | 494 | 0 | 318 | 0 / 0 |
| Yoshi's Island | 110 ExternDataBank110 | typed | 21,040 | 0 (0) | 0 (0) | 20,536 | 320 | 0 | 0 | 0 | 0 | 0 | 184 | 0 / 0 |
| Yoshi's Island | 93 StageYoshi | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Yoshi's Island | 154 MiscDataBank154 | typed | 1,712 | 488 (0) | 64 (0) | 512 | 0 | 220 | 272 | 136 | 0 | 0 | 20 | 0 / 0 |
| Dream Land | 255 GRPupupuMap | map | 192 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 188 | 4 | 0 / 0 |
| Dream Land | 104 ExternDataBank104 | typed | 17,392 | 5,144 (4,664) | 4,128 (4,000) | 2,132 | 128 | 1,804 | 3,052 | 444 | 488 | 0 | 72 | 0 / 0 |
| Dream Land | 103 ExternDataBank103 | typed | 12,224 | 0 (0) | 0 (0) | 9,344 | 2,576 | 0 | 0 | 0 | 0 | 0 | 304 | 0 / 0 |
| Dream Land | 88 StageDreamLand | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Dream Land | 152 MiscDataBank152 | typed | 14,080 | 2,216 (2,216) | 752 (752) | 3,640 | 160 | 1,320 | 5,564 | 316 | 0 | 0 | 112 | 0 / 0 |
| Saffron City | 264 GRYamabukiMap | map | 832 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 832 | 0 | 0 / 0 |
| Saffron City | 112 ExternDataBank112 | typed | 66,160 | 8,584 (7,624) | 7,120 (6,864) | 38,388 | 1,472 | 1,656 | 6,860 | 820 | 554 | 0 | 706 | 0 / 0 |
| Saffron City | 94 StagePokemon | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Saffron City | 160 MiscDataBank160 | typed | 2,704 | 1,424 (1,168) | 720 (720) | 0 | 0 | 328 | 208 | 0 | 0 | 0 | 24 | 0 / 0 |
| Saffron City | 159 MiscDataBank159 | typed | 10,976 | 1,600 (0) | 448 (0) | 6,168 | 256 | 816 | 1,248 | 288 | 0 | 0 | 152 | 0 / 0 |
| Mushroom Kingdom | 260 GRInishieMap | map | 368 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 368 | 0 | 0 / 0 |
| Mushroom Kingdom | 107 ExternDataBank107 | typed | 27,792 | 5,072 (5,072) | 3,904 (3,904) | 13,728 | 288 | 1,224 | 2,256 | 552 | 530 | 0 | 238 | 0 / 0 |
| Mushroom Kingdom | 91 StageHyruleWallpaper | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Mushroom Kingdom | 155 MiscDataBank155 | typed | 5,136 | 1,288 (440) | 1,024 (384) | 1,264 | 0 | 528 | 812 | 152 | 0 | 0 | 68 | 0 / 0 |

Walker vs tiling: 1,725,422 of 1,728,800 (99.80%); tex->pal 2,224 B; coll->other16 540 B; mobj->anim 248 B; tex->mobj 152 B; tex->pad 148 B; tex->anim 36 B; coll->pad 18 B; mobj->pad 8 B; coll->mobj 4 B

# Results general-heap allocation census: r54-results-fixture

Requests: 393; successful requested bytes: 924,932; alignment: 92 B; computed final cursor: `023df560`.

| Caller / LR | Calls | Requested B | Padding B | Sizes (B: count) |
| --- | ---: | ---: | ---: | --- |
| ndsRelocEnsureLoadedAsset / `02070065` | 20 | 210,144 | 48 | 64: 2, 144: 1, 464: 1, 656: 1, 1312: 1, 1856: 1, 1984: 1, 3632: 1, 3696: 1, 6080: 1, 6560: 1, 6816: 1, 7040: 1, 9648: 1, 10512: 1, 10752: 1, 12160: 1, 47120: 1, 79584: 1 |
| mnVSResultsFuncStart / `0211a5c3` | 1 | 130,080 | 0 | 130080: 1 |
| ndsRelocPreviewFighterLoadBegin / `0209228f` | 4 | 85,636 | 0 | 14472: 1, 14488: 1, 21404: 1, 35272: 1 |
| ndsBaseSyTaskmanMalloc / `0209cbaf` | 2 | 65,536 | 0 | 32768: 2 |
| ndsBaseEFManagerInitEffects / `02125b21` | 1 | 52,736 | 8 | 52736: 1 |
| ndsBaseSyTaskmanMalloc / `0209cbfd` | 1 | 49,152 | 8 | 49152: 1 |
| ndsLBTransitionMalloc / `0211753f` | 1 | 47,600 | 0 | 47600: 1 |
| mnVSResultsFuncStart / `0211a617` | 4 | 47,168 | 0 | 11792: 4 |
| ndsAObjEvent32ConfigureNormalizedCapacity / `020a2371` | 1 | 41,984 | 0 | 41984: 1 |
| ndsBaseSyTaskmanMalloc / `0209cb21` | 2 | 40,000 | 0 | 20000: 2 |
| ndsBaseFTManagerAllocFighter / `020e8e81` | 1 | 33,152 | 0 | 33152: 1 |
| ndsBaseEFManagerInitEffects / `02125b3b` | 1 | 28,352 | 0 | 28352: 1 |
| lbParticleAllocTransforms / `0212eced` | 80 | 15,360 | 0 | 192: 80 |
| gcSetupObjman / `0209e943` | 1 | 13,824 | 0 | 13824: 1 |
| ndsBaseEFManagerInitEffects / `02125b53` | 1 | 13,616 | 0 | 13616: 1 |
| ndsBaseFTManagerAllocFighter / `020e8e39` | 1 | 12,048 | 0 | 12048: 1 |
| lbParticleAllocStructs / `0212c055` | 112 | 10,752 | 0 | 96: 112 |
| ndsShieldPoseLoad / `020c63cd` | 2 | 6,152 | 0 | 3011: 1, 3141: 1 |
| gcGetGObjSetNextAlloc / `0209d291` | 23 | 3,128 | 4 | 136: 23 |
| ndsEFManagerInitVisualTemplates / `02129c6b` | 1 | 2,464 | 8 | 2464: 1 |
| ndsBaseEFManagerInitEffects / `02125ae7` | 1 | 2,280 | 0 | 2280: 1 |
| lbParticleAllocGenerators / `0212c511` | 24 | 2,208 | 0 | 92: 24 |
| ndsBaseFTManagerAllocFighter / `020e8eeb` | 1 | 2,096 | 0 | 2096: 1 |
| ndsBaseSyTaskmanMalloc / `0209cb37` | 2 | 2,048 | 0 | 1024: 2 |
| ndsParticleLoadEFCommonBank / `0212ef0f` | 47 | 1,504 | 0 | 32: 47 |
| gcGetCObjSetNextAlloc / `0209d40d` | 10 | 1,440 | 0 | 144: 10 |
| ndsBaseFTManagerAllocFighter / `020e8ed1` | 1 | 912 | 0 | 912: 1 |
| gcGetXObjSetNextAlloc / `0209d333` | 12 | 864 | 0 | 72: 12 |
| ndsRelocPatchCompactBattleMainExterns / `02092d39` | 3 | 784 | 16 | 224: 2, 336: 1 |
| gcGetDObjSetNextAlloc / `0209d39d` | 4 | 544 | 0 | 136: 4 |
| ndsParticleLoadEFCommonBank / `0212eea1` | 1 | 476 | 0 | 476: 1 |
| ndsBaseSyTaskmanMalloc / `0209cab9` | 1 | 272 | 0 | 272: 1 |
| ndsRelocRegisterLoadedFileImpl / `02067cbd` | 7 | 196 | 0 | 2: 1, 4: 1, 6: 1, 46: 4 |
| ndsParticleLoadEFCommonBank / `0212eead` | 1 | 188 | 0 | 188: 1 |
| ndsBaseSyTaskmanMalloc / `0209caf1` | 1 | 112 | 0 | 112: 1 |
| ndsBaseSyTaskmanMalloc / `0209cad7` | 1 | 88 | 0 | 88: 1 |
| gcGetGObjProcess / `0209d173` | 1 | 36 | 0 | 36: 1 |
| ndsBaseSyTaskmanMalloc / `0209ccd7` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `0209ce1d` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `0209cd0b` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `0209cd25` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `0209cd47` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `0209cd6d` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `0209cd87` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `0209cda1` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `0209cdbf` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `0209cddf` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `0209cb4d` | 2 | 0 | 0 | 0: 2 |
| ndsBaseSyTaskmanMalloc / `0209cb63` | 2 | 0 | 0 | 0: 2 |

## Large requests

Seq 11: 13,824 B, align 4, `gcSetupObjman`, `D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_objman.c:395`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=13824, alignment=4) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0209e8de in gcSetupObjman (setup=0x180 <ndsPlatformCommitOriginalSpritePreviewLayer+384>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_objman.c:217
#4  0x0209ccd6 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#5  ndsBaseSyTaskmanStartTask (tsetup=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1273
#6  0x0209cf50 in syTaskmanStartTask (tsetup=0x22fd800) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:97
#7  0x02080c56 in scManagerFuncUpdate (setup=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_compat_shims.c:1460
```

Seq 15: 20,000 B, align 8, `ndsBaseSyTaskmanMalloc`, `D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=20000, alignment=alignment@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209cb20 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanLoadScene (tscene=0x22e4e0c, func_start=0x88 <unref_80009FC0+136>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1212
#3  0x0209ccd6 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#4  ndsBaseSyTaskmanStartTask (tsetup=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1273
#5  0x0209cf50 in syTaskmanStartTask (tsetup=0x22fd800) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:97
#6  0x02080c56 in scManagerFuncUpdate (setup=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_compat_shims.c:1460
#7  0x0205ce0a in ndsOsWaitForQueue (queue=<optimized out>, wait_for_space=<optimized out>, flag=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/libultra_os.c:351
```

Seq 19: 20,000 B, align 8, `ndsBaseSyTaskmanMalloc`, `D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=20000, alignment=alignment@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209cb20 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanLoadScene (tscene=0x22e4e0c, func_start=0x88 <unref_80009FC0+136>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1212
#3  0x0209ccd6 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#4  ndsBaseSyTaskmanStartTask (tsetup=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1273
#5  0x0209cf50 in syTaskmanStartTask (tsetup=0x22fd800) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:97
#6  0x02080c56 in scManagerFuncUpdate (setup=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_compat_shims.c:1460
#7  0x0205ce0a in ndsOsWaitForQueue (queue=<optimized out>, wait_for_space=<optimized out>, flag=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/libultra_os.c:351
```

Seq 23: 32,768 B, align 8, `ndsBaseSyTaskmanMalloc`, `D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=32768, alignment=alignment@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209cbae in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanLoadScene (tscene=0x22e4e0c, func_start=0x88 <unref_80009FC0+136>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1225
#3  0x0209ccd6 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#4  ndsBaseSyTaskmanStartTask (tsetup=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1273
#5  0x0209cf50 in syTaskmanStartTask (tsetup=0x22fd800) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:97
#6  0x02080c56 in scManagerFuncUpdate (setup=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_compat_shims.c:1460
#7  0x0205ce0a in ndsOsWaitForQueue (queue=<optimized out>, wait_for_space=<optimized out>, flag=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/libultra_os.c:351
```

Seq 24: 32,768 B, align 8, `ndsBaseSyTaskmanMalloc`, `D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=32768, alignment=alignment@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209cbae in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanLoadScene (tscene=0x22e4e0c, func_start=0x88 <unref_80009FC0+136>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1225
#3  0x0209ccd6 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#4  ndsBaseSyTaskmanStartTask (tsetup=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1273
#5  0x0209cf50 in syTaskmanStartTask (tsetup=0x22fd800) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:97
#6  0x02080c56 in scManagerFuncUpdate (setup=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_compat_shims.c:1460
#7  0x0205ce0a in ndsOsWaitForQueue (queue=<optimized out>, wait_for_space=<optimized out>, flag=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/libultra_os.c:351
```

Seq 25: 49,152 B, align 16, `ndsBaseSyTaskmanMalloc`, `D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=49152, alignment=alignment@entry=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209cbfc in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=16) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanLoadScene (tscene=0x22e4e0c, func_start=0x88 <unref_80009FC0+136>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1237
#3  0x0209ccd6 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#4  ndsBaseSyTaskmanStartTask (tsetup=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1273
#5  0x0209cf50 in syTaskmanStartTask (tsetup=0x22fd800) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:97
#6  0x02080c56 in scManagerFuncUpdate (setup=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_compat_shims.c:1460
#7  0x0205ce0a in ndsOsWaitForQueue (queue=<optimized out>, wait_for_space=<optimized out>, flag=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/libultra_os.c:351
```

Seq 26: 41,984 B, align 4, `ndsAObjEvent32ConfigureNormalizedCapacity`, `D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_objanim.c:1179`.

```text
#0  syMallocSet (bp=0x22b461c <gSYTaskmanGeneralHeap>, bp@entry=0x0 <ndsRendererProfileGlobalStateHash>, size=size@entry=41984, alignment=alignment@entry=4) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x020a2370 in ndsAObjEvent32ConfigureNormalizedCapacity (gkind=4294967295) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_objanim.c:1179
#2  0x0206376c in ndsRelocPrepareSceneCache () at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:5291
#3  0x02087cfc in lbRelocInitSetup (setup=setup@entry=0x22e4c20) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:12291
#4  0x0211a5b4 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3335
#5  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#6  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
#7  0x0209ce0a in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1322
```

Seq 27: 130,080 B, align 16, `mnVSResultsFuncStart`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3336 (discriminator 1)`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=130080, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0211a5bc in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3336
#4  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#5  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
#6  0x0209ce0a in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1322
#7  0x0209cf7a in syTaskmanStartTask (tsetup=tsetup@entry=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:106
```

Seq 249: 2,280 B, align 8, `ndsBaseEFManagerInitEffects`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1740`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=2280, alignment=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x000000d0 in ndsRendererTask29GXRecord (command_class=NDS_TASK29_GX_VERTEX16, words=0x22e4bb8, word_count=2) at D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_renderer_preamble.c:3881
#4  ndsRendererTask29GlVertex3v16 (x=<optimized out>, y=<optimized out>, z=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_renderer_preamble.c:1560
#5  ndsRendererSubmitDebugDiamond (cx=1.26597204e-37, cy=-9.50697854e-13, cz=1.25832163e-37, top=1.25831939e-37, center=3.50324616e-44, bottom=1.28058994e-37, width=1.25831535e-37) at D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_renderer_textures_effects.c:8810
```

Seq 254: 52,736 B, align 16, `ndsBaseEFManagerInitEffects`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1754 (discriminator 1)`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=52736, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02125b1a in ndsBaseEFManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1754
#4  0x02129ac6 in efManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:2343
#5  0x0211a5f6 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3340
#6  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#7  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
```

Seq 255: 28,352 B, align 16, `ndsBaseEFManagerInitEffects`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1755 (discriminator 1)`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=28352, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02125b34 in ndsBaseEFManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1755
#4  0x02129ac6 in efManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:2343
#5  0x0211a5f6 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3340
#6  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#7  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
```

Seq 256: 13,616 B, align 16, `ndsBaseEFManagerInitEffects`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1756 (discriminator 1)`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=13616, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02125b4c in ndsBaseEFManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1756
#4  0x02129ac6 in efManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:2343
#5  0x0211a5f6 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3340
#6  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#7  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
```

Seq 310: 2,464 B, align 16, `ndsEFManagerInitVisualTemplates`, `D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:567`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=2464, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02129c32 in ndsEFManagerResolveAllDescOffsets () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:2067
#4  efManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:2350
#5  0x0211a5f6 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3340
#6  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#7  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
```

Seq 311: 12,048 B, align 8, `ndsBaseFTManagerAllocFighter`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ft/ftmanager.c:144`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=12048, alignment=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02157cb8 in __libc_lock_acquire_recursive ()
#4  0x0214f96c in __malloc_lock ()
#5  0x0214ec1a in _free_r ()
#6  0x023e1e18 in ?? ()
```

Seq 312: 33,152 B, align 8, `ndsBaseFTManagerAllocFighter`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ft/ftmanager.c:154`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=33152, alignment=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02157cb8 in __libc_lock_acquire_recursive ()
#4  0x0214f96c in __malloc_lock ()
#5  0x0214ec1a in _free_r ()
#6  0x023e1e18 in ?? ()
```

Seq 314: 2,096 B, align 16, `ndsBaseFTManagerAllocFighter`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ft/ftmanager.c:168 (discriminator 1)`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=2096, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x020e8ee4 in ndsBaseFTManagerAllocFighter (data_flags=2, allocs_num=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ft/ftmanager.c:168
#4  0x020e9e82 in ftManagerAllocFighter (data_flags=data_flags@entry=1, allocs_num=<optimized out>, allocs_num@entry=4) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:115
#5  0x0211a5fe in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3341
#6  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#7  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
```

Seq 315: 14,472 B, align 16, `ndsRelocPreviewFighterLoadBegin`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:540`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=14472, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02092082 in ndsRelocPreviewFighterLoadBegin (fkind=fkind@entry=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:510
#4  0x02093096 in ndsRelocLoadPreviewFighterUnlocked (fkind=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:873
#5  ndsRelocLoadPreviewFighter (fkind=fkind@entry=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1143
#6  0x020e9b90 in ftManagerSetupFilesAllKind (fkind=fkind@entry=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:358
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 317: 6,560 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=6560, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=36588344) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=36588344) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x02092ce8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020e9cae in ftManagerSetupFilesAllKind (fkind=fkind@entry=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:371
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 323: 10,752 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=10752, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=36588362) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=36588362) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x02092ce8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020e9cae in ftManagerSetupFilesAllKind (fkind=fkind@entry=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:371
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 324: 14,488 B, align 16, `ndsRelocPreviewFighterLoadBegin`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:540`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=14488, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x020920d4 in ndsRelocPreviewFighterLoadBegin (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:521
#4  0x02093096 in ndsRelocLoadPreviewFighterUnlocked (fkind=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:873
#5  ndsRelocLoadPreviewFighter (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1143
#6  0x020e9b90 in ftManagerSetupFilesAllKind (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:358
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 325: 3,011 B, align 4, `ndsShieldPoseLoad`, `D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_shield_pose.c:246`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=3011, alignment=4) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x020c6398 in ndsShieldPoseLoad (package=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_shield_pose.c:232
#4  0x0214a5c4 in mutexUnlock (m=0x214a5c4 <mutexUnlock+68>) at /home/davem/projects/devkitpro/pacman-packages/calico/src/calico-1.2.0/source/system/mutex.c:143
#5  0x00000000 in ?? ()
```

Seq 327: 6,816 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=6816, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=36588344) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=36588344) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x02092ce8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020e9cae in ftManagerSetupFilesAllKind (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:371
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 329: 3,632 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=3632, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=36588350) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=36588350) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x02092ce8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020e9cae in ftManagerSetupFilesAllKind (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:371
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 330: 12,160 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=12160, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=36588356) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=36588356) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x02092ce8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020e9cae in ftManagerSetupFilesAllKind (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:371
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 332: 47,120 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=47120, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=10612) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=10612, asset_id@entry=109) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x0206fe74 in ndsRelocApplyExternalPointerFixups (loaded=0x22ad670 <sNdsRelocLoadedFiles+1344>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8909
#6  ndsRelocFinalizeLoadedFile (loaded=loaded@entry=0x22ad670 <sNdsRelocLoadedFiles+1344>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9034
#7  0x02070040 in ndsRelocEnsureLoadedAsset (asset_id=161) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8759
```

Seq 337: 21,404 B, align 16, `ndsRelocPreviewFighterLoadBegin`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:540`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=21404, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x020920d4 in ndsRelocPreviewFighterLoadBegin (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:521
#4  0x02093096 in ndsRelocLoadPreviewFighterUnlocked (fkind=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:873
#5  ndsRelocLoadPreviewFighter (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1143
#6  0x020e9b90 in ftManagerSetupFilesAllKind (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:358
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 338: 6,080 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=6080, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=36588344) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=36588344) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x02092ce8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020e9cae in ftManagerSetupFilesAllKind (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:371
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 341: 3,696 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=3696, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=36588356) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=36588356) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x02092ce8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020e9cae in ftManagerSetupFilesAllKind (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:371
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 342: 79,584 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=79584, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=36588362) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=36588362) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x02092ce8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020e9cae in ftManagerSetupFilesAllKind (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:371
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 343: 7,040 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=7040, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=36588368) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=36588368) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x02092ce8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020e9cae in ftManagerSetupFilesAllKind (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:371
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 344: 35,272 B, align 16, `ndsRelocPreviewFighterLoadBegin`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:540`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=35272, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02092082 in ndsRelocPreviewFighterLoadBegin (fkind=fkind@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:510
#4  0x02093096 in ndsRelocLoadPreviewFighterUnlocked (fkind=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:873
#5  ndsRelocLoadPreviewFighter (fkind=fkind@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1143
#6  0x020e9b90 in ftManagerSetupFilesAllKind (fkind=fkind@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:358
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 345: 3,141 B, align 4, `ndsShieldPoseLoad`, `D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_shield_pose.c:246`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=3141, alignment=4) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x020c6398 in ndsShieldPoseLoad (package=5) at D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_shield_pose.c:232
#4  0x0214f3b4 in __gettzinfo ()
```

Seq 347: 9,648 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=9648, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=36588344) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=36588344) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x02092ce8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020e9cae in ftManagerSetupFilesAllKind (fkind=fkind@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:371
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 349: 10,512 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8735`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=10512, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0206ffc6 in ndsRelocAssetAllocSize (asset_id=36588350) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11320
#4  ndsRelocEnsureLoadedAsset (asset_id=36588350) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:8711
#5  0x02092ce8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020e9cae in ftManagerSetupFilesAllKind (fkind=fkind@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:371
#7  0x0211a55c in ndsMNVSResultsSetupFilesKind (fkind=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 351: 11,792 B, align 16, `mnVSResultsFuncStart`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3349`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=11792, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0211a606 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3345
#4  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#5  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
#6  0x0209ce0a in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1322
#7  0x0209cf7a in syTaskmanStartTask (tsetup=tsetup@entry=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:106
```

Seq 352: 11,792 B, align 16, `mnVSResultsFuncStart`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3349`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=11792, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0211a606 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3345
#4  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#5  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
#6  0x0209ce0a in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1322
#7  0x0209cf7a in syTaskmanStartTask (tsetup=tsetup@entry=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:106
```

Seq 353: 11,792 B, align 16, `mnVSResultsFuncStart`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3349`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=11792, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0211a606 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3345
#4  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#5  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
#6  0x0209ce0a in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1322
#7  0x0209cf7a in syTaskmanStartTask (tsetup=tsetup@entry=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:106
```

Seq 354: 11,792 B, align 16, `mnVSResultsFuncStart`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3349`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=11792, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0211a606 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3345
#4  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#5  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
#6  0x0209ce0a in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1322
#7  0x0209cf7a in syTaskmanStartTask (tsetup=tsetup@entry=0x22e4e0c) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:106
```

Seq 355: 47,600 B, align 16, `ndsLBTransitionMalloc`, `D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_lbtransition.c:35`.

```text
#0  syMallocSet (bp=bp@entry=0x22b461c <gSYTaskmanGeneralHeap>, size=47600, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:140
#1  0x0209d040 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02117524 in ndsBaseLBTransitionSetupTransition () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/lb/lbtransition.c:216
#4  0x021175ce in lbTransitionSetupTransition () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_lbtransition.c:70
#5  0x0211a64a in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3362
#6  0x0211779c in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#7  0x0209cc48 in syTaskmanLoadScene (tscene=tscene@entry=0x22e4e0c, func_start=0x211777d <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build/battleship_overlay/src/sys/taskman.c:1249
```

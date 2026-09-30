# Results general-heap allocation census: r55-native-buffers-owner

Requests: 692; successful requested bytes: 890,548; alignment: 212 B; computed final cursor: `023d4b88`.

| Caller / LR | Calls | Requested B | Padding B | Sizes (B: count) |
| --- | ---: | ---: | ---: | --- |
| ndsRelocEnsureLoadedAsset / `02078acb` | 20 | 210,144 | 56 | 64: 2, 144: 1, 464: 1, 656: 1, 1312: 1, 1856: 1, 1984: 1, 3632: 1, 3696: 1, 6080: 1, 6560: 1, 6816: 1, 7040: 1, 9648: 1, 10512: 1, 10752: 1, 12160: 1, 47120: 1, 79584: 1 |
| ndsSceneAssetAlloc / `020a6c7d` | 6 | 135,312 | 16 | 14472: 1, 14488: 1, 19196: 1, 21460: 1, 30160: 1, 35536: 1 |
| mnVSResultsFuncStart / `022952ef` | 1 | 130,080 | 0 | 130080: 1 |
| ndsBaseEFManagerInitEffects / `0210381d` | 1 | 52,736 | 8 | 52736: 1 |
| ndsLBTransitionMalloc / `020f862f` | 1 | 47,600 | 0 | 47600: 1 |
| mnVSResultsFuncStart / `02295343` | 4 | 47,168 | 0 | 11792: 4 |
| ndsAObjEvent32ConfigureNormalizedCapacity / `020aec57` | 1 | 41,984 | 0 | 41984: 1 |
| ndsBaseSyTaskmanMalloc / `020a8d15` | 2 | 40,000 | 0 | 20000: 2 |
| ndsFTManagerPoolMalloc / `020cae8f` | 3 | 36,160 | 0 | 912: 1, 2096: 1, 33152: 1 |
| ndsBaseEFManagerInitEffects / `02103837` | 1 | 28,352 | 0 | 28352: 1 |
| lbParticleAllocTransforms / `0210dba5` | 80 | 15,360 | 0 | 192: 80 |
| gcSetupObjman / `020aabc1` | 1 | 13,824 | 0 | 13824: 1 |
| ndsBaseEFManagerInitEffects / `0210384f` | 1 | 13,616 | 0 | 13616: 1 |
| gcGetDObjSetNextAlloc / `020a9591` | 100 | 13,600 | 0 | 136: 100 |
| gcGetMObjSetNextAlloc / `020a955b` | 65 | 10,920 | 0 | 168: 65 |
| lbParticleAllocStructs / `0210af0d` | 112 | 10,752 | 0 | 96: 112 |
| gcGetXObjSetNextAlloc / `020a9527` | 112 | 8,064 | 12 | 72: 112 |
| ndsShieldPoseLoad / `020c5ce5` | 2 | 6,152 | 0 | 3011: 1, 3141: 1 |
| gcGetGObjSetNextAlloc / `020a9485` | 35 | 4,760 | 40 | 136: 35 |
| gcGetGObjStackOfSize / `020a92cf` | 1 | 4,216 | 4 | 4216: 1 |
| ndsBaseSyTaskmanMalloc / `020a8da3` | 2 | 3,072 | 0 | 1536: 2 |
| ndsEFManagerInitVisualTemplates / `021079e3` | 1 | 2,464 | 8 | 2464: 1 |
| ndsBaseEFManagerInitEffects / `021037e3` | 1 | 2,280 | 0 | 2280: 1 |
| lbParticleAllocGenerators / `0210b3c9` | 24 | 2,208 | 0 | 92: 24 |
| ndsBaseSyTaskmanMalloc / `020a8d2b` | 2 | 2,048 | 0 | 1024: 2 |
| gcGetSObjSetNextAlloc / `020a95c9` | 16 | 1,728 | 32 | 108: 16 |
| gcGetCObjSetNextAlloc / `020a9601` | 11 | 1,584 | 0 | 144: 11 |
| ndsParticleLoadEFCommonBank / `0210ddc7` | 47 | 1,504 | 0 | 32: 47 |
| ndsRelocPatchCompactBattleMainExterns / `0209f17b` | 3 | 784 | 24 | 224: 2, 336: 1 |
| ndsParticleLoadEFCommonBank / `0210dd59` | 1 | 476 | 0 | 476: 1 |
| gcGetGObjThread / `020a9271` | 1 | 464 | 4 | 464: 1 |
| ndsBaseSyTaskmanMalloc / `020a8cad` | 1 | 272 | 0 | 272: 1 |
| gcGetGObjProcess / `020a9367` | 7 | 252 | 0 | 36: 7 |
| ndsRelocRegisterLoadedFileImpl / `0206fe2f` | 7 | 196 | 0 | 2: 1, 4: 1, 6: 1, 46: 4 |
| ndsParticleLoadEFCommonBank / `0210dd65` | 1 | 188 | 0 | 188: 1 |
| ndsBaseSyTaskmanMalloc / `020a8ce5` | 1 | 112 | 0 | 112: 1 |
| ndsBaseSyTaskmanMalloc / `020a8ccb` | 1 | 88 | 0 | 88: 1 |
| ndsBaseSyTaskmanMalloc / `020a8df1` | 1 | 16 | 8 | 16: 1 |
| gcGetGObjStackOfSize / `020a92db` | 1 | 12 | 0 | 12: 1 |
| ndsBaseSyTaskmanMalloc / `020a8ecb` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `020a9011` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `020a8eff` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `020a8f19` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `020a8f3b` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `020a8f61` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `020a8f7b` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `020a8f95` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `020a8fb3` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `020a8fd3` | 1 | 0 | 0 | 0: 1 |
| ndsBaseSyTaskmanMalloc / `020a8d41` | 2 | 0 | 0 | 0: 2 |
| ndsBaseSyTaskmanMalloc / `020a8d57` | 2 | 0 | 0 | 0: 2 |

## Large requests

Seq 11: 13,824 B, align 4, `gcSetupObjman`, `D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_objman.c:537`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=13824, alignment=4) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0217b274 in dSCManagerOverlays ()
```

Seq 15: 20,000 B, align 8, `ndsBaseSyTaskmanMalloc`, `D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=20000, alignment=alignment@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a8d14 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanLoadScene (tscene=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1212
#3  0x020a8eb6 in syTaskmanInitGeneralHeap (start=<optimized out>, size=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:269
#4  ndsBaseSyTaskmanStartTask (tsetup=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1271
#5  0x020a9144 in syTaskmanStartTask (tsetup=0x22fb400) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:97
#6  0x0208bcc6 in scManagerFuncUpdate (setup=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_compat_shims.c:1461
#7  0x02063d8a in ndsOsWaitForQueue (queue=<optimized out>, wait_for_space=<optimized out>, flag=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/libultra_os.c:351
```

Seq 19: 20,000 B, align 8, `ndsBaseSyTaskmanMalloc`, `D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=20000, alignment=alignment@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a8d14 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=8) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanLoadScene (tscene=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1212
#3  0x020a8eb6 in syTaskmanInitGeneralHeap (start=<optimized out>, size=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:269
#4  ndsBaseSyTaskmanStartTask (tsetup=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1271
#5  0x020a9144 in syTaskmanStartTask (tsetup=0x22fb400) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:97
#6  0x0208bcc6 in scManagerFuncUpdate (setup=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_compat_shims.c:1461
#7  0x02063d8a in ndsOsWaitForQueue (queue=<optimized out>, wait_for_space=<optimized out>, flag=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/libultra_os.c:351
```

Seq 26: 41,984 B, align 4, `ndsAObjEvent32ConfigureNormalizedCapacity`, `D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_objanim.c:1485`.

```text
#0  syMallocSet (bp=0x2ff2aa8 <gSYTaskmanGeneralHeap>, bp@entry=0x0 <gcGetAnimTotalLength>, size=size@entry=41984, alignment=alignment@entry=4) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020aec56 in ndsAObjEvent32ConfigureNormalizedCapacity (gkind=4294967295) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_objanim.c:1485
#2  0x02ff283c in gNdsStageGCDrawAllLoopNonStageCaptureCount ()
```

Seq 27: 130,080 B, align 16, `mnVSResultsFuncStart`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3336 (discriminator 1)`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=130080, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x022952e8 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3336
#4  0x022924c8 in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#5  0x020a8e3c in syTaskmanLoadScene (tscene=tscene@entry=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1249
#6  0x020a8ffe in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1322
#7  0x020a916e in syTaskmanStartTask (tsetup=tsetup@entry=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:106
```

Seq 249: 2,280 B, align 8, `ndsBaseEFManagerInitEffects`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1740`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=2280, alignment=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x000000d0 in ndsBaseGcSetupCustomDObjsWithMObj (gobj=<optimized out>, dobjdesc=0x2277aa4 <sNdsEFDeferredCount>, p_mobjsubs=0x22e296c, dobjs=0xae918dc0, tk1=<optimized out>, tk2=<optimized out>, tk3=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/objanim.c:2210
#4  0x022640f0 in llN64LogoFileID ()
```

Seq 254: 52,736 B, align 16, `ndsBaseEFManagerInitEffects`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1754 (discriminator 1)`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=52736, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02103816 in ndsBaseEFManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1754
#4  0x0210783e in efManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:2371
#5  0x02295322 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3340
#6  0x022924c8 in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#7  0x020a8e3c in syTaskmanLoadScene (tscene=tscene@entry=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1249
```

Seq 255: 28,352 B, align 16, `ndsBaseEFManagerInitEffects`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1755 (discriminator 1)`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=28352, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02103830 in ndsBaseEFManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1755
#4  0x0210783e in efManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:2371
#5  0x02295322 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3340
#6  0x022924c8 in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#7  0x020a8e3c in syTaskmanLoadScene (tscene=tscene@entry=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1249
```

Seq 256: 13,616 B, align 16, `ndsBaseEFManagerInitEffects`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1756 (discriminator 1)`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=13616, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02103848 in ndsBaseEFManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ef/efmanager.c:1756
#4  0x0210783e in efManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:2371
#5  0x02295322 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3340
#6  0x022924c8 in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#7  0x020a8e3c in syTaskmanLoadScene (tscene=tscene@entry=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1249
```

Seq 310: 2,464 B, align 16, `ndsEFManagerInitVisualTemplates`, `D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:595`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=2464, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02107acc in efManagerInitEffects () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:2395
#4  0x02295322 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3340
#5  0x022924c8 in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#6  0x020a8e3c in syTaskmanLoadScene (tscene=tscene@entry=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1249
#7  0x020a8ffe in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1322
```

Seq 311: 33,152 B, align 8, `ndsFTManagerPoolMalloc`, `D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:79`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=33152, alignment=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x0212245c in rmutexLock (m=0x212245c <__syscall_lock_acquire_recursive+36>) at /home/davem/projects/devkitpro/pacman-packages/calico/src/calico-1.2.0/include/calico/system/mutex.h:77
#4  __syscall_lock_acquire_recursive (lock=0x212245c <__syscall_lock_acquire_recursive+36>) at /home/davem/projects/devkitpro/pacman-packages/calico/src/calico-1.2.0/source/system/newlib_syscalls.c:95
#5  0x023e6c70 in ?? ()
```

Seq 313: 2,096 B, align 16, `ndsFTManagerPoolMalloc`, `D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:79`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=2096, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x00000000 in ?? ()
```

Seq 314: 14,472 B, align 16, `ndsSceneAssetAlloc`, `D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_frontend_overlay.c:140`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=14472, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x00000000 in ?? ()
```

Seq 316: 6,560 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=6560, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02078a26 in ndsRelocAssetAllocSize (asset_id=36578968) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11930
#4  ndsRelocEnsureLoadedAsset (asset_id=36578968) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9233
#5  0x0209f0a8 in ndsRelocPatchCompactBattleMainExterns (fkind=37094576, fkind@entry=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020cbd46 in ftManagerSetupFilesAllKind (fkind=fkind@entry=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:407
#7  0x02295288 in ndsMNVSResultsSetupFilesKind (fkind=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 322: 10,752 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=10752, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02078a26 in ndsRelocAssetAllocSize (asset_id=36578986) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11930
#4  ndsRelocEnsureLoadedAsset (asset_id=36578986) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9233
#5  0x0209f0a8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020cbd46 in ftManagerSetupFilesAllKind (fkind=fkind@entry=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:407
#7  0x02295288 in ndsMNVSResultsSetupFilesKind (fkind=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 323: 14,488 B, align 16, `ndsSceneAssetAlloc`, `D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_frontend_overlay.c:140`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=14488, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02124be2 in fopen ()
#4  0x0209e54c in ndsRelocPreviewFighterLoadBegin (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:526
#5  0x0209f45e in ndsRelocLoadPreviewFighterUnlocked (fkind=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:873
#6  ndsRelocLoadPreviewFighter (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1144
#7  0x020cbc28 in ftManagerSetupFilesAllKind (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:394
```

Seq 324: 3,011 B, align 4, `ndsShieldPoseLoad`, `D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_shield_pose.c:255`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=3011, alignment=4) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x020c5cb8 in ndsShieldPoseLoad (package=0) at D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_shield_pose.c:249
#4  0x02122350 in mutexUnlock (m=0x2122350 <mutexUnlock+68>) at /home/davem/projects/devkitpro/pacman-packages/calico/src/calico-1.2.0/source/system/mutex.c:143
#5  0x00000000 in ?? ()
```

Seq 326: 6,816 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=6816, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02078a26 in ndsRelocAssetAllocSize (asset_id=36578968) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11930
#4  ndsRelocEnsureLoadedAsset (asset_id=36578968) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9233
#5  0x0209f0a8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020cbd46 in ftManagerSetupFilesAllKind (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:407
#7  0x02295288 in ndsMNVSResultsSetupFilesKind (fkind=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 328: 3,632 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=3632, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02078a26 in ndsRelocAssetAllocSize (asset_id=36578974) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11930
#4  ndsRelocEnsureLoadedAsset (asset_id=36578974) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9233
#5  0x0209f0a8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020cbd46 in ftManagerSetupFilesAllKind (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:407
#7  0x02295288 in ndsMNVSResultsSetupFilesKind (fkind=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 329: 12,160 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=12160, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02078aa0 in ndsRelocEnsureLoadedAsset (asset_id=36578974) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9281
#4  0x0209f0a8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#5  0x020cbd46 in ftManagerSetupFilesAllKind (fkind=fkind@entry=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:407
#6  0x02295288 in ndsMNVSResultsSetupFilesKind (fkind=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
#7  ndsMNVSResultsSetupFilesKind (fkind=1) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:486
```

Seq 331: 47,120 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=47120, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02078a26 in ndsRelocAssetAllocSize (asset_id=10612) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11930
#4  ndsRelocEnsureLoadedAsset (asset_id=10612, asset_id@entry=109) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9233
#5  0x020788d8 in ndsRelocApplyExternalPointerFixups (loaded=0x2261c78 <sNdsRelocLoadedFiles+1344>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9431
#6  ndsRelocFinalizeLoadedFile (loaded=loaded@entry=0x2261c78 <sNdsRelocLoadedFiles+1344>) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9556
#7  0x02078aa0 in ndsRelocEnsureLoadedAsset (asset_id=161) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9281
```

Seq 336: 21,460 B, align 16, `ndsSceneAssetAlloc`, `D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_frontend_overlay.c:140`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=21460, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x00000000 in ?? ()
```

Seq 337: 6,080 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=6080, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x00004984 in ?? ()
```

Seq 340: 3,696 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=3696, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02078a26 in ndsRelocAssetAllocSize (asset_id=36578980) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11930
#4  ndsRelocEnsureLoadedAsset (asset_id=36578980) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9233
#5  0x0209f0a8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020cbd46 in ftManagerSetupFilesAllKind (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:407
#7  0x02295288 in ndsMNVSResultsSetupFilesKind (fkind=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 341: 79,584 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=79584, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02078a26 in ndsRelocAssetAllocSize (asset_id=36578986) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11930
#4  ndsRelocEnsureLoadedAsset (asset_id=36578986) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9233
#5  0x0209f0a8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020cbd46 in ftManagerSetupFilesAllKind (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:407
#7  0x02295288 in ndsMNVSResultsSetupFilesKind (fkind=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 342: 7,040 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=7040, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02078aa0 in ndsRelocEnsureLoadedAsset (asset_id=36578986) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9281
#4  0x0209f0a8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#5  0x020cbd46 in ftManagerSetupFilesAllKind (fkind=fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:407
#6  0x02295288 in ndsMNVSResultsSetupFilesKind (fkind=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
#7  ndsMNVSResultsSetupFilesKind (fkind=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:486
```

Seq 343: 35,536 B, align 16, `ndsSceneAssetAlloc`, `D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_frontend_overlay.c:140`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=35536, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02124be2 in fopen ()
#4  0x0209e432 in ndsRelocPreviewFighterLoadBegin (fkind=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:505
#5  0x0209f43c in ndsRelocLoadPreviewFighter (fkind=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1143
#6  0x020cbd46 in ftManagerSetupFilesAllKind (fkind=8, fkind@entry=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:407
#7  0x02295288 in ndsMNVSResultsSetupFilesKind (fkind=6) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 344: 3,141 B, align 4, `ndsShieldPoseLoad`, `D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_shield_pose.c:255`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=3141, alignment=4) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x020c5cb8 in ndsShieldPoseLoad (package=5) at D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_shield_pose.c:249
#4  0x02122350 in mutexUnlock (m=0x2122350 <mutexUnlock+68>) at /home/davem/projects/devkitpro/pacman-packages/calico/src/calico-1.2.0/source/system/mutex.c:143
#5  0x00000000 in ?? ()
```

Seq 346: 9,648 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=9648, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02124b2c in _fopen_r ()
#4  0x023a1ae0 in ?? ()
```

Seq 348: 10,512 B, align 16, `ndsRelocEnsureLoadedAsset`, `D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9257`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=10512, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02078a26 in ndsRelocAssetAllocSize (asset_id=36578974) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:11930
#4  ndsRelocEnsureLoadedAsset (asset_id=36578974) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_assets.c:9233
#5  0x0209f0a8 in ndsRelocPatchCompactBattleMainExterns (fkind=fkind@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_preview_pack.c:1031
#6  0x020cbd46 in ftManagerSetupFilesAllKind (fkind=fkind@entry=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_ftmanager.c:407
#7  0x02295288 in ndsMNVSResultsSetupFilesKind (fkind=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:512
```

Seq 350: 11,792 B, align 16, `mnVSResultsFuncStart`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3349`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=11792, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02295332 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3345
#4  0x022924c8 in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#5  0x020a8e3c in syTaskmanLoadScene (tscene=tscene@entry=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1249
#6  0x020a8ffe in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1322
#7  0x020a916e in syTaskmanStartTask (tsetup=tsetup@entry=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:106
```

Seq 351: 11,792 B, align 16, `mnVSResultsFuncStart`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3349`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=11792, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02295332 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3345
#4  0x022924c8 in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#5  0x020a8e3c in syTaskmanLoadScene (tscene=tscene@entry=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1249
#6  0x020a8ffe in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1322
#7  0x020a916e in syTaskmanStartTask (tsetup=tsetup@entry=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:106
```

Seq 352: 11,792 B, align 16, `mnVSResultsFuncStart`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3349`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=11792, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02295332 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3345
#4  0x022924c8 in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#5  0x020a8e3c in syTaskmanLoadScene (tscene=tscene@entry=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1249
#6  0x020a8ffe in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1322
#7  0x020a916e in syTaskmanStartTask (tsetup=tsetup@entry=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:106
```

Seq 353: 11,792 B, align 16, `mnVSResultsFuncStart`, `D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3349`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=11792, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02295332 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3345
#4  0x022924c8 in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#5  0x020a8e3c in syTaskmanLoadScene (tscene=tscene@entry=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1249
#6  0x020a8ffe in ndsBaseSyTaskmanStartTask (tsetup=tsetup@entry=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1322
#7  0x020a916e in syTaskmanStartTask (tsetup=tsetup@entry=0x22e296c) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:106
```

Seq 354: 47,600 B, align 16, `ndsLBTransitionMalloc`, `D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_lbtransition.c:35`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=47600, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x020f8614 in ndsBaseLBTransitionSetupTransition () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/lb/lbtransition.c:216
#4  0x020f86be in lbTransitionSetupTransition () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_lbtransition.c:70
#5  0x02295376 in mnVSResultsFuncStart () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:3362
#6  0x022924c8 in ndsMNVSResultsFuncStartTimed () at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_mnvsresults.c:432
#7  0x020a8e3c in syTaskmanLoadScene (tscene=tscene@entry=0x22e296c, func_start=0x22924a9 <ndsMNVSResultsFuncStartTimed>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:1249
```

Seq 475: 30,160 B, align 16, `ndsSceneAssetAlloc`, `D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_frontend_overlay.c:140`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=30160, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x00000064 in ndsBaseFTManagerSetupFileSize () at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/ft/ftmanager.c:94
```

Seq 604: 19,196 B, align 16, `ndsSceneAssetAlloc`, `D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_frontend_overlay.c:140`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=19196, alignment=16) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x020a6c7c in ndsSceneAssetAlloc (bytes=<optimized out>, alignment=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_frontend_overlay.c:140
#4  0x03007de0 in ?? ()
```

Seq 691: 4,216 B, align 8, `gcGetGObjStackOfSize`, `D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/objman.c:199`.

```text
#0  syMallocSet (bp=bp@entry=0x2ff2aa8 <gSYTaskmanGeneralHeap>, size=4216, alignment=8) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_malloc.c:217
#1  0x020a9234 in ndsBaseSyTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/builds/build-p2p8-playtest-r55/battleship_overlay/src/sys/taskman.c:275
#2  syTaskmanMalloc (size=<optimized out>, align=<optimized out>) at D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_sys_taskman.c:253
#3  0x02292848 in mnVSResultsSetFighterStatus (fighter_gobj=0x226a96c <sGCThreadStackHead>, player=player@entry=3) at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:916
#4  0x02294b7c in mnVSResultsInitFighter (player=3) at D:/Stuff/DevFolder/Smash64DS_Port/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c:2762
#5  0x022772f8 in sMNVSResultsFiles ()
```

# Task 25R authoritative same-artifact matrix

Source HEAD: `f088db98de272e9788405c2181029ad4a4c353ba`  
Profile-1 ROM: `6E90D4140E6332E8F37BB05CB8B35ED192AAB448B26E110916992F2C15701921`  
Recommendation: **M3-first**  

| Phase | Frame / logic window | Exact | Loop P50/P95/max | Deficit P50/P95/max | Update P95 | Wallpaper P95 | Stage P95 | Mario/Fox P95 | 2/3/4/5+ | Present/s | Update/s |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| countdown438-445 | 438-445 / 484-498 | True | 1680448/1680512/1680512 | 560068/560132/560132 | 501056 | 351296 | 464320 | 171392/209536 | 0/8/0/0 | 20 | 40 |
| early600-607 | 600-607 / 808-822 | False | 1680448/1680448/1680448 | 560068/560068/560068 | 445824 | 362880 | 464448 | 9984/197056 | 0/8/0/0 | 20 | 40 |
| whispy1398-1405 | 1398-1405 / 2404-2418 | False | 1680448/2240640/2240640 | 560068/1120260/1120260 | 839552 | 358592 | 464576 | 171456/209856 | 0/6/2/0 | 18.462 | 36.923 |
| late3300-3307 | 1846-1853 / 3300-3314 | False | 1120256/1120320/1120320 | -124/-60/-60 | 212480 | 2432 | 464384 | 171520/208896 | 8/0/0/0 | 30 | 60 |
| natural-ko | 566-573 / 740-754 | False | 1120256/1680512/1680512 | -124/560132/560132 | 487296 | 327360 | 468480 | 7616/197824 | 6/2/0/0 | 26.667 | 53.333 |
| natural-rebirth | 589-596 / 786-800 | False | 1680448/2240640/2240640 | 560068/1120260/1120260 | 853184 | 324032 | 464192 | 9920/197696 | 0/7/1/0 | 19.2 | 38.4 |
| timeup-results | 1988-1995 / 3584-3598 | False | 1120256/1680448/1680448 | -124/560068/560068 | 630976 | 2432 | 464320 | 171392/210240 | 7/1/0/0 | 28.235 | 56.471 |

## Profile-1 exactness

- early600-607: geometry samples=0,1,2,3,4,5,6,7 observed=91/508/202/0/306 fallback=0/0/0 expected=121/828/202/320/306 and 0/0/0
- whispy1398-1405: static residency/fence samples=0,1,2,3,4,5,6,7
- late3300-3307: static residency/fence samples=0,1,2,3,4,5,6,7
- natural-ko: geometry samples=0,1,2,3,4,5,6,7 observed=91/508/202/0/306 fallback=0/0/0 expected=121/828/202/320/306 and 0/0/0
- natural-rebirth: geometry samples=0,1,2,3,4,5,6,7 observed=91/508/202/0/306 fallback=0/0/0 expected=121/828/202/320/306 and 0/0/0
- timeup-results: static residency/fence samples=0,1,2,3,4,5,6,7

## Detailed profile-1 rows

| Timer | Class / parent | countdown438-445 | early600-607 | whispy1398-1405 | late3300-3307 | natural-ko | natural-rebirth | timeup-results |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| input | child / wholeLoop | 1600/1600/1600/8 | 1536/1600/1600/8 | 1536/1664/1664/8 | 1568/1664/1664/8 | 1536/1664/1664/8 | 1568/1664/1664/8 | 1600/1600/1600/8 |
| sourceUpdate1 | child / sourceUpdateAggregate | 100192/100928/100928/8 | 72576/74240/74240/8 | 108096/731584/731584/8 | 99392/113280/113280/8 | 97600/107648/107648/8 | 75040/775360/775360/8 | 99296/522240/522240/8 |
| sourceUpdate2 | child / sourceUpdateAggregate | 100352/100800/100800/8 | 72384/73536/73536/8 | 107232/396224/396224/8 | 99040/100352/100352/8 | 97056/116032/116032/8 | 74784/75328/75328/8 | 99104/107392/107392/8 |
| sourceUpdateAggregate | child / updateBatch | 200416/201536/201536/8 | 145120/146496/146496/8 | 215584/837248/837248/8 | 198304/210176/210176/8 | 194912/223680/223680/8 | 149792/850304/850304/8 | 198176/628736/628736/8 |
| audioUpdateShell | child / updateBatch / updateBatch-sourceUpdateAggregate | 2144/300864/300864/8 | 2688/300928/300928/8 | 2304/2304/2304/8 | 2304/2496/2496/8 | 3200/301888/301888/8 | 2752/301632/301632/8 | 2240/2240/2240/8 |
| updateBatch | child / wholeLoop | 202656/501056/501056/8 | 147968/445824/445824/8 | 217856/839552/839552/8 | 200576/212480/212480/8 | 198784/487296/487296/8 | 152416/853184/853184/8 | 200416/630976/630976/8 |
| frameBegin | child / presentActive | 192/192/192/8 | 128/192/192/8 | 128/192/192/8 | 128/192/192/8 | 128/192/192/8 | 128/192/192/8 | 128/192/192/8 |
| wallpaper | child / draw | 286176/351296/351296/8 | 340544/362880/362880/8 | 320448/358592/358592/8 | 2368/2432/2432/8 | 73888/327360/327360/8 | 319968/324032/324032/8 | 2400/2432/2432/8 |
| stage | child / draw | 464128/464320/464320/8 | 464352/464448/464448/8 | 464288/464576/464576/8 | 464256/464384/464384/8 | 468256/468480/468480/8 | 463904/464192/464192/8 | 464128/464320/464320/8 |
| mario | child / draw | 169696/171392/171392/8 | 9920/9984/9984/8 | 169728/171456/171456/8 | 169824/171520/171520/8 | 7584/7616/7616/8 | 9920/9920/9920/8 | 169728/171392/171392/8 |
| fox | child / draw | 209120/209536/209536/8 | 196992/197056/197056/8 | 209312/209856/209856/8 | 208448/208896/208896/8 | 197696/197824/197824/8 | 197632/197696/197696/8 | 208224/210240/210240/8 |
| foregroundEffects | child / draw | 0/0/0/8 | 0/0/0/8 | 0/0/0/8 | 0/0/0/8 | 0/0/0/8 | 0/0/0/8 | 0/0/0/8 |
| drawResidual | child / draw | 19744/19968/19968/8 | 50240/50368/50368/8 | 19552/19840/19840/8 | 19648/20480/20480/8 | 51008/51200/51200/8 | 50080/50240/50240/8 | 19744/49664/49664/8 |
| draw | child / presentActive | 1147136/1215360/1215360/8 | 1061856/1084480/1084480/8 | 1182016/1220032/1220032/8 | 863584/866048/866048/8 | 798528/1051904/1051904/8 | 1041536/1045248/1045248/8 | 864416/894784/894784/8 |
| gxFlush | child / presentActive | 64/64/64/8 | 64/64/64/8 | 64/64/64/8 | 64/64/64/8 | 64/64/64/8 | 64/64/64/8 | 64/64/64/8 |
| postVBlank | child / presentActive | 384/384/384/8 | 384/448/448/8 | 384/384/384/8 | 384/384/384/8 | 384/384/384/8 | 384/384/384/8 | 384/384/384/8 |
| runnableThreads | child / presentActive | 1632/1664/1664/8 | 1664/1664/1664/8 | 1632/1664/1664/8 | 1600/1664/1664/8 | 1600/1664/1664/8 | 1664/1856/1856/8 | 1600/1664/1664/8 |
| presentActive | child / wholeLoop | 1152000/1220288/1220288/8 | 1066784/1089280/1089280/8 | 1186912/1224896/1224896/8 | 868384/870912/870912/8 | 803424/1056768/1056768/8 | 1047552/1303808/1303808/8 | 869312/899712/899712/8 |
| vblankWait | child / wholeLoop | 318368/408704/408704/8 | 458848/482048/482048/8 | 257120/389824/389824/8 | 46944/50624/50624/8 | 121664/421056/421056/8 | 476544/486144/486144/8 | 48096/146624/146624/8 |
| loopResidual | child / wholeLoop | 1600/1728/1728/8 | 1600/1664/1664/8 | 1632/1664/1664/8 | 1632/1728/1728/8 | 1600/1728/1728/8 | 1600/1728/1728/8 | 1568/1728/1728/8 |
| wholeLoop | root | 1680448/1680512/1680512/8 | 1680448/1680448/1680448/8 | 1680448/2240640/2240640/8 | 1120256/1120320/1120320/8 | 1120256/1680512/1680512/8 | 1680448/2240640/2240640/8 | 1120256/1680448/1680448/8 |
| hud | child / presentActive | 1184/1216/1216/8 | 1184/1216/1216/8 | 1216/1216/1216/8 | 1216/1216/1216/8 | 1216/1216/1216/8 | 1184/267264/267264/8 | 1216/1216/1216/8 |
| presentResidual | child / presentActive | 1408/1600/1600/8 | 1472/1600/1600/8 | 1472/1536/1536/8 | 1472/1600/1600/8 | 1472/1536/1536/8 | 1472/1600/1600/8 | 1504/1536/1536/8 |
| audioBackend | nested / audioUpdateShell | 1920/300672/300672/8 | 2432/300736/300736/8 | 2048/2176/2176/8 | 2048/2240/2240/8 | 2944/301632/301632/8 | 2496/301376/301376/8 | 1984/2048/2048/8 |

## Ranked leaf owners by P95

- countdown438-445: stage=464320, vblankWait=408704, wallpaper=351296, audioUpdateShell=300864, fox=209536, mario=171392, sourceUpdate1=100928, sourceUpdate2=100800, drawResidual=19968, loopResidual=1728, runnableThreads=1664, presentResidual=1600, input=1600, hud=1216, postVBlank=384, frameBegin=192, gxFlush=64, foregroundEffects=0
- early600-607: vblankWait=482048, stage=464448, wallpaper=362880, audioUpdateShell=300928, fox=197056, sourceUpdate1=74240, sourceUpdate2=73536, drawResidual=50368, mario=9984, runnableThreads=1664, loopResidual=1664, presentResidual=1600, input=1600, hud=1216, postVBlank=448, frameBegin=192, gxFlush=64, foregroundEffects=0
- whispy1398-1405: sourceUpdate1=731584, stage=464576, sourceUpdate2=396224, vblankWait=389824, wallpaper=358592, fox=209856, mario=171456, drawResidual=19840, audioUpdateShell=2304, runnableThreads=1664, input=1664, loopResidual=1664, presentResidual=1536, hud=1216, postVBlank=384, frameBegin=192, gxFlush=64, foregroundEffects=0
- late3300-3307: stage=464384, fox=208896, mario=171520, sourceUpdate1=113280, sourceUpdate2=100352, vblankWait=50624, drawResidual=20480, audioUpdateShell=2496, wallpaper=2432, loopResidual=1728, runnableThreads=1664, input=1664, presentResidual=1600, hud=1216, postVBlank=384, frameBegin=192, gxFlush=64, foregroundEffects=0
- natural-ko: stage=468480, vblankWait=421056, wallpaper=327360, audioUpdateShell=301888, fox=197824, sourceUpdate2=116032, sourceUpdate1=107648, drawResidual=51200, mario=7616, loopResidual=1728, runnableThreads=1664, input=1664, presentResidual=1536, hud=1216, postVBlank=384, frameBegin=192, gxFlush=64, foregroundEffects=0
- natural-rebirth: sourceUpdate1=775360, vblankWait=486144, stage=464192, wallpaper=324032, audioUpdateShell=301632, hud=267264, fox=197696, sourceUpdate2=75328, drawResidual=50240, mario=9920, runnableThreads=1856, loopResidual=1728, input=1664, presentResidual=1600, postVBlank=384, frameBegin=192, gxFlush=64, foregroundEffects=0
- timeup-results: sourceUpdate1=522240, stage=464320, fox=210240, mario=171392, vblankWait=146624, sourceUpdate2=107392, drawResidual=49664, wallpaper=2432, audioUpdateShell=2240, loopResidual=1728, runnableThreads=1664, input=1600, presentResidual=1536, hud=1216, postVBlank=384, frameBegin=192, gxFlush=64, foregroundEffects=0

## Production profile-0 lifecycle

- Presentations/source updates: 2042/4084
- Presentations/s and updates/s: 18.6 / 37.3
- Interval histogram 2/3/4/5+: 61/1547/396/38
- Slip events: 2457
- Reserve: 166672/131072 bytes (pass=True)
- Exact KO FGM triplet: False
- Zero post-GO texture fence: False
- Synchronized pixels: 0/49,152 changed
- Stable-30 ready: False

| Lifecycle phase | 2 | 3 | 4 | 5+ | Slips |
|---|---:|---:|---:|---:|---:|
| countdown | 0 | 190 | 4 | 0 | 198 |
| earlyCombat | 17 | 606 | 253 | 24 | 1188 |
| lateCombat | 0 | 747 | 139 | 14 | 1067 |
| koRebirth | 0 | 0 | 0 | 0 | 0 |
| results | 44 | 4 | 0 | 0 | 4 |

## Task bounds

- Task 20R: stack/DTCM only; no timing candidate before lifecycle reserve and stack evidence
- Tasks 21R/27: Mario+Fox P95 <= 380544 ticks in countdown438-445.
- Task 22R: wallpaper P95 <= 362880 ticks.
- Tasks 23R/26: stage P95 <= 468480 ticks in natural-ko.

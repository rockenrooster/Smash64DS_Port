| kind | LOW per instance (texel B / palette B / textures) | LOW all palettes | HIGH per instance | other-file textures (B / n) | runtime keys (B) | runtime keys outside LOW enumeration |
|---|---:|---:|---:|---:|---:|---:|
| Mario | 5,120 / 192 / 6 | 9,216 / 448 / 14 | 5,120 / 192 / 6 | 1,472 / 5 | - | - |
| Fox | 3,872 / 256 / 8 | 6,944 / 448 / 14 | 3,872 / 256 / 8 | 44,896 / 50 | - | - |
| Donkey | 11,776 / 448 / 14 | 56,832 / 2,112 / 66 | 9,216 / 480 / 15 | 1,792 / 4 | 14 (11,776) | 0 |
| Samus | 11,008 / 768 / 24 | 50,688 / 2,944 / 92 | 11,008 / 800 / 25 | 2,304 / 3 | 19 (9,728) | 0 |
| Luigi | 6,144 / 224 / 7 | 14,336 / 576 / 18 | 4,096 / 160 / 5 | 1,216 / 5 | - | - |
| Link | 22,208 / 1,216 / 38 | 23,744 / 1,312 / 41 | 21,984 / 1,216 / 38 | 14,144 / 9 | 27 (15,488) | 0 |
| Yoshi | 1,184 / 320 / 10 | 16,320 / 4,224 / 132 | 160 / 64 / 2 | 48,128 / 64 | - | - |
| Captain | 5,984 / 544 / 17 | 35,648 / 3,008 / 94 | 6,208 / 640 / 20 | 10,880 / 15 | - | - |
| Kirby | 2,272 / 256 / 8 | 11,904 / 1,088 / 34 | 10,528 / 928 / 29 | 7,232 / 16 | 6 (3,072) | 4 |
| Pikachu | 5,376 / 416 / 13 | 26,880 / 2,080 / 65 | 5,376 / 416 / 13 | 8,832 / 8 | - | - |
| Purin | 2,816 / 224 / 7 | 14,080 / 1,120 / 35 | 1,792 / 160 / 5 | 11,776 / 6 | - | - |
| Ness | 9,120 / 512 / 16 | 12,480 / 896 / 28 | 9,056 / 448 / 14 | 7,168 / 5 | - | - |

hats {'Mario': [512, 32, 1], 'Fox': [0, 0, 0], 'Donkey': [1152, 96, 3], 'Samus': [1920, 128, 4], 'Luigi': [512, 32, 1], 'Link': [128, 32, 1], 'Yoshi': [0, 0, 0], 'Captain': [416, 96, 3], 'Pikachu': [864, 128, 4], 'Purin': [0, 0, 0], 'Ness': [32, 32, 1]}
heaviest by body (LOW per instance + palettes):
  57,056  (+other files 25,408)  Donkey + Samus + Link + Ness
  55,584  (+other files 25,472)  Donkey + Samus + Link + Kirby
  53,952  (+other files 29,120)  Donkey + Samus + Link + Captain
  53,792  (+other files 19,456)  Donkey + Samus + Luigi + Link
  53,216  (+other files 27,072)  Donkey + Samus + Link + Pikachu
heaviest by body + other-file textures:
  153,632  (body 35,584 + other 118,048)  Fox + Link + Yoshi + Captain
  153,024  (body 38,688 + other 114,336)  Fox + Link + Yoshi + Ness
  151,040  (body 32,096 + other 118,944)  Fox + Link + Yoshi + Purin
  150,848  (body 34,848 + other 116,000)  Fox + Link + Yoshi + Pikachu
  150,304  (body 40,832 + other 109,472)  Fox + Samus + Link + Yoshi
stress roster: body 55,584 other 25,472

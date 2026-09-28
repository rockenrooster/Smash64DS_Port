#!/bin/bash
# usage: [SRCROOT=<mirror of src+include>] cc_scene.sh <cfgdir> <out.o> [extra gcc args...]
# compile-only (no link, no make); output stays in the scratchpad.
CFG="$1"; OUT="$2"; shift 2
GCC=/c/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe
REPO=D:/Stuff/DevFolder/Smash64DS_Port
ROOT="${SRCROOT:-$REPO}"
"$GCC" -std=gnu11 -g0 -Wall -Wextra -O2 -ffunction-sections -fdata-sections -march=armv5te -mtune=arm946e-s -mthumb \
  -I$ROOT/include -I$REPO/decomp/BattleShip-main/decomp/src -I$REPO/decomp/BattleShip-main/decomp/src/sys \
  -isystem C:/devkitPro/libnds/include -I"$CFG" -DARM9 -D_LANGUAGE_C -DSSB64_TARGET_NDS -DREGION_US -DAVOID_UB \
  -Wno-error=incompatible-pointer-types -Wno-error=int-conversion -Wno-error=maybe-uninitialized -Wundef -fmax-errors=8 \
  -include "$CFG/nds_build_config.h" -D__NDS__ -IC:/devkitPro/calico/include "$@" \
  -c $ROOT/src/port/scene_backend.c -o "$OUT"

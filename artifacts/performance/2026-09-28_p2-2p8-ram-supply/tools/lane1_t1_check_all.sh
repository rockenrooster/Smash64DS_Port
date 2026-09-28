#!/bin/bash
# Compile scene_backend.c under six configs; flag-off objects must equal the pre-change baseline byte for byte.
SPW="C:/Users/Tyler/AppData/Local/Temp/claude/D--Stuff-DevFolder-Smash64DS-Port/31e5c78d-38d0-4d75-b683-91723230d2cc/scratchpad/t1"
cd "$(dirname "$0")"
SIZE=/c/devkitPro/devkitARM/bin/arm-none-eabi-size.exe
run() { # name cfg out
  ./cc_scene.sh "$SPW/$2" "out/$3.o" > "out/$3.warn" 2>&1; echo "$3 rc=$? warnings=$(grep -c 'warning:' out/$3.warn)"; }
run shell_u  cfg0    shell_u
run shell_f0 cfgf0   shell_f0
run shell_f1 cfgf1   shell_f1
run four_f0  cfg4_0  four_f0
run four_f1  cfg4_1  four_f1
run onep_f0  cfg1p_0 onep_f0
run onep_f1  cfg1p_1 onep_f1
for pair in "snap_scene_backend:shell_u" "snap_scene_backend:shell_f0" "snap4:four_f0" "snap1p:onep_f0"; do b=${pair%%:*}; o=${pair##*:}; if cmp -s base/$b.o out/$o.o; then echo "IDENTICAL   base/$b.o == out/$o.o"; else echo "DIFFERS     base/$b.o != out/$o.o"; fi; done
$SIZE base/snap_scene_backend.o out/shell_f1.o base/snap4.o out/four_f1.o base/snap1p.o out/onep_f1.o

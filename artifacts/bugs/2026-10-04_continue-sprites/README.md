# 1P Continue screen (walk sweep, 2026-10-04)

The 1P campaign sweep (all 12 fighters, walk ROM) reached the Continue screen
(scene 49) after every lost battle. Kirby halted there after Master Hand
(stage 13): `HALT malloc-overflow scene=49` in `gcAddMObjForDObj`, general heap
free 116 B at entry. Link, Ness and Purin recorded native fill-sink failures on
the same screen, and every fighter showed the screen below.

Probe: scratchpad `contcap.ps1` (walk ROM, the player's fighter dropped below
the blast zone every 200 presented frames from frame 400 until the stocks run
out; a capture every 15 Continue tics with BLDCNT/BLDALPHA, the native failure
count and general-heap free bytes). BLDY is write-only and always reads 0.

## Before (cc02, walk-all3)

`cc02/t150.png`: navy backdrop, the spotlight drawn as an opaque grey cylinder,
no fighter, one NO_PROGRAM failure a frame from the three fade rectangles.

## Causes

1. The scene reserved two 32 KiB graphics heaps and a 48 KiB RDP buffer (the
   native renderer uses neither) before making the fighter, so Kirby's parts
   ran the arena out.
2. The fallen figure (`mnPlayers1PGameContinueMakeFighter`, a Demo actor made
   with `ftManagerMakeFighter` alone) is never in the live fighter registry,
   and the transient fighter submit admitted only Intro, Ending and
   Challenger.
3. BG0 (3D) was off for the scene; the spotlight and light pool are I4
   coverage discs the source blends, presented as opaque OBJs; the three fade
   procs wrote fill rectangles with no DS program.

## After (cc05 and cr01, walk-all5)

- The scene takes the battle graphics-heap/RDP sizes (keeps its source display
  lists): free 108,484 B through tic 240, no halt.
- Kirby draws (`cc05/t060.png` falling into the spotlight, `cc05/t150.png`
  lying in it under CONTINUE? / YES / NO).
- Black backdrop; the spotlight and pool are semi-transparent OBJs in their
  prim colour between the room (BG2) and the fighter (BG0); the fades are the
  2D blend from the alphas the source procs step: BLDCNT 0x25c4, BLDALPHA
  steps 0x0100 -> 0x0d03 as the lights come up. 0 native failures.
- `cr01`: YES at the walk's A press, the retried Master Hand battle starts
  with the standing blend back (BLDCNT 0x0441, BLDALPHA 0x1010), 0 failures,
  30 FPS (`cr01/battle120.png`).

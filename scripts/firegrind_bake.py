"""Offline table generator for the DS-native FireGrind effect.

Bakes the source-derived color ramps and initial velocities into constants so the
DS path never runs trig, atan2, sqrt, or per-frame color interpolation.

Source of truth:
  decomp/BattleShip-main/decomp/src/ef/efmanager.c:5287  (FireGrind root)
  decomp/BattleShip-main/decomp/src/lb/lbparticle.c:2320  (generator spawn)
  decomp/BattleShip-main/decomp/src/lb/lbparticle.c:1155  (SETPRIMBLEND lerp)

Confirmed source values (script 0x0B -> generators 8/9/10):
  gen8: vel=(0,0,80),  size120, start(255,255,200) target(255,208,98) prim_len17
  gen9: vel=(0,0,90),  size130, start(254,254,73)  target(247,156,28)  prim_len25
  gen10:vel=(0,0,100), size140, start(243,145,17)  target(255,75,20)   prim_len25

Texture 5 is 16x16 I4, three frames. The DS atlas packs only frame 0 (frames 1/2
decimate to it), so texture animation is frozen at frame 0 regardless.
"""

import math

variants = [
    # (variant_id, speed, start_rgb, target_rgb, prim_length)
    (0, 80.0, (255, 255, 200), (255, 208, 98), 17),
    (1, 90.0, (254, 254, 73), (247, 156, 28), 25),
    (2, 100.0, (243, 145, 17), (255, 75, 20), 25),
]

def bgr555(r, g, b):
    # Match the DS draw path EXACTLY (battleship_lbparticle.c:1951-1953):
    # it folds an 8-bit primcolour channel to 5 bits with `channel >> 3` (floor),
    # so the baked native colours are bit-identical to what the generic
    # interpreter path draws for the same source RGB at the same age.
    return ((r >> 3)
            | ((g >> 3) << 5)
            | ((b >> 3) << 10))

def color_table():
    """Presentation-frame index -> source age 2*i -> lerped primcolor -> BGR555."""
    print("/* sNdsFireGrindColors[variant][frame_index], BGR555.")
    print(" * Source lerps primcolor start->target over prim_length ticks;")
    print(" * presentation advances age by 2 source ticks/frame, so i = age 2*i. */")
    for vid, _, start, target, plen in variants:
        row = []
        for i in range(13):
            age = i * 2
            t = min(1.0, age / plen)
            r = round(start[0] + (target[0] - start[0]) * t)
            g = round(start[1] + (target[1] - start[1]) * t)
            b = round(start[2] + (target[2] - start[2]) * t)
            row.append(f"0x{bgr555(r, g, b):04X}")
        print(f"  /* variant {vid} */ {{ {', '.join(row)} }},")
    print()

def velocity_table():
    """Source generator spawn cone, evaluated offline.

    Source (lbparticle.c:2358-2419) for kind 0 with generator vel=(0,0,speed):
      angle1 = atan2(vel.y, vel.z) = atan2(0, speed) = 0
      angle2 = atan2(vel.x, ...)    = atan2(0, ...)    = 0
    so the rotation into world frame is IDENTITY. The per-particle velocity is
    then literally:
      vel_x = cos(pv0) * sin(pv1) * magnitude
      vel_y = sin(pv0) * sin(pv1) * magnitude
      vel_z = cos(pv1) * magnitude
    where pv0 = azimuth (around the depth axis), pv1 = elevation, magnitude =
    speed. So the DOMINANT thrust is along Z (forward, into the screen); X/Y carry
    the azimuthal spread; gravity (source line 1386: vel.y -= gravity) pulls Y
    down over the spark's short life. Source-Z maps to DS-Z 1:1 (the renderer
    reads world Z for depth), so NO axis remapping -- just sample the cone at the
    source's elevation band.

    Eight azimuths per variant; a cheap incrementing burst serial picks one per
    spawn for variation without runtime trig.
    """
    print("/* sNdsFireGrindInitialVel[variant][dir], world-space. Representative")
    print(" * samples of the source spawn cone (lbparticle.c:2358-2419) at the")
    print(" * variant's speed, evaluated offline. Dominant thrust is +Z (into the")
    print(" * screen); X/Y carry azimuthal spread; gravity arcs Y down. No trig. */")
    for vid, speed, _, _, _ in variants:
        row = []
        for d in range(8):
            azim = (d / 8.0) * 2.0 * math.pi  # 0..2pi, 8 octants
            elev = math.radians(25.0)          # inside the source's spread band
            mag = speed
            pm1 = math.sin(elev) * mag
            vz = math.cos(elev) * mag
            vx = math.cos(azim) * pm1
            vy = math.sin(azim) * pm1
            row.append(f"{{ {vx:.3f}F, {vy:.3f}F, {vz:.3f}F }}")
        print(f"  /* variant {vid}, speed {speed:.0f} */")
        print("  { " + ", ".join(row) + " },")

color_table()
velocity_table()

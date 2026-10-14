# Explosive Flame (card): the cast, then the fire at the distance the stick
# picked: at flag2 (the twinkle) nds_p4_palutena.c makes it, a weapon of its
# own -- the Fire Flower's burn, then the blast -- and flag2's value is the
# blast's sound. Not a projectile: nothing reflects it. Owner playtest
# 2026-10-14: her "Explosive flame!" call (Ultimate's side-special voice), a
# twinkle on the spot before it goes off, the blast twice the size; no burst
# sound (round 2: Ultimate's se special_N01 is Autoreticle's ringing); round
# 3: a hit on her after the twinkle no longer cancels the fire, the burn
# holds a fighter as the Fire Flower's flames do, the blast 1.25x, the cast
# to the blast 0.85x (twinkle 12 -> 10, blast 34 -> 29).
AsyncWait frames=4
PlayVoiceStoreInfo value=cue:special_s01
Effect joint_id=18 effect_id=31 flag=0 off_x=35 off_y=146 off_z=-160 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=10
SetFlag2 value=cue:se_special_n02
End value=0

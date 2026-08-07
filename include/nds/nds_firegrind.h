#ifndef NDS_FIREGRIND_H
#define NDS_FIREGRIND_H

#include <ssb_types.h>

/* DS-native FireGrind, the ground-rebound spark a Mario fireball leaves behind.
 *
 * The source (ef/efmanager.c:5287 efManagerFireGrindMakeEffect) builds a whole
 * BattleShip particle hierarchy per bounce: one root LBParticle (script 0x0B),
 * one LBTransform, three generators (8/9/10), and six visible sparks that each
 * run the generic bytecode interpreter for colour lerp + gravity/friction +
 * random texture frame. The three generators differ only in size/speed/colour
 * band (120/130/140, 80/90/100, three warm ramps), and texture 5 is a 16x16 I4
 * with three frames already resident in the DS particle atlas.
 *
 * This replaces that hierarchy with a fixed pool of three source-derived quads
 * per bounce -- one per generator band -- drawn inside the existing particle GX
 * batch. No root particle, no generators, no transform, no bytecode, no runtime
 * trig/atan2/sqrt, no runtime texture conversion, no dynamic allocation. Three
 * quads instead of six. Gated by NDS_R2_FIREGRIND_NATIVE; owner-playtested and
 * accepted 2026-08-07, so the native path is on by default.
 *
 * The draw is owned by lbParticleDrawTextures (battleship_lbparticle.c), which
 * has the open particle batch and the camera right/up axes; this subsystem only
 * owns the pool and exposes it read-only for that pass.
 *
 * Source values confirmed byte-for-byte against the .scb bank: generators 8/9/10
 * carry texture 5, sizes 120/130/140, gravity 3, friction 0.8, vel (0,0,80/90/
 * 100), primcolour ramps as baked below. The DS atlas packs only texture-5 frame
 * 0 (frames 1/2 decimate to it via the nearest-earlier-frame rule), so sparks
 * are frozen at frame 0 -- the SAME presentation the generic path already draws.
 */

#define NDS_FIREGRIND_MAX_PARTICLES 24u
/* One presentation-frame index per 2 source ticks; 13 entries cover the 0..24
 * source-tick colour ramp used by every variant. */
#define NDS_FIREGRIND_COLOR_FRAMES 13u

typedef struct NDSFireGrindParticle
{
    Vec3f pos;
    Vec3f vel;

    /* Presentation-frame index into sNdsFireGrindColors, NOT source age.
     * Increments by 1 per presented frame (== 2 source ticks). */
    u8 age;
    /* Source-tick lifetime ceiling for this spark (10..26, baked at spawn). */
    u8 lifetime;
    /* Variant 0/1/2 == source generators 8/9/10. */
    u8 variant;
} NDSFireGrindParticle;

/* Called from efManagerFireGrindMakeEffect under the flag. Spawns exactly three
 * sparks -- one per source variant -- at pos, or drops the whole burst if fewer
 * than three slots remain (no partial 1- or 2-spark effect). Returns nothing:
 * the Mario fireball caller ignores the original LBParticle* anyway. */
void ndsFireGrindSpawn(const Vec3f *pos);

/* Advance the whole pool one presentation frame. Two source physics iterations
 * are combined into one update, so pos/vel move by the squared-friction step and
 * age advances by one presentation index. Call once per drawn frame from the
 * particle link the source used (GENLINK(0)); a paused/skipped particle link
 * must NOT advance, so this is separate from the draw. */
void ndsFireGrindUpdate(void);

/* Reset the pool to empty. Called from efParticleInitAll beside the other
 * particle-pool resets so a scene/match restart drops every live spark. */
void ndsFireGrindReset(void);

/* Read-only view for the draw pass. *count_out receives the live count; the
 * returned pointer is the packed [0, count) array and stays valid until the next
 * spawn/update/reset. Draw iterates it and submits into the open GX batch. */
const NDSFireGrindParticle *ndsFireGrindPool(u32 *count_out);

/* BGR555 colour for a spark, looked up by variant and presentation age. Caps at
 * the last ramp entry so a long-lived spark holds its final colour. */
u16 ndsFireGrindColor(u8 variant, u8 age);

/* Source size for a variant (120/130/140). */
f32 ndsFireGrindSize(u8 variant);

#endif /* NDS_FIREGRIND_H */

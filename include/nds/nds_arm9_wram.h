#ifndef NDS_ARM9_WRAM_H
#define NDS_ARM9_WRAM_H

/* P2-2p8 (NDS_P2_ARM9_WRAM): ARM9's 32 KB of shared WRAM. src/nds/main.c
 * sets WRAMCNT = 0 and maps it cached at 0x03000000; ARM7 links in its
 * private WRAM (linker/nds_arm7_ds7_arm9wram.ld). Every user here is a
 * scene-lifetime block handed out again after each scene's reset, never a
 * static:
 *
 *   BASE ..        the fighter FTStruct pool (ftManagerAllocFighter)
 *   POSE_BASE ..   one pose slot's joints + tracks per pose fighter
 *                  (nds_ft_pose.c)
 *   OBJ_BASE ..    one GObj + one DObj per battle fighter
 *                  (ndsGcDonateFighterObjs)
 */
#define NDS_ARM9_WRAM_BASE 0x03000000u
#define NDS_ARM9_WRAM_SIZE 0x8000u

/* Four FTStructs are 12,048 B; a larger pool stays on the heap. */
#define NDS_ARM9_WRAM_FTSTRUCT_POOL_MAX 0x3000u

#define NDS_ARM9_WRAM_POSE_SLOTS 4u
#define NDS_ARM9_WRAM_POSE_SLOT_BYTES 4408u
#define NDS_ARM9_WRAM_POSE_BASE                                                 \
    (NDS_ARM9_WRAM_BASE + NDS_ARM9_WRAM_FTSTRUCT_POOL_MAX)

#define NDS_ARM9_WRAM_OBJ_SLOTS 4u
#define NDS_ARM9_WRAM_GOBJ_BYTES 136u
#define NDS_ARM9_WRAM_DOBJ_BYTES 136u
#define NDS_ARM9_WRAM_OBJ_BASE                                                  \
    (NDS_ARM9_WRAM_BASE + NDS_ARM9_WRAM_SIZE -                                  \
     NDS_ARM9_WRAM_OBJ_SLOTS *                                                  \
         (NDS_ARM9_WRAM_GOBJ_BYTES + NDS_ARM9_WRAM_DOBJ_BYTES))

_Static_assert(NDS_ARM9_WRAM_POSE_BASE +
                   NDS_ARM9_WRAM_POSE_SLOTS * NDS_ARM9_WRAM_POSE_SLOT_BYTES <=
                   NDS_ARM9_WRAM_OBJ_BASE,
               "ARM9 WRAM pose slots overlap the object slots");

/* Push the next free WRAM GObj and DObj slot onto the source free lists, so
 * the next gcGetGObjSetNextAlloc / gcGetDObjSetNextAlloc takes them. */
void ndsGcDonateFighterObjs(void);

#endif

#ifndef NDS_ARM9_WRAM_H
#define NDS_ARM9_WRAM_H

/* P2-2p8 (NDS_P2_ARM9_WRAM): ARM9's 16 KB shared-WRAM block. src/nds/main.c
 * sets WRAMCNT = 2 and maps it cached at 0x03000000; ARM7 links above it
 * (linker/nds_arm7_ds7_wram16.ld). Every user here is a scene-lifetime block
 * handed out again after each scene's reset, never a static:
 *
 *   BASE ..               the fighter FTStruct pool (ftManagerAllocFighter)
 *   OBJ_BASE .. BASE+SIZE one GObj + one DObj per battle fighter
 *                         (ndsGcDonateFighterObjs)
 */
#define NDS_ARM9_WRAM_BASE 0x03000000u
#define NDS_ARM9_WRAM_SIZE 0x4000u

#define NDS_ARM9_WRAM_OBJ_SLOTS 4u
#define NDS_ARM9_WRAM_GOBJ_BYTES 136u
#define NDS_ARM9_WRAM_DOBJ_BYTES 136u
#define NDS_ARM9_WRAM_OBJ_BASE                                                  \
    (NDS_ARM9_WRAM_BASE + NDS_ARM9_WRAM_SIZE -                                  \
     NDS_ARM9_WRAM_OBJ_SLOTS *                                                  \
         (NDS_ARM9_WRAM_GOBJ_BYTES + NDS_ARM9_WRAM_DOBJ_BYTES))

/* The FTStruct pool must end below the object slots. */
#define NDS_ARM9_WRAM_FTSTRUCT_POOL_MAX (NDS_ARM9_WRAM_OBJ_BASE - NDS_ARM9_WRAM_BASE)

/* Push the next free WRAM GObj and DObj slot onto the source free lists, so
 * the next gcGetGObjSetNextAlloc / gcGetDObjSetNextAlloc takes them. */
void ndsGcDonateFighterObjs(void);

#endif

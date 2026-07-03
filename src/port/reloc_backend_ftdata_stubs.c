#include <PR/ultratypes.h>
#include <stdint.h>

/*
 * Particle ROM banks are not present in the current DS O2R manifest. Keep the
 * ftdata descriptors linkable; particle-bank runtime remains a documented stub.
 */
s32 particles_unk0_scb_ROM_START;
s32 particles_unk0_scb_ROM_END;
s32 particles_unk0_txb_ROM_START;
s32 particles_unk0_txb_ROM_END;
s32 particles_unk1_scb_ROM_START;
s32 particles_unk1_scb_ROM_END;
s32 particles_unk1_txb_ROM_START;
s32 particles_unk1_txb_ROM_END;
s32 particles_unk2_scb_ROM_START;
s32 particles_unk2_scb_ROM_END;
s32 particles_unk2_txb_ROM_START;
s32 particles_unk2_txb_ROM_END;

uintptr_t llKirbyMainMotionSpecialNFTKirbyCopy = 0u;

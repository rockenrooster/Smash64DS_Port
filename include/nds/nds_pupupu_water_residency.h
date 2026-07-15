#ifndef SSB64_NDS_PUPUPU_WATER_RESIDENCY_H
#define SSB64_NDS_PUPUPU_WATER_RESIDENCY_H

#include <PR/ultratypes.h>

#ifndef NDS_RENDERER_M4_WATER_TILED_AOT
#define NDS_RENDERER_M4_WATER_TILED_AOT 0
#endif

#define NDS_PUPUPU_WATER_RESIDENCY_TEXTURE_COUNT 2u
#define NDS_PUPUPU_WATER_RESIDENCY_PALETTE_COUNT 40u
#define NDS_PUPUPU_WATER_RESIDENCY_RESOURCE_COUNT 42u
#define NDS_PUPUPU_WATER_RESIDENCY_TEXTURE_BYTES 147456u
#define NDS_PUPUPU_WATER_RESIDENCY_PALETTE_BYTES 20480u
#define NDS_PUPUPU_WATER_RESIDENCY_SCRATCH_BYTES 4096u

enum NDSPupupuWaterResidencyFailure
{
    nNDSPupupuWaterResidencyFailureNone,
    nNDSPupupuWaterResidencyFailureDisabled,
    nNDSPupupuWaterResidencyFailureOpen,
    nNDSPupupuWaterResidencyFailureScratch,
    nNDSPupupuWaterResidencyFailureResource,
    nNDSPupupuWaterResidencyFailureTexture,
    nNDSPupupuWaterResidencyFailureTextureBank,
    nNDSPupupuWaterResidencyFailureRead,
    nNDSPupupuWaterResidencyFailurePalette,
    nNDSPupupuWaterResidencyFailurePayloadHash
};

/* Prepare is the only I/O/allocation/upload entry point. It is idempotent for
 * one residency lifetime; Reset releases that lifetime. Gameplay calls only
 * the two atlas bind functions, which select already-resident resources and
 * the requested shared RGB256 palette. */
s32 ndsPupupuWaterResidencyPrepare(void);
void ndsPupupuWaterResidencyReset(void);
s32 ndsPupupuWaterResidencyBindPrimary(u32 palette_index);
s32 ndsPupupuWaterResidencyBindSecondary(u32 palette_index);

extern volatile u32 gNdsPupupuWaterResidencyPrepared;
extern volatile u32 gNdsPupupuWaterResidencyPrepareCount;
extern volatile u32 gNdsPupupuWaterResidencyPrepareSuccessCount;
extern volatile u32 gNdsPupupuWaterResidencyPrepareFailCount;
extern volatile u32 gNdsPupupuWaterResidencyResetCount;
extern volatile u32 gNdsPupupuWaterResidencyPayloadBytes;
extern volatile u32 gNdsPupupuWaterResidencyTextureBytes;
extern volatile u32 gNdsPupupuWaterResidencyPaletteBytes;
extern volatile u32 gNdsPupupuWaterResidencyTextureCount;
extern volatile u32 gNdsPupupuWaterResidencyPaletteCount;
extern volatile u32 gNdsPupupuWaterResidencyScratchBytes;
extern volatile u32 gNdsPupupuWaterResidencyPrimaryTextureAddress;
extern volatile u32 gNdsPupupuWaterResidencySecondaryTextureAddress;
extern volatile u32 gNdsPupupuWaterResidencyPaletteFirstByte;
extern volatile u32 gNdsPupupuWaterResidencyPaletteEndByte;
extern volatile u32 gNdsPupupuWaterResidencyPrimaryBindCount;
extern volatile u32 gNdsPupupuWaterResidencySecondaryBindCount;
extern volatile u32 gNdsPupupuWaterResidencyBindFailCount;
extern volatile u32 gNdsPupupuWaterResidencyLastFailure;

#endif

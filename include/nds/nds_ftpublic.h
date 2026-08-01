#ifndef SSB64_NDS_FTPUBLIC_H
#define SSB64_NDS_FTPUBLIC_H

#include <PR/ultratypes.h>

/* BUGS.md crowd row instruments. Present only at
 * NDS_IMPORT_BATTLESHIP_FT_PUBLIC=1; a soak that names them on a ROM without
 * the flag reports missing symbols rather than zeros, which is the intended
 * difference between "the actor is not built" and "the actor ran and stayed
 * quiet". */
extern volatile u32 gNdsFtPublicActorMakeCount;
extern volatile u32 gNdsFtPublicProcUpdateCount;
extern volatile u32 gNdsFtPublicCommonCheckCount;
extern volatile u32 gNdsFtPublicPlayCommonCount;
extern volatile u32 gNdsFtPublicLastCommonFGM;
extern volatile u32 gNdsFtPublicCallStartCount;
extern volatile u32 gNdsFtPublicLastCallFGM;

#endif /* SSB64_NDS_FTPUBLIC_H */

#include "nds_renderer_preamble.c"
#include "nds_renderer_assets.c"
#include "nds_renderer_dl_core.c"
#include "nds_renderer_textures_effects.c"
#include "nds_renderer_native_common.c"
#include "nds_renderer_native_owners.c"
#if NDS_P2_STAGE_INISHIE
#include "nds_native_inishie_pakkun.generated.inc"
#include "nds_native_inishie_pakkun.exec.inc"
#endif
#if NDS_P2_STAGE_SECTOR
#include "generated/nds_native_sector_arwing_laser.generated.inc"
#include "nds_native_sector_arwing_laser.exec.inc"
#endif
#if NDS_P2_STAGE_CASTLE
#include "generated/nds_native_castle_bumper.generated.inc"
#include "nds_native_castle_bumper.exec.inc"
#endif
#include "generated/nds_native_samus_chargeshot.generated.inc"
#include "nds_native_samus_chargeshot.exec.inc"
#include "nds_renderer_native_fighter_production.c"
#include "nds_renderer_dispatch_profile.c"

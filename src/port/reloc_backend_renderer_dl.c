/* The renderer adapters of the relocation backend, in order: the matrix
 * adapter, the DL draw state, the stage adapter (its packet, lanes and
 * general submit), the fighter DObj collections and the fighter adapter. A
 * slice of reloc_backend.c. */

#include "renderer_adapter_matrix.c"
#include "renderer_adapter_matrix_math.c"
#include "renderer_adapter_matrix_world.c"
#include "renderer_adapter_matrix_owner.c"
#include "renderer_adapter_legacy_dl_probes.c"
#include "renderer_adapter_stage.c"
#include "renderer_adapter_stage_packet.c"
#include "renderer_adapter_stage_lanes.c"
#include "renderer_adapter_stage_submit.c"
#include "renderer_adapter_legacy_raster_probes.c"
#include "renderer_adapter_fighter.c"

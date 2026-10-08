/* P4 data: every compiled content's build-generated descriptors
 * (scripts/p4/generate_p4_fighter.py writes nds_p4_<name>.generated.c under
 * the ignored build tree, because every value derives from the user's ROM).
 * scripts/p4/p4_contents.py lists them; their symbols carry the content's
 * title, so they share this translation unit. */
#include <nds/nds_p4.h>

#if NDS_P4
#include "nds_p4_contents_data.generated.inc"
#endif

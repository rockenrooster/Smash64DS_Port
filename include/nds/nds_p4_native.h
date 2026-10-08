#ifndef NDS_P4_NATIVE_H
#define NDS_P4_NATIVE_H

/* P4 native owners: donor fighter models compiled by the existing owner
 * generators (scripts/p4/p4_native_owner.py, one run for every compiled
 * content) into a build-local image header, runtime programs and NitroFS
 * images. Their image slots append after the original cast's
 * (NDS_NATIVE_IMAGE_SLOT_<NAME>) and their renderer owner slots after Master
 * Hand's (NDS_P4_RENDERER_OWNER_SLOT, nds_p4_contents.h). */

#include <nds_build_config.h>
#include <nds/generated/nds_native_fighter_image.generated.h>
#include <nds/nds_p4_contents.h>

#if NDS_P4
#include <nds_p4_native_image.generated.h>
#define NDS_NATIVE_IMAGE_OWNER_SLOTS_ALL \
    (NDS_NATIVE_IMAGE_OWNER_SLOTS + NDS_P4_NATIVE_IMAGE_SLOTS)
#else
#define NDS_NATIVE_IMAGE_OWNER_SLOTS_ALL NDS_NATIVE_IMAGE_OWNER_SLOTS
#define NDS_P4_NATIVE_OWNER_IMAGE_ROWS(X)
#endif

#endif /* NDS_P4_NATIVE_H */

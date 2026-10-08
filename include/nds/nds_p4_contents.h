#ifndef NDS_P4_CONTENTS_H
#define NDS_P4_CONTENTS_H

/* The compiled P4 contents (scripts/p4/p4_contents.py from
 * scripts/p4/contents.json, docs/P4/New_Characters.md S1).
 * NDS_P4_CONTENT_ROWS(X) calls X(id, Title, NAME, name, parent_kind,
 * model_file_id, main_file_id) once per compiled content; every per-content
 * site in shared code expands it rather than naming a fighter. This header
 * carries only the list, so renderer headers can include it. */

#include <nds_build_config.h>

#ifndef NDS_P4
#define NDS_P4 0
#endif

#if NDS_P4
#include <nds_p4_contents.generated.h>
#else
#define NDS_P4_CONTENT_ROWS(X)
#define NDS_P4_CONTENT_MAX_ID 0
#endif

/* Renderer native-owner slots: content id i owns slot
 * NDS_P4_RENDERER_OWNER_BASE + i, after Master Hand's. */
#define NDS_P4_RENDERER_OWNER_BASE 24u
#define NDS_P4_RENDERER_OWNER_SLOT(id_) (NDS_P4_RENDERER_OWNER_BASE + (u32)(id_))

#endif /* NDS_P4_CONTENTS_H */

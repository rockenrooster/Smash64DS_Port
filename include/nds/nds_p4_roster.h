#ifndef NDS_P4_ROSTER_H
#define NDS_P4_ROSTER_H

#include <PR/ultratypes.h>

/* Preserve the source FTKind enum, including EnumCount=27 and Null=28.
 * Content selection and UI tables use their own compact index domain. */
#define NDS_P4_RUNTIME_METAKNIGHT 29u
#define NDS_P4_LEGACY_SELECTION_COUNT 12u
#define NDS_P4_UI_METAKNIGHT 12u
#define NDS_P4_NO_SELECTION_INDEX 0xffffffffu

static inline u32 ndsRosterSelectionIndex(u32 kind)
{
    if (kind < NDS_P4_LEGACY_SELECTION_COUNT) return kind;
#if NDS_P4_METAKNIGHT
    if (kind == NDS_P4_RUNTIME_METAKNIGHT) return NDS_P4_UI_METAKNIGHT;
#endif
    return NDS_P4_NO_SELECTION_INDEX;
}

static inline u32 ndsRosterRuntimeKind(u32 selection)
{
    if (selection < NDS_P4_LEGACY_SELECTION_COUNT) return selection;
#if NDS_P4_METAKNIGHT
    if (selection == NDS_P4_UI_METAKNIGHT) return NDS_P4_RUNTIME_METAKNIGHT;
#endif
    return 28u;
}

/* Native cue identity is supplied by the source-qualified audio producer. */
u32 ndsP4MetaKnightAnnouncerID(void);

#endif

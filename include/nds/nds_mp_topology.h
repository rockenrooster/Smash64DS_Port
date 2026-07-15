#ifndef SSB64_NDS_MP_TOPOLOGY_H
#define SSB64_NDS_MP_TOPOLOGY_H

#include <stdint.h>

/*
 * O2R retains each N64 32-bit word as a host-order word.  Its two logical
 * halfwords therefore read high then low; odd-sized records must carry an
 * absolute halfword offset instead of advancing a C struct pointer.
 */
typedef struct NDSMPO2RHalfwordView
{
    const uint32_t *words;
    uint32_t first_half;
} NDSMPO2RHalfwordView;

#define NDS_MP_LINE_INFO_HALFWORDS 9u

static inline uint16_t ndsMPO2RReadU16Kernel(const void *base,
                                             uint32_t half_index)
{
    const uint32_t *words = (const uint32_t *)base;
    uint32_t word = words[half_index / 2u];

    return (half_index & 1u) ? (uint16_t)(word & 0xffffu) :
        (uint16_t)(word >> 16);
}

static inline int16_t ndsMPO2RReadS16Kernel(const void *base,
                                            uint32_t half_index)
{
    return (int16_t)ndsMPO2RReadU16Kernel(base, half_index);
}

static inline NDSMPO2RHalfwordView ndsMPO2RMakeHalfwordView(
    const void *base, uint32_t first_half)
{
    NDSMPO2RHalfwordView view;

    view.words = (const uint32_t *)base;
    view.first_half = first_half;
    return view;
}

static inline uint16_t ndsMPO2RReadViewU16(NDSMPO2RHalfwordView view,
                                           uint32_t relative_half)
{
    return ndsMPO2RReadU16Kernel(view.words,
                                 view.first_half + relative_half);
}

static inline NDSMPO2RHalfwordView ndsMPO2RLineInfoView(
    const void *line_info_base, uint32_t record_index)
{
    return ndsMPO2RMakeHalfwordView(
        line_info_base, record_index * NDS_MP_LINE_INFO_HALFWORDS);
}

static inline uint16_t ndsMPO2RLineInfoYakumonoID(
    NDSMPO2RHalfwordView line_info)
{
    return ndsMPO2RReadViewU16(line_info, 0u);
}

static inline uint16_t ndsMPO2RLineInfoGroupID(
    NDSMPO2RHalfwordView line_info, uint32_t kind)
{
    return ndsMPO2RReadViewU16(line_info, 1u + (kind * 2u));
}

static inline uint16_t ndsMPO2RLineInfoLineCount(
    NDSMPO2RHalfwordView line_info, uint32_t kind)
{
    return ndsMPO2RReadViewU16(line_info, 2u + (kind * 2u));
}

/*
 * Scalar form of BattleShip func_ovl2_800FB31C.  Adjacency is defined by
 * shared endpoint vertex IDs, not by coordinates.  The ascending line scan
 * and last-match-wins behavior intentionally match the original runtime.
 */
typedef struct NDSMPTopologyRawLine
{
    int kind;
    int first_vertex_id;
    int last_vertex_id;
    int first_x;
    int first_y;
    int last_x;
    int last_y;
} NDSMPTopologyRawLine;

typedef int (*NDSMPTopologyReadLine)(void *context, int line_id,
                                     NDSMPTopologyRawLine *line);

static inline int ndsMPTopologyResolveEdgesKernel(
    void *context, int line_count, int source_line_id,
    NDSMPTopologyReadLine read_line, int *edge_prev_line_id,
    int *edge_next_line_id, int *prev_match_count, int *next_match_count)
{
    NDSMPTopologyRawLine source;
    int line_prev = -1;
    int line_next = -1;
    int prev_matches = 0;
    int next_matches = 0;
    int is_reverse = 0;
    int line_id;

    if (edge_prev_line_id != 0)
    {
        *edge_prev_line_id = -1;
    }
    if (edge_next_line_id != 0)
    {
        *edge_next_line_id = -1;
    }
    if (prev_match_count != 0)
    {
        *prev_match_count = 0;
    }
    if (next_match_count != 0)
    {
        *next_match_count = 0;
    }
    if ((read_line == 0) || (line_count <= 0) ||
        (source_line_id < 0) || (source_line_id >= line_count) ||
        (read_line(context, source_line_id, &source) == 0) ||
        (source.kind < 0) || (source.kind > 3))
    {
        return 0;
    }

    for (line_id = 0; line_id < line_count; line_id++)
    {
        NDSMPTopologyRawLine other;

        if ((line_id == source_line_id) ||
            (read_line(context, line_id, &other) == 0))
        {
            continue;
        }
        if ((source.first_vertex_id == other.first_vertex_id) ||
            (source.first_vertex_id == other.last_vertex_id))
        {
            line_prev = line_id;
            prev_matches++;
        }
        if ((source.last_vertex_id == other.first_vertex_id) ||
            (source.last_vertex_id == other.last_vertex_id))
        {
            line_next = line_id;
            next_matches++;
        }
    }

    if ((source.kind == 0) || (source.kind == 1))
    {
        is_reverse = (source.last_x < source.first_x) ? 1 : 0;
    }
    else
    {
        is_reverse = (source.last_y < source.first_y) ? 1 : 0;
    }
    if (edge_prev_line_id != 0)
    {
        *edge_prev_line_id = (is_reverse != 0) ? line_next : line_prev;
    }
    if (edge_next_line_id != 0)
    {
        *edge_next_line_id = (is_reverse != 0) ? line_prev : line_next;
    }
    if (prev_match_count != 0)
    {
        *prev_match_count = prev_matches;
    }
    if (next_match_count != 0)
    {
        *next_match_count = next_matches;
    }
    return 1;
}

#endif

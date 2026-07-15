#include <nds/nds_mp_topology.h>

#define O2R_PAIR(a, b) \
    (((uint32_t)(uint16_t)(a) << 16) | (uint32_t)(uint16_t)(b))

typedef struct MPTopologyFixtureContext
{
    const NDSMPTopologyRawLine *lines;
    int count;
} MPTopologyFixtureContext;

static int mpTopologyFixtureRead(void *context, int line_id,
                                 NDSMPTopologyRawLine *line)
{
    MPTopologyFixtureContext *fixture = context;

    if ((fixture == 0) || (line == 0) || (line_id < 0) ||
        (line_id >= fixture->count))
    {
        return 0;
    }
    *line = fixture->lines[line_id];
    return 1;
}

static int mpTopologyCheckTable(const NDSMPTopologyRawLine *lines,
                                int line_count, const int *expected_prev,
                                const int *expected_next,
                                int expected_shared, int expected_orphans,
                                int expected_reversed, int expected_ambiguous)
{
    MPTopologyFixtureContext context = { lines, line_count };
    int shared = 0;
    int orphans = 0;
    int reversed = 0;
    int ambiguous = 0;
    int line_id;

    for (line_id = 0; line_id < line_count; line_id++)
    {
        int edge_prev;
        int edge_next;
        int prev_matches;
        int next_matches;

        if (ndsMPTopologyResolveEdgesKernel(&context, line_count, line_id,
                mpTopologyFixtureRead, &edge_prev, &edge_next,
                &prev_matches, &next_matches) == 0)
        {
            return 100 + line_id;
        }
        if ((edge_prev != expected_prev[line_id]) ||
            (edge_next != expected_next[line_id]))
        {
            return 200 + line_id;
        }
        shared += prev_matches + next_matches;
        orphans += (prev_matches == 0) ? 1 : 0;
        orphans += (next_matches == 0) ? 1 : 0;
        ambiguous += (prev_matches > 1) ? 1 : 0;
        ambiguous += (next_matches > 1) ? 1 : 0;
        if (((lines[line_id].kind < 2) &&
             (lines[line_id].last_x < lines[line_id].first_x)) ||
            ((lines[line_id].kind >= 2) &&
             (lines[line_id].last_y < lines[line_id].first_y)))
        {
            reversed++;
        }
    }
    if ((shared != expected_shared) || (orphans != expected_orphans) ||
        (reversed != expected_reversed) ||
        (ambiguous != expected_ambiguous))
    {
        return 300;
    }
    return 0;
}

static int mpTopologyCheckMultiRecordLineInfo(void)
{
    static const uint32_t line_info[] = {
        O2R_PAIR(1, 0),
        O2R_PAIR(4, 4),
        O2R_PAIR(1, 5),
        O2R_PAIR(1, 6),
        O2R_PAIR(1, 2),
        O2R_PAIR(7, 2),
        O2R_PAIR(9, 3),
        O2R_PAIR(12, 4),
        O2R_PAIR(16, 5)
    };
    NDSMPO2RHalfwordView first = ndsMPO2RLineInfoView(line_info, 0u);
    NDSMPO2RHalfwordView second = ndsMPO2RLineInfoView(line_info, 1u);

    if ((ndsMPO2RLineInfoYakumonoID(first) != 1u) ||
        (ndsMPO2RLineInfoGroupID(first, 0u) != 0u) ||
        (ndsMPO2RLineInfoLineCount(first, 0u) != 4u) ||
        (ndsMPO2RLineInfoGroupID(first, 3u) != 6u) ||
        (ndsMPO2RLineInfoLineCount(first, 3u) != 1u))
    {
        return 1;
    }
    if ((second.first_half != 9u) ||
        (ndsMPO2RLineInfoYakumonoID(second) != 2u) ||
        (ndsMPO2RLineInfoGroupID(second, 0u) != 7u) ||
        (ndsMPO2RLineInfoLineCount(second, 0u) != 2u) ||
        (ndsMPO2RLineInfoGroupID(second, 1u) != 9u) ||
        (ndsMPO2RLineInfoLineCount(second, 1u) != 3u) ||
        (ndsMPO2RLineInfoGroupID(second, 2u) != 12u) ||
        (ndsMPO2RLineInfoLineCount(second, 2u) != 4u) ||
        (ndsMPO2RLineInfoGroupID(second, 3u) != 16u) ||
        (ndsMPO2RLineInfoLineCount(second, 3u) != 5u))
    {
        return 2;
    }
    return 0;
}

int smash64dsMPTopologyFixture(void)
{
    static const NDSMPTopologyRawLine pupupu[] = {
        { 0, 16, 18,   570,  1542,  -570,  1542 },
        { 0, 13, 15,  1892,   907,   951,   907 },
        { 0, 10, 12,  -951,   904, -1841,   904 },
        { 0,  5,  6, -2318,     0,  2318,     0 },
        { 1,  0,  1,  1972, -1072, -1972, -1072 },
        { 2,  6,  0,  2318,     0,  1972, -1072 },
        { 3,  1,  5, -1972, -1072, -2318,     0 }
    };
    static const int pupupu_prev[] = { -1, -1, -1, 6, 6, 4, 4 };
    static const int pupupu_next[] = { -1, -1, -1, 5, 5, 3, 3 };
    static const NDSMPTopologyRawLine same_coords_different_ids[] = {
        { 0, 100, 101, 0, 0, 10, 0 },
        { 2, 102, 103, 0, 0, 10, 0 }
    };
    static const int no_edges[] = { -1, -1 };
    static const NDSMPTopologyRawLine last_match_wins[] = {
        { 0, 1, 2, 0, 0, 10, 0 },
        { 2, 1, 3, 0, 0, 0, 10 },
        { 3, 1, 4, 0, 0, 0, 10 }
    };
    MPTopologyFixtureContext fanout_context = {
        last_match_wins, 3
    };
    int edge_prev;
    int edge_next;
    int prev_matches;
    int next_matches;
    int result;

    result = mpTopologyCheckMultiRecordLineInfo();
    if (result != 0)
    {
        return 10 + result;
    }

    result = mpTopologyCheckTable(pupupu, 7, pupupu_prev, pupupu_next,
                                  8, 6, 5, 0);
    if (result != 0)
    {
        return result;
    }
    result = mpTopologyCheckTable(same_coords_different_ids, 2, no_edges,
                                  no_edges, 0, 4, 0, 0);
    if (result != 0)
    {
        return 400 + result;
    }
    if ((ndsMPTopologyResolveEdgesKernel(&fanout_context, 3, 0,
            mpTopologyFixtureRead, &edge_prev, &edge_next,
            &prev_matches, &next_matches) == 0) ||
        (edge_prev != 2) || (edge_next != -1) ||
        (prev_matches != 2) || (next_matches != 0))
    {
        return 500;
    }
    return 0;
}

#if MP_TOPOLOGY_HOST_MAIN
#include <stdio.h>

int main(void)
{
    int result = smash64dsMPTopologyFixture();

    if (result != 0)
    {
        fprintf(stderr, "mp topology fixture failed: %d\n", result);
        return result;
    }
    printf("mp topology fixtures passed: Pupupu 7/8/6/5/0\n");
    return 0;
}
#endif

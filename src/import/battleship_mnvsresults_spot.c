/* The VS Results podium spot, with the N64's reading of an out-of-range index.
 *
 * Source mnVSResultsGetSpot (decomp mn/mnvsmode/mnvsresults.c:965-971) is
 *
 *     sb32 aheads[] = { 0, 0, 1, 1 };
 *     sb32 places[] = { 0, 0, 1, 1, 1 };
 *     return place + aheads[place - mnVSResultsGetPlayerCountAhead(player)] +
 *            places[mnVSResultsGetPlayerCountPlace(place)];
 *
 * and `place` is a DENSE rank (mnVSResultsSetRoyalPlace counts score changes),
 * so whenever players tie above someone, more players are ahead than the place
 * number: scores 3,3,1 give places 0,0,1 and the third player reads aheads[-1].
 * A tie is ordinary in a time match, so this runs in normal play.
 *
 * IDO lays a frame's locals out downward in declaration order, so `places`
 * sits directly below `aheads` and aheads[-1..-3] read places[4..2] -- all 1.
 * GCC puts `aheads` lowest, so on the DS the same read took a stack padding
 * word, the spot came back as garbage, and mnVSResultsSetPlayerTagPosition
 * data-aborted indexing its position table with it (owner r75: Sector Z,
 * four level-9 CPUs, Results "froze afterwards"; reproduced 2026-10-04).
 *
 * The included source definition is declared weak in battleship_mnvsresults.c,
 * so every call resolves here. One table holds both arrays in the N64's order;
 * the index range is place - ahead in [-3, 3], i.e. table[2..8]. */
#include <PR/ultratypes.h>

typedef s32 nds_sb32_spot;

extern s32 sMNVSResultsPlaces[];
s32 mnVSResultsGetPlayerCountAhead(s32 player);
s32 mnVSResultsGetPlayerCountPlace(s32 place);

s32 mnVSResultsGetSpot(s32 player)
{
    static const nds_sb32_spot frame[] = {
        0, 0, 1, 1, 1, /* places */
        0, 0, 1, 1     /* aheads */
    };
    const nds_sb32_spot *places = &frame[0];
    const nds_sb32_spot *aheads = &frame[5];
    s32 place = sMNVSResultsPlaces[player];

    return place + aheads[place - mnVSResultsGetPlayerCountAhead(player)] +
           places[mnVSResultsGetPlayerCountPlace(place)];
}

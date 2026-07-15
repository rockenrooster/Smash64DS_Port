#ifndef SSB64_NDS_MP_FLOOR_CROSSING_H
#define SSB64_NDS_MP_FLOOR_CROSSING_H

/*
 * O2R-safe scalar adapter for BattleShip's mpCollisionCheckFCSurfaceFlat and
 * mpCollisionCheckFloorSurfaceTilt policies.  The stored endpoint order is
 * preserved by callers so source vertex flags and normals retain provenance.
 */
static inline int ndsMPFloorSegmentCrossesDownwardKernel(
    float position_x, float position_y,
    float translate_x, float translate_y,
    float v1_x, float v1_y, float v2_x, float v2_y,
    float *hit_x, float *hit_y)
{
    const float epsilon = 0.001F;
    float sx;
    float sy;
    float min_x;
    float max_x;
    float min_y;
    float max_y;

    if ((hit_x == 0) || (hit_y == 0))
    {
        return 0;
    }
    sx = v2_x - v1_x;
    sy = v2_y - v1_y;
    if (sx == 0.0F)
    {
        return 0;
    }
    min_x = (v1_x < v2_x) ? v1_x : v2_x;
    max_x = (v1_x > v2_x) ? v1_x : v2_x;
    min_y = (v1_y < v2_y) ? v1_y : v2_y;
    max_y = (v1_y > v2_y) ? v1_y : v2_y;

    if (sy == 0.0F)
    {
        float delta_y;
        float x;

        if ((translate_y >= position_y) ||
            (position_y < (v1_y - epsilon)) ||
            (translate_y >= v1_y))
        {
            return 0;
        }
        delta_y = position_y - translate_y;
        x = (((v1_y - position_y) / delta_y) *
            (position_x - translate_x)) + position_x;
        if ((x < min_x) || (x > max_x))
        {
            return 0;
        }
        *hit_x = x;
        *hit_y = v1_y;
        return 1;
    }
    else
    {
        float motion_dx = position_x - translate_x;
        float motion_dy = position_y - translate_y;
        float raw_prev;
        float raw_curr;
        float orient;
        float extent_epsilon;
        float prev_height_scaled;
        float curr_height_scaled;
        float surface_prev;

        if (motion_dy > 0.0F)
        {
            if (((max_y + epsilon) < translate_y) ||
                (position_y < (min_y - epsilon)))
            {
                return 0;
            }
        }
        else if (((max_y + epsilon) < position_y) ||
                 (translate_y < (min_y - epsilon)))
        {
            return 0;
        }
        if (motion_dx > 0.0F)
        {
            if ((max_x < translate_x) || (position_x < min_x))
            {
                return 0;
            }
        }
        else if ((max_x < position_x) || (translate_x < min_x))
        {
            return 0;
        }

        raw_prev = (sx * (position_y - v1_y)) -
            (sy * (position_x - v1_x));
        raw_curr = (sx * (translate_y - v1_y)) -
            (sy * (translate_x - v1_x));
        orient = (sx > 0.0F) ? 1.0F : -1.0F;
        extent_epsilon = epsilon * (orient * sx);
        prev_height_scaled = orient * raw_prev;
        curr_height_scaled = orient * raw_curr;
        if (curr_height_scaled > -extent_epsilon)
        {
            return 0;
        }
        surface_prev = v1_y + (((position_x - v1_x) / sx) * sy);
        if (prev_height_scaled < extent_epsilon)
        {
            if ((prev_height_scaled > -extent_epsilon) &&
                (position_x >= min_x) && (position_x <= max_x))
            {
                *hit_x = position_x;
                *hit_y = surface_prev;
                return 1;
            }
            return 0;
        }
        else
        {
            float denominator = raw_prev - raw_curr;
            float numerator;
            float t;
            float u;

            if (denominator == 0.0F)
            {
                return 0;
            }
            t = raw_prev / denominator;
            numerator = ((v1_x - position_x) *
                    (translate_y - position_y)) -
                ((v1_y - position_y) *
                    (translate_x - position_x));
            u = numerator / denominator;
            if ((t < -epsilon) || (t > (1.0F + epsilon)) ||
                (u < -epsilon) || (u > (1.0F + epsilon)))
            {
                return 0;
            }
            if (u < 0.0F)
            {
                u = 0.0F;
            }
            else if (u > 1.0F)
            {
                u = 1.0F;
            }
            *hit_x = v1_x + (sx * u);
            *hit_y = v1_y + (sy * u);
            return 1;
        }
    }
}

#endif

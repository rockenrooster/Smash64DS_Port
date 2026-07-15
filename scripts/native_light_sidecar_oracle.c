#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum
{
    FRAC_BITS = 12,
    ONE = 1 << FRAC_BITS,
    OWNER_COUNT = 2,
    MAX_JOINTS = 27,
    MAX_BINDINGS = 18,
    SCENARIO_COUNT = 256,
    NORMAL_COUNT = 8,
    NO_PARENT = 0xff
};

typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint32_t u32;

typedef struct Matrix4
{
    s32 m[4][4];
} Matrix4;

typedef struct Matrix3
{
    s32 m[3][3];
} Matrix3;

typedef struct Direction
{
    s32 x;
    s32 y;
    s32 z;
} Direction;

typedef struct OwnerPlan
{
    const char *name;
    u32 joint_count;
    u32 binding_count;
    const u8 *parents;
    const u8 *binding_joints;
} OwnerPlan;

static const u8 sMarioParents[25] =
{
    NO_PARENT, 0, 1, 2, 3, 4, 5, 6, 3, 8, 3, 10, 11,
    12, 13, 2, 15, 16, 17, 18, 2, 20, 21, 22, 23
};

static const u8 sMarioBindingJoints[14] =
{
    3, 5, 6, 7, 9, 11, 12, 13, 16, 17, 19, 21, 22, 24
};

static const u8 sFoxParents[27] =
{
    NO_PARENT, 0, 1, 2, 3, 4, 5, 6, 3, 8, 3, 10, 11,
    12, 13, 2, 15, 16, 17, 18, 2, 20, 21, 22, 23, 2, 25
};

static const u8 sFoxBindingJoints[18] =
{
    2, 3, 5, 6, 7, 8, 9, 11, 12, 13, 16, 17, 19, 21,
    22, 24, 25, 26
};

static const OwnerPlan sOwners[OWNER_COUNT] =
{
    { "Mario", 25, 14, sMarioParents, sMarioBindingJoints },
    { "Fox", 27, 18, sFoxParents, sFoxBindingJoints }
};

static const int8_t sNormals[NORMAL_COUNT][3] =
{
    { 0, 0, 127 },
    { 0, 0, -128 },
    { 127, 0, 0 },
    { -128, 127, 0 },
    { 64, -64, 32 },
    { -1, 1, -1 },
    { 73, 91, -37 },
    { -95, -17, 81 }
};

static s32 clamp_s64_to_s32(s64 value)
{
    if (value > INT_MAX)
    {
        return INT_MAX;
    }
    if (value < INT_MIN)
    {
        return INT_MIN;
    }
    return (s32)value;
}

static s64 round_shift_s64(s64 value, u32 shift)
{
    s64 bias;

    if (shift == 0)
    {
        return value;
    }
    bias = (s64)(1u << (shift - 1u));
    if (value < 0)
    {
        return -(((-value) + bias) >> shift);
    }
    return (value + bias) >> shift;
}

static void identity4(Matrix4 *out)
{
    u32 i;

    memset(out, 0, sizeof(*out));
    for (i = 0; i < 4; i++)
    {
        out->m[i][i] = ONE;
    }
}

static void identity3(Matrix3 *out)
{
    u32 i;

    memset(out, 0, sizeof(*out));
    for (i = 0; i < 3; i++)
    {
        out->m[i][i] = ONE;
    }
}

static void copy3_from4(const Matrix4 *source, Matrix3 *out)
{
    u32 row;
    u32 col;

    for (row = 0; row < 3; row++)
    {
        for (col = 0; col < 3; col++)
        {
            out->m[row][col] = source->m[row][col];
        }
    }
}

static void multiply4(const Matrix4 *lhs, const Matrix4 *rhs, Matrix4 *out)
{
    Matrix4 temp;
    u32 row;
    u32 col;
    u32 k;

    for (row = 0; row < 4; row++)
    {
        for (col = 0; col < 4; col++)
        {
            s64 sum = 0;

            for (k = 0; k < 4; k++)
            {
                sum += (s64)lhs->m[row][k] * rhs->m[k][col];
            }
            temp.m[row][col] = clamp_s64_to_s32(
                round_shift_s64(sum, FRAC_BITS));
        }
    }
    *out = temp;
}

static void multiply3(const Matrix3 *lhs, const Matrix3 *rhs, Matrix3 *out)
{
    Matrix3 temp;
    u32 row;
    u32 col;

    for (row = 0; row < 3; row++)
    {
        for (col = 0; col < 3; col++)
        {
            s64 sum = (s64)lhs->m[row][0] * rhs->m[0][col] +
                (s64)lhs->m[row][1] * rhs->m[1][col] +
                (s64)lhs->m[row][2] * rhs->m[2][col];

            temp.m[row][col] = clamp_s64_to_s32(
                round_shift_s64(sum, FRAC_BITS));
        }
    }
    *out = temp;
}

static s32 signed_choice(u32 value, s32 magnitude)
{
    return ((value & 1u) != 0u) ? magnitude : -magnitude;
}

static void make_local(u32 owner, u32 joint, u32 scenario, Matrix4 *out)
{
    u32 family = scenario & 7u;
    u32 variant = scenario >> 3;
    u32 selector = owner * 37u + joint * 11u + variant * 5u;

    identity4(out);
    out->m[3][0] = signed_choice(selector, (s32)(17u + joint * 13u));
    out->m[3][1] = signed_choice(selector >> 1, (s32)(9u + variant * 7u));
    out->m[3][2] = signed_choice(selector >> 2, (s32)(3u + owner * 19u));

    switch (family)
    {
    case 0: /* neutral mode-0 identity/translation */
        break;
    case 1: /* mode-0 source rotations */
        switch (selector & 3u)
        {
        case 0:
            out->m[0][0] = 0;
            out->m[0][1] = ONE;
            out->m[1][0] = -ONE;
            out->m[1][1] = 0;
            break;
        case 1:
            out->m[1][1] = 0;
            out->m[1][2] = ONE;
            out->m[2][1] = -ONE;
            out->m[2][2] = 0;
            break;
        case 2:
            out->m[0][0] = 2896;
            out->m[0][2] = -2896;
            out->m[2][0] = 2896;
            out->m[2][2] = 2896;
            break;
        default:
            out->m[0][0] = -ONE;
            out->m[2][2] = -ONE;
            break;
        }
        break;
    case 2: /* mode-3 prebuilt 16.16-to-20.12 affine matrix */
        out->m[0][0] = 4095;
        out->m[0][1] = signed_choice(selector, 1025);
        out->m[0][2] = signed_choice(selector >> 1, 257);
        out->m[1][0] = signed_choice(selector >> 2, 513);
        out->m[1][1] = 4097;
        out->m[1][2] = signed_choice(selector >> 3, 769);
        out->m[2][0] = signed_choice(selector >> 4, 129);
        out->m[2][1] = signed_choice(selector >> 5, 385);
        out->m[2][2] = 4094;
        break;
    case 3: /* sibling restore: deliberately distinct branch transforms */
        if ((joint == 3u) || (joint == 8u) || (joint == 10u) ||
            (joint == 15u) || (joint == 20u) || (joint == 25u))
        {
            out->m[0][0] = signed_choice(joint + owner, 3072);
            out->m[0][1] = signed_choice(joint, 2048);
            out->m[1][0] = -out->m[0][1];
            out->m[1][1] = out->m[0][0];
            out->m[2][2] = signed_choice(joint >> 1, ONE);
        }
        break;
    case 4: /* nonuniform and mirrored scale */
        out->m[0][0] = signed_choice(selector, 2048 + (s32)(variant & 3u));
        out->m[1][1] = 4095 + (s32)(joint & 3u);
        out->m[2][2] = signed_choice(selector >> 1, 3072);
        break;
    case 5: /* half-step and signed rounding boundaries */
        out->m[0][0] = 4095;
        out->m[0][1] = signed_choice(selector, 1);
        out->m[1][0] = signed_choice(selector >> 1, 2048);
        out->m[1][1] = 4097;
        out->m[2][0] = signed_choice(selector >> 2, 2047);
        out->m[2][2] = 4095;
        break;
    case 6: /* singular/zero-length direction candidates */
        memset(&out->m[0][0], 0, sizeof(s32) * 12u);
        out->m[0][0] = (joint & 1u) ? 1 : 0;
        out->m[1][1] = (joint & 2u) ? -1 : 0;
        out->m[2][2] = (joint & 4u) ? 1 : 0;
        break;
    default: /* mixed permutation + bounded scale */
        out->m[0][0] = 0;
        out->m[0][2] = signed_choice(selector, 4095);
        out->m[1][1] = signed_choice(selector >> 1, 3072);
        out->m[2][0] = signed_choice(selector >> 2, 4097);
        out->m[2][2] = 0;
        break;
    }
}

static void make_camera(u32 scenario, Matrix4 *out)
{
    u32 variant = scenario >> 3;

    identity4(out);
    switch (variant & 3u)
    {
    case 0:
        out->m[0][0] = 2896;
        out->m[0][2] = -2896;
        out->m[2][0] = 2896;
        out->m[2][2] = 2896;
        break;
    case 1:
        out->m[0][0] = 0;
        out->m[0][1] = ONE;
        out->m[1][0] = -ONE;
        out->m[1][1] = 0;
        break;
    case 2:
        out->m[0][0] = 4095;
        out->m[1][1] = 4097;
        out->m[2][2] = 4094;
        out->m[0][1] = 1;
        out->m[1][2] = -1;
        break;
    default:
        out->m[0][0] = -ONE;
        out->m[1][1] = ONE;
        out->m[2][2] = -ONE;
        break;
    }
    out->m[3][0] = 12345 + (s32)variant;
    out->m[3][1] = -23456 + (s32)variant;
    out->m[3][2] = 34567 - (s32)variant;
}

static Direction make_light(u32 owner, u32 binding, u32 scenario)
{
    static const Direction choices[] =
    {
        { 0, 0, 0 },
        { 0, 0, 127 },
        { -128, 127, -1 },
        { 64, -64, 32 },
        { 1, -1, 1 },
        { 93, 17, -71 },
        { -37, -109, 53 }
    };
    u32 index = (owner * 3u + binding * 5u + scenario * 7u) %
        (sizeof(choices) / sizeof(choices[0]));

    return choices[index];
}

static Direction prepare_direction4(Direction light, const Matrix4 *modelview)
{
    Direction out = light;
    float transformed_x = (float)(
        (s64)light.x * modelview->m[0][0] +
        (s64)light.y * modelview->m[0][1] +
        (s64)light.z * modelview->m[0][2]);
    float transformed_y = (float)(
        (s64)light.x * modelview->m[1][0] +
        (s64)light.y * modelview->m[1][1] +
        (s64)light.z * modelview->m[1][2]);
    float transformed_z = (float)(
        (s64)light.x * modelview->m[2][0] +
        (s64)light.y * modelview->m[2][1] +
        (s64)light.z * modelview->m[2][2]);
    float length = sqrtf((transformed_x * transformed_x) +
                         (transformed_y * transformed_y) +
                         (transformed_z * transformed_z));

    if (length > 0.0f)
    {
        out.x = (s32)((transformed_x * 127.0f) / length);
        out.y = (s32)((transformed_y * 127.0f) / length);
        out.z = (s32)((transformed_z * 127.0f) / length);
    }
    return out;
}

static Direction prepare_direction3(Direction light, const Matrix3 *modelview)
{
    Direction out = light;
    float transformed_x = (float)(
        (s64)light.x * modelview->m[0][0] +
        (s64)light.y * modelview->m[0][1] +
        (s64)light.z * modelview->m[0][2]);
    float transformed_y = (float)(
        (s64)light.x * modelview->m[1][0] +
        (s64)light.y * modelview->m[1][1] +
        (s64)light.z * modelview->m[1][2]);
    float transformed_z = (float)(
        (s64)light.x * modelview->m[2][0] +
        (s64)light.y * modelview->m[2][1] +
        (s64)light.z * modelview->m[2][2]);
    float length = sqrtf((transformed_x * transformed_x) +
                         (transformed_y * transformed_y) +
                         (transformed_z * transformed_z));

    if (length > 0.0f)
    {
        out.x = (s32)((transformed_x * 127.0f) / length);
        out.y = (s32)((transformed_y * 127.0f) / length);
        out.z = (s32)((transformed_z * 127.0f) / length);
    }
    return out;
}

static u32 color_byte(u32 color, u32 shift)
{
    return (color >> shift) & 0xffu;
}

static u32 clamp_color(s32 value)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > 255)
    {
        return 255;
    }
    return (u32)value;
}

static u32 divide_lit_dot_by_127(u32 dot)
{
    u32 biased = dot + 1u;

    return (biased + (biased >> 7)) >> 7;
}

static u32 shade_color(Direction direction, const int8_t normal[3],
                       u32 diffuse, u32 ambient, u32 alpha)
{
    s32 dot = (s32)normal[0] * direction.x +
        (s32)normal[1] * direction.y +
        (s32)normal[2] * direction.z;
    u32 diffuse_numer;
    s32 r;
    s32 g;
    s32 b;

    if (dot <= 0)
    {
        diffuse_numer = 0;
    }
    else if (dot > (127 * 127))
    {
        diffuse_numer = 127;
    }
    else
    {
        diffuse_numer = divide_lit_dot_by_127((u32)dot);
    }
    r = (s32)color_byte(ambient, 24) +
        (s32)((color_byte(diffuse, 24) * diffuse_numer) / 127u);
    g = (s32)color_byte(ambient, 16) +
        (s32)((color_byte(diffuse, 16) * diffuse_numer) / 127u);
    b = (s32)color_byte(ambient, 8) +
        (s32)((color_byte(diffuse, 8) * diffuse_numer) / 127u);
    return (clamp_color(r) << 24) | (clamp_color(g) << 16) |
        (clamp_color(b) << 8) | (alpha & 0xffu);
}

static int direction_equal(Direction lhs, Direction rhs)
{
    return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z);
}

int main(void)
{
    u32 scenario;
    u32 owner_index;
    u32 matrix_cell_samples = 0;
    u32 binding_matrix_samples = 0;
    u32 direction_samples = 0;
    u32 shade_samples = 0;
    u32 matrix_mismatches = 0;
    u32 direction_mismatches = 0;
    u32 shade_mismatches = 0;
    u32 sibling_negative_mismatches = 0;
    u32 owner_alias_divergences = 0;
    u32 binding_alias_divergences = 0;
    Direction scenario_directions[OWNER_COUNT][MAX_BINDINGS];

    for (scenario = 0; scenario < SCENARIO_COUNT; scenario++)
    {
        Matrix4 camera4;
        Matrix3 camera3;

        make_camera(scenario, &camera4);
        copy3_from4(&camera4, &camera3);
        memset(scenario_directions, 0, sizeof(scenario_directions));

        for (owner_index = 0; owner_index < OWNER_COUNT; owner_index++)
        {
            const OwnerPlan *owner = &sOwners[owner_index];
            Matrix4 locals4[MAX_JOINTS];
            Matrix4 worlds4[MAX_JOINTS];
            Matrix3 locals3[MAX_JOINTS];
            Matrix3 worlds3[MAX_JOINTS];
            Matrix3 wrong_worlds3[MAX_JOINTS];
            Matrix4 identity_4;
            Matrix3 identity_3;
            u32 joint;
            u32 binding;

            identity4(&identity_4);
            identity3(&identity_3);
            for (joint = 0; joint < owner->joint_count; joint++)
            {
                const Matrix4 *parent4 = &identity_4;
                const Matrix3 *parent3 = &identity_3;
                const Matrix3 *wrong_parent3 = &identity_3;
                u8 parent = owner->parents[joint];
                u32 row;
                u32 col;

                make_local(owner_index, joint, scenario, &locals4[joint]);
                copy3_from4(&locals4[joint], &locals3[joint]);
                if (parent != NO_PARENT)
                {
                    parent4 = &worlds4[parent];
                    parent3 = &worlds3[parent];
                    wrong_parent3 = &wrong_worlds3[joint - 1u];
                }
                multiply4(&locals4[joint], parent4, &worlds4[joint]);
                multiply3(&locals3[joint], parent3, &worlds3[joint]);
                multiply3(&locals3[joint], wrong_parent3,
                          &wrong_worlds3[joint]);
                for (row = 0; row < 3; row++)
                {
                    for (col = 0; col < 3; col++)
                    {
                        matrix_cell_samples++;
                        if (worlds4[joint].m[row][col] !=
                            worlds3[joint].m[row][col])
                        {
                            matrix_mismatches++;
                        }
                        if (((scenario & 7u) == 3u) &&
                            (parent != NO_PARENT) &&
                            (parent != (joint - 1u)) &&
                            (wrong_worlds3[joint].m[row][col] !=
                             worlds3[joint].m[row][col]))
                        {
                            sibling_negative_mismatches++;
                        }
                    }
                }
            }

            for (binding = 0; binding < owner->binding_count; binding++)
            {
                u32 joint_index = owner->binding_joints[binding];
                Matrix4 full_modelview;
                Matrix3 sidecar_modelview;
                Direction light = make_light(owner_index, binding, scenario);
                Direction full_direction;
                Direction sidecar_direction;
                u32 diffuse = 0x804020ffu ^
                    (owner_index * 0x10203000u) ^ (binding * 0x00070100u);
                u32 ambient = 0x102030ffu ^
                    (owner_index * 0x07050300u) ^ (binding * 0x00010300u);
                u32 normal_index;
                u32 row;
                u32 col;

                multiply4(&worlds4[joint_index], &camera4, &full_modelview);
                multiply3(&worlds3[joint_index], &camera3,
                          &sidecar_modelview);
                for (row = 0; row < 3; row++)
                {
                    for (col = 0; col < 3; col++)
                    {
                        binding_matrix_samples++;
                        if (full_modelview.m[row][col] !=
                            sidecar_modelview.m[row][col])
                        {
                            matrix_mismatches++;
                        }
                    }
                }
                full_direction = prepare_direction4(light, &full_modelview);
                sidecar_direction = prepare_direction3(
                    light, &sidecar_modelview);
                direction_samples++;
                if (!direction_equal(full_direction, sidecar_direction))
                {
                    direction_mismatches++;
                }
                scenario_directions[owner_index][binding] =
                    sidecar_direction;

                for (normal_index = 0; normal_index < NORMAL_COUNT;
                     normal_index++)
                {
                    u32 full_color = shade_color(
                        full_direction, sNormals[normal_index], diffuse,
                        ambient, 0x80u + normal_index);
                    u32 sidecar_color = shade_color(
                        sidecar_direction, sNormals[normal_index], diffuse,
                        ambient, 0x80u + normal_index);

                    shade_samples++;
                    if (full_color != sidecar_color)
                    {
                        shade_mismatches++;
                    }
                }
            }
            for (binding = 1; binding < owner->binding_count; binding++)
            {
                if (!direction_equal(
                        scenario_directions[owner_index][binding - 1u],
                        scenario_directions[owner_index][binding]))
                {
                    binding_alias_divergences++;
                }
            }
        }

        for (owner_index = 0; owner_index < sOwners[0].binding_count;
             owner_index++)
        {
            if (!direction_equal(scenario_directions[0][owner_index],
                                 scenario_directions[1][owner_index]))
            {
                owner_alias_divergences++;
            }
        }
    }

    if ((matrix_mismatches != 0u) || (direction_mismatches != 0u) ||
        (shade_mismatches != 0u) || (sibling_negative_mismatches == 0u) ||
        (owner_alias_divergences == 0u) ||
        (binding_alias_divergences == 0u))
    {
        printf(
            "LIGHT_SIDECAR_ORACLE=FAIL matrixCells=%u bindingCells=%u "
            "directions=%u shades=%u matrixMismatch=%u directionMismatch=%u "
            "shadeMismatch=%u siblingNegative=%u ownerAlias=%u "
            "bindingAlias=%u eligible=0\n",
            matrix_cell_samples, binding_matrix_samples, direction_samples,
            shade_samples, matrix_mismatches, direction_mismatches,
            shade_mismatches, sibling_negative_mismatches,
            owner_alias_divergences, binding_alias_divergences);
        return 1;
    }

    printf(
        "LIGHT_SIDECAR_ORACLE=PASS owners=2 joints=25/27 bindings=14/18 "
        "scenarios=%u mode0=%u mode3=%u sibling=%u scale=%u "
        "matrixCells=%u bindingCells=%u directions=%u shades=%u "
        "matrixMismatch=0 directionMismatch=0 shadeMismatch=0 "
        "siblingNegative=%u ownerAlias=%u bindingAlias=%u eligible=1\n",
        SCENARIO_COUNT, SCENARIO_COUNT / 8u, SCENARIO_COUNT / 8u,
        SCENARIO_COUNT / 8u, SCENARIO_COUNT / 8u,
        matrix_cell_samples, binding_matrix_samples, direction_samples,
        shade_samples, sibling_negative_mismatches,
        owner_alias_divergences, binding_alias_divergences);
    return 0;
}

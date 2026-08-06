typedef unsigned int u32;
typedef int s32;
typedef int Fix12i;
typedef short s16;
typedef unsigned char u8;

struct Vector3 { Fix12i x, y, z; };
struct Vector3_16 { s16 x, y, z; };

extern void* _ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(
    u32 actorID, u32 param1, const struct Vector3* pos, const struct Vector3_16* rot,
    s32 areaID, s32 deathTableID);

void _Z15LoadExitObjectsRN11LVL_Overlay11ObjSubTableEij(char* t, int a1, int a2)
{
    char* e;
    int i;
    struct Vector3 pos;
    struct Vector3_16 rot;

    e = *(char**)(t + 4);
    for (i = 0; i < (int)*(u8*)(t + 1); i++) {
        {
            int pz = *(s16*)(e + 4) << 12;
            int py = *(s16*)(e + 2) << 12;
            int px = *(s16*)(e + 0) << 12;
            pos.x = px;
            pos.y = py;
            pos.z = pz;
        }
        rot.x = -*(s16*)(e + 6);
        rot.y = -*(s16*)(e + 8);
        _ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(
            0x15d,
            ((u32)*(u8*)(e + 0xa) << 24) | ((u32)*(u8*)(e + 0xb) << 16)
                | ((u32)*(u8*)(e + 0xc) << 8) | (u32)*(u8*)(e + 0xd),
            &pos, &rot, -1, -1);
        e += 0xe;
    }
}

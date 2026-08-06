typedef unsigned int u32;
typedef int s32;
typedef short s16;
typedef unsigned short u16;

typedef struct { s32 x, y, z; } Vector3;
typedef struct { s16 x, y, z; } Vector3_16;

struct Entry { s16 x, y, z; u16 param; };

struct ObjSubTable {
    char pad0;
    unsigned char count;
    char pad2[2];
    struct Entry* entries;
};

extern struct Actor* _ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(
    u32 actorID, u32 param1, const Vector3* pos, const Vector3_16* rot, s32 areaID, s32 deathTableID);

void _Z25LoadTeleportSourceObjectsRN11LVL_Overlay11ObjSubTableEij(struct ObjSubTable* tbl, int a, u32 b)
{
    struct Entry* e = tbl->entries;
    s32 i = 0;
    if ((int)tbl->count <= 0)
        return;
    do {
        Vector3 pos;
        s32 vz = e->z << 12;
        s32 vy = e->y << 12;
        s32 vx = e->x << 12;
        pos.x = vx;
        pos.y = vy;
        pos.z = vz;
        _ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(
            0x15b, e->param, &pos, (const Vector3_16*)0, 0, -1);
        i++;
        e++;
    } while (i < (int)tbl->count);
}

//cpp
typedef int Fix12i;
struct Vector3 { Fix12i x, y, z; };
struct Actor;
struct RaycastGround {
    char buf[0x50];
    RaycastGround();
    ~RaycastGround();
    void SetObjAndPos(const Vector3 &pos, Actor *actor);
    int DetectClsn();
};
namespace Particle { struct System {
    static void NewSimple(unsigned int id, Fix12i a, Fix12i b, Fix12i c);
}; }

extern "C" void _ZN5Actor13LandingDustAtER7Vector3b(Actor *self, Vector3 *pos, bool b)
{
    if (b) {
        RaycastGround rc;
        *(Fix12i*)(((int)pos + 4) & 0xFFFFFFFFFFFFFFFFULL) += 0x32000;
        rc.SetObjAndPos(*pos, 0);
        if (rc.DetectClsn())
            pos->y = *(Fix12i*)((char*)&rc + 0x44);
    }
    *(Fix12i*)(((int)pos + 4) & 0xFFFFFFFFFFFFFFFFULL) += 0x5a000;
    Particle::System::NewSimple(0xb1, pos->x, pos->y, pos->z);
}

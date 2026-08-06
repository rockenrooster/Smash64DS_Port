typedef int Fix12i;
typedef unsigned short u16;
typedef unsigned int u32;

typedef struct Vector3 { Fix12i x, y, z; } Vector3;

typedef struct Clipper {
    void* vtable;
    Vector3 unk04[4];
    u32 unk34;
    u32 unk38;
    u32 unk3c;
    u32 unk40;
    u32 unk44;
    u32 unk48;
    Fix12i aspectRatio;
    Fix12i unk50;
    Fix12i unk54;
    u16 unk58;
} Clipper;

extern void* _ZTV7Clipper;
extern void _ZN7Clipper13Func_020156DCEv(Clipper* self, u32 a, u16 b, Fix12i c, Fix12i d);

Clipper* _ZN7ClipperC1Ev(Clipper* this) {
    this->vtable = &_ZTV7Clipper;
    _ZN7Clipper13Func_020156DCEv(this, 0x1555, 0xe38, 0x1000, 0x01388000);
    return this;
}
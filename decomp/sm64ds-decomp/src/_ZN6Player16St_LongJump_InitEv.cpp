//cpp
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef int s32;
typedef long long s64;

struct Vector3 { int x, y, z; };

extern "C" int func_ov002_020e2be4(char* self);
extern "C" int func_ov002_020e2ba8(char* c);
extern "C" int func_ov002_020e2b6c(char* c);
extern "C" void func_ov002_020e2ad0(char* c);

extern "C" void _ZN6Player7SetAnimEji5Fix12IiEj(void* self, unsigned int a, int b, int c, unsigned int d);
extern "C" void _ZN5Sound9PlayBank0EjRK7Vector3(unsigned int id, Vector3 const & v);
extern "C" void _ZN5Sound13PlayCharVoiceEjjRK7Vector3(unsigned int a, unsigned int b, Vector3 const & v);

extern "C" int _ZN6Player16St_LongJump_InitEv(char* self) {
    *(u8*)(self + 0x71b) = 0;
    if (func_ov002_020e2be4(self)) {
        return 1;
    }
    if (func_ov002_020e2ba8(self)) {
        return 1;
    }
    if (func_ov002_020e2b6c(self)) {
        return 1;
    }
    *(u8*)(self + 0x712) = 1;
    *(u8*)(self + 0x70d) = 0;
    *(u8*)(self + 0x6e1) = 0;
    *(u8*)(self + 0x6de) = 1;
    *(u8*)(self + 0x6df) = 0;
    _ZN6Player7SetAnimEji5Fix12IiEj(self, 0x1a, 0x40000000, 0x1000, 0);
    *(s32*)(self + 0xa8) = 0x1e000;
    func_ov002_020e2ad0(self);
    *(s32*)(self + 0x9c) = -0x2000;
    *(s32*)(self + 0x98) = (int)(((s64)*(s32*)(self + 0x98) * 0x1800 + 0x800) >> 12);
    if (*(s32*)(self + 0x98) >= 0x3c000) {
        *(s32*)(self + 0x98) = 0x3c000;
    }
    if (*(u8*)(self + 0x6f9) == 0) {
        _ZN5Sound9PlayBank0EjRK7Vector3(*(u32*)(self + 0x66c) + 0x30, *(Vector3*)(self + 0x74));
    } else {
        _ZN5Sound9PlayBank0EjRK7Vector3(0xa0, *(Vector3*)(self + 0x74));
    }
    _ZN5Sound13PlayCharVoiceEjjRK7Vector3(*(u8*)(self + 0x6d9), 4, *(Vector3*)(self + 0x74));
    return 1;
}

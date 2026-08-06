//cpp
typedef int Fix12i;
extern "C" Fix12i _ZN4cstd4fdivEii(Fix12i a, Fix12i b);

struct C {
    char pad0[0x8c];
    short field_8c;
    char pad8e[0x98 - 0x8e];
    int field_98;
    char pad9c[0x398 - 0x9c];
    int field_398;
};

extern "C" void func_ov072_0211f158(C* c)
{
    int d = (int)(((long long)(c->field_398 << 1) * 0x3243F6A89LL + 0x80000000LL) >> 32);
    Fix12i q = _ZN4cstd4fdivEii(c->field_98, d);
    c->field_8c = (short)(c->field_8c + (int)(((long long)q * 0xffff + 0x800) >> 12));
}

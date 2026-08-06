//cpp
struct Base { virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4(); virtual void m(int); };
struct E { Base b; char pad[0x4c]; };
extern "C" int func_ov002_020b4bc4(char *t) {
  E *o = &((E *)(t + 0x320))[*(unsigned char *)(t + 0xdc8)];
  o->b.m(0);
  return 1;
}

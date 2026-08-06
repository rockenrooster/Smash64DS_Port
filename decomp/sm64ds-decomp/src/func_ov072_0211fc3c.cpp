//cpp
struct C; typedef void (C::*PMF)();
struct C { char pad[0x38c]; PMF *pp; };
extern "C" void func_ov072_0211fc3c(C *c) { PMF *p = c->pp + 1; (c->**p)(); }

//cpp
struct C; typedef void (C::*PMF)();
struct C { char pad[0x350]; PMF *pp; };
extern "C" void func_ov085_0212a46c(C *c) { PMF *p = c->pp; (c->**p)(); }

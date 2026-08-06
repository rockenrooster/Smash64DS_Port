//cpp
struct C; typedef void (C::*PMF)();
struct C { char pad[0x328]; PMF *pp; };
extern "C" void func_ov072_0212059c(C *c) { PMF *p = c->pp; (c->**p)(); }

// Stage::GraphCallback1 - renders all particles then returns success
typedef int s32;

extern void _ZN8Particle9RenderAllEv(void);

s32 _ZN5Stage14GraphCallback1Ev(void) {
    _ZN8Particle9RenderAllEv();
    return 1;
}
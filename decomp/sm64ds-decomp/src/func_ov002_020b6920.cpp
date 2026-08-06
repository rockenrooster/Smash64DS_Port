//cpp
extern "C" {
extern void _ZN8Platform21UpdateModelPosAndRotYEv(void*);
extern int _ZN8Platform13IsClsnInRangeE5Fix12IiES1_(void*, int, int);
extern void _ZN8Platform19UpdateClsnPosAndRotEv(void*);
int func_ov002_020b6920(void* c) {
    _ZN8Platform21UpdateModelPosAndRotYEv(c);
    if (_ZN8Platform13IsClsnInRangeE5Fix12IiES1_(c, 0, 0))
        _ZN8Platform19UpdateClsnPosAndRotEv(c);
    return 1;
}
}

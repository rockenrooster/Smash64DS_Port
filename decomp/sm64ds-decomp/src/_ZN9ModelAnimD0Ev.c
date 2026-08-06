struct ModelAnim { void **vtable; };
extern void *_ZTV9ModelAnim[];
extern void *VTable_Animation_ModelAnimThunk[];
extern void _ZN9AnimationD2Ev(void *thiz);
extern void _ZN5ModelD2Ev(struct ModelAnim *thiz);
extern void _ZN6Memory16operator_delete2EPv(void *ptr);

struct ModelAnim *_ZN9ModelAnimD0Ev(struct ModelAnim *thiz)
{
    thiz->vtable = (void **)_ZTV9ModelAnim;
    *(void ***)((char *)thiz + 0x50) = (void **)VTable_Animation_ModelAnimThunk;
    _ZN9AnimationD2Ev((char *)thiz + 0x50);
    _ZN5ModelD2Ev(thiz);
    _ZN6Memory16operator_delete2EPv(thiz);
    return thiz;
}

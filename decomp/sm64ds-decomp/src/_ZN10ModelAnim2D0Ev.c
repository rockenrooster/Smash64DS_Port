struct ModelAnim2 { void **vtable; };
extern void *_ZTV10ModelAnim2[];
extern void *VTable_Animation_ModelAnim2Thunk[];
extern void _ZN9AnimationD1Ev(void *thiz);
extern void _ZN9ModelAnimD2Ev(struct ModelAnim2 *thiz);
extern void _ZN6Memory16operator_delete2EPv(void *ptr);

struct ModelAnim2 *_ZN10ModelAnim2D0Ev(struct ModelAnim2 *thiz)
{
    thiz->vtable = (void **)_ZTV10ModelAnim2;
    *(void ***)((char *)thiz + 0x50) = (void **)VTable_Animation_ModelAnim2Thunk;
    _ZN9AnimationD1Ev((char *)thiz + 0x68);
    _ZN9ModelAnimD2Ev(thiz);
    _ZN6Memory16operator_delete2EPv(thiz);
    return thiz;
}

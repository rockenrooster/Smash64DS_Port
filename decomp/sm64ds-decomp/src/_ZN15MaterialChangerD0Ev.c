struct MaterialChanger { void **vtable; };
extern void *_ZTV15MaterialChanger[];
extern void _ZN9AnimationD2Ev(struct MaterialChanger *thiz);
extern void _ZN6Memory16operator_delete2EPv(void *ptr);

struct MaterialChanger *_ZN15MaterialChangerD0Ev(struct MaterialChanger *thiz)
{
    thiz->vtable = (void **)_ZTV15MaterialChanger;
    _ZN9AnimationD2Ev(thiz);
    _ZN6Memory16operator_delete2EPv(thiz);
    return thiz;
}

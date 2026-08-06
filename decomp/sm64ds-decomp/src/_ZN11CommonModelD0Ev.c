struct CommonModel { void **vtable; };
extern void *_ZTV11CommonModel[];
extern void _ZN9ModelBaseD2Ev(struct CommonModel *thiz);
extern void _ZN6Memory16operator_delete2EPv(void *ptr);

struct CommonModel *_ZN11CommonModelD0Ev(struct CommonModel *thiz)
{
    thiz->vtable = (void **)_ZTV11CommonModel;
    _ZN9ModelBaseD2Ev(thiz);
    _ZN6Memory16operator_delete2EPv(thiz);
    return thiz;
}

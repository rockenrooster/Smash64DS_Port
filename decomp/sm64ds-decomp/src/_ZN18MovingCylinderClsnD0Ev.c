struct MovingCylinderClsn { void **vtable; };
extern void *_ZTV18MovingCylinderClsn[];
extern void _ZN12CylinderClsnD2Ev(struct MovingCylinderClsn *thiz);
extern void _ZN6Memory16operator_delete2EPv(void *ptr);

struct MovingCylinderClsn *_ZN18MovingCylinderClsnD0Ev(struct MovingCylinderClsn *thiz)
{
    thiz->vtable = (void **)_ZTV18MovingCylinderClsn;
    _ZN12CylinderClsnD2Ev(thiz);
    _ZN6Memory16operator_delete2EPv(thiz);
    return thiz;
}

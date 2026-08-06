struct MovingCylinderClsnWithPos { void **vtable; };
extern void *_ZTV25MovingCylinderClsnWithPos[];
extern void _ZN18MovingCylinderClsnD2Ev(struct MovingCylinderClsnWithPos *thiz);
extern void _ZN6Memory16operator_delete2EPv(void *ptr);

struct MovingCylinderClsnWithPos *_ZN25MovingCylinderClsnWithPosD0Ev(struct MovingCylinderClsnWithPos *thiz)
{
    thiz->vtable = (void **)_ZTV25MovingCylinderClsnWithPos;
    _ZN18MovingCylinderClsnD2Ev(thiz);
    _ZN6Memory16operator_delete2EPv(thiz);
    return thiz;
}

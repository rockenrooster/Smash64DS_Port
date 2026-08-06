struct CylinderClsnWithPos { void **vtable; };
extern void *_ZTV19CylinderClsnWithPos[];
extern void _ZN12CylinderClsnD2Ev(struct CylinderClsnWithPos *thiz);
extern void _ZN6Memory16operator_delete2EPv(void *ptr);

struct CylinderClsnWithPos *_ZN19CylinderClsnWithPosD0Ev(struct CylinderClsnWithPos *thiz)
{
    thiz->vtable = (void **)_ZTV19CylinderClsnWithPos;
    _ZN12CylinderClsnD2Ev(thiz);
    _ZN6Memory16operator_delete2EPv(thiz);
    return thiz;
}

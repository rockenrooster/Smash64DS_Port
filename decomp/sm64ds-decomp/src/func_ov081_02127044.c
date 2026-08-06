extern void _ZN12CylinderClsn5ClearEv(void *);
int func_ov081_02127044(char *c)
{
    *(int *)(c + 0x98) = 0;
    _ZN12CylinderClsn5ClearEv((char *)c + 0x1b0);
    *(int *)(c + 0x3e0) = 6;
    return 1;
}

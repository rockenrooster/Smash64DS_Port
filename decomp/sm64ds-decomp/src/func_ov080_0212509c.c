extern void _ZN12WithMeshClsn13SetLimMovFlagEv(void *);
int func_ov080_0212509c(char *c)
{
    *(int *)(c + 0xa8) = 49152;
    _ZN12WithMeshClsn13SetLimMovFlagEv((char *)c + 0x180);
    *(int *)(c + 0x370) = 0;
    return 1;
}

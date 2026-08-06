extern void Matrix4x3_FromRotationY(void *, int);
void func_ov045_021118f8(char *t)
{
    Matrix4x3_FromRotationY(t + 0xf4, *(short *)(t + 0x8e));
    *(int *)(t + 0x118) = *(int *)(t + 0x5c) >> 3;
    *(int *)(t + 0x11c) = *(int *)(t + 0x60) >> 3;
    *(int *)(t + 0x120) = *(int *)(t + 0x64) >> 3;
}

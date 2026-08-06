extern void Matrix4x3_FromRotationZXYExt(void* m, int x, int y, int z);
extern void Matrix4x3_FromRotationY(void* m, int angle);

void func_ov065_0211990c(char* c)
{
    Matrix4x3_FromRotationZXYExt(c + 0xf0, 0, *(short*)(c + 0x8e), *(short*)(c + 0x90));
    *(int*)(c + 0x114) = *(int*)(c + 0x5c) >> 3;
    *(int*)(c + 0x118) = (*(int*)(c + 0x60) + *(int*)(c + 0x370)) >> 3;
    *(int*)(c + 0x11c) = *(int*)(c + 0x64) >> 3;
    Matrix4x3_FromRotationY(c + 0x33c, *(short*)(c + 0x8e));
    *(int*)(c + 0x360) = *(int*)(c + 0x114);
    *(int*)(c + 0x364) = *(int*)(c + 0x60) >> 3;
    *(int*)(c + 0x368) = *(int*)(c + 0x11c);
}

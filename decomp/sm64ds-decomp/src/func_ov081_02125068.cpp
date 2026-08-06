//cpp
struct BCA_File;
struct ModelAnim { int SetAnim(BCA_File *, int, int, unsigned int); };
struct S2 { int w[2]; };
extern S2 data_ov081_02128d98;
extern "C" int func_ov081_02125068(char *c)
{
    int v = *(int *)(c + 0x41c);
    if (v != 2 && v != 3) {
        *(int *)(c + 0xb0) = 3;
    } else if (v == 2) {
        *(int *)(c + 0xb0) = 0x8000002;
    }
    ((ModelAnim *)(c + 0x30c))->SetAnim((BCA_File *)data_ov081_02128d98.w[1], 0, 0x1000, 0);
    return 1;
}

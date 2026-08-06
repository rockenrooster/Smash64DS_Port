//cpp
struct BCA_File;
struct ModelAnim {
    void SetAnim(BCA_File *, int, int, unsigned int);
};

extern int data_ov070_02123500;

extern "C" int func_ov070_0211f5f0(char *c) {
    unsigned int flags = 0;
    BCA_File *file = (BCA_File *)(((int *)&data_ov070_02123500)[1]);
    ((ModelAnim *)(c + 0x300))->SetAnim(file, 0x40000000, 0x1000, flags);
    return 1;
}

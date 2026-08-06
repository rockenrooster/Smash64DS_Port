//cpp
struct BCA_File;
struct ModelAnim {
    void SetAnim(BCA_File *, int, int, unsigned int);
};

extern int data_ov085_02130490;

extern "C" void func_ov085_0212943c(void *c) {
    unsigned int flags = 0;
    BCA_File *file = (BCA_File *)(((int *)&data_ov085_02130490)[1]);
    ((ModelAnim *)((char *)c + 0x108))->SetAnim(file, 0, 0x1000, flags);
}

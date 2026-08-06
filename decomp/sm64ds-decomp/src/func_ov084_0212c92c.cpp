//cpp
struct BCA_File;
struct ModelAnim {
    void SetAnim(BCA_File *, int, int, unsigned int);
};

extern int data_ov084_02130d9c;

extern "C" void func_ov084_0212c92c(void *c) {
    unsigned int flags = 0;
    BCA_File *file = (BCA_File *)(((int *)&data_ov084_02130d9c)[1]);
    ((ModelAnim *)((char *)c + 0x108))->SetAnim(file, 0, 0x1000, flags);
}

//cpp
struct BCA_File;
struct ModelAnim {
    void SetAnim(BCA_File *, int, int, unsigned int);
};

extern int data_ov081_02128db8;

extern "C" int func_ov081_02124d14(char *c) {
    unsigned int flags = 0;
    BCA_File *file = (BCA_File *)(((int *)&data_ov081_02128db8)[1]);
    ((ModelAnim *)(c + 0x30c))->SetAnim(file, 0x40000000, 0x1000, flags);
    return 1;
}

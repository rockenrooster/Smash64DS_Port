extern int func_01ffabe4(int, int);
int func_ov080_02125cd4(void *c, int n) {
    void *p = *(void **)((char *)c + 0x1ac);
    int r4 = *(int *)p;
    int r1 = *(int *)((char *)p + 0x10);
    int r = func_01ffabe4(r4, r1);
    return r4 - r * n;
}

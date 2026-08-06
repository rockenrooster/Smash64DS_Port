void func_ov002_020b567c(char* a, char* b){
  unsigned short v = *(unsigned short*)(b+0xc);
  int t1 = (int)(v == 0xbf);
  if (t1 != 0) goto body;
  {
    int t2 = (int)(v == 0xc2);
    if (t2 == 0) return;
  }
body:
  if (*(int*)(b+0x60) > *(int*)(a+0x60) + 0x64000)
    *(unsigned char*)(a+0x71a) = 1;
}

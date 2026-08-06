extern int data_ov002_020ff090[];
void func_ov002_020b41b8(int *dst, char *src){
  unsigned char idx=src[0xd7];
  int c64=*(int*)(src+0x64);
  int b=*(int*)(src+0x60)+data_ov002_020ff090[idx];
  int c5c=*(int*)(src+0x5c);
  dst[0]=c5c; dst[1]=b; dst[2]=c64;
}

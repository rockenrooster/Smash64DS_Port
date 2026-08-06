extern int func_ov002_020b4714(char*);
void func_ov002_020b47ec(char* c){
  if(*(int*)(c+0xf8)==0) return;
  if((*(int*)(c+0xf4)&0x20)==0) return;
  func_ov002_020b4714(c);
}

extern int _ZN6Player17St_EndingFly_MainEv();
extern int func_ov007_020c937c();
int func_ov007_020adbc8(char* c){
  unsigned char x = *(unsigned char*)(c+8);
  if(x==1 || x==5)
    _ZN6Player17St_EndingFly_MainEv(*(int*)(*(int*)(c+4)+0x10));
  func_ov007_020c937c(*(int*)c);
  return _ZN6Player17St_EndingFly_MainEv(c);
}

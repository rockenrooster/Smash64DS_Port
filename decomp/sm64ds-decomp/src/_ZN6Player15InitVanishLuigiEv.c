extern int func_ov002_020bda48();
extern int func_ov002_020bd9ec();
extern int func_ov002_020c43c4();
void _ZN6Player15InitVanishLuigiEv(char* c){
  func_ov002_020bda48(c);
  *(char*)(c+0x6fb)=1;
  *(short*)(c+0x600+0xae)=0x258;
  func_ov002_020bd9ec(c,0x33);
  func_ov002_020c43c4(c,3);
}

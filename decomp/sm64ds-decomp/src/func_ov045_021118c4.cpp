//cpp
extern "C" {
void Matrix4x3_FromRotationY(void*, int);
void func_ov045_021118c4(char* c){
  Matrix4x3_FromRotationY(c+0x128, *(short*)(c+0x8e));
  *(int*)(c+0x14c)=*(int*)(c+0x5c);
  *(int*)(c+0x150)=*(int*)(c+0x60);
  *(int*)(c+0x154)=*(int*)(c+0x64);
}
}

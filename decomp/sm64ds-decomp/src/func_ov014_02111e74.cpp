//cpp
struct BCA_File;
struct ModelAnim { void SetAnim(BCA_File*, int, int, unsigned int); };
struct S { int a; BCA_File* b; };
extern S data_ov014_02114980;
extern "C" void func_ov014_02111e74(char* c){
  *(int*)(c+0xa8)=0;
  *(int*)(c+0x98)=0;
  *(short*)(c+0x500+0xfc)=0x78;
  ((ModelAnim*)(c+0x150))->SetAnim(data_ov014_02114980.b, 0, 0x1000, 0);
}

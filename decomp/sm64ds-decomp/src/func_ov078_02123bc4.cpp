//cpp
typedef int Fix12;
struct BCA_File;
struct BlendModelAnim { int SetAnim(BCA_File&, int, int, Fix12, unsigned short); };
extern int* data_ov078_02126ee8[];
extern "C" int func_ov078_02123bc4(char* c){
  *(int*)(c+0x9c)=-0x2000;
  *(int*)(c+0x4fc)=2;
  *(int*)(c+0x98)=0xa000;
  ((BlendModelAnim*)(c+0x2cc))->SetAnim(*(BCA_File*)data_ov078_02126ee8[1], 4, 0, 0x1000, 0);
  return 1;
}

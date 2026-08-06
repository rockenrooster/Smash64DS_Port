//cpp
struct Vector3 { int x,y,z; Vector3(int a,int b,int d){x=a;y=b;z=d;} Vector3(){} };
struct RaycastGround {
  char buf[0x54];
  RaycastGround();
  ~RaycastGround();
  void SetObjAndPos(const Vector3 &pos, void *actor);
  int DetectClsn();
};
extern "C" int func_ov102_02149610(char *c){
  Vector3 pos(*(int*)(c+0x5c), *(int*)(c+0x60)+0x28000, *(int*)(c+0x64));
  RaycastGround rg;
  rg.SetObjAndPos(pos, 0);
  *(int*)((char*)&rg + 0x4c) = 0x3e8000;
  int r = *(int*)(c+0x60);
  if (rg.DetectClsn()) r = *(int*)((char*)&rg + 0x44);
  return r;
}

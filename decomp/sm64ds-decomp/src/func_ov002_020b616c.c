extern int _ZN16MeshColliderBase9IsEnabledEv(char* c);
extern void _ZN16MeshColliderBase7DisableEv(char* c);
extern unsigned char DecIfAbove0_Byte(unsigned char* p);
extern void Quaternion_SLerp(char* out, char* a, int t, char* b);
extern void func_ov002_020b6074(char* c);
extern int _ZN8Platform13IsClsnInRangeE5Fix12IiES1_(char* c, int a, int b);
extern void _ZN8Platform19UpdateClsnPosAndRotEv(char* c);
extern int data_02092768[4];
int func_ov002_020b616c(char* c){
  if((int)((*(int*)(c+0xb0) & 8) != 0) != 0){
    if(_ZN16MeshColliderBase9IsEnabledEv(c+0x124)){
      _ZN16MeshColliderBase7DisableEv(c+0x124);
    }
    return 1;
  }
  if(DecIfAbove0_Byte((unsigned char*)(c+0x34d)) == 0){
    *(int*)(c+0x330) = data_02092768[0];
    *(int*)(c+0x334) = data_02092768[1];
    *(int*)(c+0x338) = data_02092768[2];
    *(int*)(c+0x33c) = data_02092768[3];
  }
  Quaternion_SLerp(c+0x320, c+0x330, 0x199, c+0x320);
  func_ov002_020b6074(c);
  if(_ZN8Platform13IsClsnInRangeE5Fix12IiES1_(c, 0, 0)){
    _ZN8Platform19UpdateClsnPosAndRotEv(c);
  }
  *(unsigned char*)(c+0x34c) = 0;
  return 1;
}

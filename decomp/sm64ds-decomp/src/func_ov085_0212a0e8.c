extern int _ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj(char*, void*, int, int, unsigned int);
extern void _ZN9Animation8SetFlagsEi(char*, int);
extern void* data_ov085_021304ec[];
int func_ov085_0212a0e8(char *c){
 _ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj(c+0xd4, data_ov085_021304ec[1], 0, 0x1000, 0);
 _ZN9Animation8SetFlagsEi(c+0x124, 0x40000000);
 *(int*)(c+0x98)=0x4000;
 *(int*)(c+0xa8)=0xa000;
 *(int*)(c+0x354)=4;
 return 1;
}

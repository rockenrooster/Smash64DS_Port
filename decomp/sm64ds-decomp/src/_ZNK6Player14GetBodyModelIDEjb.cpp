//cpp
extern "C" unsigned int _ZNK6Player14GetBodyModelIDEjb(char* c, unsigned int a, char b){
  if(b==0){
    if(*(unsigned char*)(c+0x6f9)) a=4;
  } else {
    if(*(unsigned char*)(c+0x6fa)) a=4;
  }
  return a;
}

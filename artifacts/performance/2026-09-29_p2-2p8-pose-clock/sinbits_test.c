#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
  unsigned bad=0;
  for (uint32_t v=0; v<65536; v++){
    for (int neg=0; neg<2; neg++){
      float ref=(float)(uint16_t)v*(1.0F/32768.0F);
      if(neg) ref=-ref;
      uint32_t rb; memcpy(&rb,&ref,4);
      uint32_t out=0;
      if(v){uint32_t lead=__builtin_clz(v); out=((143u-lead)<<23)|(((v<<lead)<<1)>>9);}
      if(neg) out^=0x80000000u;
      if(out!=rb){ if(bad<5) printf("bad v=%u neg=%d ref=%08x out=%08x\n",v,neg,rb,out); bad++; }
    }
  }
  printf("bad %u\n",bad); return bad!=0;
}

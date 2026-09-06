"""Compile and execute the production compact renderer storage layouts."""
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'scripts/menus'))
from source_test_helpers import braced, function

TYPES = '''#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef int16_t s16; typedef int32_t s32;
#define TRUE 1
#define FALSE 0
'''


class RendererStorageTest(unittest.TestCase):
    def compile_run(self, program, directory, name):
        cc=shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(cc)
        cfile,exe=directory/(name+'.c'),directory/(name+'.exe')
        cfile.write_text(program)
        result=subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror',
                               str(cfile),'-o',str(exe)],capture_output=True)
        self.assertEqual(result.returncode,0,result.stderr.decode())
        result=subprocess.run([str(exe)],capture_output=True)
        self.assertEqual(result.returncode,0,result.stdout.decode()+result.stderr.decode())
        return cfile

    def test_vertex_words_and_arm_load_alignment(self):
        source=(ROOT/'src/nds/nds_renderer_assets.c').read_text()
        a=source.index('typedef struct NDSNativePreparedDenseVertex')
        b=source.index('typedef struct NDSNativeRoot',a)
        record=source[a:b].rsplit('#endif',1)[0]
        with tempfile.TemporaryDirectory() as directory:
            d=Path(directory)
            for hw, size in ((0,16),(1,10)):
                program=TYPES+f'#define NDS_R2_FIGHTER_HW_LIGHT {hw}\n'+record+f'''
_Static_assert(sizeof(NDSNativePreparedDenseVertex)=={size}, "vertex size");
__attribute__((noinline)) u32 read_odd(const NDSNativePreparedDenseVertex *p) {{
    return p[1].gx_xy;
}}
int main(void) {{
    NDSNativePreparedDenseVertex p[7];
    const u32 xy[7]={{0x80007fff,0xffff0001,0x80008000,0,0xffffffff,0x1234edcb,0x01020304}};
    const u16 z[7]={{0x8000,0x7fff,0xffff,0,1,0x8001,0xfedc}};
    const s16 s[7]={{-32768,32767,-1,0,1,-64,64}};
    for (u32 i=0;i<7;++i) {{
        p[i].gx_xy=xy[i]; p[i].gx_z=z[i]; p[i].s=s[i]; p[i].t=s[6-i];
    }}
    for (u32 n=0;n<21;++n) {{
        u32 i=(n*3)%7;
        u32 tex=(u32)(u16)p[i].s|((u32)(u16)p[i].t<<16);
        if (p[i].gx_xy!=xy[i] || (u32)p[i].gx_z!=z[i] ||
            tex!=((u32)(u16)s[i]|((u32)(u16)s[6-i]<<16))) return 1;
        if ((s16)(p[i].gx_xy>>16)!=(s16)(xy[i]>>16) ||
            (s16)p[i].gx_z!=(s16)z[i]) return 2;
    }}
    return read_odd(p)!=xy[1];
}}
'''
                cfile=self.compile_run(program,d,'vertex'+str(hw))
                arm=Path('C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe')
                asm=d/('vertex'+str(hw)+'.s')
                result=subprocess.run([str(arm),'-O2','-mcpu=arm946e-s','-marm',
                    '-S',str(cfile),'-o',str(asm)],capture_output=True)
                self.assertEqual(result.returncode,0,result.stderr.decode())
                if hw:
                    text=asm.read_text(); a=text.index('read_odd:'); b=text.index('.size\tread_odd',a)
                    code=text[a:b]
                    self.assertGreaterEqual(len(re.findall(r'\bldrh\b',code)),2)
                    self.assertNotRegex(code,r'\bldr\s', 'Packed coordinates must not use an unaligned word load')

    def test_texture_memo_roundtrip_and_decline(self):
        source=(ROOT/'src/nds/nds_renderer_native_common.c').read_text()
        source=source.replace('__attribute__((noinline))', '')
        record=braced(source,r'typedef struct NDSR2RunTextureMemo\s*\{',True)
        bodies='\n'.join(function(source,name) for name in (
            'ndsRendererR2RunTextureMemoFor','ndsRendererR2RunTextureMemoApply',
            'ndsRendererR2RunTextureMemoFill'))
        counters='\n'.join('static u32 '+name+';' for name in sorted(set(re.findall(r'\bgNds\w+',bodies))))
        program=TYPES+record+'''
#define NDS_R2_RUN_MEMO_MAX 67u
#define NDS_RENDERER_HW_TEXTURE_CACHE_COUNT 300u
_Static_assert(sizeof(NDSR2RunTextureMemo)==28, "memo size");
typedef struct {u32 hardware_texture_ready_count, hardware_texture_format,
    hardware_texture_width, hardware_texture_height;} NDSRendererStats;
typedef struct {s32 ready,name; u32 key_generation,last_used_frame,pinned,params;} NDSRendererHardwareTextureCacheEntry;
static NDSRendererHardwareTextureCacheEntry sNdsRendererHardwareTextureCache[300];
static const NDSRendererHardwareTextureCacheEntry *sNdsRendererHardwareActiveTextureEntry;
static u32 sNdsRendererHardwareFrameSerial=10;
static struct {u32 texture_memo_owner_key;} sNdsNativeFighterOwnerExecution;
static NDSR2RunTextureMemo sNdsR2RunTextureMemo[67][4];
static u32 bound_name,bound_params,static_hits;
static void ndsRendererHardwareBindTextureName(NDSRendererStats *s,u32 name) {(void)s;bound_name=name;}
static void ndsRendererHardwareApplyTextureParams(u32 p) {bound_params=p;}
static void ndsRendererHardwareRecordBattleStaticTextureHit(const NDSRendererHardwareTextureCacheEntry *e) {(void)e;++static_hits;}
'''+counters+'\n'+bodies+r'''
#include <stdio.h>
#define CHECK(x) do {if (!(x)) {printf("line%d: %s\n",__LINE__,#x);return 1;}} while(0)
int main(void) {
    NDSRendererStats s={0,4,256,128}; u32 name,ss,st,os,ot; s32 off;
    for(u32 lane=0;lane<4;++lane) {
        NDSRendererHardwareTextureCacheEntry *e=&sNdsRendererHardwareTextureCache[lane];
        *e=(NDSRendererHardwareTextureCacheEntry){1,(s32)(lane+10),100,0,1,0xdeadbeef};
        sNdsNativeFighterOwnerExecution.texture_memo_owner_key=0x10000|(lane<<9);
        sNdsRendererHardwareActiveTextureEntry=e;
        ndsRendererR2RunTextureMemoFill(2,&s,lane+10,65535,32768,65535,0,-32768);
    }
    for(u32 lane=0;lane<4;++lane) {
        sNdsNativeFighterOwnerExecution.texture_memo_owner_key=0x10000|(lane<<9);
        sNdsRendererHardwareActiveTextureEntry=NULL;
        CHECK(ndsRendererR2RunTextureMemoApply(2,&s,&name,&ss,&st,&os,&ot,&off));
        CHECK(name==lane+10 && ss==65535 && st==32768 && os==65535 && ot==0 && off==-32768);
        CHECK(s.hardware_texture_format==4 && s.hardware_texture_width==256 && s.hardware_texture_height==128);
        CHECK(bound_name==name && bound_params==0xdeadbeef && static_hits==lane+1);
        CHECK(sNdsRendererHardwareTextureCache[lane].last_used_frame==11);
    }
    sNdsNativeFighterOwnerExecution.texture_memo_owner_key=0x20000;
    CHECK(!ndsRendererR2RunTextureMemoApply(2,&s,&name,&ss,&st,&os,&ot,&off));
    sNdsNativeFighterOwnerExecution.texture_memo_owner_key=0x10000;
    ++sNdsRendererHardwareTextureCache[0].key_generation;
    CHECK(!ndsRendererR2RunTextureMemoApply(2,&s,&name,&ss,&st,&os,&ot,&off));
    for(u32 fault=0;fault<11;++fault) {
        NDSRendererHardwareTextureCacheEntry *e=&sNdsRendererHardwareTextureCache[fault==10?255:0];
        u32 n=10,a=65535,b=0,c=1,d=2; s32 o=32767;
        s.hardware_texture_format=4; s.hardware_texture_width=256; s.hardware_texture_height=128;
        if(fault==0)n=65536;
        if(fault==1)s.hardware_texture_format=256;
        if(fault==2)s.hardware_texture_width=65536;
        if(fault==3)s.hardware_texture_height=65536;
        if(fault==4)a=65536;
        if(fault==5)b=65536;
        if(fault==6)c=65536;
        if(fault==7)d=65536;
        if(fault==8)o=32768;
        if(fault==9)o=-32769;
        *e=(NDSRendererHardwareTextureCacheEntry){1,(s32)n,101,0,1,0};
        sNdsRendererHardwareActiveTextureEntry=e;
        sNdsR2RunTextureMemo[2][0].valid=1;
        ndsRendererR2RunTextureMemoFill(2,&s,n,a,b,c,d,o);
        CHECK(!sNdsR2RunTextureMemo[2][0].valid);
        CHECK(!ndsRendererR2RunTextureMemoApply(2,&s,&name,&ss,&st,&os,&ot,&off));
    }
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            self.compile_run(program,Path(directory),'memo')


if __name__ == '__main__':
    unittest.main()

"""Execute the actual end-message preparation and VRAM writes on the host."""
import re
import shutil
import subprocess
from pathlib import Path
from source_test_helpers import function, braced

ROOT = Path(__file__).resolve().parents[2]


def test_actual_end_bank_preserves_go_sparks_and_tags_and_retries(tmp_path):
    source = (ROOT / 'src/nds/nds_ifcommon_oam.c').read_text()
    defines = '\n'.join(re.findall(r'^#define NDS_IFCOMMON_(?:ASSET_COUNT|MAX_TILES|OBJ_VRAM_BYTES|GAME_STATUS_SIZE|END_FIRST|\w+_BANK_\w+|ANNOUNCE_\w+|USED_BYTES)\b[^\n]*', source, re.M))
    # Keep full macro names: the noncapturing expression above returns lines.
    types = '\n'.join(re.search(r'typedef struct '+n+r'\b.*?\} '+n+r';', source, re.S)[0]
                      for n in ('NDSIFCommonTileSpec','NDSIFCommonAssetSpec','NDSIFCommonNativeTile','NDSIFCommonNativeAsset','NDSIFCommonEndSlot'))
    tables = '\n'.join(braced(source, p, True) for p in (
        r'enum NDSIFCommonNativeAssetKind\b', r'enum NDSIFCommonNativeFallbackReason\b',
        r'static const NDSIFCommonAssetSpec sNdsIFCommonAssetSpecs\[',
        r'static const NDSIFCommonEndSlot sNdsIFCommonTimeUpSlots\[',
        r'static const NDSIFCommonEndSlot sNdsIFCommonGameSetSlots\['))
    code = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef uint32_t u32; typedef uint16_t u16; typedef uint8_t u8; typedef int32_t s32;
typedef int SpriteSize;
typedef struct { int unused; } Sprite;
typedef struct { int unused; } Bitmap;
#define TRUE 1
#define FALSE 0
#define NDS_RENDERER_HW_TRIANGLES 1
#define SpriteColorFormat_Bmp 3
#define TILE(sx,sy,sw,sh,cw,ch,px,py) {sx,sy,sw,sh,cw,ch,px,py}
#define NO_TILE TILE(0,0,0,0,0,0,0,0)
''' + defines + '\n' + types + '\n' + tables + r'''
static u16 vram[32768], preserved[32768];
#define SPRITE_GFX vram
static NDSIFCommonNativeAsset sNdsIFCommonAssets[NDS_IFCOMMON_ASSET_COUNT];
static const void *sNdsIFCommonPreparedFile;
static size_t sNdsIFCommonPreparedFileSize;
static u32 sNdsIFCommonPrepared,sNdsIFCommonAnnounceActive,sNdsIFCommonAnnounceGameSet;
static u32 gNdsIFCommonNativeOamPrepareTiles,gNdsIFCommonNativeOamPrepareAssets;
static u32 gNdsIFCommonNativeOamPrepareFailCount,gNdsIFCommonNativeOamLastFallbackReason;
static u32 gNdsIFCommonNativeOamPrepareTicks,gNdsIFCommonNativeOamPrepareSuccessCount;
static unsigned writes;
static Sprite sprite;
static u32 cpuGetTiming(void) { return 1; }
static SpriteSize ndsIFCommonSpriteSize(u32 w,u32 h) { assert(w&&h); return 1; }
static void dmaFillHalfWords(u16 value,void *dst,u32 bytes)
{ assert(value==0);assert((u16*)dst>=vram&&((u8*)dst+bytes)<=(u8*)(vram+32768)); memset(dst,0,bytes); ++writes; }
static u16 ndsIFCommonDecodePrefilteredGoPixel(const Sprite *s,const void *f,size_t n,u32 x,u32 y)
{ (void)s;(void)f;(void)n; return (u16)(0x8000u|((x+y)&0x7fff)); }
''' + function(source,'ndsIFCommonBakeDirectAsset') + '\n' + function(source,'ndsIFCommonEndAssetActive') + '\n' + function(source,'ndsIFCommonNativeOamPrepareAnnouncement') + r'''
int main(void) {
 unsigned i,before;
 assert(!ndsIFCommonNativeOamPrepareAnnouncement(0));
 assert(!ndsIFCommonNativeOamPrepareAnnouncement(2));
 for(i=0;i<NDS_IFCOMMON_ASSET_COUNT;i++) {
   sNdsIFCommonAssets[i].sprite=&sprite;
   sNdsIFCommonAssets[i].width=sNdsIFCommonAssets[i].height=256;
   sNdsIFCommonAssets[i].tile_count=sNdsIFCommonAssetSpecs[i].tile_count;
 }
 sNdsIFCommonPrepared=1;sNdsIFCommonPreparedFile=&sprite;
 sNdsIFCommonPreparedFileSize=NDS_IFCOMMON_GAME_STATUS_SIZE;
 memset(vram,0x55,sizeof(vram));memcpy(preserved,vram,sizeof(vram));
 assert(ndsIFCommonNativeOamPrepareAnnouncement(0));
 assert(ndsIFCommonEndAssetActive(nNDSIFCommonAssetEndT));
 assert(!ndsIFCommonEndAssetActive(nNDSIFCommonAssetEndG));
 before=writes;assert(ndsIFCommonNativeOamPrepareAnnouncement(0));assert(writes==before);
 /* Failure after partial writes must leave the phase unavailable. */
 sNdsIFCommonAssets[nNDSIFCommonAssetEndM].sprite=NULL;
 assert(!ndsIFCommonNativeOamPrepareAnnouncement(1));
 assert(!sNdsIFCommonAnnounceActive && writes>before);
 assert(!ndsIFCommonEndAssetActive(nNDSIFCommonAssetEndT));
 sNdsIFCommonAssets[nNDSIFCommonAssetEndM].sprite=&sprite;
 assert(ndsIFCommonNativeOamPrepareAnnouncement(1));
 assert(ndsIFCommonEndAssetActive(nNDSIFCommonAssetEndG));
 assert(!ndsIFCommonEndAssetActive(nNDSIFCommonAssetEndP));
 before=writes;assert(ndsIFCommonNativeOamPrepareAnnouncement(1));assert(writes==before);
 assert(!memcmp(vram,preserved,NDS_IFCOMMON_GO_BANK_BYTES));
 assert(!memcmp((u8*)vram+NDS_IFCOMMON_SPARK_BANK_BASE,(u8*)preserved+NDS_IFCOMMON_SPARK_BANK_BASE,
                sizeof(vram)-NDS_IFCOMMON_SPARK_BANK_BASE));
 return 0;
}
'''
    cc = shutil.which('gcc') or shutil.which('clang')
    assert cc
    c, exe = tmp_path/'banks.c', tmp_path/'banks.exe'
    c.write_text(code)
    result = subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],capture_output=True,text=True)
    assert result.returncode == 0, result.stderr
    result = subprocess.run([str(exe)],capture_output=True,text=True)
    assert result.returncode == 0, result.stdout+result.stderr

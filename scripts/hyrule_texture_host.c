/* Runs the production Hyrule loader/release and shared upload helper against
 * a recording GL allocator. Pytest inserts those source functions below. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;
#define TRUE 1
#define FALSE 0
#define NDS_P2_STAGE_HYRULE 1
#define NDS_RENDERER_HW_TRIANGLES 1
#define NDS_RENDERER_BENCHMARK_MODE 0
#define NDS_RENDERER_BENCHMARK_NONE 0
#define NDS_RENDERER_HW_TEXTURE_CACHE_COUNT 128
#define GL_TEXTURE_2D 0
#define GL_RGB16 3
#define GL_RGB8_A5 6
#define GL_RGB32_A3 1
#define GL_RGB256 4
#define GL_RGBA 7
#define GL_TEXTURE_COLOR0_TRANSPARENT (1u<<29)
#define GL_COLOR_TABLE_WIDTH_EXT 1
#define TEXGEN_TEXCOORD 0
#define NDS_PARTICLE_FORMAT_PAL16 GL_RGB16
#define NDS_PARTICLE_FORMAT_A5I3 GL_RGB8_A5
#define NDS_PARTICLE_FORMAT_PAL256 GL_RGB256
#define NDS_FIGHTER_PACKET_DMA_WAIT() (++dma_waits)
typedef s32 (*NDSRendererTextureFillCallback)(u8*,u32,void*);
static u16 sNdsRendererHardwareTextureScratch[4096];
static u32 sNdsRendererHardwareBoundTextureName;
static void *sNdsRendererHardwareActiveTextureEntry;
static const char *asset_path;
static int next_name=1, active_name, upload_count, fail_upload;
static int file_open_count, file_close_count, dma_waits, upload_bytes;
static struct { int live,palette,width; } textures[32];
static int palette_references[32], next_palette=1;
static FILE *ndsRendererHardwareFencedTextureFopen(const char *path,const char *mode)
{ (void)path;file_open_count++;return fopen(asset_path,mode); }
static int ndsRendererHardwareFencedTextureFseek(FILE *f,long offset,int origin)
{ return fseek(f,offset,origin); }
static long ndsRendererHardwareFencedTextureFtell(FILE *f) { return ftell(f); }
static size_t ndsRendererHardwareFencedTextureFread(void *p,size_t s,size_t n,FILE *f)
{ return fread(p,s,n,f); }
static int ndsRendererHardwareFencedTextureFclose(FILE *f)
{ file_close_count++;return fclose(f); }
static void ndsRendererHardwareEndBatch(void) {}
static int ndsRendererHardwareTextureSizeEnum(u32 value,int *out)
{ *out=(int)value;return value==16 || value==32; }
static void ndsRendererHardwareBindTextureState(int name)
{ active_name=name;sNdsRendererHardwareBoundTextureName=(u32)name; }
static int ndsRendererHardwareFencedGlGenTextures(int count,int *name)
{ assert(count==1 && next_name<32);*name=next_name++;textures[*name].live=1;return 1; }
static int ndsRendererHardwareFencedGlDeleteTextures(int count,int *name)
{ assert(count==1);assert(textures[*name].live);textures[*name].live=0;
  if(textures[*name].palette) palette_references[textures[*name].palette]--;return 1; }
static int ndsRendererHardwareFencedGlTexImage2D(int target,int level,int fmt,
    int w,int h,int unused,int params,const void *data)
{ (void)target;(void)level;(void)unused;(void)data; upload_count++;
  assert((fmt==GL_RGB16 || fmt==GL_RGB256)==!!(params&GL_TEXTURE_COLOR0_TRANSPARENT));
  if(upload_count==fail_upload)return 0;
  upload_bytes+=w*h/(fmt==GL_RGB16?2:1);return 1; }
static int ndsRendererHardwareEvictTexture(void *ignored) { (void)ignored;return 0; }
static void glColorTableEXT(int target,int level,int width,int a,int b,const u16 *palette)
{ (void)target;(void)level;(void)a;(void)b;assert(palette && width>0);
  assert(next_palette<32);textures[active_name].palette=next_palette;
  textures[active_name].width=width;palette_references[next_palette++]=1; }
static void glAssignColorTable(int target,int owner)
{ (void)target;assert(textures[owner].live);
  palette_references[textures[active_name].palette]--;
  textures[active_name].palette=textures[owner].palette;
  textures[active_name].width=textures[owner].width;
  palette_references[textures[owner].palette]++; }
static void glGetColorTableParameterEXT(int target,int parameter,int *width)
{ (void)target;assert(parameter==GL_COLOR_TABLE_WIDTH_EXT);*width=textures[active_name].width; }
static void ndsRendererHardwareReleaseIFCommonCloudAtlas(u32 *name);

/* HYRULE_PRODUCTION_CODE */

static int live_names(void)
{ int n=0;for(int i=1;i<32;i++)n+=textures[i].live;return n; }
static int live_palettes(void)
{ int n=0;for(int i=1;i<32;i++){assert(palette_references[i]>=0);n+=palette_references[i]>0;}return n; }
int main(int argc,char **argv)
{
    assert(argc==2);asset_path=argv[1];
    assert(ndsRendererHardwarePrepareHyruleTextures());
    assert(live_names()==7 && live_palettes()==3);
    assert(upload_count==7 && upload_bytes==6272 && file_open_count==1 && file_close_count==1);
    u32 old=ndsRendererHardwareHyruleTextureName(2,4);
    assert(old && old!=ndsRendererHardwareHyruleTextureName(2,0));
    assert(!ndsRendererHardwareHyruleTextureName(2,5));
    assert(!ndsRendererHardwareHyruleTextureName(3,0));
    assert(ndsRendererHardwarePrepareHyruleTextures());
    assert(upload_count==7 && file_open_count==1);
    ndsRendererHardwareReleaseHyruleTextures();
    assert(live_names()==0 && live_palettes()==0 && dma_waits>0);
    ndsRendererHardwareReleaseHyruleTextures();
    assert(!ndsRendererHardwareHyruleTextureName(0,0));
    assert(ndsRendererHardwarePrepareHyruleTextures());
    assert(ndsRendererHardwareHyruleTextureName(2,4)!=old);
    assert(live_names()==7 && live_palettes()==3);
    ndsRendererHardwareReleaseHyruleTextures();
    fail_upload=upload_count+3;
    assert(!ndsRendererHardwarePrepareHyruleTextures());
    assert(live_names()==0 && live_palettes()==0);
    assert(file_open_count==3 && file_close_count==3);
    assert(gNdsHyruleNativeTextureFailCount==1);
    return 0;
}

"""Run the actual per-present score queue/draw code with a captured OAM sink."""
from pathlib import Path
import shutil
import subprocess

from test_menu_repair import function


def test_score_hud_native_queue(tmp_path):
    text = (Path(__file__).resolve().parents[2] / "src/nds/nds_battle_hud.c").read_text()
    code = r'''
#include <assert.h>
#include <stdint.h>
typedef uint32_t u32; typedef int32_t s32; typedef int16_t s16;
typedef uint16_t u16; typedef float f32; typedef int sb32;
#define FALSE 0
#define TRUE 1
#define NDS_BATTLE_HUD_SCORE_MAX 16u
#define NDS_BATTLE_HUD_SCORE_FRAMES 2u
#define NDS_BATTLE_HUD_MAX_OAM 64u
#define NDS_NATIVE_FAILURE_SPRITE 3
#define NDS_NATIVE_FAILURE_REJECTED_PROGRAM 2
#define SpriteSize_64x32 0
#define SpriteColorFormat_16Color 0
#define false 0
typedef struct { s16 x,y,scale; u16 frame; } NDSBattleHudScore;
NDSBattleHudScore sNdsBattleHudScores[16];
u32 sNdsBattleHudScoreFrame=~0u, sNdsBattleHudScoreCount, gNdsFrameCounter;
u32 gNdsBattleHudScoreSubmitCount, gNdsBattleHudScoreDrawCount, gNdsBattleHudScoreFrameMask;
u32 failures, draws, affine[16], palette[16]; s32 x_pos[16], y_pos[16], inverse[16];
int oamSub; u16 *sNdsBattleHudScoreGfx[2];
#define ndsRendererRecordNativeFailure(a,b,c,d,e,f,g) (++failures)
void oamRotateScale(void *o,int i,int a,int sx,int sy) { (void)o;(void)a;assert(sx==sy);inverse[i-4]=sx; }
void oamSet(void *o,int id,int x,int y,int p,int pal,int sz,int fmt,void *gfx,int af,int dbl,int h,int fx,int fy,int mosaic) {
    (void)o;(void)id;(void)p;(void)sz;(void)fmt;(void)gfx;(void)dbl;(void)h;(void)fx;(void)fy;(void)mosaic;
    x_pos[draws]=x;y_pos[draws]=y;palette[draws]=pal;affine[draws++]=af;
}
'''
    for name in ("ndsBattleHudSourceCoord", "ndsBattleHudSubmitScoreParticle", "ndsBattleHudDrawScores"):
        code += function(text, name)
    code += r'''
int main(void) {
    u32 next=10; gNdsFrameCounter=1;
    assert(ndsBattleHudSubmitScoreParticle(0,55,223,128));
    assert(ndsBattleHudSubmitScoreParticle(1,125,223,64));
    ndsBattleHudDrawScores(&next);
    assert(next==12 && draws==2 && gNdsBattleHudScoreFrameMask==3);
    assert(x_pos[0]==12 && y_pos[0]==162 && palette[0]==13 && palette[1]==14);
    assert(inverse[0]==256 && inverse[1]==512 && affine[0]==4 && affine[1]==5);
    gNdsFrameCounter=2;
    assert(ndsBattleHudSubmitScoreParticle(0,55,223,-3));
    assert(sNdsBattleHudScoreCount==1 && sNdsBattleHudScores[0].scale==-6);
    for (u32 i=1;i<16;++i) assert(ndsBattleHudSubmitScoreParticle(1,125,223,128));
    assert(!ndsBattleHudSubmitScoreParticle(1,125,223,128));
    assert(sNdsBattleHudScoreCount==16 && failures==1);
    gNdsFrameCounter=3;
    assert(!ndsBattleHudSubmitScoreParticle(2,0,0,128));
    assert(sNdsBattleHudScoreCount==0 && failures==2);
    return 0;
}
'''
    source, binary = tmp_path / "score.c", tmp_path / "score.exe"
    source.write_text(code)
    cc = shutil.which("gcc") or shutil.which("clang")
    assert cc
    build = subprocess.run([cc, "-std=c11", str(source), "-o", str(binary)], capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True)
    assert run.returncode == 0, run.stderr

"""Execute the 1P CSS backend wrappers: ownership and visibility."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]


class PreviewLifetimeTests(unittest.TestCase):
    def test_actual_c_wrappers(self):
        source = (ROOT / 'src/import/battleship_mnplayers1pgame.c').read_text()
        names = ('ndsMNPlayers1PGameMakeFighter',
                 'ndsMNPlayers1PGameDestroyFighter',
                 'ndsMNPlayers1PGameDraw')
        wrappers = '\n'.join(function(source, name) for name in names)
        harness = r'''
#include <assert.h>
#include <stddef.h>
#define NDS_RENDERER_HW_TRIANGLES 1
#define NDS_RENDERER_PROFILE_LEVEL 0
typedef unsigned u32;
typedef int s32;
typedef struct { u32 nds_slot; } FTStruct;
typedef struct { unsigned flags; FTStruct fp; } GObj;
typedef struct { int player; } FTDesc;
#define GOBJ_FLAG_HIDDEN 1
static struct { GObj *player; } sMNPlayers1PGameSlot;
static GObj object;
static int phase, enabled, viewport, draws;
static GObj *registered;
static FTStruct *ftGetStruct(GObj *g) { return &g->fp; }
static void ndsFighterRendererInvalidateMaterialCachesForSlot(u32 slot) {
    assert(slot == 2); assert(phase == 0); phase = 1;
}
static GObj *ftManagerMakeFighter(FTDesc *d) {
    assert(phase == 1 && d->player == 2); phase = 2; return &object;
}
static void ndsFighterManagerRegisterDisplayFighter(GObj *g, u32 slot) {
    assert(slot == 2); registered = g;
    if (g) { assert(phase == 2); g->fp.nds_slot = slot; }
    else { assert(phase == 0); phase = 1; }
}
static void ftManagerDestroyFighter(GObj *g) {
    assert(g == &object && registered == NULL && phase == 1); phase = 2;
}
static void ndsPlatformSet3DLayerEnabled(int b) { enabled = b; }
static void ndsPlatformSet3DViewportSource(int a,int b,int c,int d) {
    assert(a == 10 && b == 10 && c == 310 && d == 230); viewport = 1;
}
static void gcDrawAll(void) { assert(viewport == 1); draws++; }
static void ndsPlatformReset3DViewport(void) { viewport = 0; }
'''
        main = r'''
int main(void) {
    FTDesc d = {2};
    assert(ndsMNPlayers1PGameMakeFighter(&d) == &object);
    assert(registered == &object && object.fp.nds_slot == 2);
    sMNPlayers1PGameSlot.player = &object;
    ndsMNPlayers1PGameDraw(); assert(enabled && !viewport && draws == 1);
    object.flags = GOBJ_FLAG_HIDDEN;
    ndsMNPlayers1PGameDraw(); assert(!enabled && !viewport && draws == 2);
    sMNPlayers1PGameSlot.player = NULL;
    ndsMNPlayers1PGameDraw(); assert(!enabled && !viewport && draws == 3);
    phase = 0; ndsMNPlayers1PGameDestroyFighter(&object); assert(phase == 2);
    return 0;
}
'''
        cc = shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(cc)
        with tempfile.TemporaryDirectory() as directory:
            c = Path(directory) / 'test.c'
            exe = Path(directory) / 'test.exe'
            c.write_text(harness + wrappers + main)
            subprocess.run([cc, '-std=c11', '-Wall', '-Werror', str(c),
                            '-o', str(exe)], check=True, capture_output=True)
            subprocess.run([str(exe)], check=True, capture_output=True)


if __name__ == '__main__':
    unittest.main()

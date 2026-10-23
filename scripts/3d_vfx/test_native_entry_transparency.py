"""Source fades must never select DS alpha-zero wireframe rendering."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/menus'))
from source_test_helpers import function, source


def test_zero_coverage_is_culled_before_texture_or_primitive_submission(tmp_path):
    runtime = source(ROOT / 'src/nds/nds_renderer_native_common.c')
    submit = function(runtime, 'ndsRendererSubmitNativeEntryEffect')
    start = submit.index('polygon_alpha = ndsRendererHardwareAlpha(stats, NULL);')
    end = submit.index('lit = ndsRendererHardwareLitShadeCombine(stats);', start)
    guard = submit[start:end]
    assert submit.index('ndsRendererEntryEffectApplyLiveMaterial(material, stats);') < start
    assert submit.index('sNdsRendererEntryEffectModelview[root_slot] =') < start
    # The full path: nothing is bound or begun between the alpha read and the
    # cull, and the bind and the batch follow it.
    assert 'ndsRendererHardwareBindTextureName' not in guard
    assert 'ndsRendererHardwareBeginTriangleBatch' not in guard
    assert end < submit.index('ndsRendererHardwareBindTextureName(stats, texture_name);', start)
    assert end < submit.index('ndsRendererHardwareBeginTriangleBatch(', start)
    # The replay path skips a group the full path culled (NDS_ENTRY_STATE_SKIP)
    # before its own bind.
    replay_skip = submit.index('if ((state->flags & NDS_ENTRY_STATE_SKIP) != 0u)')
    assert replay_skip < submit.index('ndsRendererHardwareBindTextureName(stats, texture_name);')
    assert 'ndsRendererHardwarePolyFmt(stats, polygon_alpha)' in submit
    # Execute the actual submission guard, with all possible DS alpha levels.
    program = '''
#include <assert.h>
#include <stddef.h>
typedef unsigned u32;
static unsigned alpha, writes;
/* The submit guard counts zero-coverage skips on its own witness (HEAD
 * 381f24af321); the harness only needs it declared. */
static u32 gNdsEntryEffectNativeAlphaSkipCount;
static u32 ndsRendererHardwareAlpha(const void *stats, const void *v)
{ (void)stats; (void)v; return alpha; }
/* A recording pass marks the culled group so the replay path skips it. */
#define FALSE 0
#define NDS_ENTRY_STATE_SKIP 0x01u
static u32 state_recording = 1;
static struct { u32 flags; } state_groups[32];
int main(void) {
  const void *stats=NULL;
  u32 group_offset = 0;
  for (alpha=0; alpha<32; ++alpha, ++group_offset) {
    u32 polygon_alpha;
''' + guard + '''
    assert(polygon_alpha!=0); ++writes;
  }
  assert(writes==31); assert(gNdsEntryEffectNativeAlphaSkipCount==1);
  assert(state_groups[0].flags==NDS_ENTRY_STATE_SKIP);
  assert(state_groups[1].flags==0); return 0;
}
'''
    cc = shutil.which('gcc') or shutil.which('clang')
    assert cc
    c, exe = tmp_path / 'alpha.c', tmp_path / 'alpha.exe'
    c.write_text(program)
    built = subprocess.run([cc, '-std=c99', '-Wall', '-Werror', str(c), '-o', str(exe)], capture_output=True, text=True)
    assert built.returncode == 0, built.stderr
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stderr

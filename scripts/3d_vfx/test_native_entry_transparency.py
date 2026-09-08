"""Source fades must never select DS alpha-zero wireframe rendering."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/menus'))
from source_test_helpers import function


def test_zero_coverage_is_culled_before_texture_or_primitive_submission(tmp_path):
    runtime = (ROOT / 'src/nds/nds_renderer_native_common.c').read_text()
    submit = function(runtime, 'ndsRendererSubmitNativeEntryEffect')
    start = submit.index('polygon_alpha = ndsRendererHardwareAlpha(stats, NULL);')
    end = submit.index('lit = ndsRendererHardwareLitShadeCombine(stats);', start)
    guard = submit[start:end]
    assert submit.index('ndsRendererEntryEffectApplyLiveMaterial(material, stats);') < start
    assert submit.index('sNdsRendererEntryEffectModelview[root_index] =') < start
    assert end < submit.index('ndsRendererHardwareBindTextureName(stats, texture_name);')
    assert end < submit.index('ndsRendererHardwareBeginTriangleBatch(')
    assert 'ndsRendererHardwarePolyFmt(stats, polygon_alpha)' in submit
    # Execute the actual submission guard, with all possible DS alpha levels.
    source = '''
#include <assert.h>
#include <stddef.h>
typedef unsigned u32;
static unsigned alpha, writes;
static u32 ndsRendererHardwareAlpha(const void *stats, const void *v)
{ (void)stats; (void)v; return alpha; }
int main(void) {
  const void *stats=NULL;
  for (alpha=0; alpha<32; ++alpha) {
    u32 polygon_alpha;
''' + guard + '''
    assert(polygon_alpha!=0); ++writes;
  }
  assert(writes==31); return 0;
}
'''
    cc = shutil.which('gcc') or shutil.which('clang')
    assert cc
    c, exe = tmp_path / 'alpha.c', tmp_path / 'alpha.exe'
    c.write_text(source)
    built = subprocess.run([cc, '-std=c99', '-Wall', '-Werror', str(c), '-o', str(exe)], capture_output=True, text=True)
    assert built.returncode == 0, built.stderr
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stderr

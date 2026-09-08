"""Exercise real native matrix-validity stores across the 32-root boundary."""
import re
import shutil
import subprocess
from pathlib import Path


def test_new_effect_roots_cannot_alias_the_pipe_matrix(tmp_path):
    repo = Path(__file__).resolve().parents[2]
    assets = (repo / "src/nds/nds_renderer_assets.c").read_text()
    runtime = (repo / "src/nds/nds_renderer_native_common.c").read_text()
    macro = re.search(r"#define NDS_ENTRY_EFFECT_MASK_WORDS[^\n]+", assets)[0]
    declaration = re.search(
        r"static u32 sNdsRendererEntryEffectModelviewValidMask[^;]+;", assets)[0]
    mark = re.search(
        r"sNdsRendererEntryEffectModelviewValidMask\[root_index >> 5\]\s*\|=\s*[^;]+;",
        runtime)[0]
    read = re.search(
        r"sNdsRendererEntryEffectModelviewValidMask\[source_root >> 5\]\s*&\s*"
        r"\(1u << \(source_root & 31u\)\)", runtime)[0]
    source = ("#include <stdint.h>\n#include <assert.h>\n#include <string.h>\n"
              "typedef uint32_t u32;\n#define NDS_ENTRY_EFFECT_ROOT_COUNT 35u\n" +
              macro + "\n" + declaration + "\n" +
              "static void mark(u32 root_index) { " + mark + " }\n" +
              "static u32 valid(u32 source_root) { return (" + read + ") != 0; }\n" +
              "int main(void) { unsigned i; mark(0); mark(31); mark(32); mark(34);"
              "for (i=0; i<35; i++) assert(valid(i)==(i==0||i==31||i==32||i==34));"
              "memset(sNdsRendererEntryEffectModelviewValidMask,0,"
              "sizeof(sNdsRendererEntryEffectModelviewValidMask));"
              "mark(32); assert(!valid(0) && valid(32) && !valid(31)); return 0; }")
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler, "A host C compiler is required"
    c_file, binary = tmp_path / "root_masks.c", tmp_path / "root_masks.exe"
    c_file.write_text(source)
    compiled = subprocess.run([compiler, "-std=c99", "-Wall", "-Werror",
                               str(c_file), "-o", str(binary)],
                              capture_output=True, text=True, timeout=60)
    assert compiled.returncode == 0, compiled.stdout + compiled.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert run.returncode == 0, run.stdout + run.stderr

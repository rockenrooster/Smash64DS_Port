"""Execute the reset encoder and enforce its width/alignment boundary."""
from pathlib import Path
import shutil
import subprocess


def test_reset_words_preserve_values_and_reject_narrow_targets(tmp_path):
    text = (Path(__file__).resolve().parents[1] / "src/port/taskman_seam_core.c").read_text()
    helper = text[text.index("typedef u32 NDSDiagnosticWord"):text.index("void ndsResetStartupDiagnostics(void)")]
    preamble = "#include <stdint.h>\n#include <assert.h>\ntypedef uint32_t u32;\n"
    main = r'''
volatile u32 a = 13, b = 14, c = 15, untouched = 16;
volatile int32_t signed_word = 17;
int main(void) {
    static const uintptr_t words[] = {
        NDS_DIAG_WORD(a,0), NDS_DIAG_WORD(b,1), NDS_DIAG_WORD(c,2),
        NDS_DIAG_WORD(signed_word,1)
    };
    ndsResetDiagnosticWords(words,4);
    assert(a == 0 && b == UINT32_MAX && c == 1 && signed_word == -1);
    assert(untouched == 16);
    ndsResetDiagnosticWords(words,0);
    assert(a == 0 && b == UINT32_MAX && c == 1);
    return 0;
}
'''
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler
    source, binary = tmp_path / "reset.c", tmp_path / "reset.exe"
    source.write_text(preamble + helper + main)
    subprocess.run([compiler, "-std=c11", "-O2", str(source), "-o", str(binary)], check=True, capture_output=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    for declaration in ("uint16_t bad;", "float bad;", "uint32_t bad __attribute__((aligned(1))); "):
        source.write_text(preamble + helper + declaration +
                          "static const uintptr_t words[] = {NDS_DIAG_WORD(bad,0)}; int main(void) {return 0;}")
        result = subprocess.run([compiler, "-std=c11", str(source), "-o", str(binary)], capture_output=True)
        assert result.returncode != 0, declaration

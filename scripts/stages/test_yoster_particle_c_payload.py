"""Compile the actual Yoster C arrays and compare their bytes with source.

Text-only bank checks missed extra braces that made C discard all but the
first byte. No shared generated files are written by this test.
"""
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import generate_nds_particle_banks as banks


class YosterParticlePayloadTests(unittest.TestCase):
    def test_compiled_arrays_equal_source(self):
        cc = shutil.which('gcc') or shutil.which('clang')
        if cc is None:
            self.skipTest('Host C compiler unavailable')
        banks.YOSTER_BAKE_ENABLED = True
        pack = banks.build_pack(ROOT)
        emitted = banks.render_inc(pack)
        section = re.search(r'#if NDS_P2_STAGE_YOSTER\n(.*?)\n#endif',
                            emitted, re.DOTALL).group(1)
        source = ('#include <stdint.h>\n#include <stdio.h>\n'
                  'typedef uint8_t u8; typedef uint32_t u32;\n' + section +
                  '\nint main(int argc, char **argv) {\n'
                  'FILE *f = argc == 2 ? fopen(argv[1], "wb") : NULL;\n'
                  'if (!f) return 1;\n'
                  'fwrite(gNdsYosterScriptOffsets, 1, sizeof(gNdsYosterScriptOffsets), f);\n'
                  'fwrite(gNdsYosterTextureDims, 1, sizeof(gNdsYosterTextureDims), f);\n'
                  'fwrite(gNdsYosterScriptBank, 1, sizeof(gNdsYosterScriptBank), f);\n'
                  'return fclose(f); }\n')
        expected = (b''.join(x.to_bytes(4, sys.byteorder)
                             for x in pack['yoster']['offsets']) +
                    bytes(x for row in pack['yoster']['texture_rows'] for x in row) +
                    banks.load_o2r_blob(ROOT, *banks.YOSTER_SCRIPT_BANK))
        with tempfile.TemporaryDirectory() as temp:
            temp = Path(temp)
            cfile, exe, data = temp / 'bank.c', temp / 'bank.exe', temp / 'bank.bin'
            cfile.write_text(source, encoding='utf-8')
            subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                            str(cfile), '-o', str(exe)], check=True,
                           capture_output=True)
            subprocess.run([str(exe), str(data)], check=True)
            self.assertEqual(data.read_bytes(), expected)
            # Reintroduce the offending bank braces: warnings-as-errors must
            # reject the generated program before corrupted data can ship.
            broken = source.replace('aligned(4))) = {', 'aligned(4))) = {{')
            broken = broken.replace('\n};\n\nint main', '\n}};\n\nint main')
            self.assertNotEqual(broken, source)
            cfile.write_text(broken, encoding='utf-8')
            result = subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                                     '-c', str(cfile), '-o', str(temp / 'bad.o')],
                                    capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(b'excess', result.stderr)


if __name__ == '__main__':
    unittest.main()

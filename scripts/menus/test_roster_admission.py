"""Preprocess the production roster guard with every individual admission flag.

The Polygon expansion left unmatched parentheses in a macro hidden from normal
two-player builds. This checks its real preprocessor expansion, including each
disabled kind, against the BattleShip ordinal contract (ft/ftdef.h:1090-1125).
"""
from pathlib import Path
import re
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
KINDS = {
    2: 'DONKEY', 3: 'SAMUS', 4: 'LUIGI', 5: 'LINK', 6: 'YOSHI',
    7: 'CAPTAIN', 8: 'KIRBY', 9: 'PIKACHU', 10: 'PURIN', 11: 'NESS',
    13: 'MMARIO', 14: 'NMARIO', 15: 'NFOX', 16: 'NDONKEY',
    17: 'NSAMUS', 18: 'NLUIGI', 19: 'NLINK', 20: 'NYOSHI',
    21: 'NCAPTAIN', 22: 'NKIRBY', 23: 'NPIKACHU', 24: 'NPURIN',
    25: 'NNESS', 26: 'GDONKEY',
}


class RosterAdmissionTests(unittest.TestCase):
    def test_each_flag_admits_only_its_source_kind(self):
        source = (ROOT / 'src/port/nds_match_config.c').read_text()
        macro = re.search(
            r'^#define NDS_P2_KIND_ADMITTED\(k\).*?(?=^#if)',
            source, re.MULTILINE | re.DOTALL).group(0)
        cc = shutil.which('arm-none-eabi-gcc') or shutil.which('gcc')
        if cc is None:
            candidate = Path('C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe')
            cc = str(candidate) if candidate.exists() else None
        if cc is None:
            self.skipTest('C preprocessor unavailable')
        cases = [set(), set(KINDS)] + [{kind} for kind in KINDS]
        for enabled in cases:
            with self.subTest(enabled=sorted(enabled)):
                text = ''.join(
                    f'#define NDS_P2_{flag} {int(kind in enabled)}\n'
                    for kind, flag in KINDS.items()) + macro
                for kind in range(-1, 29):
                    expected = int(kind in (0, 1) or kind in enabled)
                    text += (f'#if !!NDS_P2_KIND_ADMITTED({kind}) != {expected}\n'
                             f'#error incorrect_admission_{kind}\n#endif\n')
                result = subprocess.run(
                    [cc, '-E', '-P', '-x', 'c', '-'], input=text,
                    text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()

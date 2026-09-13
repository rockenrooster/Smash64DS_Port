#!/usr/bin/env python3
"""Lossless s8 storage proof for the staffroll credit character-ID tables.

The campaign links the staffroll credit tables into battle-resident RAM. The
DS overlay (scripts/import-overlays/battleship/
src_sc_sccommon_scstaffroll.patch) stores five of them as s8 instead of s32:
dSCStaffrollNameCharacters, dSCStaffrollJobCharacters,
dSCStaffrollStaffRoleCharacters, dSCStaffrollCompanyCharacters and
dSCStaffrollCompanyIDs. This test proves, against the REAL artifacts:

1. The real overlay generator runs clean and emits the patched scstaffroll.c
   (s8 declarations, credits/*.narrow includes) plus the four narrowed
   initializer files derived from the pristine credits/*.encoded files.
2. A C program compiled with -Wall -Wextra -Werror from the REAL
   source-generated values (pristine credits/*.encoded as s32, generated
   credits/*.narrow as s8 via the verbatim patched declarations) round-trips
   every element exactly, preserves every reader guard predicate, keeps every
   sprite-table index in bounds, and keeps every metadata start/count window
   inside its table.

Run: python scripts/test_staffroll_data_width.py
"""

from __future__ import annotations

import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP_SRC = ROOT / 'decomp/BattleShip-main/decomp/src'
CREDITS = DECOMP_SRC / 'credits'
SCROLL_SRC = DECOMP_SRC / 'sc/sccommon/scstaffroll.c'
GMDEF = DECOMP_SRC / 'gm/gmdef.h'
SCTYPES = DECOMP_SRC / 'sc/sctypes.h'
SCDEF = DECOMP_SRC / 'sc/scdef.h'
GENERATOR = ROOT / 'scripts/generate-battleship-import-overlay.ps1'
SCRATCH = ROOT / 'builds/tmp-staffroll-datawidth-test'

TABLES = (
    ('NameCharacters', 'staff'),
    ('JobCharacters', 'titles'),
    ('StaffRoleCharacters', 'info'),
    ('CompanyCharacters', 'companies'),
)

DECL_RE = re.compile(
    r'((?:s32|s8)\s+dSCStaffroll(?:NameCharacters|JobCharacters|'
    r'StaffRoleCharacters|CompanyCharacters|CompanyIDs)\[.*?\]\s*=\s*\n\{\n.*?\n\};)',
    re.DOTALL,
)


def extract_decl(text, name):
    found = [m.group(1) for m in DECL_RE.finditer(text)
             if 'dSCStaffroll%s' % name in m.group(1)]
    assert len(found) == 1, 'expected one %s declaration, found %d' % (name, len(found))
    return found[0]


class StaffrollDataWidthTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        for tool in ('git',):
            assert shutil.which(tool) is not None, '%s is required' % tool
        cls.pwsh = shutil.which('pwsh') or shutil.which('powershell')
        assert cls.pwsh is not None, 'pwsh/powershell is required'
        cls.cc = shutil.which('gcc') or shutil.which('clang')
        assert cls.cc is not None, 'gcc or clang is required'
        for path in (SCROLL_SRC, GMDEF, SCTYPES, SCDEF, GENERATOR):
            assert path.is_file(), 'missing source artifact %s' % path
        for _, short in TABLES:
            assert (CREDITS / ('%s.credits.encoded' % short)).is_file()
            assert (CREDITS / ('%s.credits.metadata' % short)).is_file()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(SCRATCH, ignore_errors=True)

    def run_generator(self):
        if SCRATCH.exists():
            shutil.rmtree(SCRATCH)
        result = subprocess.run(
            [self.pwsh, '-NoProfile', '-ExecutionPolicy', 'Bypass',
             '-File', str(GENERATOR), '-OutputRoot', str(SCRATCH)],
            capture_output=True, timeout=300)
        self.assertEqual(result.returncode, 0, result.stderr.decode())
        return SCRATCH / 'src/sc/sccommon/scstaffroll.c'

    def test_overlay_declares_narrow_storage(self):
        patched = self.run_generator()
        narrow_dir = patched.parent / 'credits'
        for _, short in TABLES:
            narrow = narrow_dir / ('%s.credits.narrow' % short)
            self.assertTrue(narrow.is_file(), 'missing %s' % narrow)
            self.assertGreater(narrow.stat().st_size, 0)
        text = patched.read_text(encoding='utf-8')
        for name, short in TABLES:
            self.assertIn('s8 dSCStaffroll%s[/* */] =' % name, text)
            self.assertIn('#include "credits/%s.credits.narrow"' % short, text)
            self.assertNotIn('s32 dSCStaffroll%s[/* */] =' % name, text)
        self.assertIn('s8 dSCStaffrollCompanyIDs[/* */] =', text)
        self.assertNotIn('s32 dSCStaffrollCompanyIDs[/* */] =', text)
        self.assertIn('s8 *cadd, *cbase = &dSCStaffrollStaffRoleCharacters[0];', text)
        # Adjacent non-ID storage the patch must not touch.
        self.assertIn('s32 dSCStaffrollUnused0x80136794[/* */] =', text)
        self.assertIn('SCStaffrollText dSCStaffrollStaffRoleTextInfo[/* */] =', text)

    def test_narrow_values_match_source_and_readers(self):
        patched = self.run_generator()
        pristine = SCROLL_SRC.read_text(encoding='utf-8')
        patched_text = patched.read_text(encoding='utf-8')

        # Real DS declarations, verbatim from the patched overlay copy; the
        # credits/*.narrow includes resolve through the overlay tree layout.
        ds_decls = [extract_decl(patched_text, n) for n, _ in TABLES]
        ds_decls.append(extract_decl(patched_text, 'CompanyIDs'))
        # Source-truth declarations, verbatim from pristine source, renamed so
        # both layouts coexist in one translation unit.
        src_decls = []
        for name, _ in TABLES:
            src_decls.append(extract_decl(pristine, name).replace(
                'dSCStaffroll%s' % name, 'src_SCStaffroll%s' % name, 1))
        src_decls.append(extract_decl(pristine, 'CompanyIDs').replace(
            'dSCStaffrollCompanyIDs', 'src_SCStaffrollCompanyIDs', 1))

        # Real SCStaffrollText layout and SCStaffrollCompany enum, verbatim.
        struct = re.search(r'struct SCStaffrollText\n\{.*?\n\};',
                           SCTYPES.read_text(encoding='utf-8'), re.DOTALL)
        self.assertIsNotNone(struct)
        scdef_text = SCDEF.read_text(encoding='utf-8')
        struct_typedef = re.search(r'typedef struct SCStaffrollText\s+SCStaffrollText;',
                                   scdef_text)
        self.assertIsNotNone(struct_typedef)
        enum = re.search(r'typedef enum SCStaffrollCompany\n\{.*?\n\} SCStaffrollCompany;',
                         scdef_text, re.DOTALL)
        self.assertIsNotNone(enum)

        # Real sprite-table sizes: entry rows only (the two TextBox bracket
        # SObj references are not entries; the BracketOpen/BracketClose and
        # EAccent glyph rows inside the arrays are).
        namejob_rows = len(re.findall(r'^\s*\{[^}\n]*&llSCStaffrollNameAndJob',
                                      pristine, re.MULTILINE))
        textbox_rows = len(re.findall(r'^\s*\{[^}\n]*&llSCStaffrollTextBox',
                                      pristine, re.MULTILINE))
        self.assertEqual(namejob_rows, 56)
        self.assertEqual(textbox_rows, 74)

        gmdef = GMDEF.as_posix()
        program = '\n'.join([
            '#include <stdint.h>',
            '#include <stdio.h>',
            'typedef int8_t s8; typedef int32_t s32; typedef uint32_t u32;',
            '#include "%s"' % gmdef,
            struct.group(0),
            struct_typedef.group(0),
            enum.group(0),
        ] + src_decls + ds_decls + [
            '#pragma GCC diagnostic push',
            '#pragma GCC diagnostic ignored "-Wmissing-braces"',
            '/* Verbatim source shape: flat per-struct initializers. The',
            ' * pragma covers only these four metadata declarations; the',
            ' * narrowed character tables above stay under full -Werror. */',
            'SCStaffrollText meta_Name[] = {',
            '#include "%s"' % (CREDITS / 'staff.credits.metadata').as_posix(),
            '};',
            'SCStaffrollText meta_Job[] = {',
            '#include "%s"' % (CREDITS / 'titles.credits.metadata').as_posix(),
            '};',
            'SCStaffrollText meta_StaffRole[] = {',
            '#include "%s"' % (CREDITS / 'info.credits.metadata').as_posix(),
            '};',
            'SCStaffrollText meta_Company[] = {',
            '#include "%s"' % (CREDITS / 'companies.credits.metadata').as_posix(),
            '};',
            '#pragma GCC diagnostic pop',
            '_Static_assert(GMSTAFFROLL_QUESTION_MARK_PARA_FONT_INDEX <= 127,',
            '    "question-mark write must fit s8");',
            '#define CHECK(c, n) do { if (!(c)) { \\',
            '    printf("FAIL %d\\n", (n)); return (n); } } while (0)',
            'static int check_table(const s32 *src, const s8 *ds, s32 n,',
            '                       s32 space, s32 newline, s32 limit, s32 code) {',
            '    s32 i;',
            '    for (i = 0; i < n; i++) {',
            '        s32 v = (s32)ds[i];',
            '        if (v != src[i]) return code;',
            '        if ((src[i] == space) != (v == space)) return code + 1;',
            '        if ((src[i] == newline) != (v == newline)) return code + 2;',
            '        if ((v != space) && (v != newline) && ((v < 0) || (v >= limit)))',
            '            return code + 3;',
            '    }',
            '    return 0;',
            '}',
            'static int check_meta(const SCStaffrollText *meta, s32 entries, s32 total) {',
            '    s32 i;',
            '    for (i = 0; i < entries; i++) {',
            '        if ((meta[i].character_start < 0) || (meta[i].character_count < 0))',
            '            return 1;',
            '        if (meta[i].character_start + meta[i].character_count > total)',
            '            return 2;',
            '    }',
            '    return 0;',
            '}',
            'int main(void) {',
            '    s32 space = GMSTAFFROLL_ASCII_LETTER_TO_FONT_INDEX(\' \');',
            '    s32 newline = GMSTAFFROLL_ASCII_LETTER_TO_FONT_INDEX(\'\\n\');',
            '    s32 n_name = (s32)(sizeof(src_SCStaffrollNameCharacters) / sizeof(s32));',
            '    s32 n_job = (s32)(sizeof(src_SCStaffrollJobCharacters) / sizeof(s32));',
            '    s32 n_role = (s32)(sizeof(src_SCStaffrollStaffRoleCharacters) / sizeof(s32));',
            '    s32 n_comp = (s32)(sizeof(src_SCStaffrollCompanyCharacters) / sizeof(s32));',
            '    s32 n_ids = (s32)(sizeof(src_SCStaffrollCompanyIDs) / sizeof(s32));',
            '    s32 n_company_meta = (s32)(sizeof(meta_Company) / sizeof(SCStaffrollText));',
            '    s32 i, saved;',
            '    CHECK(sizeof(s8) == 1, 10);',
            '    CHECK(sizeof(SCStaffrollText) == 8, 11);',
            '    CHECK(n_name == (s32)(sizeof(dSCStaffrollNameCharacters) / sizeof(s8)), 12);',
            '    CHECK(n_job == (s32)(sizeof(dSCStaffrollJobCharacters) / sizeof(s8)), 13);',
            '    CHECK(n_role == (s32)(sizeof(dSCStaffrollStaffRoleCharacters) / sizeof(s8)), 14);',
            '    CHECK(n_comp == (s32)(sizeof(dSCStaffrollCompanyCharacters) / sizeof(s8)), 15);',
            '    CHECK(n_ids == (s32)(sizeof(dSCStaffrollCompanyIDs) / sizeof(s8)), 16);',
            '    CHECK(check_table(src_SCStaffrollNameCharacters, dSCStaffrollNameCharacters,',
            '        n_name, space, newline, NAMEJOB_COUNT, 20) == 0, 20);',
            '    CHECK(check_table(src_SCStaffrollJobCharacters, dSCStaffrollJobCharacters,',
            '        n_job, space, newline, NAMEJOB_COUNT, 24) == 0, 24);',
            '    CHECK(check_table(src_SCStaffrollStaffRoleCharacters, dSCStaffrollStaffRoleCharacters,',
            '        n_role, space, newline, TEXTBOX_COUNT, 28) == 0, 28);',
            '    CHECK(check_table(src_SCStaffrollCompanyCharacters, dSCStaffrollCompanyCharacters,',
            '        n_comp, space, newline, TEXTBOX_COUNT, 32) == 0, 32);',
            '    for (i = 0; i < n_ids; i++) {',
            '        s32 v = (s32)dSCStaffrollCompanyIDs[i];',
            '        if (v != src_SCStaffrollCompanyIDs[i]) return 36;',
            '        if ((v != nSCStaffrollCompanyNull) && ((v < 0) || (v >= n_company_meta)))',
            '            return 37;',
            '    }',
            '    CHECK(check_meta(meta_Name, (s32)(sizeof(meta_Name) / sizeof(SCStaffrollText)), n_name) == 0, 40);',
            '    CHECK(check_meta(meta_Job, (s32)(sizeof(meta_Job) / sizeof(SCStaffrollText)), n_job) == 0, 41);',
            '    CHECK(check_meta(meta_StaffRole, (s32)(sizeof(meta_StaffRole) / sizeof(SCStaffrollText)), n_role) == 0, 42);',
            '    CHECK(check_meta(meta_Company, (s32)(sizeof(meta_Company) / sizeof(SCStaffrollText)), n_comp) == 0, 43);',
            '    saved = 3 * (n_name + n_job + n_role + n_comp + n_ids);',
            '    printf("staffroll-narrow counts name=%d job=%d role=%d company=%d ids=%d saved=%d\\n",',
            '        n_name, n_job, n_role, n_comp, n_ids, saved);',
            '    return 0;',
            '}',
        ]) + '\n'

        with tempfile.TemporaryDirectory() as directory:
            cfile = Path(directory) / 'staffroll_data_width.c'
            exe = Path(directory) / 'staffroll_data_width.exe'
            cfile.write_text(program, encoding='utf-8')
            compile_result = subprocess.run(
                [self.cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                 '-DNAMEJOB_COUNT=%d' % namejob_rows,
                 '-DTEXTBOX_COUNT=%d' % textbox_rows,
                 '-I%s' % DECOMP_SRC.as_posix(),
                 '-I%s' % (SCRATCH / 'src/sc/sccommon').as_posix(),
                 str(cfile), '-o', str(exe)],
                capture_output=True, timeout=120)
            self.assertEqual(compile_result.returncode, 0,
                             compile_result.stderr.decode())
            run_result = subprocess.run([str(exe)], capture_output=True, timeout=60)
            out = run_result.stdout.decode() + run_result.stderr.decode()
            self.assertEqual(run_result.returncode, 0, out)
            self.assertRegex(out, r'saved=\d+')
            print(out.strip())


if __name__ == '__main__':
    unittest.main()

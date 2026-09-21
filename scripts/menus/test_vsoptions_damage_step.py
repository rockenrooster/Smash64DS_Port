"""Enumerate every Damage value through the SHIPPING adjuster arithmetic.

The owner asked for a 5x held repeat on the Damage row (2026-09-21). The risk
in that one-character class of change is not the magnitude, it is the boundary:
the row WRAPS across an inclusive 50..200 domain by adding or subtracting the
domain size, and it stores back into a u8. A step that is not a divisor of
anything in particular can step over the bound from either side, and a signed
intermediate that wraps wrong becomes an out-of-range u8 silently.

So this compiles the adjuster's actual Damage case text out of the shipping C
-- not a Python restatement of it -- and runs all 151 values against both
directions and both input kinds. If someone later changes the wrap law, the
constants or the step, this recompiles the new text and re-derives the answer.
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'src/nds/nds_menu_shell_vsoptions.c'

HARNESS = r'''
#include <stdio.h>
typedef signed int s32;
typedef unsigned char u8;
%(defines)s
static u8 sMenuVsOptionsDamage;
static void ndsUiKitSfx(int kind) { (void)kind; }
static void ndsMenuShellVsOptionsDrawDamage(void) { }
#define NDS_UI_KIT_SFX_VALUE 0

static void damage_adjust(s32 direction)
{
    /* The case body ends in the switch's own `break`, so keep it in one. */
    switch (0) { default:
%(body)s
    }
}

int main(void)
{
    int start;
    int dir;
    static const int dirs[4] = { 1, -1, %(held)d, -%(held)d };
    for (start = %(min)d; start <= %(max)d; start++)
    {
        for (dir = 0; dir < 4; dir++)
        {
            sMenuVsOptionsDamage = (u8)start;
            damage_adjust(dirs[dir]);
            printf("%%d %%d %%d\n", start, dirs[dir],
                   (int)sMenuVsOptionsDamage);
        }
    }
    return 0;
}
'''


def _define(text, name):
    match = re.search(r'#define\s+%s\s+(\S+)' % name, text)
    assert match is not None, 'lost #define %s' % name
    return match.group(1)


class VsOptionsDamageStepTests(unittest.TestCase):
    def setUp(self):
        self.cc = shutil.which('gcc') or shutil.which('clang')
        if self.cc is None:
            self.skipTest('host C compiler required')
        self.text = SOURCE.read_text(encoding='utf-8')

    def _case_body(self):
        """The adjuster's Damage case statements, taken verbatim.

        Scoped to the adjuster on purpose: the row label also appears in the
        surface-lookup switch, and that one returns a texture id.
        """
        adjuster = self.text.index(
            'static void ndsMenuShellVsOptionsAdjust(s32 direction)')
        start = self.text.index('case NDS_MENU_VSOPTIONS_DAMAGE:', adjuster)
        opening = self.text.index('{', start)
        depth = 1
        end = opening + 1
        while depth:
            depth += (self.text[end] == '{') - (self.text[end] == '}')
            end += 1
        return self.text[opening + 1:end - 1]

    def _run(self):
        minimum = int(_define(self.text,
                              'NDS_MENU_VSOPTIONS_DAMAGE_MIN').rstrip('uU'))
        maximum = int(_define(self.text,
                              'NDS_MENU_VSOPTIONS_DAMAGE_MAX').rstrip('uU'))
        held = int(_define(self.text,
                           'NDS_MENU_VSOPTIONS_DAMAGE_HELD_STEP').rstrip('uU'))
        program = HARNESS % {
            'defines': '\n'.join(
                '#define %s %s' % (name, _define(self.text, name))
                for name in ('NDS_MENU_VSOPTIONS_DAMAGE_MIN',
                             'NDS_MENU_VSOPTIONS_DAMAGE_MAX')),
            'body': self._case_body(),
            'held': held,
            'min': minimum,
            'max': maximum,
        }
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            csrc = temp / 'damage.c'
            exe = temp / 'damage.exe'
            csrc.write_text(program, encoding='utf-8', newline='\n')
            built = subprocess.run(
                [self.cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                 str(csrc), '-o', str(exe)], capture_output=True)
            self.assertEqual(built.returncode, 0, built.stderr.decode())
            out = subprocess.check_output([str(exe)], text=True)
        rows = [tuple(int(v) for v in line.split())
                for line in out.strip().splitlines()]
        return minimum, maximum, held, rows

    def test_every_value_stays_in_the_inclusive_domain(self):
        minimum, maximum, _held, rows = self._run()
        self.assertEqual(len(rows), (maximum - minimum + 1) * 4)
        for start, direction, result in rows:
            self.assertGreaterEqual(
                result, minimum,
                'start %d direction %+d left the domain' % (start, direction))
            self.assertLessEqual(
                result, maximum,
                'start %d direction %+d left the domain' % (start, direction))

    def test_result_is_the_wrap_law_not_a_clamp(self):
        minimum, maximum, _held, rows = self._run()
        size = maximum - minimum + 1
        for start, direction, result in rows:
            expected = start + direction
            if expected < minimum:
                expected += size
            if expected > maximum:
                expected -= size
            self.assertEqual(
                result, expected,
                'start %d direction %+d: clamping or truncation' %
                (start, direction))
            # A clamp would have produced the bound itself; prove we did not.
            if start + direction < minimum:
                self.assertNotEqual(result, minimum)
            if start + direction > maximum:
                self.assertNotEqual(result, maximum)

    def test_taps_move_one_and_held_moves_the_owner_step(self):
        minimum, maximum, held, rows = self._run()
        size = maximum - minimum + 1
        self.assertEqual(held, 5, 'owner asked for a 5x held Damage repeat')
        moved = {}
        for start, direction, result in rows:
            moved[(start, direction)] = (result - start) % size
        for start in range(minimum, maximum + 1):
            self.assertEqual(moved[(start, 1)], 1)
            self.assertEqual(moved[(start, -1)], size - 1)
            self.assertEqual(moved[(start, held)], held)
            self.assertEqual(moved[(start, -held)], size - held)

    def test_the_owner_named_boundary_cases(self):
        _minimum, _maximum, held, rows = self._run()
        table = {(start, direction): result
                 for start, direction, result in rows}
        self.assertEqual(table[(198, held)], 52)
        self.assertEqual(table[(52, -held)], 198)

    def test_only_the_damage_row_takes_the_larger_step(self):
        """Handicap/Team/Stage branch on the SIGN, so the step is row-local."""
        self.assertIn('ndsMenuShellVsOptionsHeldStep', self.text)
        helper = self.text[self.text.index('static s32 ndsMenuShellVsOptionsHeldStep'):]
        helper = helper[:helper.index('\n}')]
        self.assertIn('NDS_MENU_VSOPTIONS_DAMAGE', helper)
        self.assertIn('NDS_MENU_VSOPTIONS_DAMAGE_HELD_STEP', helper)
        self.assertIn('NDS_MENU_VSOPTIONS_DEFAULT_HELD_STEP', helper)
        # The callers must ask the helper, not carry a literal magnitude.
        for call in re.findall(r'ndsMenuShellVsOptionsAdjust\(([^;]*)\);',
                               self.text):
            if 'HeldStep' not in call:
                continue
            self.assertNotRegex(call, r':\s*-?\d')


if __name__ == '__main__':
    unittest.main()

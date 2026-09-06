"""Verify the actual overlay preserves object/audio order in both VS entries."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]
RELATIVE = Path('src/sc/sccommon/scvsbattle.c')
PATCH = ROOT / 'scripts/import-overlays/battleship/src_sc_sccommon_scvsbattle.patch'
AUDIO = ('mpCollisionSetPlayBGM();',
         'func_800269C0_275C0(nSYAudioVoicePublicExcited);')


class BattleConstructorOrderTests(unittest.TestCase):
    def test_both_source_constructors(self):
        cc = shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(cc, 'Host C preprocessor required')
        original = (ROOT / 'decomp/BattleShip-main/decomp' / RELATIVE).read_text()
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            target = temp / RELATIVE
            target.parent.mkdir(parents=True)
            target.write_text(original, newline='\n')
            applied = subprocess.run(['git', '-C', str(temp), 'apply', str(PATCH)],
                                     capture_output=True)
            self.assertEqual(applied.returncode, 0, applied.stderr.decode())
            patched = target.read_text()
            for name in ('scVSBattleStartBattle', 'scVSBattleStartSuddenDeath'):
                with self.subTest(name=name):
                    body = function(patched, name)
                    n64 = subprocess.check_output(
                        [cc, '-E', '-P', '-x', 'c', '-'], input=body, text=True)
                    ds = subprocess.check_output(
                        [cc, '-E', '-P', '-DSSB64_TARGET_NDS', '-x', 'c', '-'],
                        input=body, text=True)
                    source = subprocess.check_output(
                        [cc, '-E', '-P', '-x', 'c', '-'],
                        input=function(original, name), text=True)
                    compact = lambda s: re.sub(r'\s+', '', s)
                    self.assertEqual(compact(n64), compact(source))
                    if name == 'scVSBattleStartBattle':
                        self.assertNotIn('func_kseg1()', ds)
                        start = n64.index('if (!(gSCManagerBackupData.error_flags')
                        end = n64.index('gcMakeDefaultCameraGObj(', start)
                        n64 = n64[:start] + n64[end:]
                    for call in AUDIO:
                        self.assertEqual(ds.count(call), 1)
                        self.assertEqual(n64.count(call), 1)
                    self.assertLess(n64.index(AUDIO[0]),
                                    n64.index('ifCommonTimerMakeInterface('))
                    self.assertGreater(ds.index(AUDIO[0]),
                                       ds.index('lbFadeMakeActor('))
                    self.assertEqual(ds.count('ndsSCVSBattlePrepareBeforeTimer();'), 1)
                    self.assertLess(ds.index('ndsSCVSBattlePrepareBeforeTimer();'),
                                    ds.index('ifCommonTimerMakeInterface('))
                    self.assertLess(ds.index(AUDIO[0]), ds.index(AUDIO[1]))
                    ds = ds.replace('ndsSCVSBattlePrepareBeforeTimer();', '')
                    for call in AUDIO:
                        ds = ds.replace(call, '')
                        n64 = n64.replace(call, '')
                    self.assertEqual(compact(ds), compact(n64))


if __name__ == '__main__':
    unittest.main()

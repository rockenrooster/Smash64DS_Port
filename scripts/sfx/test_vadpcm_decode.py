"""VADPCM decoder oracle: the bank's own loop states, not a shared decoder.

Every looped ALWaveTable stores ALADPCMloop.state[16], the history its encoder
recorded at the loop start, so decoding up to that frame must reproduce it.
The port decoder must match 71/72 B1_sounds1 and 26/26 B1_sounds2 waves.
BattleShip's decoder, which zeroes scale-index-12 frames, must match only
65/72 and 24/26: that falsifier proves the test fails on the defect it guards.
"""
import importlib.util
import struct
import sys
import unittest
from pathlib import Path

_scripts = Path(__file__).resolve().parent
while _scripts.name != "scripts":
    _scripts = _scripts.parent
sys.path.insert(0, str(_scripts))
import _paths  # noqa: E402

import vadpcm_decode  # noqa: E402

TOOLS = _paths.REPO_ROOT / "decomp/BattleShip-main/decomp/tools"
AUDIO = _paths.REPO_ROOT / "decomp/BattleShip-main/BattleShip_o2r/audio"


def _load_tool(name):
    spec = importlib.util.spec_from_file_location(name, TOOLS / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def _loop_state_matches(bank, decode):
    decode_ctl = _load_tool("decode_ctl")
    ctl = (AUDIO / f"{bank}_ctl").read_bytes()[0x44:]
    tbl = (AUDIO / f"{bank}_tbl").read_bytes()[0x44:]
    items = decode_ctl.walk(ctl)
    by_offset = {item["offset"]: item for item in items}
    matched = total = 0
    for wave in items:
        if (wave.get("kind") != "ALWaveTable" or wave.get("type") != 0 or
                not wave.get("loop_off")):
            continue
        start, _end, count = struct.unpack_from(">III", ctl, wave["loop_off"])
        if count == 0:
            continue
        state = list(struct.unpack_from(">16h", ctl, wave["loop_off"] + 12))
        book = by_offset[wave["book_off"]]
        frame = start // 16
        encoded = tbl[wave["base"]:wave["base"] + (frame + 1) * 9]
        pcm = decode(encoded, book["entries"], book["order"], book["npredictors"],
                     initial_state=[0] * book["order"])
        total += 1
        matched += pcm[frame * 16:frame * 16 + 16] == state
    return matched, total


@unittest.skipUnless(AUDIO.is_dir(), "BattleShip O2R audio is not fetched")
class VadpcmLoopStateOracle(unittest.TestCase):
    def test_port_decoder_reproduces_encoder_loop_states(self):
        self.assertEqual(
            _loop_state_matches("B1_sounds1", vadpcm_decode.adpcm_decode), (71, 72))
        self.assertEqual(
            _loop_state_matches("B1_sounds2", vadpcm_decode.adpcm_decode), (26, 26))

    def test_battleship_decoder_fails_the_same_oracle(self):
        original = _load_tool("audio_codec").adpcm_decode
        self.assertEqual(_loop_state_matches("B1_sounds1", original), (65, 72))
        self.assertEqual(_loop_state_matches("B1_sounds2", original), (24, 26))


if __name__ == "__main__":
    unittest.main()

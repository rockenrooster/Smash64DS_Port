"""BGM IMA trellis encoder: scored against the decode the DS SPU runs.

render-audio-bgm.py encodes Mushroom Kingdom's two sequences (2 and 3) with a
step-index Viterbi search, because the greedy nibble choice cannot follow a
voice that holds a plateau and then jumps: the step index decays on the
plateau and every edge pays several samples of slope overload. On a synthetic
pulse train with silent gaps -- that shape -- this checks that:

- the search's reported cost is exactly the squared error of the DS decode of
  the nibbles it emitted, so it optimizes the decoder the hardware runs;
- that decoder reproduces the greedy encoder's own reconstruction, the path
  the 45 accepted greedy tracks already play through on hardware;
- it clearly beats greedy on the pulse train and does not lose on a sine;
- the container keeps the greedy layout (size, header, packet census, loop
  record); only packet payloads differ;
- two runs emit identical bytes, and only sequences 2 and 3 take the search,
  so Dream Land's accepted greedy payload cannot move.
"""
import importlib.util
import math
import struct
import sys
import unittest
from pathlib import Path

_scripts = Path(__file__).resolve().parent
while _scripts.name != "scripts":
    _scripts = _scripts.parent
sys.path.insert(0, str(_scripts))
import _paths  # noqa: E402,F401

_spec = importlib.util.spec_from_file_location(
    "render_audio_bgm", _scripts / "sfx/bgm/render-audio-bgm.py")
rab = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(rab)


def _pulse_train(count: int) -> list[int]:
    """Square-wave notes (plateaus, one-sample edges) with silent gaps."""
    samples = []
    half_periods = (27, 41, 18, 55)
    amplitudes = (2600, 1900, 3100, 1400)
    position = 0
    note = 0
    while position < count:
        half = half_periods[note % len(half_periods)]
        level = amplitudes[note % len(amplitudes)]
        for i in range(1800):
            samples.append(level if (i // half) % 2 == 0 else -level)
        samples.extend([0] * 700)
        position += 2500
        note += 1
    return samples[:count]


def _sine(count: int) -> list[int]:
    return [int(round(4000.0 * math.sin(2.0 * math.pi * 220.0 * i / 22050.0)))
            for i in range(count)]


def _pcm(samples: list[int]) -> bytes:
    return struct.pack(f"<{len(samples)}h", *samples)


class TrellisEncoderTest(unittest.TestCase):
    def test_cost_is_the_ds_decode_error(self):
        for samples in (_pulse_train(3000), _sine(3000)):
            start = samples[0]
            codes, index, cost = rab.trellis_ima_encode_packet(samples, start)
            self.assertEqual(len(codes), len(samples))
            self.assertTrue(all(0 <= code <= 15 for code in codes))
            self.assertTrue(0 <= index <= 88)
            decoded = rab.ds_ima_decode_packet(codes, start, index)
            self.assertEqual(
                sum((s - d) ** 2 for s, d in zip(samples, decoded)), cost)

    def test_decoder_matches_greedy_reconstruction(self):
        samples = _pulse_train(6000)
        predictor = samples[0]
        index = rab.initial_ima_index(samples)
        start = (predictor, index)
        codes, trajectory = [], []
        for sample in samples:
            code, predictor, index = rab.ima_encode_sample(
                sample, predictor, index)
            codes.append(code)
            trajectory.append(predictor)
        self.assertEqual(rab.ds_ima_decode_packet(codes, *start), trajectory)

    def _snr(self, samples, encoder):
        _payload, metadata = rab.build_ima_packets(
            _pcm(samples), 0, False, encoder)
        return metadata["ima_snr_db"]

    def test_beats_greedy_where_greedy_overloads(self):
        samples = _pulse_train(20000)
        greedy = self._snr(samples, rab.IMA_ENCODER_GREEDY)
        trellis = self._snr(samples, rab.IMA_ENCODER_TRELLIS)
        self.assertGreater(trellis, greedy + 6.0, (greedy, trellis))

    def test_no_loss_on_a_smooth_tone(self):
        samples = _sine(20000)
        greedy = self._snr(samples, rab.IMA_ENCODER_GREEDY)
        trellis = self._snr(samples, rab.IMA_ENCODER_TRELLIS)
        self.assertGreaterEqual(trellis, greedy, (greedy, trellis))

    def test_container_layout_is_the_greedy_layout(self):
        samples = _pulse_train(40000)
        loop_byte = 2 * 21111
        greedy, greedy_meta = rab.build_ima_packets(
            _pcm(samples), loop_byte, True, rab.IMA_ENCODER_GREEDY)
        trellis, trellis_meta = rab.build_ima_packets(
            _pcm(samples), loop_byte, True, rab.IMA_ENCODER_TRELLIS)
        self.assertEqual(len(greedy), len(trellis))
        self.assertEqual(greedy[:rab.BGM_IMA_HEADER.size],
                         trellis[:rab.BGM_IMA_HEADER.size])
        for key in ("packet_count", "loop_packet_index", "loop_record_offset"):
            self.assertEqual(greedy_meta[key], trellis_meta[key], key)
        offset = rab.BGM_IMA_HEADER.size
        packet = 0
        while offset < len(greedy):
            record = greedy[offset:offset + rab.BGM_IMA_PACKET.size]
            self.assertEqual(
                record, trellis[offset:offset + rab.BGM_IMA_PACKET.size])
            if packet == greedy_meta["loop_packet_index"]:
                self.assertEqual(offset, greedy_meta["loop_record_offset"])
            count, payload = rab.BGM_IMA_PACKET.unpack(record)
            header = trellis[offset + 8:offset + 12]
            self.assertLessEqual(header[2], 88)
            self.assertEqual(header[3], 0)
            offset += rab.BGM_IMA_PACKET.size + payload
            packet += 1
        self.assertEqual(packet, trellis_meta["packet_count"])
        self.assertEqual(trellis_meta["ima_encoder"], rab.IMA_ENCODER_TRELLIS)

    def test_deterministic(self):
        pcm = _pcm(_pulse_train(12000))
        first, _ = rab.build_ima_packets(pcm, 0, False, rab.IMA_ENCODER_TRELLIS)
        second, _ = rab.build_ima_packets(pcm, 0, False, rab.IMA_ENCODER_TRELLIS)
        self.assertEqual(first, second)

    def test_only_mushroom_kingdom_takes_the_search(self):
        self.assertEqual(rab.TRELLIS_IMA_SEQUENCES, frozenset((2, 3)))
        self.assertNotIn(rab.SEQ_INDEX_PUPUPU, rab.TRELLIS_IMA_SEQUENCES)


if __name__ == "__main__":
    unittest.main()

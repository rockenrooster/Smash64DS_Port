"""Identify source BGA1 music in mixed melonDS disk PCM; not a quality gate.

Foreground sound effects remain present. A normalized match and coherent packet
timeline demonstrate output engagement; they do not prove all audio or lifetimes.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import struct
import sys
import wave

import numpy as np

ROOT = Path(__file__).resolve().parents[2]


def correlate(signal, reference):
    reference = reference - reference.mean()
    energy = float(reference @ reference)
    if not energy or len(reference) > len(signal):
        return np.empty(0)
    size = 1 << (len(signal) + len(reference) - 2).bit_length()
    cross = np.fft.irfft(np.fft.rfft(signal, size) *
                        np.conj(np.fft.rfft(reference, size)), size)[:len(signal)-len(reference)+1]
    sums = np.r_[0., np.cumsum(signal)]
    squares = np.r_[0., np.cumsum(signal * signal)]
    local = squares[len(reference):] - squares[:-len(reference)] - (
        sums[len(reference):] - sums[:-len(reference)])**2 / len(reference)
    values = cross / np.sqrt(np.maximum(local, 1) * energy)
    values[local < len(reference) * 100**2] = 0
    return values


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("wave", type=Path)
    ap.add_argument("asset", type=Path)
    ap.add_argument("--json", required=True, type=Path)
    args = ap.parse_args()
    spec = importlib.util.spec_from_file_location("bgm_ima_reference", ROOT / "scripts/sfx/render-audio-fgm-phase-pack.py")
    decoder = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = decoder
    spec.loader.exec_module(decoder)
    with wave.open(str(args.wave), "rb") as w:
        if (w.getframerate(), w.getnchannels(), w.getsampwidth()) != (48000, 2, 2):
            raise ValueError("expected melonDS stereo S16LE at 48 kHz")
        audio = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").reshape(-1, 2).astype(float).mean(axis=1)
    audio = audio[:len(audio)//4*4].reshape(-1, 4).mean(axis=1)
    rate = 12000
    # melonDS 1.0 audioGetNumSamplesOut uses this input rate. The DS sound
    # channel's /760 timer relative to SPU /1024 gives 512/760 input samples.
    source_hz = 32823.6328125 * 512 / 760
    data = args.asset.read_bytes()
    cursor, sample_start, packets = 40, 0, []
    while sample_start < 16 * 22050:
        n, size = struct.unpack_from("<II", data, cursor)
        pcm = np.asarray(decoder.ima_decode_nibbles(data[cursor+8:cursor+8+size], n), dtype=float)
        reference = np.interp(np.arange(int(n*rate/source_hz))*source_hz/rate, np.arange(n), pcm)
        packets.append((sample_start, reference))
        sample_start += n
        cursor += 8 + size
    whole = np.concatenate([x[1] for x in packets])
    match = correlate(audio, whole)
    anchor = int(match.argmax()) / rate
    rows = []
    for i, (start_sample, reference) in enumerate(packets):
        if not np.var(reference): continue
        start = max(0, int((anchor + start_sample/source_hz - .025) * rate))
        end = min(len(audio)-len(reference), int((anchor + start_sample/source_hz + .025) * rate))
        values = correlate(audio[start:end+len(reference)], reference)
        at = int(values.argmax())
        rows.append({"packet": i, "source_sample_start": start_sample,
                     "capture_start_seconds": (start+at)/rate,
                     "normalized_correlation": float(values[at])})
    report = {"wave_sha256": hashlib.sha256(args.wave.read_bytes()).hexdigest().upper(),
              "asset_sha256": hashlib.sha256(data).hexdigest().upper(),
              "anchor_seconds": anchor, "whole_correlation": float(match.max()),
              "effective_source_hz": source_hz, "packets": rows,
              "limitation": "Mixed foreground SFX; output engagement only, not a complete fidelity gate."}
    args.json.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "packets"}))


if __name__ == "__main__":
    main()

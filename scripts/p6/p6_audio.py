#!/usr/bin/env python3
"""P6 audio (A8): an authored content's sound cues from local recordings.

`scripts/p6/<name>/audio.json` (committed: names and parameters, no sound)
lists each cue's recording under the owner's local folder (`source`,
ignored, never tracked: Ultimate's voices and effects, plan D5-D8), its
kind (voice or effect) and optional trim, gain and pitch
(`pitch_semitones`: played that much higher, and shorter, as a sampler
plays a note), and maps every FGM id the content's scripts, attributes and
kind tables play (its base's: Roy's voices, the vanilla sword swings) to a
cue.

Each cue becomes one entry of the P4 sound pack (`p4_audio.py`, FGP4) under
an id of the content's own window: P6_FGM_BASE plus 0x80 per authored
content in registry order, then the cues in name order. The pack and the
generator (which rewrites the content's scripts and attributes through
`fgm_map`) both derive the ids from audio.json, so neither reads the other's
output.

A cue: the recording read (16-bit WAV), downmixed to mono, its silence
trimmed (below -50 dBFS at either end), peak-normalized to -1 dBFS with the
cue's gain, resampled (band-limited, by FFT) to the kind's rate (read as
recorded at its rate times the pitch ratio) and encoded IMA by the vanilla
pack's encoder. Voices pause with the game, as Remix's voice template
does.

A cue with "frozen" plays its stored render instead (frozen_cue): the
Ultimate-derived cues are rendered once by scripts/p6/p6_freeze.py, so the
build no longer reads the recordings (owner 2026-10-29).
"""
from __future__ import annotations

import json
import math
import sys
import wave
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "scripts" / "p4"))

AUDIO_SCHEMA = "p6-audio-v1"
# Past every Remix cue (the largest is 0x658).
P6_FGM_BASE = 0x700
P6_FGM_WINDOW = 0x80
RATES = {"voice": 16000, "effect": 22050}
FLAG_PAUSE_WITH_GAME = 1 << 1
# Remix's fighter cues play at 124 (FGM.asm's voice template, ds_volume).
CUE_VOLUME = 124
SILENCE_DBFS = -50.0
PEAK_DBFS = -1.0


class AudioError(SystemExit):
    pass


def spec_path(name: str) -> Path:
    return REPO / "scripts" / "p6" / name / "audio.json"


def load(name: str) -> dict | None:
    """The content's audio spec, or None when it has none."""
    path = spec_path(name)
    if not path.exists():
        return None
    spec = json.loads(path.read_text(encoding="utf-8"))
    if spec.get("schema") != AUDIO_SCHEMA:
        raise AudioError(f"{path}: schema {spec.get('schema')!r}, expected {AUDIO_SCHEMA}")
    for cue, row in spec["cues"].items():
        if row.get("kind") not in RATES:
            raise AudioError(f"{path}: cue {cue}: kind {row.get('kind')!r}")
    for old, cue in spec.get("map", {}).items():
        if cue not in spec["cues"]:
            raise AudioError(f"{path}: map {old}: no cue {cue!r}")
    return spec


def cue_ids(name: str) -> dict[str, int]:
    """Each cue's FGM id (the content's window, cues in name order)."""
    import p4_contents
    spec = load(name)
    if spec is None:
        return {}
    authored = [r["name"] for r in p4_contents.load_registry() if "base" in r]
    base = P6_FGM_BASE + P6_FGM_WINDOW * authored.index(name)
    names = sorted(spec["cues"])
    if len(names) > P6_FGM_WINDOW:
        raise AudioError(f"{name}: {len(names)} cues, the window holds {P6_FGM_WINDOW}")
    return {cue: base + i for i, cue in enumerate(names)}


def fgm_map(name: str) -> dict[int, int]:
    """The base's FGM ids the content plays -> its cues' ids."""
    spec = load(name)
    if spec is None:
        return {}
    ids = cue_ids(name)
    return {int(old, 0): ids[cue] for old, cue in spec.get("map", {}).items()}


def _read_wav(path: Path) -> tuple[list[float], int]:
    with wave.open(str(path), "rb") as w:
        if w.getsampwidth() != 2:
            raise AudioError(f"{path}: {8 * w.getsampwidth()}-bit, expected 16")
        channels = w.getnchannels()
        rate = w.getframerate()
        raw = w.readframes(w.getnframes())
    import numpy as np
    pcm = np.frombuffer(raw, dtype="<i2").astype(np.float64) / 32768.0
    if channels > 1:
        pcm = pcm.reshape(-1, channels).mean(axis=1)
    return pcm, rate


def _resample(pcm, rate: float, target: int):
    """Band-limited resampling by FFT (the spectrum truncated or padded)."""
    import numpy as np
    n = len(pcm)
    m = max(1, int(round(n * target / rate)))
    spectrum = np.fft.rfft(pcm)
    keep = m // 2 + 1
    out = np.zeros(keep, dtype=complex)
    k = min(keep, len(spectrum))
    out[:k] = spectrum[:k]
    return np.fft.irfft(out, m) * (m / n)


def render_cue(row: dict, source: Path) -> tuple[list[int], int]:
    """A cue's 16-bit samples and rate."""
    import numpy as np
    path = source / row["file"]
    if not path.exists():
        raise AudioError(f"{path}: missing (the owner's local recordings, plan D5-D8)")
    pcm, rate = _read_wav(path)
    if "trim" in row:
        a, b = row["trim"]
        pcm = pcm[int(a * rate):int(b * rate)] if b else pcm[int(a * rate):]
    level = 10 ** (SILENCE_DBFS / 20)
    loud = np.nonzero(np.abs(pcm) > level)[0]
    if len(loud) == 0:
        raise AudioError(f"{path}: silent")
    pad = int(0.005 * rate)
    pcm = pcm[max(0, loud[0] - pad):loud[-1] + pad + 1]
    target = RATES[row["kind"]]
    pitch = 2.0 ** (float(row.get("pitch_semitones", 0.0)) / 12.0)
    pcm = _resample(pcm, rate * pitch, target)
    peak = float(np.max(np.abs(pcm))) or 1.0
    gain = 10 ** ((PEAK_DBFS + float(row.get("gain_db", 0.0))) / 20) / peak
    pcm = np.clip(pcm * gain, -1.0, 32767 / 32768)
    return [int(round(x * 32767)) for x in pcm], target


def frozen_cue(row: dict) -> tuple[list[int], int]:
    """A cue's stored render ("frozen": {"pcm", "sha256"}): render_cue's
    samples as scripts/p6/p6_freeze.py wrote them, a 16-bit mono WAV at the
    kind's rate under assets/p6-art/<name>/audio (local, rip-derived, never
    tracked), checked against the cue's digest."""
    import hashlib
    import numpy as np
    path = REPO / row["frozen"]["pcm"]
    if not path.exists():
        raise AudioError(f"{path}: not stored (scripts/p6/p6_freeze.py writes it)")
    if hashlib.sha256(path.read_bytes()).hexdigest() != row["frozen"]["sha256"]:
        raise AudioError(f"{path}: not the cue's stored render (re-freeze: scripts/p6/p6_freeze.py)")
    rate = RATES[row["kind"]]
    with wave.open(str(path), "rb") as w:
        if (w.getnchannels(), w.getsampwidth(), w.getframerate()) != (1, 2, rate):
            raise AudioError(f"{path}: not 16-bit mono at {rate} Hz")
        raw = w.readframes(w.getnframes())
    return np.frombuffer(raw, dtype="<i2").astype(int).tolist(), rate


def records(name: str, renderer, source: Path | None = None) -> list[dict]:
    """The content's pack entries (p4_audio.py's record form)."""
    spec = load(name)
    if spec is None:
        return []
    source = source if source is not None else REPO / spec["source"]
    ids = cue_ids(name)
    tick_us = renderer.FGM_TIMER_MICROSECONDS
    out = []
    for cue in sorted(spec["cues"]):
        row = spec["cues"][cue]
        pcm, rate = frozen_cue(row) if "frozen" in row else render_cue(row, source)
        ima = renderer.ima_encode(pcm)
        decoded = renderer.ima_decode(ima, len(pcm))
        metrics = renderer.audio_metrics(pcm, decoded)
        if metrics["ima_snr_db"] < 14.0:
            ima = renderer.ima_encode_lookahead(pcm)
            decoded = renderer.ima_decode(ima, len(pcm))
            metrics = renderer.audio_metrics(pcm, decoded)
        out.append({
            "id": ids[cue],
            "name": f"{name}/{cue}",
            "type": row["kind"],
            "flags": FLAG_PAUSE_WITH_GAME,
            "frequency": rate,
            "volume": CUE_VOLUME,
            "pan": 64,
            "duration_ticks": math.ceil(len(pcm) * 1_000_000 / (rate * tick_us)),
            "note_ticks": [],
            "sample_count": len(pcm),
            "ima": ima,
            "source": row["file"],
            "ima_snr_db": metrics["ima_snr_db"],
        })
    return out


def cue_header() -> str:
    """The authored contents' cue ids for their own code
    (nds_p6_cues.generated.h): NDS_P6_<NAME>_CUE_<CUE> for every cue of
    every authored content, the ids the sound pack and the generator give
    them (cue_ids)."""
    import p4_contents
    lines = [
        "/* Generated by scripts/p6/p6_audio.py --cue-header from scripts/p6/<name>/",
        " * audio.json: each authored content's sound cue ids, for its code. */",
        "#ifndef NDS_P6_CUES_GENERATED_H",
        "#define NDS_P6_CUES_GENERATED_H",
    ]
    for row in p4_contents.load_registry():
        if "base" not in row:
            continue
        ids = cue_ids(row["name"])
        for cue in sorted(ids, key=ids.get):
            lines.append(f"#define NDS_P6_{row['name'].upper()}_CUE_{cue.upper()} {ids[cue]:#x}")
    lines += ["#endif", ""]
    return "\n".join(lines)


def main() -> int:
    import argparse
    ap = argparse.ArgumentParser(description="P6 audio: the authored contents' cue ids as a C header.")
    ap.add_argument("--cue-header", type=Path, required=True)
    args = ap.parse_args()
    text = cue_header()
    # Rewritten only when it changes, so its readers rebuild only then.
    if not args.cue_header.exists() or args.cue_header.read_text(encoding="utf-8") != text:
        args.cue_header.parent.mkdir(parents=True, exist_ok=True)
        args.cue_header.write_text(text, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())

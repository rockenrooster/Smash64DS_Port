#!/usr/bin/env python3
"""VADPCM decoder for the BattleShip audio banks, port-side.

A copy of `adpcm_decode` from decomp/BattleShip-main/decomp/tools/audio_codec.py
(BattleShip, MIT License, Copyright (c) 2026 JRickey and contributors) with one
correction: that decoder turns every frame whose scale index is 12 into
silence-plus-prediction, which garbled Mushroom Kingdom's lead and Yoshi's
Island's channel-4 instrument, among 33 music and 153 sound-effect waves.
`decomp/` is read-only, so the generators load this module instead.
See docs/p2/BUG_NOTES.md, "Inishie and Yoster garble".
"""
import struct  # noqa: F401  (kept with the copied module layout)


ADPCM_FRAME_BYTES = 9
ADPCM_FRAME_SAMPLES = 16


def _clip_s16(v: int) -> int:
    if v > 32767:
        return 32767
    if v < -32768:
        return -32768
    return v


def adpcm_decode(adpcm_bytes: bytes,
                 codebook: list,
                 order: int,
                 npredictors: int,
                 initial_state=None) -> list:
    """Decode VADPCM bytes to a list of s16 PCM samples.

    Args:
      adpcm_bytes: raw bytes; length must be a multiple of 9.
      codebook:    flat list of `npredictors * order * 8` s16 values, in
                   the same on-disk order as the .ctl's ALADPCMBook.
      order:       prediction order (always 2 in baserom; we don't assume).
      npredictors: number of codebook entries.
      initial_state: optional list of `order` s16 values seeding the
                   history (used when decoding into a loop point); zero
                   if None.

    Returns: list of decoded s16 samples (length = nframes * 16).
    """
    # Lay out the codebook as coefs[predictor][k][j] for 0<=k<order, 0<=j<8.
    coefs = []
    for p in range(npredictors):
        rows = []
        base = p * order * 8
        for k in range(order):
            rows.append(list(codebook[base + k * 8 : base + k * 8 + 8]))
        coefs.append(rows)

    state = list(initial_state) if initial_state else [0] * order
    out = []

    for fstart in range(0, len(adpcm_bytes), ADPCM_FRAME_BYTES):
        frame = adpcm_bytes[fstart : fstart + ADPCM_FRAME_BYTES]
        if len(frame) < ADPCM_FRAME_BYTES:
            break

        header = frame[0]
        scale_idx = header >> 4
        predictor_idx = header & 0x0F
        # Scale index 12 is a real scale (1 << 12), not zero. The bank's own
        # ALADPCMloop.state -- the history its encoder recorded at every
        # loop start -- matches this decode for 71/72 B1_sounds1 and 26/26
        # B1_sounds2 looped waves, against 65/72 and 24/26 when 12 decodes
        # as zero. No frame in either bank uses an index above 12.
        scale = 1 << scale_idx

        # 16 signed 4-bit nibbles, pre-multiplied by scale.
        residuals = []
        for b in frame[1:]:
            hi = (b >> 4) & 0x0F
            lo = b & 0x0F
            if hi >= 8:
                hi -= 16
            if lo >= 8:
                lo -= 16
            residuals.append(hi * scale)
            residuals.append(lo * scale)

        pred_book = coefs[predictor_idx]  # order rows of 8 cols

        # Two halves of 8 samples each. Standard VADPCM decode: the
        # codebook coefficients encode the within-half AR structure
        # implicitly, so each half's samples are computed in a single
        # pass against the history state from before the half.
        for half in range(2):
            half_resid = residuals[half * 8 : half * 8 + 8]
            decoded = [0] * 8
            # Prefix sums of (residual << 11) encode each within-half
            # earlier residual's contribution to later samples through
            # the predictor's "newest history" row (book[order-1]).
            for j in range(8):
                acc = 0
                # History from before the half: state[k] paired with
                # book row k.
                for k in range(order):
                    acc += state[k] * pred_book[k][j]
                # Within-half residuals act as additional "history" via
                # the order-1 (newest) row, evaluated at the offset
                # between this sample and the residual position.
                for m in range(j):
                    acc += half_resid[m] * pred_book[order - 1][j - 1 - m]
                acc >>= 11
                decoded[j] = _clip_s16(acc + half_resid[j])
            out.extend(decoded)
            # Update state to last `order` samples for the next half.
            state = decoded[8 - order : 8]

    return out

#!/usr/bin/env python3
"""Demonstrate a boundary difference between float32 and rounded Q12 clocks.

This is a host arithmetic reproduction, NOT a test of an NDS ROM, the complete
pose engine, or reachability of a particular speed/wait combination in gameplay.

Source basis: rockenrooster/Smash64DS_Port, commit
505234457af0b2f3aff00439605ad6b262ceaede:
  decomp/BattleShip-main/decomp/src/ft/ftanim.c
    ftAnimParseDObjFigatree: subtract anim_speed; return if anim_wait > 0.
  src/nds/nds_ft_pose.c
    Q12 wait/speed update and the comment claiming exact event boundaries.
  include/nds/nds_anim_fixed.h
    NDS_R2_AQ_LF = 12; round-to-nearest speed conversion.

For the small positive, finite values tested here, multiplying a float32 by
4096 and rounding half away from zero has the same Q12 result as the project's
bit-based converter. This script does not purport to validate that converter
for arbitrary inputs, overflow, infinities, or NaNs.

Run: python repro_pose_clock_boundary.py
No third-party packages are required. Exit 0 means the documented arithmetic
counterexamples were reproduced, NOT that the game's clock passes parity.
"""
from __future__ import annotations

from dataclasses import asdict, dataclass
import json
import math
import struct


def f32(value: float) -> float:
    """Round through IEEE-754 binary32 storage after each source operation."""
    return struct.unpack('<f', struct.pack('<f', value))[0]


@dataclass(frozen=True)
class CaseResult:
    initial_wait: int
    numerator: int
    denominator: int
    speed_float32: float
    speed_q12: int
    float32_crossing_tick: int
    q12_crossing_tick: int
    float32_wait_at_expected_boundary: float
    q12_wait_at_expected_boundary: int


def run_case(wait: int, numerator: int, denominator: int) -> CaseResult:
    if min(wait, numerator, denominator) <= 0:
        raise ValueError('Inputs must be positive integers.')
    speed = f32(f32(float(numerator)) / f32(float(denominator)))
    speed_q = math.floor(speed * 4096.0 + 0.5)
    if speed_q <= 0:
        raise ValueError('This demonstrator excludes speeds rounding to zero.')
    float_wait = f32(float(wait))
    fixed_wait = wait * 4096
    boundary = wait * denominator // numerator
    if boundary * numerator != wait * denominator:
        raise ValueError('This case must have an integral ideal crossing tick.')
    f_cross = q_cross = 0
    f_at_boundary = 0.0
    q_at_boundary = 0
    for tick in range(1, 10001):
        float_wait = f32(float_wait - speed)
        fixed_wait -= speed_q
        if tick == boundary:
            f_at_boundary, q_at_boundary = float_wait, fixed_wait
        if f_cross == 0 and float_wait <= 0.0:
            f_cross = tick
        if q_cross == 0 and fixed_wait <= 0:
            q_cross = tick
        if f_cross and q_cross and tick >= boundary:
            return CaseResult(wait, numerator, denominator, speed, speed_q,
                              f_cross, q_cross, f_at_boundary, q_at_boundary)
    raise RuntimeError('No boundary within the 10000-tick safety limit.')


def main() -> None:
    results = [run_case(1, 1, 3), run_case(16, 16, 3)]
    for result in results:
        assert result.float32_crossing_tick == 3, result
        assert result.q12_crossing_tick == 4, result
        assert result.q12_wait_at_expected_boundary == 1, result
    control = run_case(1, 1, 2)
    assert control.float32_crossing_tick == control.q12_crossing_tick == 2
    print(json.dumps({
        'scope': 'Host arithmetic only; gameplay reachability and impact unproven',
        'conclusion': 'Q12 boundary-exactness premise is false in general',
        'counterexamples': [asdict(result) for result in results],
        'dyadic_control': asdict(control),
    }, indent=2))


if __name__ == '__main__':
    main()

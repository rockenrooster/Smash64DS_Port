from collections import Counter

import pytest

import generate_nds_native_owners as native


def test_unlit_alpha_zero_refuses_ds_wireframe():
    with pytest.raises(ValueError, match=(
            r"ness high: root 0x6760 unlit run 12 .*POLY_ALPHA\(0\) wireframe")):
        native._collapse_unlit_run_alpha(
            "ness", "high", 0x6760, 12, Counter({0: 3}))


def test_unlit_alpha_spread_is_bounded_to_one_alpha5_step():
    too_wide = native.NDS_NATIVE_UNLIT_ALPHA_MAX_SPREAD + 1
    with pytest.raises(ValueError, match=(
            rf"kirby low: root 0x1234 unlit run 7 source alpha spread "
            rf"100\.\.{100 + too_wide} exceeds "
            rf"{native.NDS_NATIVE_UNLIT_ALPHA_MAX_SPREAD}")):
        native._collapse_unlit_run_alpha(
            "kirby", "low", 0x1234, 7, Counter({100: 2, 100 + too_wide: 1}))


def test_unlit_alpha_spread_at_bound_keeps_dominant_alpha():
    alphas = Counter({200: 4, 200 + native.NDS_NATIVE_UNLIT_ALPHA_MAX_SPREAD: 1})
    assert native._collapse_unlit_run_alpha(
        "ness", "high", 0x6760, 12, alphas) == (200 * 31 + 127) // 255

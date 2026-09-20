import generate_nds_native_kirby_vulcan as vulcan


def test_rgba_palette_partition_has_no_alpha_overlap():
    pixels = [(i * 8, 0, 0, a) for i in range(30) for a in range(256)]
    palettes, banks = vulcan.color_banks(pixels)
    assert len(banks) == 4
    for n, (r, g, b, a) in enumerate(pixels):
        decoded = [(palettes[k][bank[n] & 7], bank[n] >> 3)
                   for k, bank in enumerate(banks) if bank[n] >> 3]
        alpha = (a * 31 + 127) // 255
        assert decoded == ([(r >> 3, alpha)] if alpha else [])


def test_source_vulcan_geometry_and_material_are_baked():
    header, packet = vulcan.generate()
    assert "ROOT0 0x09b0u" in header and "ROOT1 0x0a78u" in header
    assert "sVulcanIndices0[36]" in packet
    assert "sVulcanIndices1[6]" in packet
    assert "sVulcanPixels4[256]" in packet

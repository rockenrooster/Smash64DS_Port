import generate_nds_native_pikachu_thunder as thunder


def test_source_thunder_roots_keep_live_and_fixed_materials():
    header, packet = thunder.generate()
    assert "THUNDER_ROOT 0x94f8u" in header
    assert "THUNDER_MATERIAL 0x0200u" in header
    assert "SHOCK_MATERIAL 0x1600u" in header
    assert "SHOCK_ROOT0 0x14b8u" in header and "SHOCK_ROOT1 0x1598u" in header
    assert packet.count("ndsRendererNativeApplyMaterial(material, stats, state)") == 2
    assert packet.count("static const u16 sThunderIndices") == 3
    assert "ndsRendererRecordSetImage(stats, 0xfd100000u, (u32)(uintptr_t)palette)" in packet
    assert "ndsRendererRecordSetImage(stats, 0xfd500000u, (u32)(uintptr_t)image)" in packet

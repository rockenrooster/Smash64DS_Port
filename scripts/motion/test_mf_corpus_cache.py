"""A motion corpus cache must notice O2R, table and verification changes."""
from pathlib import Path
import sys
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))
import mf_corpus as mc


def test_cache_tracks_non_bps_inputs(tmp_path):
    pack = tmp_path / "pack.bin"
    pack.write_bytes(b"same BPS pack throughout this test")
    bank = tmp_path / "bank"
    bank.mkdir()
    clip = bank / "FTMarioAnim000"
    clip.write_bytes(b"O2R version one")
    tables = {"mario": [("llFTMarioAnimFileID", 1)]}
    builds = []

    def build(**kwargs):
        builds.append(kwargs["verify_pack"])
        return {"header": {"sha256": "fixture"}, "build_number": len(builds)}

    with patch.object(mc, "PACK", pack), patch.object(mc, "BANK", bank), \
            patch.object(mc, "CACHE", tmp_path / "cache.pickle"), \
            patch.object(mc, "generated_rows", return_value={}), \
            patch.object(mc, "motion_tables", side_effect=lambda _rows: tables), \
            patch.object(mc, "aobj32_ids", return_value=set()), \
            patch.object(mc, "build_corpus", side_effect=build):
        assert mc.load_corpus()["build_number"] == 1
        assert mc.load_corpus()["build_number"] == 1
        clip.write_bytes(b"O2R version two")
        assert mc.load_corpus()["build_number"] == 2
        tables["luigi"] = [("llFTMarioAnimFileID", 1)]
        assert mc.load_corpus()["build_number"] == 3
        assert mc.load_corpus(verify_pack=False)["build_number"] == 4
        assert mc.load_corpus(verify_pack=True)["build_number"] == 5
        (bank / "unrelated").write_bytes(b"not a corpus input")
        assert mc.load_corpus()["build_number"] == 5

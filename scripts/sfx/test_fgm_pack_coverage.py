"""Negative controls for the FGM source-coverage audit (no pack rendering)."""
import contextlib
import importlib.util
import io
from pathlib import Path
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    "fgm_coverage", Path(__file__).with_name("check-fgm-pack-coverage.py"))
coverage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(coverage)

SOURCE = """
#define NDS_AUDIO_FGM_SAMUS_CHARGE_AUX_ID 673u
static int ndsAudioFgmIDIsIncluded(unsigned id)
{
    switch (id) {
    case nSYAudioFGMBatHit:
    case 188u:
    case NDS_AUDIO_FGM_SAMUS_CHARGE_AUX_ID:
        return 1;
    default: return 0;
    }
}
static int unrelated(unsigned id)
{
    switch (id) {
    case 99u: return 1;
    default: return 0;
    }
}
"""


class FgmCoverageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location("fgm_generator", coverage.GENERATOR)
        cls.generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.generator)

    def parse(self, source):
        with patch.object(coverage, "read", return_value=source):
            return coverage.runtime_included({"nSYAudioFGMBatHit": 52})

    def test_cases_are_scoped_and_macro_values_follow_source(self):
        self.assertEqual(self.parse(SOURCE), ({52, 188, 673}, [], []))
        self.assertEqual(self.parse(SOURCE.replace("673u", "674u")),
                         ({52, 188, 674}, [], []))

    def test_unknown_macro_cannot_silently_disappear(self):
        source = SOURCE.replace("#define NDS_AUDIO_FGM_SAMUS_CHARGE_AUX_ID 673u", "")
        self.assertEqual(self.parse(source)[1], ["NDS_AUDIO_FGM_SAMUS_CHARGE_AUX_ID"])

    def test_aliased_duplicate_cases_are_rejected(self):
        source = SOURCE.replace("case 188u:", "case 52u:")
        self.assertEqual(self.parse(source)[2], [52])

    def test_duplicate_generator_ids_fail_the_audit(self):
        with (patch.object(coverage, "coverage_ids", return_value=(set(), [52], [])),
              patch.object(coverage, "port_enum", return_value={}),
              patch.object(coverage, "runtime_included", return_value=(set(), [], [])),
              patch.object(coverage, "referenced_names", return_value={}),
              patch.object(coverage, "hit_table_ids", return_value={}),
              contextlib.redirect_stdout(io.StringIO())):
            self.assertEqual(coverage.main(), 1)

    def test_matching_lists_without_a_selector_factory_fail(self):
        with (patch.object(coverage, "coverage_ids", return_value=({52}, [], [52])),
              patch.object(coverage, "port_enum", return_value={}),
              patch.object(coverage, "runtime_included", return_value=({52}, [], [])),
              patch.object(coverage, "referenced_names", return_value={}),
              patch.object(coverage, "hit_table_ids", return_value={}),
              contextlib.redirect_stdout(io.StringIO())):
            self.assertEqual(coverage.main(), 1)

    def test_sample_extent_includes_delayed_child(self):
        ucd = {"entries": {
            1: {"program": [["note", 13, 7, 10], ["fork_voice", 2],
                             ["note", 13, 7, 5], ["stop_voice"]]},
            2: {"program": [["note", 13, 7, 20], ["stop_voice"]]},
        }}
        self.assertEqual(self.generator.fgm_composite_sample_count(1, ucd),
                         30 * self.generator.fgm_samples_per_tick(32000))

    def test_nested_fork_is_not_silently_omitted(self):
        ucd = {"entries": {
            1: {"program": [["fork_voice", 2], ["note", 13, 7, 10], ["stop_voice"]]},
            2: {"program": [["fork_voice", 3], ["note", 13, 7, 20], ["stop_voice"]]},
        }}
        with self.assertRaisesRegex(ValueError, "nested or looped fork"):
            self.generator.fgm_composite_sample_count(1, ucd)


if __name__ == "__main__":
    unittest.main()

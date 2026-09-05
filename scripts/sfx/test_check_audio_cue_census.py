"""Negative controls for the audio cue census (no build, no pack, no ROM).

Every test here can fail: each one pairs the shape the checker must catch with
the neighbouring shape it must not, so a regex that stops discriminating shows
up as a red test rather than as a quietly shorter report.
"""
import contextlib
import importlib.util
import io
from pathlib import Path
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    "audio_cue_census", Path(__file__).with_name("check_audio_cue_census.py"))
census = importlib.util.module_from_spec(spec)
spec.loader.exec_module(census)


class StripCommentsTests(unittest.TestCase):
    def test_comment_text_is_blanked_and_lines_are_preserved(self):
        source = "a();\n/* nSYAudioBGMStar\n   nSYAudioBGMHammer */\nb();\n"
        stripped = census.strip_comments(source)
        self.assertNotIn("nSYAudioBGMStar", stripped)
        self.assertEqual(stripped.count("\n"), source.count("\n"))
        self.assertIn("a();", stripped)
        self.assertIn("b();", stripped)

    def test_a_commented_cue_is_not_a_request(self):
        """Every scene wrapper's header comment lists the audio entry points it
        needs, and nds_audio_bgm.c names each id twice (row plus provenance);
        counting those would make the census report itself as coverage."""
        scenes = {"tu.c": {"<wrapper>": (
            "/* needs syAudioPlayBGM, plays nSYAudioBGMStar */\n"
            "void f(void) { syAudioPlayBGM(0, nSYAudioBGMResults); }\n")}}
        requests, indirect = census.bgm_requests(scenes, {})
        self.assertEqual([(n, r) for n, r, _w in requests],
                         [("nSYAudioBGMResults", "PLAY")])
        self.assertEqual(indirect, [])


class RouteTests(unittest.TestCase):
    SOURCE = (
        "void f(void) {\n"
        "    syAudioPlayBGM(0, nSYAudioBGMResults);\n"
        "    ftParamTryPlayItemMusic(nSYAudioBGMHammer);\n"
        "    gMPCollisionBGMDefault = nSYAudioBGMLast;\n"
        "    u32 t[] = { nSYAudioBGMZako };\n"
        "}\n")

    def routes(self, source, stages=None):
        requests, indirect = census.bgm_requests(
            {"tu.c": {"<wrapper>": source}}, stages or {})
        return {(n, r) for n, r, _w in requests}, indirect

    def test_each_route_is_named_separately(self):
        found, indirect = self.routes(self.SOURCE)
        self.assertEqual(found, {
            ("nSYAudioBGMResults", "PLAY"),
            ("nSYAudioBGMHammer", "ITEM"),
            ("nSYAudioBGMLast", "DEFAULT"),
            ("nSYAudioBGMZako", "DATA"),
        })
        self.assertEqual(indirect, [])

    def test_an_indirect_play_is_reported_not_dropped(self):
        """mnsoundtest.c plays `dMNSoundTestMusicIDs[...]`; a checker that only
        matched a literal enumerator would silently score the Sound Test as
        starting no music at all."""
        found, indirect = self.routes(
            "void f(void) { syAudioPlayBGM(0, dMNSoundTestMusicIDs[i]); }\n")
        self.assertEqual(found, set())
        self.assertEqual([(r, a) for r, a, _w in indirect],
                         [("PLAY", "dMNSoundTestMusicIDs[i]")])

    def test_a_staged_stage_descriptor_is_a_request(self):
        found, _indirect = self.routes("void f(void) {}\n",
                                       {"GRZakoMap": "nSYAudioBGMZako"})
        self.assertEqual(found, {("nSYAudioBGMZako", "STAGE")})

    def test_the_sentinel_is_never_a_request(self):
        found, _indirect = self.routes("u32 t[] = { nSYAudioBGMEnd };\n")
        self.assertEqual(found, set())


class StageMarkerTests(unittest.TestCase):
    ROW = "    nSYAudioBGM1PBonusStage,  /* bgm_id */\n"

    def test_the_field_marker_is_the_comment(self):
        """Regression control: staged_stage_bgm() must read the relocData source
        raw.  The field is identified only by its `/* bgm_id */` comment, so
        running strip_comments() first erases the anchor and the whole STAGE
        route silently reports zero staged stages."""
        self.assertEqual(census.MAP_BGM_RE.search(self.ROW).group(1),
                         "nSYAudioBGM1PBonusStage")
        self.assertIsNone(census.MAP_BGM_RE.search(census.strip_comments(self.ROW)))


class DefinitionTests(unittest.TestCase):
    SOURCE = (
        "void ftParamTryPlayItemMusic(s32 bgm_id)\n"
        "{\n"
        "    (void)bgm_id;\n"
        "}\n"
        "\n"
        "void mpCollisionSetPlayBGM(void)\n"
        "{\n"
        "    if (x) { syAudioPlayBGM(0, gMPCollisionBGMDefault); }\n"
        "}\n")

    def test_bodies_are_brace_matched_not_line_matched(self):
        found = census.definitions(self.SOURCE)
        self.assertEqual(set(found), {"ftParamTryPlayItemMusic",
                                      "mpCollisionSetPlayBGM"})
        self.assertEqual(found["ftParamTryPlayItemMusic"][0], 1)
        self.assertNotIn("syAudioPlayBGM", found["ftParamTryPlayItemMusic"][1])
        self.assertIn("syAudioPlayBGM", found["mpCollisionSetPlayBGM"][1])
        self.assertTrue(found["mpCollisionSetPlayBGM"][1].endswith("}"))


class BlockedSinkTests(unittest.TestCase):
    SINK = [("ftParamTryPlayItemMusic", "ft/ftparam.c", 114, None)]

    def blocked(self, body):
        with patch.object(census, "port_definition",
                          return_value=("src/port/x.c", 7823, body)):
            return census.blocked_sinks(self.SINK)

    def test_an_empty_port_body_is_blocked(self):
        blocked = self.blocked("{ (void)bgm_id; }")
        self.assertEqual(len(blocked), 1)
        self.assertEqual(blocked[0][:3], ("ftParamTryPlayItemMusic",
                                          "src/port/x.c", 7823))

    def test_a_port_body_that_reaches_the_player_is_not_blocked(self):
        self.assertEqual(
            self.blocked("{ if (a >= b) { syAudioPlayBGM(0, bgm_id); } }"), [])

    def test_an_imported_sink_is_never_inspected(self):
        """A sink compiled from its own decomp source through a src/import
        wrapper needs no port body; only a hand-port can stub one out."""
        sink = [("mnMessageFuncStart", "mn/mncommon/mnmessage.c", 350,
                 "battleship_mnmessage.c")]
        with patch.object(census, "port_definition",
                          side_effect=AssertionError("must not be consulted")):
            self.assertEqual(census.blocked_sinks(sink), [])

    def test_a_sink_with_no_implementation_at_all_is_blocked(self):
        with patch.object(census, "port_definition", return_value=None):
            blocked = census.blocked_sinks(self.SINK)
        self.assertEqual(len(blocked), 1)
        self.assertIn("neither imported nor ported", blocked[0][3])


class StrictTests(unittest.TestCase):
    """--strict is what turns the report into a gate; without it a gap must
    still print but must not fail a caller that only wants the census."""

    def run_main(self, argv, rows, requests):
        patches = {
            "load_reloc_census": None,
            "scene_sources": {},
            "audio_enum": {"nSYAudioBGMResults": 22, "nSYAudioBGMZako": 36},
            "bgm_track_ids": rows,
            "staged_stage_bgm": {},
            "bgm_requests": (requests, []),
            "bgm_sinks": [],
            "blocked_sinks": [],
            "pack_ids": (set(), set(), {}),
            "fgm_by_scene": ({}, {}),
        }
        with contextlib.ExitStack() as stack:
            for name, value in patches.items():
                stack.enter_context(patch.object(census, name, return_value=value))
            out = io.StringIO()
            stack.enter_context(contextlib.redirect_stdout(out))
            return census.main(argv), out.getvalue()

    def test_a_requested_track_with_no_row_fails_strict(self):
        requests = [("nSYAudioBGMZako", "STAGE", "relocData/GRZakoMap")]
        code, out = self.run_main(["--strict"], ["nSYAudioBGMResults"], requests)
        self.assertEqual(code, 1)
        self.assertIn("BGM-MISSING", out)
        self.assertEqual(self.run_main([], ["nSYAudioBGMResults"], requests)[0], 0)

    def test_a_row_nothing_requests_fails_strict(self):
        code, out = self.run_main(
            ["--strict"], ["nSYAudioBGMResults", "nSYAudioBGMZako"],
            [("nSYAudioBGMResults", "PLAY", "tu.c[<wrapper>]")])
        self.assertEqual(code, 1)
        self.assertIn("BGM-UNREQUESTED", out)

    def test_a_table_only_row_reports_but_does_not_fail(self):
        """nSYAudioBGMOpening is named only by dMNSoundTestMusicIDs -- the
        opening cinematic's own start sits in the `!SSB64_TARGET_NDS` arm the
        overlay patch parks (P2-7, owner-deferred).  That must be visible in
        the report and must not fail the gate."""
        code, out = self.run_main(
            ["--strict"], ["nSYAudioBGMResults"],
            [("nSYAudioBGMResults", "DATA", "tu.c[<wrapper>]")])
        self.assertEqual(code, 0)
        self.assertIn("BGM-DATA-ONLY", out)

    def test_a_matched_census_passes(self):
        code, out = self.run_main(
            ["--strict"], ["nSYAudioBGMResults"],
            [("nSYAudioBGMResults", "PLAY", "tu.c[<wrapper>]")])
        self.assertEqual(code, 0)
        self.assertIn("AUDIO_CUE_CENSUS_OK", out)


class AllowTests(unittest.TestCase):
    def test_every_allow_key_is_shaped_and_reasoned(self):
        self.assertTrue(census.ALLOW)
        for key, reason in census.ALLOW.items():
            kind, _, subject = key.partition(":")
            self.assertIn(kind, {"BGM_SINK", "BGM_UNREQUESTED"}, key)
            self.assertTrue(subject, key)
            self.assertGreater(len(reason), 40, key)


if __name__ == "__main__":
    unittest.main()

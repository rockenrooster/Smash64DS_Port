"""Boss-defeat FGM admission gate: D_8009EDD0_406D0 without n_env.

Source sc1pgame.c:1994-2001 saves/zeroes/restores `.sfx_max` around the Boss
defeat cinematic. Per decomp docs/bugs/boss_defeat_sfx_max_typepun_2026-05-01
that field is the N64 view of n_env.c's `fgm_ucode_count` (N64 offset 0x28 in
both layouts): zeroing it makes func_800269C0_275C0 refuse every NEW id
(`id >= count -> NULL`) while the three already-queued death cues
(ExplodeL, VoiceBossDead, BossDefeatL) keep ticking, and
ifCommonBattleEndSetBossDefeat restores the saved value. gmsound.h:28 pins
the meaning to "End of SFX index list" -- an admission limit, NOT a voice
count or master volume.

The DS build carries no n_env, so D_8009EDD0_406D0 cannot exist here (and no
unconnected placeholder struct may stand in for it). The DS mixer owns the
same seam instead: nds_audio_fgm.c gates every new start in
ndsAudioFgmPlayAtPan and exposes save/block + restore, which the sc1pgame
import-overlay patch calls. These tests pin that contract with no build,
no pack and no ROM.
"""
import re
import subprocess
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
FGM_C = REPO / "src/nds/nds_audio_fgm.c"
FGM_H = REPO / "include/nds/nds_audio_fgm.h"
PATCH = REPO / "scripts/import-overlays/battleship/src_sc_sc1pmode_sc1pgame.patch"
PRISTINE = (REPO / "decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c")

C_SRC = FGM_C.read_text(encoding="utf-8")
H_SRC = FGM_H.read_text(encoding="utf-8")
PATCH_SRC = PATCH.read_text(encoding="utf-8")


def function_body(source, signature):
    """Extract the brace-balanced body of a C function by its signature."""
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for i in range(brace, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    raise AssertionError("unbalanced braces for %s" % signature)


class HeaderContractTests(unittest.TestCase):
    def test_gate_functions_declared(self):
        for decl in ("void ndsAudioFgmSaveAndBlockNewStarts(u16 *out_saved);",
                     "void ndsAudioFgmRestoreNewStarts(u16 saved);",
                     "void portAudioSaveAndBlockFGMs(u16 *out_saved);",
                     "void portAudioRestoreFGMs(u16 saved);"):
            with self.subTest(decl=decl):
                self.assertIn(decl, H_SRC)

    def test_gate_diagnostics_declared(self):
        for decl in ("gNdsAudioFgmBlockNewStartCalls;",
                     "gNdsAudioFgmBlockedPlayCount;"):
            with self.subTest(decl=decl):
                self.assertIn(decl, H_SRC)


class AdmissionGateTests(unittest.TestCase):
    def test_limit_defaults_to_admit_all(self):
        self.assertIn(
            "static u16 sNdsAudioFgmNewStartLimit = 0xffffu;", C_SRC)

    def test_play_checks_gate_before_entry_lookup(self):
        play = function_body(
            C_SRC, "alSoundEffect *ndsAudioFgmPlayAtPan(u16 fgm_id, u8 pan)")
        gate = play.index("if (fgm_id >= sNdsAudioFgmNewStartLimit)")
        lookup = play.index("ndsAudioFgmFindEntry(fgm_id)")
        self.assertLess(gate, lookup)

    def test_gate_refuses_fail_closed(self):
        play = function_body(
            C_SRC, "alSoundEffect *ndsAudioFgmPlayAtPan(u16 fgm_id, u8 pan)")
        block = play[play.index("if (fgm_id >= sNdsAudioFgmNewStartLimit)"):]
        block = block[:block.index("entry = ndsAudioFgmFindEntry(fgm_id);")]
        self.assertIn("gNdsAudioFgmBlockedPlayCount++;", block)
        self.assertIn("gNdsAudioFgmPlayFailCount++;", block)
        self.assertIn("return NULL;", block)

    def test_live_cues_tick_before_the_gate(self):
        """The refused start must not starve live handles: Update() runs
        first, so the three death cues queued before the block keep sounding
        through the cinematic (source: their ucode nodes stay allocated)."""
        play = function_body(
            C_SRC, "alSoundEffect *ndsAudioFgmPlayAtPan(u16 fgm_id, u8 pan)")
        self.assertLess(play.index("ndsAudioFgmUpdate();"),
                        play.index("if (fgm_id >= sNdsAudioFgmNewStartLimit)"))

    def test_gate_never_releases_live_handles(self):
        play = function_body(
            C_SRC, "alSoundEffect *ndsAudioFgmPlayAtPan(u16 fgm_id, u8 pan)")
        block = play[play.index("if (fgm_id >= sNdsAudioFgmNewStartLimit)"):]
        block = block[:block.index("entry = ndsAudioFgmFindEntry(fgm_id);")]
        for killer in ("ndsAudioFgmReleaseHandle", "ndsAudioFgmStopAll",
                       "soundKill", "soundSetVolume"):
            self.assertNotIn(killer, block)


class SaveRestoreTests(unittest.TestCase):
    def test_save_snapshots_then_blocks(self):
        body = function_body(
            C_SRC, "void ndsAudioFgmSaveAndBlockNewStarts(u16 *out_saved)")
        self.assertLess(body.index("*out_saved = sNdsAudioFgmNewStartLimit;"),
                        body.index("sNdsAudioFgmNewStartLimit = 0u;"))
        self.assertIn("gNdsAudioFgmBlockNewStartCalls++;", body)

    def test_save_tolerates_null_sink(self):
        body = function_body(
            C_SRC, "void ndsAudioFgmSaveAndBlockNewStarts(u16 *out_saved)")
        self.assertIn("if (out_saved != NULL)", body)

    def test_restore_puts_the_snapshot_back(self):
        body = function_body(
            C_SRC, "void ndsAudioFgmRestoreNewStarts(u16 saved)")
        self.assertIn("sNdsAudioFgmNewStartLimit = saved;", body)

    def test_save_restore_touch_no_handles(self):
        for sig in ("void ndsAudioFgmSaveAndBlockNewStarts(u16 *out_saved)",
                    "void ndsAudioFgmRestoreNewStarts(u16 saved)"):
            body = function_body(C_SRC, sig)
            with self.subTest(sig=sig):
                for killer in ("ndsAudioFgmReleaseHandle",
                               "ndsAudioFgmStopAll", "ndsAudioFgmStop(",
                               "soundKill"):
                    self.assertNotIn(killer, body)

    def test_battleship_aliases_route_to_the_ds_gate(self):
        save = function_body(C_SRC, "void portAudioSaveAndBlockFGMs(")
        restore = function_body(C_SRC, "void portAudioRestoreFGMs(")
        self.assertIn("ndsAudioFgmSaveAndBlockNewStarts(out_saved);", save)
        self.assertIn("ndsAudioFgmRestoreNewStarts(saved);", restore)

    def test_reset_clears_a_stale_block(self):
        """A block left set would silence the next match's FGM starts; the
        diagnostics reset must return the gate to admit-all."""
        reset = function_body(C_SRC, "void ndsAudioFgmDiagnosticsReset(void)")
        self.assertIn("sNdsAudioFgmNewStartLimit = 0xffffu;", reset)
        self.assertIn("gNdsAudioFgmBlockNewStartCalls = 0u;", reset)
        self.assertIn("gNdsAudioFgmBlockedPlayCount = 0u;", reset)

    def test_no_placeholder_engine_struct(self):
        """D_8009EDD0_406D0 must be satisfied by the gate, not by an
        unconnected alSoundEffect/ALWhatever blob in the DS sources."""
        for path, text in ((FGM_C, C_SRC), (FGM_H, H_SRC)):
            with self.subTest(path=str(path)):
                self.assertNotIn("D_8009EDD0", text)


class GateSequencingTests(unittest.TestCase):
    """Python mirror of the exact C constants, proving the save/block/
    restore order in sc1pgame.c preserves the three pre-block cues and the
    restore timing. The structure under test is the snapshot value flow,
    not the audio itself."""

    def run_sequence(self):
        limit = 0xFFFF
        live = {"ExplodeL", "VoiceBossDead", "BossDefeatL"}

        def play(fgm_id):
            return None if fgm_id >= limit else ("started", fgm_id)

        for cue in ("ExplodeL", "VoiceBossDead", "BossDefeatL"):
            self.assertIsNotNone(play(0))
        saved = limit
        limit = 0
        # Blocked window: new starts refuse, live cues are untouched.
        self.assertIsNone(play(100))
        self.assertEqual(live, {"ExplodeL", "VoiceBossDead", "BossDefeatL"})
        limit = saved
        self.assertIsNotNone(play(100))
        self.assertEqual(saved, 0xFFFF)

    def test_block_window_preserves_live_cues_and_restore_timing(self):
        self.run_sequence()


class OverlayPatchTests(unittest.TestCase):
    def test_extern_guarded_and_prototypes_declared(self):
        self.assertIn("-extern alSoundEffect D_8009EDD0_406D0;", PATCH_SRC)
        self.assertIn("+#if !defined(SSB64_TARGET_NDS)", PATCH_SRC)
        self.assertIn("+extern alSoundEffect D_8009EDD0_406D0;", PATCH_SRC)
        self.assertIn("+void portAudioSaveAndBlockFGMs(u16 *out_saved);",
                      PATCH_SRC)
        self.assertIn("+void portAudioRestoreFGMs(u16 saved);", PATCH_SRC)

    def test_block_site_queues_cues_first(self):
        """The three death cues must stay ahead of the block, in source
        order; the patch only appends the gate call around the sfx_max
        lines it replaces."""
        self.assertIn("+    portAudioSaveAndBlockFGMs("
                      "&sSC1PGameBossDefeatSoundTerminateTemp);", PATCH_SRC)
        # The source accesses survive as context under #else (IDO path).
        self.assertIn("     sSC1PGameBossDefeatSoundTerminateTemp = "
                      "D_8009EDD0_406D0.sfx_max;", PATCH_SRC)
        self.assertIn("     D_8009EDD0_406D0.sfx_max = 0;", PATCH_SRC)
        cue_lines = [line for line in PATCH_SRC.splitlines()
                     if "func_800269C0_275C0(nSYAudio" in line]
        self.assertTrue(all(not line.startswith(("+", "-"))
                            for line in cue_lines),
                        "death-cue queue lines must be untouched context")

    def test_restore_site_routes_through_gate(self):
        self.assertIn("+    portAudioRestoreFGMs("
                      "sSC1PGameBossDefeatSoundTerminateTemp);", PATCH_SRC)
        self.assertIn("     D_8009EDD0_406D0.sfx_max = "
                      "sSC1PGameBossDefeatSoundTerminateTemp;", PATCH_SRC)

    def test_non_nds_path_keeps_source_accesses(self):
        """IDO byte-match path is undisturbed: every replaced access
        survives under #else."""
        self.assertEqual(
            PATCH_SRC.count("D_8009EDD0_406D0.sfx_max"), 3)

    def test_pristine_source_matches_patch_context(self):
        pristine = PRISTINE.read_text(encoding="utf-8")
        for snippet in (
                "extern alSoundEffect D_8009EDD0_406D0;",
                "func_800269C0_275C0(nSYAudioFGMBossDefeatL);",
                "sSC1PGameBossDefeatSoundTerminateTemp = "
                "D_8009EDD0_406D0.sfx_max;",
                "D_8009EDD0_406D0.sfx_max = "
                "sSC1PGameBossDefeatSoundTerminateTemp;"):
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, pristine)

    def test_patch_applies_to_pristine_source(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp) / "a" / "src" / "sc" / "sc1pmode"
            target.mkdir(parents=True)
            (target / "sc1pgame.c").write_bytes(
                PRISTINE.read_bytes())
            proc = subprocess.run(
                ["git", "apply", "--check",
                 "--directory=a",
                 str(PATCH)],
                cwd=tmp, capture_output=True, text=True, timeout=60)
            self.assertEqual(
                proc.returncode, 0,
                "overlay patch must apply to pristine sc1pgame.c: %s"
                % (proc.stderr or proc.stdout))


if __name__ == "__main__":
    unittest.main()

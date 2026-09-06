"""1P/Bonus CSS entry/reentry: handles the N64 overlay BSS clear used to reset.

Source facts (decomp/BattleShip-main/decomp/src/mn/mnplayers/):
- mnplayers1pgame.c InitVars (:3387-3409) nulls HiScore/Bonuses/Level/Stock
  GObjs but NOT sMNPlayers1PGameTimeGObj; MakeTimeSelect (:1019-1026)
  ejects it when non-NULL, so a DS revisit (resident TU, no overlay
  reload) would eject the torn-down GObj on entry via MakeLabels.
- mnplayers1pbonus.c InitVars (:2755-2776) nulls only TotalTimeGObj, NOT
  sMNPlayers1PBonusHiScoreGObj; MakeBestTime/MakeBestTaskCount (:1043,
  :1116) eject it when non-NULL, so a revisit would eject it on the
  first cursor move.
- Transitions and manager-owned settings ride the textual include
  untouched: 1PGame FuncRun goes Title on timeout / 1PGame on START-ready,
  Back goes 1PMode; Bonus FuncRun goes Title on timeout / 1PBonusStage
  with scene_prev per BonusKind; SetSceneData writes manager-owned
  player/difficulty/stock/fkind/costume (bonus: player/bonus_fkind/
  bonus_costume).

Host-run source-evidence test: pins the facts above and the two entry
stores in src/import/battleship_mnplayers1p{game,bonus}.c, and proves
sensitivity by showing the assertions fail with the stores stripped.
"""
import re
import unittest
from pathlib import Path

from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]
DECOMP = ROOT / "decomp/BattleShip-main/decomp/src/mn/mnplayers"
GAME_SRC = (DECOMP / "mnplayers1pgame.c").read_text()
BONUS_SRC = (DECOMP / "mnplayers1pbonus.c").read_text()
GAME_TU = (ROOT / "src/import/battleship_mnplayers1pgame.c").read_text()
BONUS_TU = (ROOT / "src/import/battleship_mnplayers1pbonus.c").read_text()


class OnePlayerCSSReentryTest(unittest.TestCase):
    def test_1pgame_time_handle_reentry(self):
        init = function(GAME_SRC, "mnPlayers1PGameInitVars")
        self.assertIn("sMNPlayers1PGameHiScoreGObj = NULL;", init)
        self.assertIn("sMNPlayers1PGameLevelGObj = NULL;", init)
        self.assertIn("sMNPlayers1PGameStockGObj = NULL;", init)
        self.assertNotIn("sMNPlayers1PGameTimeGObj = NULL;", init)
        make = function(GAME_SRC, "mnPlayers1PGameMakeTimeSelect")
        self.assertIn("if (sMNPlayers1PGameTimeGObj != NULL)", make)
        wrapper = function(GAME_TU, "mnPlayers1PGameStartScene")
        self.assertIn("sMNPlayers1PGameTimeGObj = NULL;", wrapper)
        self.assertLess(wrapper.index("sMNPlayers1PGameTimeGObj = NULL;"),
                        wrapper.index("ndsBaseMNPlayers1PGameStartScene();"))
        stripped = wrapper.replace("sMNPlayers1PGameTimeGObj = NULL;", "")
        with self.assertRaises(AssertionError):
            self.assertIn("sMNPlayers1PGameTimeGObj = NULL;", stripped)

    def test_bonus_hiscore_handle_reentry(self):
        init = function(BONUS_SRC, "mnPlayers1PBonusInitVars")
        self.assertIn("sMNPlayers1PBonusTotalTimeGObj = NULL;", init)
        self.assertNotIn("sMNPlayers1PBonusHiScoreGObj = NULL;", init)
        for name in ("mnPlayers1PBonusMakeBestTime",
                     "mnPlayers1PBonusMakeBestTaskCount"):
            self.assertIn("if (sMNPlayers1PBonusHiScoreGObj != NULL)",
                          function(BONUS_SRC, name))
        wrapper = function(BONUS_TU, "mnPlayers1PBonusStartScene")
        self.assertIn("sMNPlayers1PBonusHiScoreGObj = NULL;", wrapper)
        self.assertLess(wrapper.index("sMNPlayers1PBonusHiScoreGObj = NULL;"),
                        wrapper.index("ndsBaseMNPlayers1PBonusStartScene();"))
        stripped = wrapper.replace("sMNPlayers1PBonusHiScoreGObj = NULL;", "")
        with self.assertRaises(AssertionError):
            self.assertIn("sMNPlayers1PBonusHiScoreGObj = NULL;", stripped)

    def test_transitions_and_scene_data_preserved(self):
        run = function(GAME_SRC, "mnPlayers1PGameFuncRun")
        self.assertIn("gSCManagerSceneData.scene_curr = nSCKindTitle;", run)
        self.assertIn("gSCManagerSceneData.scene_curr = nSCKind1PGame;", run)
        back = function(GAME_SRC, "mnPlayers1PGameBackTo1PMode")
        self.assertIn("gSCManagerSceneData.scene_curr = nSCKind1PMode;", back)
        setup = function(GAME_SRC, "mnPlayers1PGameSetSceneData")
        for field in ("spgame_time_limit", "spgame_difficulty",
                      "spgame_stock_count", "gSCManagerSceneData.fkind",
                      "gSCManagerSceneData.costume", "lbBackupWrite();"):
            self.assertIn(field, setup)
        # Per-player semantics: 1P CSS keeps the last fighter across visits
        # (seeds the slot from scene data); Bonus CSS never does.
        init_player = function(GAME_SRC, "mnPlayers1PGameInitPlayer")
        self.assertIn("sMNPlayers1PGameSlot.fkind = gSCManagerSceneData.fkind;",
                      init_player)
        reset = function(BONUS_SRC, "mnPlayers1PBonusResetPlayer")
        self.assertIn("sMNPlayers1PBonusSlot.fkind = nFTKindNull;", reset)
        bonus_init = function(BONUS_SRC, "mnPlayers1PBonusInitVars")
        self.assertIn("sMNPlayers1PBonusBonusKind = 0;", bonus_init)
        bonus_run = function(BONUS_SRC, "mnPlayers1PBonusFuncRun")
        self.assertIn("gSCManagerSceneData.scene_curr = nSCKindTitle;",
                      bonus_run)
        self.assertIn("gSCManagerSceneData.scene_curr = nSCKind1PBonusStage;",
                      bonus_run)
        self.assertIn("gSCManagerSceneData.scene_prev = nSCKind1PBonus1Players;",
                      bonus_run)
        self.assertIn("gSCManagerSceneData.scene_prev = nSCKind1PBonus2Players;",
                      bonus_run)
        bonus_setup = function(BONUS_SRC, "mnPlayers1PBonusSetSceneData")
        for field in ("gSCManagerSceneData.player",
                      "gSCManagerSceneData.bonus_fkind",
                      "gSCManagerSceneData.bonus_costume"):
            self.assertIn(field, bonus_setup)

    def test_wrappers_touch_no_manager_state(self):
        for text, entry in ((GAME_TU, "mnPlayers1PGameStartScene"),
                            (BONUS_TU, "mnPlayers1PBonusStartScene")):
            wrapper = function(text, entry)
            self.assertNotIn("gSCManagerSceneData", wrapper)
            self.assertNotIn("gSCManagerBackupData", wrapper)
            self.assertNotIn("syTaskman", wrapper)


if __name__ == "__main__":
    unittest.main()

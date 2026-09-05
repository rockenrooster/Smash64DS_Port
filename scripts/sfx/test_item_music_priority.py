"""Execute the port's item-theme arbitration against BattleShip on the host."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

import check_audio_cue_census as census


ROOT = Path(__file__).resolve().parents[2]
FUNCTIONS = {
    "ftParamGetItemMusicLength": "s32 ftParamGetItemMusicLength(u32 bgm_id)",
    "ftParamTryPlayItemMusic": "void ftParamTryPlayItemMusic(u32 bgm_id)",
    "ftParamTryUpdateItemMusic": "void ftParamTryUpdateItemMusic(void)",
}


def function_source(path, prefix):
    definitions = census.definitions(path.read_text(encoding="utf-8"))
    source = "\n".join(signature + definitions[name][1]
                       for name, signature in FUNCTIONS.items())
    for name in FUNCTIONS:
        source = re.sub(r"\b" + name + r"\b", prefix + name, source)
    return source


PRELUDE = r"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint32_t u32;
typedef int32_t s32;
typedef struct GObj { struct GObj *link_next; void *data; } GObj;
typedef struct { s32 kind; } ITStruct;
typedef struct { GObj *item_gobj; s32 star_invincible_tics; } FTStruct;
#define ftGetStruct(gobj) ((FTStruct *)(gobj)->data)
#define itGetStruct(gobj) ((ITStruct *)(gobj)->data)
enum { nGCCommonLinkIDFighter = 0, nITKindHammer = 13,
       nSYAudioBGMHammer = 45, nSYAudioBGMStar = 46 };
static GObj *gGCCommonLinks[1];
static u32 gMPCollisionBGMDefault, gMPCollisionBGMCurrent, last_bgm;
static unsigned play_count;
static void syAudioPlayBGM(s32 channel, u32 id) {
    if (channel != 0) abort();
    ++play_count;
    last_bgm = id;
}
"""

DRIVER = r"""
static GObj fighters[4], items[4];
static FTStruct fighter_data[4];
static ITStruct item_data[4];
static const s32 star_ticks[] = {
    0, ITSTAR_WARN_BEGIN_FRAME - 1, ITSTAR_WARN_BEGIN_FRAME,
    ITSTAR_WARN_BEGIN_FRAME + 1, ITSTAR_INVINCIBLE_TIME
};
static const u32 themes[] = { 3, nSYAudioBGMHammer, nSYAudioBGMStar };
static void reset_audio(u32 current) {
    gMPCollisionBGMDefault = 3;
    gMPCollisionBGMCurrent = current;
    play_count = 0;
    last_bgm = 0xffffffffu;
}
int main(void) {
    unsigned checks = 0;
    for (unsigned count = 0; count <= 4; ++count) {
        unsigned combinations = 1;
        for (unsigned i = 0; i < count; ++i) combinations *= 15;
        for (unsigned combination = 0; combination < combinations; ++combination) {
            unsigned state = combination;
            for (unsigned i = 0; i < count; ++i) {
                unsigned one = state % 15;
                state /= 15;
                fighters[i].link_next = i + 1 < count ? &fighters[i + 1] : NULL;
                fighters[i].data = &fighter_data[i];
                items[i].data = &item_data[i];
                item_data[i].kind = one % 3 == 2 ? nITKindHammer : 1;
                fighter_data[i].item_gobj = one % 3 ? &items[i] : NULL;
                fighter_data[i].star_invincible_tics = star_ticks[one / 3];
            }
            gGCCommonLinks[0] = count ? fighters : NULL;
            for (unsigned current = 0; current < 3; ++current) {
                reset_audio(themes[current]);
                source_ftParamTryUpdateItemMusic();
                u32 expected = gMPCollisionBGMCurrent, played = last_bgm;
                unsigned calls = play_count;
                reset_audio(themes[current]);
                port_ftParamTryUpdateItemMusic();
                if (expected != gMPCollisionBGMCurrent || played != last_bgm ||
                    calls != play_count) {
                    fprintf(stderr, "update mismatch fighters=%u state=%u current=%u\n",
                            count, combination, current);
                    return 1;
                }
                ++checks;
            }
        }
    }
    for (unsigned current = 0; current < 3; ++current) {
        for (unsigned request = 0; request < 3; ++request) {
            reset_audio(themes[current]);
            source_ftParamTryPlayItemMusic(themes[request]);
            u32 expected = gMPCollisionBGMCurrent, played = last_bgm;
            unsigned calls = play_count;
            reset_audio(themes[current]);
            port_ftParamTryPlayItemMusic(themes[request]);
            if (expected != gMPCollisionBGMCurrent || played != last_bgm ||
                calls != play_count) return 2;
            ++checks;
        }
    }
    /* Equal priority retriggers; Hammer cannot replace active Star music. */
    reset_audio(nSYAudioBGMStar);
    port_ftParamTryPlayItemMusic(nSYAudioBGMHammer);
    if (play_count != 0) return 3;
    port_ftParamTryPlayItemMusic(nSYAudioBGMStar);
    if (play_count != 1 || last_bgm != nSYAudioBGMStar) return 4;
    /* Expiration restores the stage track exactly once, including no fighters. */
    gGCCommonLinks[0] = NULL;
    port_ftParamTryUpdateItemMusic();
    port_ftParamTryUpdateItemMusic();
    if (play_count != 2 || last_bgm != 3) return 5;
    printf("ITEM_MUSIC_DIFFERENTIAL_OK checks=%u\n", checks);
    return 0;
}
"""


class ItemMusicPriorityTests(unittest.TestCase):
    def test_port_matches_original_for_zero_to_four_fighters(self):
        compiler = shutil.which("gcc") or shutil.which("clang")
        if compiler is None:
            self.skipTest("host C compiler required")
        constants = ("ITSTAR_INVINCIBLE_TIME", "ITSTAR_WARN_BEGIN_FRAME",
                     "ITSTAR_BGM_DURATION", "ITHAMMER_BGM_DURATION")
        header = (ROOT / "include/it/item.h").read_text(encoding="utf-8")
        defines = "\n".join(re.search(r"^#define " + name + r"\s+[^\n]+",
                                     header, re.M).group() for name in constants)
        source = function_source(ROOT / "decomp/BattleShip-main/decomp/src/ft/ftparam.c",
                                 "source_")
        port = function_source(ROOT / "src/port/reloc_backend_compat_shims.c", "port_")
        with tempfile.TemporaryDirectory(prefix="smash64ds-item-music-") as folder:
            unit = Path(folder) / "item_music.c"
            exe = Path(folder) / "item_music.exe"
            unit.write_text(PRELUDE + defines + "\n" + source + port + DRIVER,
                            encoding="utf-8")
            result = subprocess.run([compiler, "-std=c99", "-O2", "-Wall", "-Werror",
                                     str(unit), "-o", str(exe)], capture_output=True,
                                    text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("ITEM_MUSIC_DIFFERENTIAL_OK checks=162732", result.stdout)


if __name__ == "__main__":
    unittest.main()

"""Host fuzz test of the event-script ledger (src/import/battleship_sys_objanim.c).

The ledger's functions -- the main and recent sorted runs, the commit, the
fold, the forget and the lookup -- are compiled for the host out of the source
file itself and driven by a random sequence of commits (small scripts into the
recent run, big ones straight into the main run), range forgets and lookups,
against a reference set of normalized command words. After every operation
both runs must be strictly sorted, disjoint, and together exactly the
reference; every lookup must agree with it, signature included.
"""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "menus"))
from source_test_helpers import function  # noqa: E402

SOURCE = ROOT / "src" / "import" / "battleship_sys_objanim.c"
NAMES = ("ndsAObjEvent32WordSig", "ndsAObjEvent32Key", "ndsAObjEvent32NormalizedTotal",
         "ndsAObjEvent32LowerBound", "ndsAObjEvent32RecentLowerBound", "ndsAObjEvent32MoveUp",
         "ndsAObjEvent32MergeIntoMain", "ndsAObjEvent32CommitKeys",
         "ndsAObjEvent32ForgetRange", "ndsAObjEvent32FindNormalized", "ndsAObjEvent32PlanReserve")


def _compiler():
    for name in ("clang", "gcc", "cc"):
        path = shutil.which(name)
        if path:
            return path
    return None


def _program() -> str:
    text = SOURCE.read_text()
    functions = "\n".join(function(text, name) for name in NAMES)
    return r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;
typedef int sb32;
#define TRUE 1
#define FALSE 0
#define NDS_DIAG(x) (x)
typedef union { u32 u; } AObjEvent32;
#define NDS_AOBJ_EVENT32_RAM_BASE 0x02000000u
#define NDS_AOBJ_EVENT32_RAM_END 0x02400000u
#define NDS_AOBJ_EVENT32_MERGE_BUFFER 256u
#define NDS_AOBJ_EVENT32_RECENT_MAX NDS_AOBJ_EVENT32_MERGE_BUFFER
#define NDS_AOBJ_EVENT32_INTERP_DESC_FIXED_MAX 8u
static u32 *sNdsAObjEvent32Keys;
static u32 sNdsAObjEvent32NormalizedCount;
static u32 sNdsAObjEvent32RecentCount;
static u32 sNdsAObjEvent32PlanCount;
static u32 sNdsAObjEvent32NormalizedLimit;
static u32 sNdsAObjEvent32MergeBuffer[NDS_AOBJ_EVENT32_MERGE_BUFFER];
static volatile u32 gNdsAObjEvent32RecentFolds, gNdsAObjEvent32ForgetHoleRemovals,
    gNdsAObjEvent32ForgetSkips, gNdsAObjEvent32HashHitCount, gNdsAObjEvent32HashMissCount;
static void *sNdsEvent32InterpDescFixed[NDS_AOBJ_EVENT32_INTERP_DESC_FIXED_MAX];
static u32 sNdsEvent32InterpDescFixedWord[NDS_AOBJ_EVENT32_INTERP_DESC_FIXED_MAX];
static u32 sNdsEvent32InterpDescFixedCount;
''' + functions + r'''
#define WORDS 20000u
#define LIMIT 3000u
static u8 ref[WORDS];
static u8 refsig[WORDS];
static u32 refcount;
static u32 rng = 12345u;
static u32 next(void) { rng = rng * 1103515245u + 12345u; return rng >> 8; }
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "op %u line %d: %s\n", op, __LINE__, #x); exit(1); } } while (0)
static u32 op;
static AObjEvent32 *at(u32 word) { return (AObjEvent32 *)(uintptr_t)(NDS_AOBJ_EVENT32_RAM_BASE + word * 4u); }
static void verify(void)
{
    const u32 *k = sNdsAObjEvent32Keys;
    u32 n = sNdsAObjEvent32NormalizedCount, v = sNdsAObjEvent32RecentCount, i, seen = 0;
    static u8 mark[WORDS];
    memset(mark, 0, sizeof(mark));
    CHECK(n + v <= LIMIT);
    for (i = 0; i < n; i++) {
        if (i) CHECK(k[i - 1] < k[i]);
        CHECK((k[i] >> 8) < WORDS); CHECK(ref[k[i] >> 8]); CHECK(!mark[k[i] >> 8]);
        CHECK((u8)k[i] == refsig[k[i] >> 8]); mark[k[i] >> 8] = 1; seen++;
    }
    for (i = LIMIT - v; i < LIMIT; i++) {
        if (i > LIMIT - v) CHECK(k[i - 1] < k[i]);
        CHECK((k[i] >> 8) < WORDS); CHECK(ref[k[i] >> 8]); CHECK(!mark[k[i] >> 8]);
        CHECK((u8)k[i] == refsig[k[i] >> 8]); mark[k[i] >> 8] = 1; seen++;
    }
    CHECK(seen == refcount);
    CHECK(ndsAObjEvent32NormalizedTotal() == refcount);
}
int main(int argc, char **argv)
{
    u32 *storage = calloc(LIMIT, sizeof(u32));
    if (argc > 1) rng = (u32)strtoul(argv[1], NULL, 10);
    u32 folds = 0, bigs = 0, forgets = 0;
    sNdsAObjEvent32Keys = storage;
    sNdsAObjEvent32NormalizedLimit = LIMIT;
    for (op = 0; op < 40000u; op++) {
        u32 kind = next() % 100u;
        if (kind < 55u) {
            /* A script: count fresh words, written as the plan's keys. */
            u32 count = (next() % 8u == 0u) ? 257u + next() % 300u : 1u + next() % 120u;
            u32 base = sNdsAObjEvent32NormalizedCount, i, run = next() % WORDS;
            if (ndsAObjEvent32NormalizedTotal() + count > LIMIT) continue;
            if (count > NDS_AOBJ_EVENT32_RECENT_MAX) bigs++;
            for (i = 0; i < count; i++) {
                u32 word, tries = 0;
                do { word = (next() % 4u == 0u) ? next() % WORDS : (run + i * (1u + next() % 3u)) % WORDS; }
                while ((ref[word] || refsig[word] == 0xEEu) && ++tries < 50u);
                if (ref[word]) { count = i; break; }
                /* mark planned so the plan holds no duplicate */
                refsig[word] = 0xEEu;
                sNdsAObjEvent32PlanCount = i;
                CHECK(ndsAObjEvent32PlanReserve());
                sNdsAObjEvent32Keys[base + i] = ndsAObjEvent32Key(at(word), next() & 0xffu);
            }
            sNdsAObjEvent32PlanCount = 0;
            for (i = 0; i < count; i++) {
                u32 key = sNdsAObjEvent32Keys[base + i];
                ref[key >> 8] = 1; refsig[key >> 8] = (u8)key; refcount++;
            }
            {
                u32 f0 = gNdsAObjEvent32RecentFolds;
                ndsAObjEvent32CommitKeys(count);
                folds += gNdsAObjEvent32RecentFolds - f0;
            }
        } else if (kind < 70u) {
            /* A retired buffer: every key inside it goes. */
            u32 start = next() % WORDS, len = 1u + next() % 600u, w;
            ndsAObjEvent32ForgetRange(at(start), len * 4u);
            for (w = start; (w < start + len) && (w < WORDS); w++) {
                if (ref[w]) { ref[w] = 0; refcount--; }
            }
            forgets++;
        } else {
            u32 i;
            for (i = 0; i < 16; i++) {
                u32 word = next() % WORDS;
                s32 idx = ndsAObjEvent32FindNormalized(at(word));
                CHECK((idx >= 0) == (ref[word] != 0));
                if (idx >= 0) CHECK((u8)sNdsAObjEvent32Keys[idx] == refsig[word]);
            }
        }
        verify();
        /* clear stale plan marks */
        if ((op & 1023u) == 0u) { u32 w; for (w = 0; w < WORDS; w++) if (!ref[w] && refsig[w] == 0xEEu) refsig[w] = 0; }
    }
    printf("ok folds=%u bigs=%u forgets=%u final=%u\n", folds, bigs, forgets, refcount);
    return 0;
}
'''


class Event32LedgerTest(unittest.TestCase):
    def test_runs_match_reference(self):
        compiler = _compiler()
        self.assertIsNotNone(compiler, "Host C compiler required")
        with tempfile.TemporaryDirectory() as temp:
            src = Path(temp) / "ledger.c"
            exe = Path(temp) / "ledger.exe"
            src.write_text(_program())
            subprocess.run([compiler, "-std=c11", "-O1", "-Wall", "-Wno-unused-function",
                            str(src), "-o", str(exe)], check=True)
            for seed in (12345, 1, 77, 4242, 90210):
                result = subprocess.run([str(exe), str(seed)], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, f"seed {seed}: {result.stderr}")
                self.assertIn("ok", result.stdout)
                print(seed, result.stdout.strip())


if __name__ == "__main__":
    unittest.main()

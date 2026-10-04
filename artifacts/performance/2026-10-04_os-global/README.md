# Whole-program -Os on the gate ROM, rejected, 2026-10-04

The late-window lab census puts half of all cycles in main-RAM code at 4.08
cycles an instruction, 35% of them non-memory stall (mostly instruction
fetch through the 8 KB I-cache). The question: does a size-optimized build
fetch enough less to pay for its extra instructions?

`build-gate-1004n`: the committed tree (`build-gate-1004l`) with `-Os`
appended to every C compile (a temporary `NDS_LAB_CFLAGS` hook, since
removed). Sections: `.main` 1,458,032 -> 1,247,736 B, `.itcm` 32,416 ->
25,832 B, `.text.hot` 3,796 -> 3,000 B.

Official gate, same session:

| | WORK P50 | WORK P95 | > 1.12M | two-VBlank | FTR P50 | STG P50 | SRC P50 | MISC P50 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `-O2` (`build-gate-1004l`, rerun) | 920,000 | 1,259,776 | 276 | -- | 138,432 | 152,256 | 394,048 | 171,008 |
| `-Os` (`build-gate-1004n`) | 980,288 | 1,341,184 | 435 | 1,516 / 1,961 | 155,520 | 161,408 | 418,304 | 181,760 |

Every bucket loses; replay digest identical (no latent undefined behaviour
surfaced). The code is tuned for `-O2` inlining; keep it. The rerun also
reproduces the committed pack's figures (P95 1,259,776 against 1,259,712).

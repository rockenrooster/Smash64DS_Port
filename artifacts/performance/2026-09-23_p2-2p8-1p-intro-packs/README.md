# 1P intro on compact packs: first attempt, reverted (2026-09-23)

The 1P campaign's stage-2 intro halts in `ndsSyMallocOverflowHalt`: it asks for
Yoshi's full 144,640 B Main through the source loader
(`sc1PIntroSetupFighterFiles` -> `ftManagerSetupFilesAllKind` ->
`ftManagerSetupFilesMainKind`) with ~50 KB free
(`../2026-09-23_p2-2p8-if-gamestatus-compact/README.md`). The intro takes full files
because the pack loader's scene gate (`ndsRelocUseBattleCoreFighterData`,
`src/port/reloc_preview_pack.c`) admits only the CSS scenes, battle and VS Results.

## Tried: the intro as a battle-core scene

`ndsRelocUseBattleCoreFighterData()` also returned TRUE for `nSCKind1PIntro`, the
treatment VS Results (another demo-status display scene) already gets. Probed with
`scripts/menus/probe-p2-campaign.ps1 -TransitionProof` on the all-content config
with the menu walk (`build-p2p8-intro`), then on a probe-only
`NDS_PREVIEW_HALT_NONFATAL=1` twin (`build-p2p8-intro-nf`) so the run continues past
the first decline and latches its witnesses (the probe now prints them as
`CPNONFATAL` at every stop):

- **Stage 1 intro (vs Link) loads from packs**: Link cost 53,536 B instead of
  108,254 B on the full-file path.
- **But Link's intro draw declines natively** on the packed file: 6,840 declines,
  decline stage 4 (validate), owner slot 6, HIGH detail, 19 selected roots,
  validator reject code 4 at root 5 -- observed 41,216 against expected 9,776. The
  same pack draws cleanly in the stage-1 battle (the count does not move there), so
  the intro's transient submit path (`ndsFighterIntroTransientSubmit`) resolves a
  packed file's root offsets differently from the battle path. On a shipping build
  this is the packed-1P halt (`ndsPreviewPackLoadHalt` reason 20,
  `renderer_adapter_fighter.c`), so the change breaks the stage-1 intro that works
  today (`pack-gate-halt-probe.txt`).
- **Stage 2 intro (Yoshi team)**: Yoshi's pack loads (14,472 B), then its Main extern
  closure halts (`ndsBattleCoreExternHalt`, reason 14) with 33,980 B free. The stage-2
  intro reaches its fighter loads with ~200 KB less heap than stage 1's (75,624 B free
  after Mario vs 290,328 B before Mario in stage 1) -- a separate residency question
  (what the team intro holds, or what survives from the stage-1 battle).

The gate change was reverted; nothing here is in the shipping code.

## Next (queued for one subagent)

1. Why the intro path's packed-root offsets differ from the battle path's
   (validator code 4, root 5, 41,216 vs 9,776) -- fix at that seam so packed intro
   fighters validate.
2. Where stage 2's intro heap goes (census the scene's allocations; compare with
   stage 1's intro and check for survivors of the stage-1 battle).
3. Then the gate change again, proved through stage 2's battle and beyond.

Evidence: `1p-intro-packs-probe.txt` / `pack-gate-halt-probe.txt` (shipping-halt
run), `1p-intro-nf-probe.txt` (non-fatal run with `CPNONFATAL` witnesses).

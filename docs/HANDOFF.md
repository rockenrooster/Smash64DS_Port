# Handoff

Updated: 2026-08-03 07:10 Central. **The `BUGS.md` queue is worked out.** All
nine rows carry a fix in the tree on `codex/r2-runtime2`; Boundary
(`battle_playable_realtime`, mode 163) is green against
`smash64ds-battle-playable-hwtri.nds`.

## What is owed next: the owner's eye, not more engineering

Six of the nine rows are visual. Under the render-fidelity doctrine the owner is
the oracle for those, so the queue cannot close from this side.

**Test `smash64ds-battle-playable-hwtri.nds`.** That is the configuration every
row was reported against and the only published ROM with hardware triangles and
the three Task 39 FX flags on. `smash64ds.nds` is **not part of P1** (owner,
2026-08-02) and builds with all four flags at 0 — it has no particle quads at
all, so none of this queue is visible in it. Do not spend a cycle rebuilding it.

What to look for, in the order the rows appear in `BUGS.md`:

- Crowd cues play to the end of the sample instead of fading at the note end.
- The respawn pad is a solid source glow under the fighter, not a faint outline.
- Fox's down B is a deep blue barrier with a cyan edge, not pale cyan on blue.
- The shield is the source's textured bubble, not a black ring and disc.
- Hard landing throws a shockwave as well as dust.
- KO bursts play on **every** KO, not four in six.
- Results confetti covers the scene rather than falling in one column.
- The Star KO twinkle fires at the top blast zone as the fighter vanishes.
- Side-A hits no longer throw an oversized orange ball.

## What the 2026-08-03 soak settled

One clean five-minute both-CPU run (`NO-FREEZE`, full counter dump) closed three
rows that had been argued from theory:

- **Nothing steals a crowd cue.** `PrematureRetire` 0, `PoolExhaust` 0,
  `GenerationMismatch` 0, `StaleStop` 0, `StopAll` 0, handles 7 of 8. The
  cut-off was the release window opening at the NOTE end while the SAMPLE still
  had 129–309 ms to run; `nds_audio_fgm.c:1014` releases at
  `max(note, audible)` for non-looping cues, and `DurationStop` 695 is now the
  correct stop rather than the early one.
- **The Star KO caller is right.** Three KOs, spawn `(3451, 2399, -14999)`:
  `y` is `camera_bound_top * 0.6` and `z` is the source's own DeadUpStar
  recession, both set in `ftcommondead.c` case 0 before the sparkle fires in
  case 1. The row's "not at the fighter" premise is refuted by the numbers.
- **The KO burst builds in full.** `Attempt` 3, `Complete` 3, `DropMask` 0,
  `QuadMiss` 0. Transform and generator pools read 13 and 11 against a cap of
  24 — strictly below it, so the demand is measured rather than floored.

**Actionable, not done:** `MissRingIDs[0]=17` twice, the only cue a natural
match still asks for and does not get. Appending it to `FULL_COVERAGE_IDS`
raises `KeyError: 17` — it is in neither the declared selectors nor
`ATTACK_CUE_AUDIT`, so it needs a new audit entry rather than a list edit. The
reasoning is recorded at the append point in
`scripts/sfx/render-audio-fgm-phase-pack.py`.

## Standing hazards this cycle re-proved

- **A saturated counter is a floor, not a measurement.** The particle pools were
  graded twice from matches that never ran a KO burst, then trimmed to fit a
  reading that had hit its cap; two of six KO bursts drew nothing. Now proven
  below cap — if either ever pins at 24 again the demand is unmeasured.
- **Check that the all-clear counter covers the failure.**
  `gNdsParticleRejectCount` read 0 through both saturations — struct rejects only.
- **A symbol's guard must be the guard of the thing it belongs to**, not of
  whatever it was typed next to. `ndsRendererSetParticleCamera` lived inside
  `#if NDS_RENDERER_HW_TRIANGLES` with an unguarded caller, so the one ROM that
  ships was the only one that failed to link; the rebirth display proc repeated
  the shape the same day inside `#if NDS_TASK39_FX_SHIELD`.
- **Read the asset before assuming the atlas can hold it.** The Fox reflector
  row looked like the shield and rebirth rows and is not: those two carry SHAPE,
  which A5I3's one shared 8-entry palette can encode as white plus coverage.
  The reflector carries COLOUR — two flat tones, no shape — so the same
  treatment maps its 81% body to alpha 0 and deletes it. A twenty-line offline
  probe over `relocData/346.vpk0.bin` settled that without spending a ROM.

## Restart surface

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

`docs/P1_EXECUTION_BOARD.md` is the only dynamic queue. `docs/BUGS.md` carries
the owner's verdicts and one stage line per row; the owner edits it directly
during a session, so preserve their wording and ordering verbatim.

Uncommitted in the tree: the owner's own `AGENTS.md` edit and their in-progress
`.agents/skills` / `.claude/skills` rename to the `nds-*` set. Leave both alone.

Useful captures:

```powershell
# Five minutes ends MID-MATCH, which is why this one completed. A seven-minute
# run ends into the static post-match screen and trips the detector honestly.
# -PollSeconds also HALVES the threshold: it trips at IdenticalFramesToTrip x
# PollSeconds, so 8 x 5 = 40s lands on the ~30s NitroFS scene-load dead air.
.\scripts\soak-freeze-watch.ps1 -Build build-r2-bothcpu -MinutesToRun 5 -IdenticalFramesToTrip 16
.\scripts\probe-ko-vfx.ps1
.\scripts\capture-sudden-death-entry.ps1 -CaptureAnnounce 20   # TIME UP
```

A clean checkout must build through `build.ps1`, not bare `make`: four of six
generated `.inc` files are gitignored. `-j`/`MAKEFLAGS` rules are in `AGENTS.md`
`## Builds`. Preserve canonical mode 163, renderer mode 9, mip 0, static
textures, source countdown, Dream Land water at frame 0, Task 16 `1/1/1`. Do not
edit `decomp/`.

If the owner rejects a visual row: shield/halo cell resolution is capped by the
8,192-byte sheet bound, and the way to buy more is to name non-live common
textures to drop, **not** to grow the sheet — every larger allocation has been
measured to break stage texture resolves. The halo's source spin (node[2], rotY
0 -> 2*pi over 30 frames) is deliberately not reproduced; a camera-facing quad
has no meaningful rotY, so the honest route would be a second quad with a
rotating UV.

Run `New-Smash64DSSnapshot.ps1` last, and nothing after it.

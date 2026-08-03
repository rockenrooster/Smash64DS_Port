# Handoff

Updated: 2026-08-02 21:00 Central. **The `BUGS.md` queue is worked out.** All
thirteen rows carry a fix in the tree on `codex/r2-runtime2`, verified by
`verify-current.ps1 -Build` (Latest: runtime + `battle_playable_realtime`).

## What is owed next: the owner's eye, not more engineering

Nine of the thirteen rows are visual. Under the render-fidelity doctrine the
owner is the oracle for those, so the queue cannot close from this side.

**Test `smash64ds-battle-playable-hwtri.nds`.** That is the configuration every
row was reported against and the only published ROM with hardware triangles and
the three Task 39 FX flags on. `smash64ds.nds` builds with
`NDS_RENDERER_HW_TRIANGLES=0` and all three FX flags at 0 — it has no particle
quads at all, so none of this queue is visible in it. Both published ROMs and
the tick-HUD ROM were rebuilt from this tree.

What to look for, in the order the rows appear in `BUGS.md`:

- Whispy's wind particles fade out instead of turning flat at end of life.
- Crowd cues no longer cut off by a big hit (FGM 360 was the example).
- The respawn pad is the source glow under the fighter, not a procedural disc.
- The rolling dodge is quieter again (FGM 11, a further -3 dB).
- Named effects — foot dust, fireball hit, Fox down-B, hard landing — at 16x16.
- KO bursts play on **every** KO, not four in six.
- Results confetti covers the scene.
- FGM 153 AltitudeWarn fires at Dream Land's real lower bound.
- Fox's shield does not freeze the match.
- The shield is the source's textured bubble, not a ring and disc.
- VFX on the right of the stage no longer look squashed.
- The Star KO twinkle sits at the top blast zone.
- FGM 12 DeadUpStar is less harsh.

## Open question for the owner, not an action

`smash64ds.nds` is the software-renderer configuration with every Task 39 FX
flag off. It is a published ROM and a materially poorer game than the hwtri one.
Nobody has said whether that is intended. Raised because it is now known, not
because anything is blocked on it.

## Two things that were only findable because a link broke

`make` with no overrides — the published `smash64ds.nds` — **did not link**.
`ndsRendererSetParticleCamera` lives inside `#if NDS_RENDERER_HW_TRIANGLES` and
its caller does not; nine Makefile targets force that flag to 1, so every lab,
tickhud and hwtri build had the symbol and the one ROM that ships had never been
built since it appeared. Behind it, from the same commit, `TITLE_LOGO_FIRE`
counted `efParticleInitAll`'s two pool GObjs inside a delta meant to grade one
function. **Neither failure was visible while the other existed.**

`docs/VERIFYING.md` now records that `verify-current.ps1 -Build` is the only
routine command that compiles the default configuration. Run it before any
commit that publishes.

## Standing hazards this cycle re-proved

- **A saturated counter is a floor, not a measurement.** The particle transform
  and generator pools were graded twice from matches that never ran a KO burst,
  then trimmed to fit a reading that had hit its cap; two of six KO bursts drew
  nothing. Both pools are now 24, sized to be provable: the next both-CPU soak
  must report `TransformsMax` and `GeneratorsMax` **strictly below 24**, and if
  either pins at 24 the demand is still unmeasured.
- **Check that the all-clear counter covers the failure.**
  `gNdsParticleRejectCount` read 0 through both saturations — struct rejects only.
- **A symbol's guard must be the guard of the thing it belongs to**, not of
  whatever it was typed next to. The rebirth display proc landed inside
  `#if NDS_TASK39_FX_SHIELD` and failed to compile in the default config — same
  shape as the link break, same day.
- **Test the packer offline before spending a ROM.** Five atlas configurations
  were compared by monkeypatching the generator's `extra_candidates`; native
  resolution for the two new cells drops live texture 41.

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
.\scripts\soak-freeze-watch.ps1 -Build build-r2-bothcpu -MinutesToRun 7 -PollSeconds 5
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

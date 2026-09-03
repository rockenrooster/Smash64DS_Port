# P2-6 — 1P Game (full campaign)

The original 1P mode, Mario-first, then every character. Depends on the full
roster (opponents), P2-2 (ally battles are 4-fighter), stages, and items.

## Campaign flow (mechanical equivalence to `mn/mn1pmode` + `gm/` 1P logic)

Stage sequence, **read from `dSC1PGameStageDesc`** (`sc/sc1pmode/sc1pgame.c:295-566`),
not from memory: Link (Hyrule) → Yoshi Team ×18 (**YosterSmall**, `:314`) →
Fox (Sector Z) → **Bonus 1: Break the Targets** → Mario + Luigi ×2 with one
ally (**Castle**, `:360`) → Pikachu (Yamabuki) → Giant DK (Jungle, +2 allies)
→ **Bonus 2: Board the Platforms** → Kirby Team ×8 (Pupupu, 2 simultaneous,
`sc1pgame.h:16-17`) → Samus (Zebes) → Metal Mario (Metal) → **Bonus 3: Race to
the Finish** → Fighting Polygon Team ×30 (Zako, `sc1pgame.h:9`) → Master Hand
(Last). Then the challenger fights: Luigi (Castle), Ness (Pupupu), Jigglypuff
(Yamabuki), Captain Falcon (Zebes) — `sc1pgame.c:507-565`.

Two entries in the earlier from-memory list were **wrong** and are corrected
above: the two-on-one Mario Bros. fight is on Peach's Castle, not Mushroom
Kingdom, and the Yoshi team fight is on the *small* Yoshi's Island variant.
Item toggles are `0xFFFFFFFF` on every entry. Enemy stock counts are still
unverified. The driver is `sc/sc1pmode/sc1pmanager.c` (`spgame_stage` loop at
`:318,473,506`); difficulty and stock come from the 1P character select
(`mn/mnplayers/mnplayers1pgame.c:3266-3268`), and a continue halves the score
(`mn/mn1pmode/mn1pcontinue.c:1075`).

There are **58** bonuses, not a short list: `nSC1PGameBonusCheapShot` through
`nSC1PGameBonusTrueFriend` (`sc/scdef.h:319-379`), with their scoring table in
`sc/sc1pmode/sc1pstageclear.c:36-410`.

## Work breakdown

1. **Campaign driver**: difficulty (Very Easy–Very Hard) + stock selection,
   stage sequencing, inter-stage results (score tally), continue flow
   (halved score), Game Over, character-complete congratulations + ending,
   credits ("A Brief History of…" staff-roll shooting minigame — part of the
   game's identity, keep it).
2. **Scoring/bonus system**: full original bonus list (No Damage Clear,
   Speedster, Pacifist, etc.) from source tables; high-score persistence
   (save stub until P2-7).
3. **Variants** (`fighters/variants.md`): Metal Mario (`ftmmario` — Mario
   moveset, metal material/SFX, stat overrides), Giant DK (`ftgdonkey`),
   Fighting Polygons (`ftn*` ×12 — low-poly models are a gift to the DS;
   reduced movesets per source).
4. **Master Hand** (`fighters/master-hand.md`): HP boss, scripted attack
   patterns, no hitstun/knockback physics, `ftboss` reference.
5. **1P-only venues**: `stages/final-destination.md`, `stages/meta-crystal.md`,
   `stages/duel-zone.md`, `stages/race-to-the-finish.md` through the P2-4
   pipeline.
6. **Bonus stages** (`stages/bonus-stages.md`): Break the Targets + Board the
   Platforms. Logic is shared (`gr/grbonus/`); boards are per-fighter data.
   Mario's two boards land first (campaign needs them); the other 22 ride as
   their fighters' campaigns come online and back the standalone Bonus
   Practice mode (P2-7 menu entry).
7. **Team/ally battles**: 2v1 and 1+allies fights on the P2-2 engine; ally
   CPU behavior per source.
8. **Every-character campaigns**: after Mario's is accepted, remaining 11 are
   data (opponent = player substitutions, endings, bonus boards) — batch rows.

## Risks

- Master Hand is a bespoke actor (own moveset interpreter/scripts) — budget
  it like a new fighter, not a stage prop.
- Sequential-team fights (18 Yoshis) stress spawn/despawn paths — reuse
  respawn machinery, watch heap watermarks across waves.
- Race to the Finish is the only scrolling course in the game — camera and
  KO-boundary semantics differ; keep it stage-owned code.

## Exit criteria

- [ ] Mario campaign start-to-credits on every difficulty, mechanically
      equivalent (owner plays it through).
- [ ] All variants + Master Hand per unit DoD.
- [ ] Bonus 1/2/3 + score/bonus tally + continues + endings + credits.
- [ ] All-character campaigns landed (batch rows).
- [ ] Every new screen holds its cadence; stress config unaffected (1P fights
      are ≤ the 4-CPU stress envelope) — spot-verify Polygon ×3 + items.

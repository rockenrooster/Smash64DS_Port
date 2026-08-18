# P2-6 — 1P Game (full campaign)

The original 1P mode, Mario-first, then every character. Depends on the full
roster (opponents), P2-2 (ally battles are 4-fighter), stages, and items.

## Campaign flow (mechanical equivalence to `mn/mn1pmode` + `gm/` 1P logic)

Stage sequence: Link (Hyrule) → Yoshi Team ×18 (Yoshi's Island) → Fox
(Sector Z) → **Bonus 1: Break the Targets** → Mario Bros. (Mushroom Kingdom,
2v1) → Pikachu (Saffron) → Giant DK (Congo Jungle, 1+2 allies v1) →
**Bonus 2: Board the Platforms** → Kirby Team ×8 (Dream Land) → Samus (Zebes)
→ Metal Mario (Meta Crystal) → **Bonus 3: Race to the Finish** → Fighting
Polygon Team ×30 (Duel Zone) → Master Hand (Final Destination).
Verify counts/order/allies from source at implementation, never from memory.

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

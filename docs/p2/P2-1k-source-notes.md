# P2-1k — Historical Source-Pin Index

Reference-only record of the August 18 shell transcription. The original text is preserved by the installer and linked below. It is not a current task list, a patch to apply, or evidence that today's shell lacks those elements.

## Useful source facts to retrieve selectively

| Topic | Original source / fact category | Current owner |
|---|---|---|
| CSS occupied-slot name/emblem/CPU-level behavior | `mnplayersvs.c`: name and CP-level row switching, token carry, panel fighter creation | `P2-1-vs-shell.md` |
| SSS ordering and source art placements | `mnmaps.c`: wallpaper/plaque/labels/icons/name/emblem/cursor/preview | `P2-1-vs-shell.md` |
| Menu BGM starts/stops | `mntitle.c`, `mnmodeselect.c`, `mnvsmode.c`, `mnplayersvs.c` source entry/exit conditions | Shell/audio owner of the actual bug |
| Title label/logo/Press Start animation | `mntitle.c: mnTitlePlayAnim` and source AnimJoint data | Shell/title native owner |
| Historical UI bake and allocation changes | Original document's scratch/patch and now-retired surface names | `P2-1c-vram-map.md` for actual current ownership |

## Supersession rules

Do not apply the old unwired bake patch, recreate retired placeholder art, or repeat old “current shell is wrong” statements without a current candidate symptom. Original source pins remain useful; original implementation-state claims are dated. Follow the current native-only and 30 Hz contract. Host baking is allowed; an old suggestion to keep a generic source-sprite renderer in a ROM is not.

When a relevant source constant changes, verify its defining symbol in the pinned/source revision rather than trusting an old line number. Preserve already-qualified artwork and behavior. New evidence goes to the existing owning unit/bug note, not another chronological layer in this index.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayersvs.c`.
- `decomp/BattleShip-main/decomp/src/mn/mnmaps/mnmaps.c`.
- `docs/p2/P2-1-vs-shell.md`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-1k-source-notes.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-1k-source-notes.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.

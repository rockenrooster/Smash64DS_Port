# 1P stage clear: the battle frame held through the tally load, and its photo (2026-10-04)

Owner (r74 BUGS.md): "Transition from match end to score has black background,
should hold last match end frame and the score should draw over the last frame
with a tint like the N64 does" (N64 reference: the tally over the darkened
battle frame).

## What r74 did

The battle-exit capture (`ndsPlatformTransitionSnapshotOverWallpaper`,
`2b853e8a60f`) worked on the Hyrule exit it was built on, and declined on most
other stages (`gNdsTransitionSnapshotDecline` 4, no free bank): the tally's
whole load was black and its photo (`sc1PStageClearCopyFramebufToWallpaper`)
was empty. Walk ROM at r74's source, exit preconditions at the hold
(`exitpre.ps1`):

| Exit | VRAMCNT A-D | live texture blocks in D | result |
|---|---|---|---|
| 0 Hyrule (Link) | 83 8B 81 9B | none | held, photo |
| 4 Mario Bros (Castle) | 83 8B 81 9B | 6 blocks, 0x06861560-0x06864160 | decline 4, black |
| 12 Polygon Team (Duel Zone) | 83 8B 81 9B | 1 block, 0x06860A80 (+0x400) | decline 4, black |
| 3 Break the Targets (scene 53) | -- | -- | not a 1P-game kind: decline 4, black |

D is the battle's loan from BG3 (lent and asked back at the exit), so every
block in it is the leaving battle's; the held frame replaces the 3D layer, and
the next scene's entry drops every texture name (`glResetTextures`). The
capture no longer refuses a D with live blocks, and bonus stages (scene kind
53) take it too.

## After (walk ROM, `stclr3.ps1`, VBlank stamps)

| Exit | hold | stage clear start | photo | first tally frame |
|---|---|---|---|---|
| 4 Mario Bros | 1716 | 1724, snapshot in D | 1737 | 1782 (held until then) |
| 12 Polygon Team | 3065 | 3074, snapshot in D | 3087 | 3133 |
| 3 Break the Targets | 1807 | 1815, snapshot in D | 1828 | 1871 |

Captures per exit: `a-start` (the held battle frame at the tally's start),
`c-draw` (the tally's first draw, still held), `e-uncover` (the held frame at
the release), `f-tally` (40 frames later: STAGE CLEAR / RESULT over the
darkened photo).

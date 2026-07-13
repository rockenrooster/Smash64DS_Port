# Morning Test Checklist

Use a fresh emulator boot with
`D:\Stuff\DevFolder\Smash64DS_Port\smash64ds-battle-playable-hwtri.nds`.

SHA-256: `B4A89E279445E3EB44C0931E5C718B3A81D5FD4B578F71702C3D0ADDB9A6E258`

- Walk off both ends of the main stage at full stick. Mario should fall as soon
  as he loses support.
- Walk slowly toward an edge. A brief teeter is source-correct; extended
  walking in air is not.
- Walk off every floating platform in both directions.
- Jump and double-jump from stage center; check ascent, descent, animation, and
  landing.
- Use Up-B near center and near an edge; watch recovery, camera tracking, pose,
  landing, and clipping.
- Try neutral, side, down, and up A. Record whether each starts an animation and
  whether it can hit Fox.
- Try neutral B and confirm whether a fireball projectile appears.
- Try Down-B and note any other special behavior.
- Watch for ground clipping, platform fall-through, freezes, crashes, or a
  one-frame flash.

For each problem, send the exact input sequence, result, rough delay, how often
it reproduces, and a screenshot/video if convenient.

# Helper code status

This pack contains helper modules that are intentionally not wired into the active build.

Host compile smoke test was run against the current port headers extracted from `Smash64DS_Port_62026839.7z`:

```bash
gcc -std=c11 -Wall -Wextra \
  -I renderer_accel_pack/include \
  -I renderer_more/include \
  -c src/nds/renderer_work/*.c
```

The smoke test checks basic C syntax and header compatibility only. It does not prove devkitARM/libnds integration or runtime correctness.

## Implemented helper coverage

| Area | Helper coverage |
|---|---|
| N64 display-list parser | opcode constants and state callback support, meant to plug into current `ndsRendererExecuteDisplayList` |
| RSP matrix stack behavior | fixed-point stack, identity/multiply/translate/scale/rotate, N64 `Mtx` load helper |
| RDP combiner/material behavior | command-state tracking for colors, combine words, texture state, image/tile/load/tile-size state |
| texture decoding/conversion | CPU decode to DS-style 15-bit texels |
| palette / CI texture handling | CI4/CI8 with RGBA16 palette lookup |
| RGBA / IA / I texture handling | RGBA16/RGBA32, IA4/IA8/IA16, I4/I8 |
| sprite drawing | design notes only; use startup logo path as seed |
| DObj/SObj/MObj traversal | design notes only; original traversal should stay in BattleShip code |
| camera transforms | matrix helper only; CObj projection integration remains future work |
| depth handling | design notes only |
| DS polygon/VRAM upload | design notes only; start with monotonic allocator after CPU texture decode works |

## Integration warning

Do not add every helper to `Makefile` at once. Integrate one module per milestone and add verifier markers before expanding further.

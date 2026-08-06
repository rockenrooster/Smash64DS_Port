# OSContPad LUS-vs-decomp sizeof Overrun Clobbers Edge-Detection State (2026-04-24) — FIXED

**Symptoms:** Menu-only input double/triple-fire. Gameplay attack/jump fine. Concrete reproductions:
- Pause button: game starts pausing and unpausing on a single press.
- VS Character Select: a fighter puck cannot be picked up after being placed — an `A_BUTTON` tap registers as grab-then-immediate-drop.
- CPU skill level slider: one keypress advances the value by 2–3 steps instead of 1.

Instrumentation in `src/sys/controller.c` showed each physical press producing a *fresh* `unk02` edge on 4+ consecutive frames (`edge+=0x1000`, `unk04=0x1000`, `hold=0x1000` published every frame), which should be impossible — `syControllerReadDeviceData` stamps `sSYControllerDescs[i].unk00 = button` at the end of every read, so the next frame's XOR edge `(button ^ unk00) & button` should be zero while the button is held.

**Root cause:** The typename `OSContPad` refers to **two different struct layouts in the same build**:
- Decomp view (`include/PR/os.h`, seen by C in `src/sys/controller.c`): `{ u16 button; s8 stick_x; s8 stick_y; u8 errno; }` — ~6 bytes.
- libultraship view (`libultraship/include/libultraship/libultra/controller.h`): adds `err_no`, `gyro_x`, `gyro_y`, `right_stick_x`, `right_stick_y` — `size = 0x24` (36 bytes).

`syControllerReadDeviceData` declares `OSContPad sSYControllerData[MAXCONTROLLERS]` using the decomp's 6-byte layout, so the array reserves ~24 bytes. The controller I/O stub `osContGetReadData` lives in `libultraship/src/libultraship/libultra/os.cpp` (C++, sees the 0x24-byte LUS layout) and did:

```cpp
void osContGetReadData(OSContPad* pad) {
    memset(pad, 0, sizeof(OSContPad) * __osMaxControllers);   // LUS sizeof = 0x24 → 144 bytes
    Ship::Context::GetInstance()->GetControlDeck()->WriteToPad(pad);  // writes 0x24 per entry too
}
```

The memset walked 144 bytes starting at `sSYControllerData`. The linker placed `sSYControllerDescs[]` immediately after `sSYControllerData[]` in BSS, so every Read was zeroing the leading fields of `sSYControllerDescs[]` — specifically `unk00` (prior-frame button state). With `unk00 = 0` at the start of every Read, `(button ^ unk00) & button == button` → a fresh edge fired every frame the button was held, and the menu edge-triggered consumers (`button_tap`) saw a new press on every tick. `WriteToPad` then wrote 36 bytes per entry through the same mis-sized pointer, piling on more corruption.

Gameplay used `syControllerScheduleRead` (AutoRead=FALSE) instead of `syControllerFuncRead` — but both paths go through the same Read, so the corruption happened there too. Gameplay got away with it because its consumers read `button_hold` (current state), not `button_tap` (edge): `button_hold` just reflects whatever's in `unk00`, which gets re-stamped with the true button state at the end of each Read a few lines later. Edge-driven menus were unique in relying on `unk00` surviving between Reads.

**Fix:** In `libultraship/src/libultraship/libultra/os.cpp`, write into a local LUS-layout stack buffer, then copy only the four fields the decomp struct actually has into the caller's pointer:

```cpp
void osContGetReadData(OSContPad* pad) {
    struct GameContPad { uint16_t button; int8_t stick_x; int8_t stick_y; uint8_t err_no; };

    OSContPad lus_pads[MAXCONTROLLERS] = {};
    Ship::Context::GetInstance()->GetControlDeck()->WriteToPad(lus_pads);

    GameContPad* out = reinterpret_cast<GameContPad*>(pad);
    for (int i = 0; i < __osMaxControllers; i++) {
        out[i].button  = static_cast<uint16_t>(lus_pads[i].button);
        out[i].stick_x = lus_pads[i].stick_x;
        out[i].stick_y = lus_pads[i].stick_y;
        out[i].err_no  = lus_pads[i].err_no;
    }
}
```

No memset of the caller's buffer is needed: only the fields the game reads get written, and they're written unconditionally on every call.

**Files:**
- `libultraship/src/libultraship/libultra/os.cpp` — rewrite `osContGetReadData` to marshal through a local LUS-sized buffer.

Note: there is also a `/os.cpp` at the project root with an identical buggy `osContGetReadData`. It is **not** in the CMake build (the root `CMakeLists.txt` globs only `port/*` and `src/*`, and `libultraship` supplies the linked stub). It appears to be a leftover template from the LUS reference — harmless but misleading; treat as dead code.

**Class-of-bug lesson:** When a header file from a third-party library (libultraship here, but OOT/MM ports have the same risk) re-uses a typename that also exists in the decomp's own `PR/os.h`, every compilation unit silently picks one layout or the other based on include order. Sizes diverge the moment either side adds a field. Any function that crosses the C↔C++ boundary with a pointer to one of these shared-name types is effectively calling across an ABI boundary with no type checking. Audit candidates: `OSContPad`, `OSContStatus`, `OSMesg`, `OSPifRam`, anything whose LUS definition has a size comment like `// size = 0xNN`.

The failure mode here — a stack-adjacent BSS array getting zeroed through an oversized memset — is very quiet: no crash, no visible glitch on the write, just a specific set of bytes in the next global becoming zero every frame. Easy to miss unless something downstream is sensitive to that specific location (here, the first field of the struct that happens to sit right after the array in link order).

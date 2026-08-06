# Second gamepad unusable: routing defaults, event-driven resets, missing port mappings

**Date:** 2026-06-10
**Status:** FIXED (libultraship `ssb64` branch)
**Reported as:** "both PS5 controllers are selected under 1P; can't bind
the second controller" with an ImGui `2 visible items with conflicting ID!`
warning in the input editor (two identical "DualSense Wireless Controller"
rows).

## Symptoms

- Two same-model controllers rendered as indistinguishable rows in the
  input editor's per-port device list, plus the ImGui ID-conflict warning.
- Both pads drove player 1; routing the second pad to port 2 either did
  nothing in-game or silently reverted.

## Root causes (three independent defects, all controller-type agnostic)

1. **ImGui ID collision** (`InputEditorWindow.cpp`): the device-row button
   label is the device name, and ImGui derives widget IDs from labels —
   identical pads → identical IDs. Fixed with a `###gamepadName_<sdl
   instance id>` ID suffix; rows are also sorted by instance id and
   deduplicated visually with `#1`/`#2` ordinals.

2. **Routing reset on every device event**
   (`ConnectedPhysicalDeviceManager::RefreshConnectedSDLGamepads`): the
   refresh runs on EVERY `SDL_CONTROLLERDEVICEADDED`/`REMOVED` event and
   unconditionally re-inserted every instance id into ports 2-4's ignore
   sets — any device event (pad reconnect, Bluetooth hiccup, third
   device) silently reverted the user's checkbox assignments. Now only
   never-before-seen instance ids get default routing.

3. **Defaults assumed a single player**: every pad defaulted to port 1,
   and `ControlDeck::Init` only seeded default button mappings for port
   1 — so a pad routed to port 2 produced zero input until the user
   found "Set Defaults" on the Port 2 tab. Now the Nth new pad
   auto-assigns to the first port with no gamepad (1st pad → port 1,
   2nd → port 2, ...), and ports 2-4 get SDL-gamepad default mappings at
   init (dormant until a pad is routed there).

## Diagnostic notes (macOS, 2× DualSense over Bluetooth)

- SDL probe: both pads enumerate with distinct instance ids and **identical
  GUIDs** (GUIDs are per-model, not per-unit). Serial numbers (BT MACs) are
  exposed by the hidapi driver but are **null** under the macOS
  GCController driver the game uses (`SDL_HINT_JOYSTICK_HIDAPI=0`) — so
  per-unit persistence keyed on serial is not portable; routing is
  in-memory per session, with deterministic connection-order defaults.
- A headless SDL probe (no window/runloop) sees **zero** pads under
  `SDL_HINT_JOYSTICK_HIDAPI=0` — the GCController backend needs a pumped
  runloop. Probe with a hidden window before concluding driver breakage.
- The game-side N64 "is a controller plugged in" state is NOT the issue:
  the port's `osContGetQuery` stub reports all four ports connected, and
  `ControlDeck::Init`'s `*mControllerBits |= 1 << 0` is dead data for this
  game (the decomp ignores the bits and reads the all-zero status array).

## Expected behavior after fix

Boot with two pads: Port 1 tab shows pad #1 checked, Port 2 tab shows pad
#2 checked, both with working default bindings — co-op/VS works without
touching the editor. Manual re-routing via the checkboxes persists for the
session and is no longer reverted by device events.

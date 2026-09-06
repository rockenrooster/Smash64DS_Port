# Nintendo DS Project Context

This optional repository-local file supplies facts that a generic DS coding
skill cannot know. Keep it factual and current. Do not turn it into a mandatory
status-report workflow.

## Toolchain

- devkitARM version/package snapshot:
- SDK generation and libnds/Calico/libdvm/libfat/Maxmod versions:
- build entry point:
- ARM7 strategy: stock runtime / Maxmod / custom:
- DS mode / DSi mode:
- C/C++ standard and important flags:

## Source and behavior authority

- original/source implementation:
- local reference repositories:
- accepted behavior/visual/audio approximations:
- unsupported features:

## Platform ownership

- display and VRAM-map owner:
- main/sub OAM owners:
- 3D frame-finalization owner:
- DMA channel reservations:
- timer reservations:
- ARM9/ARM7 IPC owner:
- audio owner:
- storage owner:

## Memory layout

- VRAM banks A-I by scene/mode:
- actual ARM9 application/heap range from linker and runtime:
- main-RAM arenas and capacities:
- main/worker stack placement and TLS cost:
- ITCM/DTCM policy:
- shared ARM9/ARM7 regions and cache rules:
- asset residency/streaming strategy:

## Timing

- logical update rate:
- presentation target:
- subsystem update rates:
- VBlank commit order:
- representative performance scenario, when needed:
- authoritative performance target: hardware / validated emulator / other:

## Correctness gates

- build/test commands:
- deterministic replays/scenes:
- visual/audio comparison method:
- hardware-only checks:
- known resource limits that must assert:

## Repository-specific prohibitions

- deprecated APIs or duplicate paths:
- directories that are read-only/reference-only:
- previously exhausted techniques that should not be proposed without a new
  mechanism:

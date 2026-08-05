# Input, Interrupts, Timers, and the Frame Loop

## One logical input sample per update

Call `scanKeys()` once at a defined logical-update boundary, then cache
`keysDown()`, `keysHeld()`, and `keysUp()` for all systems in that update.
Multiple independent scans can make subsystems observe different edges.

```c
#include <nds.h>

struct InputFrame {
    uint32_t down;
    uint32_t held;
    uint32_t up;
    touchPosition touch;
    bool has_touch;
};

static struct InputFrame read_input_frame(void)
{
    struct InputFrame input = {0};
    scanKeys();
    input.down = keysDown();
    input.held = keysHeld();
    input.up = keysUp();
    input.has_touch = (input.held & KEY_TOUCH) != 0;
    if (input.has_touch) {
        touchRead(&input.touch);
    }
    return input;
}
```

Use current runtime APIs and loop guards such as `pmMainLoop()` when that is the
project's generation. Do not transplant a legacy power-management loop into a
modern project.

## Frame-loop architecture

Separate responsibilities:

1. poll/publish input;
2. advance fixed or variable logical updates;
3. build render state and shadow OAM;
4. submit 3D/2D work;
5. wait or synchronize with presentation;
6. perform bounded VBlank commits;
7. collect timing/overrun information if enabled.

A simple 60 Hz program may combine these, but ownership should remain clear.

```c
while (pmMainLoop()) {
    const struct InputFrame input = read_input_frame();
    update_game(&input);
    build_render_state();
    submit_3d();

    swiWaitForVBlank();
    commit_2d();
}
```

Do not assume that waiting for VBlank means the preceding work met the frame
budget. Count overruns or skipped presentation intervals when timing matters.

## Fixed-step updates

For deterministic gameplay, define a logical tick rate independent of raw CPU
speed. When rendering at a lower rate than logic:

- sample edges once per logical tick or explicitly queue them;
- advance timers in logical units, not presentation count;
- define interpolation/extrapolation policy;
- do not run collision twice accidentally because rendering was skipped;
- ensure audio/event emission is not duplicated by catch-up updates;
- cap catch-up work to avoid a permanent spiral.

Make deliberate choices for pause, lid close, sleep, and long storage stalls.

## VBlank practices

VBlank is a synchronization window, not a general-purpose callback budget.
Good VBlank work:

- small register commits;
- OAM shadow copy/update;
- palette swaps or bounded dirty ranges;
- frame counters and timestamps;
- triggering a preplanned DMA operation whose timing is correct.

Bad VBlank work:

- decompression;
- filesystem access;
- heap allocation;
- scene traversal;
- large debug printing;
- unbounded queues;
- waiting for ARM7 or another interrupt.

Prepare data before VBlank; commit during it.

## IRQ rules

An interrupt handler must be bounded and reentrant with respect to the main
code's data ownership.

Allowed shape:

```c
static volatile uint32_t g_vblank_count;

static void on_vblank(void)
{
    ++g_vblank_count;
}
```

Even this counter has a contract: one IRQ writer, one same-CPU reader, naturally
aligned word access, and wrap tolerated. More complex payloads need a protected
handoff.

Do not call `printf`, allocate/free, access FAT/NitroFS, wait on DMA/IPC, or run
gameplay from an IRQ.

## Sharing data with an IRQ

For a one-bit event, a volatile flag can be adequate when lost/coalesced events
are acceptable. For exact event counts, use a counter. For multiword state:

- disable the relevant IRQ briefly around a copy;
- use a double buffer plus generation/index publication;
- use a runtime synchronization primitive appropriate to the CPU/context;
- keep the protected region short.

Do not rely on `volatile struct` to make a coherent snapshot.

## Timer practices

Reserve timers centrally. Record for each timer:

- CPU owner;
- mode/frequency and prescaler;
- reload value;
- overflow/IRQ behavior;
- reader width and wrap behavior;
- whether another library uses it.

When combining cascaded timers or reading a free-running counter, use a method
that handles rollover between low/high reads. Convert ticks using integer or
fixed-point arithmetic with proven range.

Profiling timers must not collide with audio, runtime, or gameplay timers.

## Sleep, lid, and lifecycle

Use the current platform/runtime path for lid close and power management. On
resume or repeated scene entry, explicitly restore state that may be lost or
changed:

- display enable/routing;
- VRAM mappings;
- OAM/BG/GX state;
- timer/IRQ registration;
- audio handles/queues;
- input edge baseline;
- pending asynchronous transfers.

Do not initialize the same service twice without a paired shutdown/reset.

## Debouncing and repeat

Hardware key edges are already represented by `keysDown`/`keysUp`. UI repeat
should be a logical timer layered on `keysHeld`, not repeated rescanning.
Touch input needs explicit validity, calibration/runtime handling, and policy
for drag outside bounds.

## Performance timing

For frame timing, record at least:

- update start/end;
- render preparation;
- GX submission/finalization;
- VBlank wait;
- missed/deferred frames;
- representative median and tail rather than only best case.

Instrumentation should have low and known overhead and be removable from release
builds.

## Common failures

### Button pressed once but action fires twice

The edge was consumed by multiple scans or the action is emitted in both update
and catch-up paths. Centralize the input snapshot and event ownership.

### Game slows permanently after one spike

Catch-up loop has no cap or each update emits more work than the next frame can
absorb.

### Random corruption around VBlank

Main code and IRQ share compound state without a protocol, or VBlank is doing
unbounded work and overrunning into active display.

### Timer values jump backward

Counter rollover was not handled, cascaded halves were read inconsistently, or
signed narrowing occurred.

## Review checklist

- [ ] Input is sampled once per logical update.
- [ ] Frame/update/presentation rates and catch-up policy are explicit.
- [ ] IRQ handlers are bounded and own only tiny state.
- [ ] Compound IRQ/main handoff is synchronized.
- [ ] VBlank contains commits, not arbitrary work.
- [ ] Timer channels, wrap, and units are documented.
- [ ] Resume and repeated initialization are deterministic.

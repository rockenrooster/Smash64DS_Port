#ifndef NDS_TASK37_PROFILE_H
#define NDS_TASK37_PROFILE_H

/* Task 37 census instrument.
 *
 * The repo-owned melonDS build carries a host-side ARM9 performance profiler
 * that attributes emulated cycles per program counter, including cache fills,
 * cache streaming, write-buffer drains, bus waits, interlocks, and pipeline
 * refills. It is driven entirely from the host environment
 * (MELONDS_ARM9_PROFILE_CSV=<path>) and needs no cooperation from the ROM to
 * collect, but it accumulates from reset -- so a plain run mixes several
 * seconds of boot, title, and menu traversal into the battle-loop numbers.
 *
 * The profiler exposes a control channel through the ARM946E-S Trace Process ID
 * register (CP15 c13,c0,1). Writes whose high halfword is 0x4D50 are consumed as
 * profiler commands. This header implements only that protocol: the constants
 * are the emulator's published interface, not vendored emulator code.
 *
 * On retail hardware these are ordinary CP15 writes. They cost a few cycles and
 * produce no output, so one ROM is valid on both, but nothing in a shipping
 * configuration should be paying for them: NDS_TASK37_PROFILE stays 0 in every
 * published target and compiles to nothing at all.
 *
 * Window: reset the profiler at NDS_TASK37_PROFILE_START presented frames --
 * far enough in that the scene has settled -- and dump the CSV
 * NDS_TASK37_PROFILE_FRAMES later. The dump is applied at the next instruction
 * boundary and does not stop the run.
 */

#ifndef NDS_TASK37_PROFILE
#define NDS_TASK37_PROFILE 0
#endif

#if NDS_TASK37_PROFILE

#ifndef NDS_TASK37_PROFILE_START
#define NDS_TASK37_PROFILE_START 438u
#endif

#ifndef NDS_TASK37_PROFILE_FRAMES
#define NDS_TASK37_PROFILE_FRAMES 128u
#endif

#define NDS_TASK37_PROFILE_MARKER_MAGIC 0x4D500000u
#define NDS_TASK37_PROFILE_MARKER_DUMP  0x4D50FFFDu
#define NDS_TASK37_PROFILE_MARKER_RESET 0x4D50FFFEu
#define NDS_TASK37_PROFILE_MARKER_OFF   0x4D50FFFFu

/* MCR is an ARM-mode instruction; the battle seam compiles to Thumb, so this
 * one function is forced to ARM and kept out of line. The interworking call is
 * paid twice per profiling run, on the two frames that open and close the
 * window, and never in a shipping build.
 *
 * Deliberately typed in plain unsigned int rather than u32 so the header has no
 * dependency on the port's type headers and can be included anywhere. */
__attribute__((noinline, target("arm")))
static void ndsTask37ProfileWriteMarker(unsigned int value)
{
    __asm__ volatile("mcr p15, 0, %0, c13, c0, 1" : : "r"(value) : "memory");
}

/* Called once per presented battle iteration, after the frame counter has
 * advanced. Opens the census window on one frame and closes it on one frame;
 * every other frame costs a compare. */
static inline void ndsTask37ProfileFrameTick(unsigned int presented_frames)
{
    if (presented_frames == (unsigned int)NDS_TASK37_PROFILE_START) {
        ndsTask37ProfileWriteMarker(NDS_TASK37_PROFILE_MARKER_RESET);
    } else if (presented_frames ==
               ((unsigned int)NDS_TASK37_PROFILE_START +
                (unsigned int)NDS_TASK37_PROFILE_FRAMES)) {
        ndsTask37ProfileWriteMarker(NDS_TASK37_PROFILE_MARKER_DUMP);
    }
}

#define NDS_TASK37_PROFILE_FRAME_TICK(frames) \
    ndsTask37ProfileFrameTick((unsigned int)(frames))

#else

#define NDS_TASK37_PROFILE_FRAME_TICK(frames) ((void)0)

#endif /* NDS_TASK37_PROFILE */

#endif /* NDS_TASK37_PROFILE_H */

# Wayland `SDL_SetWindowIcon` corrupts Fast3D renderer state on Fedora 43

**Date resolved:** 2026-05-22
**Status:** RESOLVED via Wayland gate (defensive — proper fix is in libultraship)
**Surface:** native local builds on Fedora 43 / Nobara (any glibc-2.40-era
distro running SDL2 ≥ 2.30 with a Wayland session)

## Symptom

Geometry intermittently missing from in-game rendering — most visible in
the attract-mode fighter-intro sequence ("Mario's legs gone" was the
canonical user report, but any frame with enough display-list traffic
shows dropped vertex chains). 100% reproducible per build on affected
hosts, environment-deterministic. CI Linux AppImages, macOS, Windows,
and any X11 session are unaffected.

## Root cause

Bisected to commit `9938319 icon(linux): SDL_SetWindowIcon at boot from
embedded PNG`. SDL2's Wayland backend (`Wayland_SetWindowIcon` in
`src/video/wayland/SDL_waylandwindow.c` on release-2.32.x) implements
icon setting via the `xdg_toplevel_icon_v1` protocol:

```c
Wayland_AllocSHMBuffer(icon->w, icon->h, &wind->icon);
SDL_PremultiplyAlpha(...);
wind->xdg_toplevel_icon_v1 = xdg_toplevel_icon_manager_v1_create_icon(...);
xdg_toplevel_icon_v1_add_buffer(wind->xdg_toplevel_icon_v1, wind->icon.wl_buffer, 1);
xdg_toplevel_icon_manager_v1_set_icon(..., toplevel, wind->xdg_toplevel_icon_v1);
```

That code path doesn't touch the GL context, EGL surface, or main
`wl_surface` *directly*. What it does do:

1. **`Wayland_AllocSHMBuffer`** — `memfd_create` + `ftruncate` + `mmap` +
   `wl_shm_pool_create_buffer`. The pool creation flushes pending
   protocol traffic and synchronously processes the resulting compositor
   replies (the wl_display dispatch loop runs as part of the buffer
   plumbing).
2. **Three new protocol requests** queued for the next flush.

During (1)'s implicit roundtrip, any `xdg_toplevel.configure` /
`wl_output.scale` / `xdg_surface.configure` events the compositor has
queued for our window since `Wayland_CreateWindow` get dispatched. The
call site for our `SetWindowIcon` runs in `PortInit`, immediately after
`gui->SetMenu()` and well before the first frame — by that point
libultraship's Fast3D backend has cached the initial viewport / output
scale, but the renderer is not yet pumping the per-frame configure
re-read that would pick up the late event. The cached state then drives
GBI translation against the wrong viewport for the rest of the session,
which manifests as missing geometry whenever the affected GBI commands
fan out (dense in attract-mode fighter intros).

This is the same class as SDL issue
[#6056 — stale viewport on Wayland with fractional DPI scaling](https://github.com/libsdl-org/SDL/issues/6056)
and [#9072 — EGL error when using render thread on Wayland](https://github.com/libsdl-org/SDL/issues/9072):
delayed configure / scale events landing after the renderer has cached
its assumptions, with no per-frame recheck.

CI Linux didn't catch it because the GitHub `ubuntu-22.04` runner is
headless (no Wayland session — SDL2 falls back to `dummy`/`x11`) and
jammy's SDL2 2.0.20 predates the `xdg_toplevel_icon_v1` backend entirely
(the protocol landed in SDL ~2.30); jammy `Wayland_SetWindowIcon` either
returned with no-op or used `_NET_WM_ICON` via XWayland.

## Why this is a layered bug

The proper fix is in **libultraship**, not the port: Fast3D's Wayland
surface code should either (a) re-read viewport / scale per frame on
Wayland targets, or (b) attach a `xdg_surface.configure` /
`wl_output.scale` callback that invalidates the cached surface
attributes. `SDL_SetWindowIcon` is just the first thing in our init
sequence that happens to trigger a Wayland roundtrip while configure
events are pending; any other Wayland-bound SDL call between
`InitWindow` and the first frame (e.g. a hypothetical
`SDL_SetWindowOpacity`, `SDL_SetWindowMinimumSize`, future protocol
features) is a latent re-trigger.

The port-side gate documented here is a defensive shim that closes the
*observed* trigger without the broader libultraship rework. Filing it
upstream is a follow-up.

## Fix

`port/port_window_icon.cpp::SetWindowIcon`: query
`SDL_GetCurrentVideoDriver()` and skip the entire icon-set call when the
active driver is `"wayland"`. X11, KMS/DRM, macOS, Windows paths
unchanged.

```cpp
const char* drv = SDL_GetCurrentVideoDriver();
if (drv && std::strcmp(drv, "wayland") == 0) {
    port_log("SetWindowIcon: skipping on Wayland (see this docs/bugs entry)\n");
    return;
}
```

## Verification

Four orthogonal A/B tests on Fedora 43 (clang 21, SDL2 2.32.64), all
against the same `9938319` build, confirm the call site is the trigger
and the Wayland backend is what makes the trigger fatal:

| A/B | Result |
|-----|--------|
| Disable `ssb64::SetWindowIcon()` call entirely (data still embedded in .rodata) | clean |
| Keep PNG decode + `SDL_CreateRGBSurfaceFrom`, comment out only `SDL_SetWindowIcon` line | clean |
| Force `SDL_VIDEODRIVER=x11`, original call active | clean |
| `SDL_PumpEvents()` immediately after `SDL_SetWindowIcon` (Wayland) | still broken — the renderer's cached state isn't re-read by event pumping; the bad state is locked in at init |
| Apply the Wayland gate (this fix), default Wayland session | clean, geometry renders correctly through all attract-mode intros |

The gate also confirmed the trade: on Wayland dev-build raw runs
(`./BattleShip` from a build dir, no installed `.desktop`), the
generic-app placeholder shows in the taskbar instead of the bundled
icon. Production installs are unaffected — both AppImage and any
distro-installed copy ship a `.desktop` with `Icon=BattleShip`, which
the Wayland WM resolves via `xdg_toplevel.app_id` / `WM_CLASS`.

## Cross-check: the "regression" was the icon code, not a Fedora update

User reported local builds had worked "until recently." Established via
`git bisect` between `v0.9.1-beta` and `HEAD`:

```
$ git bisect log
good: v0.9.1-beta (6d79b44)
bad:  main (48a6550)
good: f24a21c (perf(gfx): drop per-GBI-command mutex+scan — port_dl_ranges.cpp)
good: 83fc0a7 (Merge PR #191 JP bifurcation)
good: 5f3d972 (Merge PR #192 hires-rebase)
bad:  9938319 (icon(linux): SDL_SetWindowIcon at boot from embedded PNG)
good: e725555 (scripts/new-worktree — pre-9938319, script-only)
first bad commit: 9938319
```

So the regression is purely a repo commit (`9938319` introduced the
runtime SDL call); no Fedora system update is involved. The dist binary
appearing to "work earlier" was a small-sample observation under
intermittent timing — the bug was always 100% present on Fedora 43, the
investigation just took several false steps before instrumenting
correctly.

## Audit hook

Any future `SDL_*` call placed in port code between `InitWindow` and the
first rendered frame on Linux/Wayland is a candidate for the same class
of bug: a Wayland roundtrip during the call can deliver a delayed
configure that the cached Fast3D surface state never picks up. The
clean long-term fix is the libultraship per-frame re-read (or
configure-event invalidation); until then, any new such call should be
defensively gated or moved to post-first-frame.

Related: `linux_stale_scene_data_family_2026-05-11.md` (different
class — heap-pointer staleness, not surface-state staleness — but
same shape of "Linux glibc + GL driver makes an N64-port memory
assumption fatal, other platforms quietly tolerate it").

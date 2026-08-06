#pragma once

/* Linux taskbar / window-decoration icon. Windows reads the icon from
 * a Win32 resource compiled into the .exe via port/ssb64.rc; macOS
 * reads .icns from the .app bundle's Contents/Resources/. Neither path
 * exists on Linux — the WM only picks up an icon if the running app
 * calls SDL_SetWindowIcon(), so we do that explicitly post-InitWindow.
 *
 * The PNG bytes are baked into the binary at build time
 * (CMake → tools/embed_icon.py → generated/port_icon_data.h) so this
 * code has no runtime filesystem-layout coupling. The PNG source is
 * region-aware: US picks assets/icon.png, JP picks assets/icon-jp.png.
 *
 * No-op on non-Linux platforms — the OS-supplied icon is already
 * correct there. Safe to call early; if the SDL window isn't up yet
 * (non-OpenGL backend, or out-of-order init) it silently returns. */

namespace ssb64 {

void SetWindowIcon();

}  // namespace ssb64

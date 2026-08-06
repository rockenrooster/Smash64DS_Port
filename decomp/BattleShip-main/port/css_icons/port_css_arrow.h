#ifndef PORT_CSS_ARROW_H
#define PORT_CSS_ARROW_H

/**
 * port_css_arrow.h — CSS scroll-arrow sprite for Final Destination pagination
 *                    (port-only).
 *
 * Two getters — one per direction — backed by independently-baked PNGs so the
 * left variant is a genuinely flipped RGBA16 image rather than relying on
 * SObj-side scalex mirroring (which the renderer doesn't currently honor for
 * synthetic sprites).
 *
 *   portCSSGetScrollArrowSprite()      — right-pointing ">"
 *   portCSSGetScrollArrowLeftSprite()  — left-pointing  "<" (mirrored PNG)
 *
 * Arrow geometry (see tools/generate_arrow_svg.py for tuning):
 *   Visual: W = 6 px, H = 44 px → apex angle = 2 * atan(44/12) ≈ 149.5°
 *   Canvas: width is padded up to the next multiple of 4 px (RGBA16 row stride
 *           must be 8-byte aligned for Fast3D's TMEM line size, or the decode
 *           shears diagonally — see docs/bugs/sprite_decode_stride_mismatch).
 *           So W=6 visual → 8 px canvas with 1 px transparent padding each side.
 *   Fill: RGB(255, 128, 0) — bright orange, fully opaque
 *   Anti-aliasing: 8x supersampling via Pillow LANCZOS downsample
 *
 * Iteration workflow (rebuild only — no game restart needed):
 *   1. Edit tools/generate_arrow_svg.py (change W, H, color, or geometry).
 *      OR: hand-edit either arrow.png / arrow_left.png directly.
 *   2. Rebuild:  cmake --build <build> --target ssb64 -j 4
 *      CMake re-runs generate_arrow_svg.py (twice, once with --mirror) and
 *      png_to_c_array.py to regenerate arrow_data.h / arrow_left_data.h, then
 *      recompiles this TU and relinks.
 *   3. Run.
 *
 * To re-render at a new apex angle:
 *   python3 tools/generate_arrow_svg.py port/assets/css_icons/arrow.png      <W> <H>
 *   python3 tools/generate_arrow_svg.py port/assets/css_icons/arrow_left.png <W> <H> --mirror
 */

#ifdef PORT

#ifdef __cplusplus
extern "C" {
#endif

struct sprite;
typedef struct sprite Sprite;

/**
 * Return a pointer to the cached scroll-arrow Sprite (right-pointing ">").
 * Thread-safe; first call builds the Sprite from baked RGBA16 data.
 */
Sprite *portCSSGetScrollArrowSprite(void);

/**
 * Return a pointer to the cached scroll-arrow Sprite (left-pointing "<").
 * Backed by a separate mirrored PNG so the renderer doesn't depend on SObj
 * negative-scalex mirroring (which doesn't work for synthetic sprites).
 */
Sprite *portCSSGetScrollArrowLeftSprite(void);

#ifdef __cplusplus
}
#endif

#endif /* PORT */

#endif /* PORT_CSS_ARROW_H */

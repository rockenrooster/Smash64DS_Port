/* NDS_COLD: a function no battle profile gives a cycle (setup, loaders,
 * validation, learning, menus' one-off work) is built at -Os, and as Thumb
 * inside an ARM unit: half the bytes of ARM, the battle's hot code untouched
 * (P8 R1). The sets come from the profile census over every VS stage with the
 * cast, the heavy roster and the mixes. Host harnesses drop the marker
 * (scripts/menus/source_test_helpers.function). */

#ifndef NDS_COLD_H
#define NDS_COLD_H

#if defined(__arm__) && !defined(__thumb__)
#define NDS_COLD \
    __attribute__((cold, noinline, target("thumb"), optimize("Os")))
#else
#define NDS_COLD __attribute__((cold, noinline, optimize("Os")))
#endif

#endif /* NDS_COLD_H */

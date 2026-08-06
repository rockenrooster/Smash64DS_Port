#pragma once

/*
 * Selector shim — the actual RELOC_FILE_COUNT define and the
 * gRelocFileTable[] extern live in RelocFileTable.<version>.h, generated
 * per-version by tools/generate_reloc_table.py. This mirrors the decomp's
 * include/reloc_data.h design: switching versions only invalidates the
 * version-specific header, so US and JP generated tables coexist in the
 * source tree and separate build dirs never fight over one file.
 *
 * REGION_US / REGION_JP come from SSB64_COMPILE_DEFS (CMake, driven by
 * SSB64_VERSION).
 */

#if defined(REGION_US)
#include "RelocFileTable.us.h"
#elif defined(REGION_JP)
#include "RelocFileTable.jp.h"
#else
#error "RelocFileTable.h included without REGION_US or REGION_JP defined"
#endif

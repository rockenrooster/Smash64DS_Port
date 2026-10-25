/* Host-only build config shadow for the blob loader test.
 * ROM builds get this via -include $(BUILD)/nds_build_config.h;
 * the host harness defines the same default (Makefile ?= 0). */
#ifndef SSB64_NDS_BUILD_CONFIG_H
#define SSB64_NDS_BUILD_CONFIG_H

#define NDS_STAGE_BAKED_WORLD 0
/* The generated header's counters-on arm. */
#define NDS_DIAG(...) do { __VA_ARGS__; } while (0)

#endif

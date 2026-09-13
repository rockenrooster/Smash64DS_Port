/* Host-only build config shadow for the blob loader test.
 * ROM builds get this via -include $(BUILD)/nds_build_config.h;
 * the host harness defines the same default (Makefile ?= 0). */
#ifndef SSB64_NDS_BUILD_CONFIG_H
#define SSB64_NDS_BUILD_CONFIG_H

#define NDS_TASK51_STAGE_NATIVE 0

#endif

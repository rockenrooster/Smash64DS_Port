#ifndef SSB64_NDS_OS_BACKEND_H
#define SSB64_NDS_OS_BACKEND_H

/* Resume each runnable/waiting emulated N64 thread once. */
void ndsOsRunThreads(void);
void ndsOsPostVBlank(void);

/* Boot-time architecture check; returns zero on success. */
int ndsOsSelfTest(void);

#endif

#ifndef SSB64_NDS_SYS_DEBUG_H
#define SSB64_NDS_SYS_DEBUG_H

void syDebugStartRmonThread8(void);
void syDebugPrintf(const char *format, ...);
void syDebugSetFuncPrint(void (*function)(void));
void syDebugStartRmonThread5Hang(void);

#endif

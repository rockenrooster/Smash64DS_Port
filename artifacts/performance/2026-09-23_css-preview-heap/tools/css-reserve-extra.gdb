break ndsR2AnimCacheReserveCSSWorkingSet
commands
silent
printf "CSSRESERVE free=%u ptr=0x%x end=0x%x start=0x%x fail=%u\n", (unsigned)gSYTaskmanGeneralHeap.end - (unsigned)gSYTaskmanGeneralHeap.ptr, (unsigned)gSYTaskmanGeneralHeap.ptr, (unsigned)gSYTaskmanGeneralHeap.end, (unsigned)gSYTaskmanGeneralHeap.start, gNdsR2AnimCacheArenaReserveFailCount
continue
end

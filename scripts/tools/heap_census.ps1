param([string]$Tag = 'census', [string]$Build = 'build-sweep-s61', [int]$Slot = 11,
      [string]$Kinds = '117899270', [string]$Content = '236128516', [string]$Gkind = '43', [int]$Until = 1800,
      [switch]$NoMal)
# One four-CPU match on a gate ROM: every general-heap allocation (size,
# caller, heap left, frame), the reloc files the battle ensures and the stage
# program's load, then at frame $Until the heap low-water, the motion cache's
# and the lent dead spans' counters. Defaults: mix A on Hyrule Temple.
# Record: builds/probes/<Tag>.log ("MAL"/"ENS"/"SGX"/"END" lines).
. "$PSScriptRoot\common.ps1"
$persist = @()
if (-not $NoMal) {
    $persist += 'dprintf syTaskmanMalloc,"MAL %u lr=%#x free=%d f=%u\n", $r0, $lr, (int)((char *)gSYTaskmanGeneralHeap.end - (char *)gSYTaskmanGeneralHeap.ptr), gNdsBattlePlayablePacingPresentedFrames'
    $persist += 'dprintf ndsRelocEnsureLoadedAsset,"ENS id=%#x f=%u\n", $r0, gNdsBattlePlayablePacingPresentedFrames'
}
$persist += 'dprintf ndsStageGxLoad,"SGX f=%u free=%u\n", gNdsBattlePlayablePacingPresentedFrames, (unsigned)gSYTaskmanGeneralHeap.end - (unsigned)gSYTaskmanGeneralHeap.ptr'
$end = @(
    'printf "END1 heapmin=%u free=%u sgx=%u reason=%u\n", gNdsTaskmanGeneralHeapFreeMin, (unsigned)gSYTaskmanGeneralHeap.end - (unsigned)gSYTaskmanGeneralHeap.ptr, gNdsStageProgLoads, gNdsStageProgReason',
    'printf "END2 hits=%u misses=%u zc=%u\n", gNdsAnimCacheHits, gNdsAnimCacheMisses, gNdsAnimZeroCopyHits',
    'printf "END4 arena=%u used=%u\n", gNdsAnimCacheArenaReservedBytes, gNdsAnimCacheArenaUsedBytes',
    'printf "END5 stagerej=%u first=%u\n", gNdsRendererStageOwnerRejectCount, gNdsRendererStageOwnerFirstRejectReason',
    'printf "END6 lent=%u served=%u count=%u layers=%u skips=%u\n", gNdsBattleDeadSpanLentBytes, gNdsBattleDeadSpanServedBytes, gNdsBattleDeadSpanServedCount, gNdsNativeStageLayersLent, gNdsNativeStageLentLayerSkips'
)
$plan = @($end | ForEach-Object { "$Until|gdb=$_" })
& "$PSScriptRoot\capture_battle.ps1" -Slot $Slot -Tag $Tag -Build $Build -Kinds $Kinds -Content $Content -Gkind $Gkind `
    -Sets @('gNdsLabForceInputSlots=0') -Plan $plan -Persist $persist -TimeoutSeconds 2400 | Out-Null
Get-Content (Join-Path $S64Probes "$Tag.log") | Select-String '^END|^SGX|No symbol|no member' | ForEach-Object { $_.Line }
"DONE $Tag"

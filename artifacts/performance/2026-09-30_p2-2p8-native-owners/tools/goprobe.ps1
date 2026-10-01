param(
    [int]$Slot = 3,
    [Parameter(Mandatory)][string]$Tag,
    [string]$Build = 'build-lab-baked5',
    [string]$Gkind = '1',
    [string]$Kinds = '',
    [int]$Force = 0
)
# Free heap at the first GO frame and the match low-water (post-GO drop).
$S = 'C:\Users\Tyler\AppData\Local\Temp\claude\D--Stuff-DevFolder-Smash64DS-Port\4aa2b87d-ceb4-40ba-b9bb-cfd56e625cd7\scratchpad'
$boot = @("set var gNdsLabFourCpuGkind = $Gkind")
if ($Kinds -ne '') { $boot += "set var gNdsLabFourCpuKinds = $Kinds" }
if ($Force -ne 0) {
    $boot += 'set var gNdsLabItemToggles = 524288'
    $boot += 'set var gNdsLabItemRate = 5'
    $boot += "set var dITManagerForceMonsterKind = $Force"
}
$watch = @(
    'set $go = 0',
    'break ndsBattlePlayableFrameCompleteMarker if $go == 0 && gSCManagerBattleState->game_status == 1',
    'commands',
    '  silent',
    '  printf "GO f=%u free=%u\n", gNdsBattlePlayablePacingPresentedFrames, (unsigned)gSYTaskmanGeneralHeap.end - (unsigned)gSYTaskmanGeneralHeap.ptr',
    '  set $go = 1',
    '  continue',
    'end')
$end = @(
    'printf "LOW=%u\n", gNdsTaskmanGeneralHeapFreeMin',
    'printf "FREE=%u\n", (unsigned)gSYTaskmanGeneralHeap.end - (unsigned)gSYTaskmanGeneralHeap.ptr')
& "$S\gprobe.ps1" -Slot $Slot -Build $Build -Tag "go-$Tag" -Boot $boot -Watch $watch -EndFrame 1950 -AtEnd $end -TimeoutSeconds 900 |
    Select-String -Pattern 'GO f=|LOW=|FREE=|END' | ForEach-Object { $_.Line } | Out-File "$S\go-$Tag.txt"

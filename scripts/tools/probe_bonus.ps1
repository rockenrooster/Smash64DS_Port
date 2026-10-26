param([string]$Build = 'walk-u16', [string]$Tag = 'bp', [int]$Content = 0,
      [int]$Portrait = 255, [int]$Downs = 2, [int]$Seconds = 150, [int]$Every = 0,
      [int]$Slot = 11, [switch]$FailBt, [string[]]$More = @(),
      [string]$Image = '', [switch]$Fresh)
# One bonus practice run on the walk ROM (route 1, the 1P menu's tab $Downs:
# 2 Break the Targets, 3 Board the Platforms) through the native bonus select:
# per-scene heap low-water, the board kind, native failures, halts, the select
# captured at its commit. Record: builds/probes/<Tag>.log ("X " lines).
. "$PSScriptRoot\common.ps1"
$shot = Join-Path $S64Shots "$Tag-select.png"
$bp = @('break ndsSceneManagerEnter', 'commands', 'silent',
  'printf "X enter scene=%u prev=%u prevHeapLow=%u loan=%u native=%u dom=%u id=%x st=%u reason=%u bfk=%u pc=%u pm=%u board=%u ev32=%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsTaskmanGeneralHeapFreeMin, gNdsFrontendOverlayLoanBytes, gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.domain, gNdsRendererNativeFailure.identity, gNdsRendererNativeFailure.status, gNdsRendererNativeFailure.reason, gSCManagerSceneData.bonus_fkind, gNdsP7BonusPracticeContent, gNdsP7BonusPracticeMetal, gNdsP7BonusBoardKind, gNdsAObjEvent32NormalizeFailCount',
  'set var gNdsTaskmanGeneralHeapFreeMin = 0xffffffff',
  'continue', 'end',
  'break ndsMenuShellCssBonusCommit', 'commands', 'silent',
  'printf "X bcommit sel=%u selected=%u kind=%u total=%x rec=%x\n", sCssFkind[0], sCssSelected[0], sCssBonusKind, sCssBonusTotalKey, sCssBonusRecordShown',
  "shell pwsh -NoProfile -ExecutionPolicy Bypass -File `"$PSScriptRoot\capture_slot.ps1`" -Slot $Slot -Out `"$shot`"",
  'continue', 'end',
  'break __arm_excpt_dabt', 'commands', 'printf "X DABT lr-8=%x\n", $lr-8', 'bt 8', 'end',
  'break ndsSyMallocOverflowHalt', 'commands', 'printf "X OOM req=%u head=%u\n", gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom', 'bt 12', 'end')
$bp += $More
if ($FailBt) {
    $bp += @('break ndsRendererRecordNativeFailure', 'commands', 'silent',
      'printf "X FAIL dom=%u scene=%u id=%x st=%u root=%u mat=%x reason=%u\n", domain, scene, identity, status, root, material, reason',
      'bt 10', 'disable $bpnum', 'continue', 'end')
}
$pokes = @('set var gNdsMenuShellWalkRoute = 1', "set var gNdsMenuShellWalk1PModeDowns = $Downs",
  "set var gNdsMenuShellWalk1PPortrait = $Portrait",
  'set $a = (unsigned)&gNdsP4OnePlayerContent', 'set $w = *(unsigned*)($a & ~3)', 'set $sh = ($a & 3) * 8',
  "set var *(unsigned*)(`$a & ~3) = (`$w & ~(0xff << `$sh)) | ($Content << `$sh)")
& "$PSScriptRoot\probe_session.ps1" -Build $Build -Slot $Slot -Tag $Tag -Seconds $Seconds -Every $Every `
    -Pokes $pokes -Lines $bp -Image $Image -Fresh:$Fresh

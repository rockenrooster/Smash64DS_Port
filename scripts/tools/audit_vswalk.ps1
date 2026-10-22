param([string]$Build = 'walk-u16', [int]$Slot = 9, [int]$Seconds = 1500, [string]$Tag = 'avs', [int]$Laps = 4,
      [string]$Out = '')
# Stability audit: the walk's VS route (Mode Select, VS Mode, the character
# select's tour of the twelve with their 3D previews, the random chooser, the
# stage select, the match, Results, back to the character select) for $Laps
# laps: every scene entry with the scene's heap low-water, the tour's masks
# (kinds parked on, kinds that drew), native failures, halts. Record:
# builds/probes/<Tag>.log; a DONE line in $Out (default
# builds/probes/audit-vswalk.txt).
. "$PSScriptRoot\common.ps1"
if ($Out -eq '') { $Out = Join-Path $S64Probes 'audit-vswalk.txt' }
$lines = @(
  'break ndsSceneManagerEnter', 'commands', 'silent',
  'printf "X enter scene=%u prev=%u prevHeapLow=%u native=%u lean=%u ev32fail=%u gkind=%u fk=%u,%u,%u,%u c=%u,%u,%u,%u tour=%x/%x/%u/%u laps=%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsTaskmanGeneralHeapFreeMin, gNdsRendererNativeFailure.count, gNdsFtrLeanDeclineCount, gNdsAObjEvent32NormalizeFailCount, gSCManagerSceneData.gkind, gSCManagerTransferBattleState.players[0].fkind, gSCManagerTransferBattleState.players[1].fkind, gSCManagerTransferBattleState.players[2].fkind, gSCManagerTransferBattleState.players[3].fkind, gNdsP4PlayerContent[0], gNdsP4PlayerContent[1], gNdsP4PlayerContent[2], gNdsP4PlayerContent[3], gNdsMenuShellCssWalkTourKindMask, gNdsMenuShellCssWalkTourDrewMask, gNdsMenuShellCssWalkTourDoneCount, gNdsMenuShellCssWalkTourNotReadyCount, gNdsMenuShellWalkLoops',
  'set var gNdsTaskmanGeneralHeapFreeMin = 0xffffffff',
  'continue', 'end',
  'break __arm_excpt_dabt', 'commands', 'printf "X DABT lr-8=%x\n", $lr-8', 'bt 8', 'end',
  'break ndsSyMallocOverflowHalt', 'commands', 'printf "X OOM req=%u head=%u\n", gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom', 'bt 12', 'end',
  'break ndsRendererRecordNativeFailure', 'commands', 'silent',
  'printf "X FAIL dom=%u scene=%u id=%x st=%u root=%u mat=%x reason=%u\n", domain, scene, identity, status, root, material, reason',
  'bt 10', 'disable $bpnum', 'continue', 'end')
& "$PSScriptRoot\probe_session.ps1" -Build $Build -Slot $Slot -Tag $Tag -Seconds $Seconds -Every 120 `
    -Pokes @("set var gNdsMenuShellWalkBudget = $Laps") -Lines $lines | Out-Null
"VSWALK $Tag DONE" | Add-Content -LiteralPath $Out

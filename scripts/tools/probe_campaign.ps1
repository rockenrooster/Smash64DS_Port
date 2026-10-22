param([string]$Build = 'walk-u16', [string]$Tag = 'c1', [int]$Content = 20,
      [int]$Portrait = 255, [int]$Seconds = 420, [int]$Every = 0, [int]$Slot = 11,
      [string]$WaitFile = '', [string]$Wait = '', [switch]$FailBt, [int]$SkipTo = 0,
      [int]$TimeLimit = 0, [int]$Level = -1, [int]$Stock = -1, [string[]]$More = @(),
      [string]$Image = '', [switch]$Fresh)
# One 1P campaign run on the walk ROM (route 1, through the native 1P select):
# per-scene heap low-water (reset at each scene entry, so each line reports
# the scene that just ended), the 1P loan, native failures, halts, and the
# select captured at its commit. -Portrait picks the original fighter
# (255: none), -Content a P4 content; -SkipTo starts the ladder at a stage;
# -TimeLimit, -Level, -Stock poke the select's rules. Record:
# builds/probes/<Tag>.log ("X " lines).
. "$PSScriptRoot\common.ps1"
$shot = Join-Path $S64Shots "$Tag-select.png"
$bp = @('break ndsSceneManagerEnter', 'commands', 'silent',
  'printf "X enter scene=%u stage=%u prevHeapLow=%u loan=%u used=%u spill=%u native=%u dom=%u id=%x st=%u root=%u reason=%u fkind=%u content=%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.spgame_stage, gNdsTaskmanGeneralHeapFreeMin, gNdsFrontendOverlayLoanBytes, gNdsFrontendOverlayUsedBytes, gNdsFrontendOverlaySpillBytes, gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.domain, gNdsRendererNativeFailure.identity, gNdsRendererNativeFailure.status, gNdsRendererNativeFailure.root, gNdsRendererNativeFailure.reason, gSCManagerSceneData.fkind, gNdsP4OnePlayerContent',
  'set var gNdsTaskmanGeneralHeapFreeMin = 0xffffffff',
  "if (gSCManagerSceneData.scene_curr == 14) && (`$skip == 0) && ($SkipTo != 0)",
  'set $sa = (unsigned)&gSCManagerSceneData.spgame_stage', 'set $sw = *(unsigned*)($sa & ~3)', 'set $ss = ($sa & 3) * 8',
  "set var *(unsigned*)(`$sa & ~3) = (`$sw & ~(0xff << `$ss)) | ($SkipTo << `$ss)",
  'set $skip = 1', 'printf "X skipped to stage %u\n", gSCManagerSceneData.spgame_stage', 'end',
  'continue', 'end',
  'break ndsMenuShellCssOnePCommit', 'commands', 'silent',
  "if ($TimeLimit != 0)", 'set $ta = (unsigned)&sCssOnePTime', 'set $tw = *(unsigned*)($ta & ~3)', 'set $ts = ($ta & 3) * 8',
  "set var *(unsigned*)(`$ta & ~3) = (`$tw & ~(0xff << `$ts)) | ($TimeLimit << `$ts)", 'end',
  'printf "X commit sel=%u selected=%u level=%u stock=%u time=%u arena=%u\n", sCssFkind[0], sCssSelected[0], sCssOnePLevel, sCssOnePStock, sCssOnePTime, gNdsTaskmanArenaChosenSize',
  "shell pwsh -NoProfile -ExecutionPolicy Bypass -File `"$PSScriptRoot\capture_slot.ps1`" -Slot $Slot -Out `"$shot`"",
  'continue', 'end',
  'break __arm_excpt_dabt', 'commands', 'printf "X DABT lr-8=%x\n", $lr-8', 'bt 8', 'end',
  'break ndsSyMallocOverflowHalt', 'commands', 'printf "X OOM req=%u head=%u\n", gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom', 'bt 12', 'end')
# Byte pokes go through word read-modify-writes (memory gdb-byte-poke-crashes-melonds).
foreach ($pair in @(@('sCssOnePLevel', $Level), @('sCssOnePStock', $Stock))) {
    if ($pair[1] -ge 0) {
        $at = $bp.IndexOf('break ndsMenuShellCssOnePCommit') + 2
        $bp = @($bp[0..$at]) +
              @("set `$la = (unsigned)&$($pair[0])", 'set $lw = *(unsigned*)($la & ~3)', 'set $ls = ($la & 3) * 8',
                "set var *(unsigned*)(`$la & ~3) = (`$lw & ~(0xff << `$ls)) | ($($pair[1]) << `$ls)") +
              @($bp[($at + 1)..($bp.Count - 1)])
    }
}
$bp += $More
if ($FailBt) {
    # The first native failure, once: who recorded it, and the direct-reject words.
    $bp += @('break ndsRendererRecordNativeFailure', 'commands', 'silent',
      'printf "X FAIL dom=%u scene=%u id=%x st=%u root=%u mat=%x reason=%u\n", domain, scene, identity, status, root, material, reason',
      'bt 10',
      'printf "X DIRECT n=%u site=%x om=%x cw0=%x cw1=%x env=%x geom=%x\n", gNdsRendererNativeDirectReject.count, gNdsRendererNativeDirectReject.site, gNdsRendererNativeDirectReject.othermode_l, gNdsRendererNativeDirectReject.combine_w0, gNdsRendererNativeDirectReject.combine_w1, gNdsRendererNativeDirectReject.env_color, gNdsRendererNativeDirectReject.geometry_mode',
      'disable $bpnum', 'continue', 'end')
}
$pokes = @('set $skip = 0', 'set var gNdsMenuShellWalkRoute = 1', 'set var gNdsMenuShellWalk1PModeDowns = 0',
  "set var gNdsMenuShellWalk1PPortrait = $Portrait",
  'set $a = (unsigned)&gNdsP4OnePlayerContent', 'set $w = *(unsigned*)($a & ~3)', 'set $sh = ($a & 3) * 8',
  "set var *(unsigned*)(`$a & ~3) = (`$w & ~(0xff << `$sh)) | ($Content << `$sh)")
& "$PSScriptRoot\probe_session.ps1" -Build $Build -Slot $Slot -Tag $Tag -Seconds $Seconds -Every $Every `
    -Pokes $pokes -Lines $bp -WaitFile $WaitFile -Wait $Wait -Image $Image -Fresh:$Fresh

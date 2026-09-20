[CmdletBinding()]
param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\\devkitPro\\devkitARM\\bin\\arm-none-eabi-gdb.exe',
    [ValidateRange(1, 12)][int]$RunnerSlot = 6,
    [ValidateRange(30, 300)][int]$TimeoutSeconds = 120,
    [string]$Build = 'build-p2-ness-vfx-realtime',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\\gdb-markers.ps1')

function Assert-UpBVfx([bool]$Condition, [string]$Message, [string]$Evidence = '') {
    if (-not $Condition) {
        if ($Evidence) { throw "$Message`n$Evidence" }
        throw $Message
    }
}

$target = $Target
$buildDir = Join-Path $root (Join-Path 'builds' $Build)
$rom = Join-Path $buildDir ($target + '.nds')
$elf = Join-Path $buildDir ($target + '.elf')
$config = Join-Path $buildDir 'nds_build_config.h'
foreach ($path in @($rom, $elf, $config)) {
    Assert-UpBVfx (Test-Path -LiteralPath $path -PathType Leaf) "Missing VFX proof input: $path"
}
$configText = Get-Content -LiteralPath $config -Raw
foreach ($definition in @(
    '#define NDS_P2_NESS 1',
    '#define NDS_P2_PROOF_FIGHTER0 11',
    '#define NDS_P2_NESS_VFX_PROOF 1'
)) {
    Assert-UpBVfx $configText.Contains($definition) "VFX proof build is missing: $definition"
}

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\\verification\\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_ness-upb-vfx.txt')
} elseif (-not [IO.Path]::IsPathRooted($Artifact)) {
    $Artifact = Join-Path $root $Artifact
}

$ctx = Initialize-MelonDSVerifierContext -Root $root -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot -NoBuild
$state = $null
$emu = $null
try {
    $state = Enable-MelonDSGdbConfig -MelonDSPath $ctx.MelonDSPath `
        -GdbPort $ctx.GdbPort -Persistent -MuteAudio
    $emu = Start-Process -FilePath $ctx.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path $ctx.MelonDSPath) `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emu -Port $ctx.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        'set $armed = 0',
        'set $fault = 0',
        'set $wave_calls = 0',
        'break __excpt_entry',
        'commands',
        'silent',
        'set $fault = 1',
        'printf "UPB_VFX_EXCEPTION pc=%#x lr=%#x\\n",$pc,$lr',
        'bt 12',
        'detach',
        'quit 2',
        'end',
        'break efManagerNessPKThunderWaveMakeEffect',
        'commands',
        'silent',
        'set $wave_calls = $wave_calls + 1',
        'continue',
        'end',
        'break ndsRendererSubmitNativeNessPKThunder',
        'commands',
        'silent',
        'if $armed != 0 && gNdsNessPKThunderNativeProofDraw[0] > 0 && gNdsNessPKThunderNativeProofDraw[1] > 0 && gNdsNessPKThunderNativeProofDraw[2] > 0',
        'printf "UPB_VFX wave=%u head=%u trail=%u headnative=%u trailnative=%u wavenative=%u wsubmit=%u wvisible=%u wtri=%u wready=%u wtexreject=%u wreject=%u esubmit=%u etri=%u eready=%u etexreject=%u ereject=%u eadmit=%u\\n",$wave_calls,gNdsNessSpecialTourPKThunderHeadObserved,gNdsNessSpecialTourPKThunderTrailObserved,gNdsNessPKThunderNativeProofDraw[0],gNdsNessPKThunderNativeProofDraw[1],gNdsNessPKThunderNativeProofDraw[2],gNdsWeaponRendererSubmitCount-$ws,gNdsWeaponRendererVisibleDrawCount-$wv,gNdsWeaponRendererTriangleCount-$wt,gNdsWeaponRendererTextureReadyCount-$wr,gNdsWeaponRendererTextureRejectCount-$wx,gNdsWeaponRendererRejectedDrawCount-$wj,gNdsEffectRendererSubmitCount-$es,gNdsEffectRendererTriangleCount-$et,gNdsEffectRendererTextureReadyCount-$er,gNdsEffectRendererTextureRejectCount-$ex,gNdsEffectRendererRejectedDrawCount-$ej,gNdsEffectRendererSourceModelAdmitCount-$ea',
        'detach',
        'quit',
        'end',
        'continue',
        'end',
        'break ftNessSpecialHiHoldSetStatus',
        'commands',
        'silent',
        'if $armed == 0',
        'set $ws = gNdsWeaponRendererSubmitCount',
        'set $wv = gNdsWeaponRendererVisibleDrawCount',
        'set $wt = gNdsWeaponRendererTriangleCount',
        'set $wr = gNdsWeaponRendererTextureReadyCount',
        'set $wx = gNdsWeaponRendererTextureRejectCount',
        'set $wj = gNdsWeaponRendererRejectedDrawCount',
        'set $es = gNdsEffectRendererSubmitCount',
        'set $et = gNdsEffectRendererTriangleCount',
        'set $er = gNdsEffectRendererTextureReadyCount',
        'set $ex = gNdsEffectRendererTextureRejectCount',
        'set $ej = gNdsEffectRendererRejectedDrawCount',
        'set $ea = gNdsEffectRendererSourceModelAdmitCount',
        'set $armed = 1',
        'end',
        'continue',
        'end',
        'break ftNessSpecialAirHiHoldSetStatus',
        'commands',
        'silent',
        'if $armed == 0',
        'set $ws = gNdsWeaponRendererSubmitCount',
        'set $wv = gNdsWeaponRendererVisibleDrawCount',
        'set $wt = gNdsWeaponRendererTriangleCount',
        'set $wr = gNdsWeaponRendererTextureReadyCount',
        'set $wx = gNdsWeaponRendererTextureRejectCount',
        'set $wj = gNdsWeaponRendererRejectedDrawCount',
        'set $es = gNdsEffectRendererSubmitCount',
        'set $et = gNdsEffectRendererTriangleCount',
        'set $er = gNdsEffectRendererTextureReadyCount',
        'set $ex = gNdsEffectRendererTextureRejectCount',
        'set $ej = gNdsEffectRendererRejectedDrawCount',
        'set $ea = gNdsEffectRendererSourceModelAdmitCount',
        'set $armed = 1',
        'end',
        'continue',
        'end',
        'break ftNessSpecialHiJibakuSetStatus',
        'commands',
        'silent',
        'if $armed != 0',
        'printf "UPB_VFX wave=%u head=%u trail=%u headnative=%u trailnative=%u wavenative=%u wsubmit=%u wvisible=%u wtri=%u wready=%u wtexreject=%u wreject=%u esubmit=%u etri=%u eready=%u etexreject=%u ereject=%u eadmit=%u\\n",$wave_calls,gNdsNessSpecialTourPKThunderHeadObserved,gNdsNessSpecialTourPKThunderTrailObserved,gNdsNessPKThunderNativeProofDraw[0],gNdsNessPKThunderNativeProofDraw[1],gNdsNessPKThunderNativeProofDraw[2],gNdsWeaponRendererSubmitCount-$ws,gNdsWeaponRendererVisibleDrawCount-$wv,gNdsWeaponRendererTriangleCount-$wt,gNdsWeaponRendererTextureReadyCount-$wr,gNdsWeaponRendererTextureRejectCount-$wx,gNdsWeaponRendererRejectedDrawCount-$wj,gNdsEffectRendererSubmitCount-$es,gNdsEffectRendererTriangleCount-$et,gNdsEffectRendererTextureReadyCount-$er,gNdsEffectRendererTextureRejectCount-$ex,gNdsEffectRendererRejectedDrawCount-$ej,gNdsEffectRendererSourceModelAdmitCount-$ea',
        'detach',
        'quit',
        'end',
        'continue',
        'end',
        'break ftNessSpecialAirHiJibakuSetStatus',
        'commands',
        'silent',
        'if $armed != 0',
        'printf "UPB_VFX wave=%u head=%u trail=%u headnative=%u trailnative=%u wavenative=%u wsubmit=%u wvisible=%u wtri=%u wready=%u wtexreject=%u wreject=%u esubmit=%u etri=%u eready=%u etexreject=%u ereject=%u eadmit=%u\\n",$wave_calls,gNdsNessSpecialTourPKThunderHeadObserved,gNdsNessSpecialTourPKThunderTrailObserved,gNdsNessPKThunderNativeProofDraw[0],gNdsNessPKThunderNativeProofDraw[1],gNdsNessPKThunderNativeProofDraw[2],gNdsWeaponRendererSubmitCount-$ws,gNdsWeaponRendererVisibleDrawCount-$wv,gNdsWeaponRendererTriangleCount-$wt,gNdsWeaponRendererTextureReadyCount-$wr,gNdsWeaponRendererTextureRejectCount-$wx,gNdsWeaponRendererRejectedDrawCount-$wj,gNdsEffectRendererSubmitCount-$es,gNdsEffectRendererTriangleCount-$et,gNdsEffectRendererTextureReadyCount-$er,gNdsEffectRendererTextureRejectCount-$ex,gNdsEffectRendererRejectedDrawCount-$ej,gNdsEffectRendererSourceModelAdmitCount-$ea',
        'detach',
        'quit',
        'end',
        'continue',
        'end',
        'continue'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'ness-upb-vfx.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null

    $outPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'ness-upb-vfx.gdb.out'
    $errPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'ness-upb-vfx.gdb.err'
    $stdout = Get-Content -LiteralPath $outPath -Raw
    $stderr = if (Test-Path $errPath) { Get-Content $errPath -Raw } else { '' }
    Assert-UpBVfx ($stdout -notmatch 'UPB_VFX_EXCEPTION') 'ARM exception during Up-B VFX proof.' ($stdout + $stderr)
    $match = [regex]::Match($stdout, 'UPB_VFX wave=(\d+) head=(\d+) trail=(\d+) headnative=(\d+) trailnative=(\d+) wavenative=(\d+) wsubmit=(\d+) wvisible=(\d+) wtri=(\d+) wready=(\d+) wtexreject=(\d+) wreject=(\d+) esubmit=(\d+) etri=(\d+) eready=(\d+) etexreject=(\d+) ereject=(\d+) eadmit=(\d+)')
    Assert-UpBVfx $match.Success 'Up-B VFX render marker is missing.' ($stdout + $stderr)
    $v = 1..18 | ForEach-Object { [uint32]$match.Groups[$_].Value }
    Assert-UpBVfx (($v[0] -gt 0) -and ($v[1] -eq 1) -and ($v[2] -eq 1)) `
        'PK Thunder source wave/head/trail did not all engage.' $match.Value
    Assert-UpBVfx (($v[3] -gt 0) -and ($v[4] -gt 0) -and ($v[5] -gt 0)) `
        'PK Thunder head, trail, and Wave did not each emit native triangles.' $match.Value
    Assert-UpBVfx (($v[6] -gt 0) -and ($v[7] -gt 0) -and ($v[8] -gt 0) -and `
        ($v[10] -eq 0) -and ($v[11] -eq 0)) `
        'PK Thunder weapon head/trail did not produce clean visible textured triangles.' $match.Value
    Assert-UpBVfx (($v[12] -gt 0) -and ($v[13] -gt 0) -and `
        ($v[15] -eq 0) -and ($v[17] -gt 0)) `
        'PK Thunder Wave did not produce clean source-model effect triangles.' $match.Value

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    @(
        'NESS_UPB_VFX=PASS',
        "ROM_SHA256=$((Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash)",
        "BUILD=$Build",
        $match.Value
    ) | Set-Content -LiteralPath $Artifact
    Get-Content -LiteralPath $Artifact
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}

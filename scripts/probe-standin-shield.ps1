[CmdletBinding()]
param(
    [string]$Build = 'build-c61-flag0',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    # The timeout is the normal exit: the last gdb command is an unbounded
    # `continue` and the hit cap is only reached if the defect ISN'T there. Size
    # this for the match (boot plus one minute plus GAME SET), not for the cap.
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 240,
    [string]$Artifact = ''
)

# WHY DID THE FLAG-0 STAND-IN SHIELD DRAW ON EXACTLY ONE FRAME?
#
# Measured twice, before and after the cycle-50 ejection fix and unchanged by it:
# gNdsTask39FxShieldDrawCount totals 1 for a whole match, at tic 3264
# (BUGS.md, "FLAG-0 RE-MEASURED AFTER THE FIX"). The fix is inert at flag 0 --
# gNdsEFManagerSourceEffectStopCount reads 0 there, which is the negative
# control -- so this is its own defect on the stand-in path and it has never been
# localized.
#
# ONE DRAW IS TWO DIFFERENT BUGS AND NOBODY HAS SEPARATED THEM. Either the CPU
# guarded once, in which case one draw per guard is the real number and the
# defect is a one-frame LIFETIME; or it guarded several times and only the first
# guard drew at all. Counting the maker is what tells them apart, and it costs
# the same run.
#
# Source says the shield must outlive its first frame. ftCommonGuardOnSetStatus
# passes FTSTATUS_PRESERVE_NONE and then makes the shield
# (ftcommonguard1.c:419,430), and the very next transition, GuardOn -> Guard,
# passes FTSTATUS_PRESERVE_MODELPART | PRESERVE_HITSTATUS | PRESERVE_EFFECT
# (:495) precisely so the bubble survives it. At flag 1 that is measurably what
# happens -- the source shield draws every frame from its spawn to the end of
# the guard. So a one-frame stand-in is not source behaviour.
#
# The port's stand-in also cannot expire on its own: ndsEFManagerVisualProcUpdate
# RETURNS EARLY for any effect with a non-NULL ep->fighter_gobj
# (battleship_efmanager.c:912-955), before the anim_frame decrement that drives
# ndsEFManagerDestroyVisualEffect. An attached stand-in therefore lives until
# something ejects it, which makes "who ejected it" a question with a specific
# answer rather than a lifetime table lookup.
#
# Three breakpoints, three counters, one run:
#   1 efManagerShieldMakeEffect     -- one hit per guard-on. The guard count.
#   2 ndsEFManagerShieldProcDisplay -- one hit per drawn frame. The draw count.
#   3 ndsEFManagerDestroyVisualEffect, conditioned on the effect's own recorded
#     kind being nNDSVisualEffectShield (= 8, effect_vars.common.size, written
#     at creation) -- the eject, with the tic that dates it against 1 and 2.
#
# Only registers at function entries and file-scope globals are read; a stack
# object is not readable through this stub (BUGS.md, cycle 55). Addresses come
# from nm because gdb's name lookup resolves through debug info that still
# describes gc-sectioned NDS_WEAK twins, and efManagerShieldMakeEffect has
# exactly such a twin -- at flag 0 the weak shim IS the live definition and the
# strong one in battleship_efmanager.c is preprocessed away, so the name is
# ambiguous in debug info in both directions.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_standin-shield-probe.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.standin-shield-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.standin-shield-probe.stderr.log'
$config_state = $null
$emulator = $null

$required = @(
    'efManagerShieldMakeEffect',
    'gSCManagerBattleState',
    'gNdsTask39FxShieldDrawCount',
    'gNdsVisualEffectCreateCount',
    'gNdsVisualEffectDestroyCount',
    'gNdsVisualEffectActiveCount',
    'gNdsVisualEffectDropCount',
    'gNdsEFManagerSourceEffectStopCount'
)
$nmLines = & $nm $elf
$symbols = $nmLines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw ("Stand-in shield probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}
function Get-TextAddress {
    param([string]$Name, [switch]$Optional)
    # [TtWw], not [Tt]. efManagerShieldMakeEffect is `W` on the flag-0 ELF: the
    # strong definition lives inside NDS_R2_SOURCE_EFFECTS_FULL, so at the
    # tracked default the NDS_WEAK shim is the ONLY and therefore the LIVE
    # definition. A [Tt]-only lookup throws "has no text symbol" on the one
    # configuration this probe exists to measure.
    $line = $nmLines |
        Where-Object { $_ -match ('^([0-9a-fA-F]{8})\s+[TtWw]\s+' + [regex]::Escape($Name) + '$') } |
        Select-Object -First 1
    if ($null -eq $line) {
        if ($Optional) { return $null }
        throw "$Name has no text symbol in $elf."
    }
    return '0x' + ($line -split '\s+')[0]
}
# The two draw/destroy helpers are `static`, so they are 't' rather than 'T' and
# an -O2 build may have inlined them out of existence. That is itself a result
# worth naming rather than a crash: say which one vanished.
$makeAddress = Get-TextAddress 'efManagerShieldMakeEffect'
$drawAddress = Get-TextAddress 'ndsEFManagerShieldProcDisplay' -Optional
$destroyAddress = Get-TextAddress 'ndsEFManagerDestroyVisualEffect' -Optional
if ($null -eq $drawAddress) {
    Write-Warning 'ndsEFManagerShieldProcDisplay has no text symbol (inlined); draw hits will be absent.'
}
if ($null -eq $destroyAddress) {
    Write-Warning 'ndsEFManagerDestroyVisualEffect has no text symbol (inlined); eject hits will be absent.'
}

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $counters = 'gNdsTask39FxShieldDrawCount, gNdsVisualEffectCreateCount, ' +
        'gNdsVisualEffectDestroyCount, gNdsVisualEffectActiveCount, ' +
        'gNdsVisualEffectDropCount, gNdsEFManagerSourceEffectStopCount'
    $countersFmt = 'draw=%u create=%u destroy=%u active=%u drop=%u srcstop=%u'

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $hits = 0',
        ("break *{0}" -f $makeAddress)
    )
    if ($null -ne $drawAddress) { $commands += ("break *{0}" -f $drawAddress) }
    if ($null -ne $destroyAddress) {
        $commands += ("break *{0}" -f $destroyAddress)
        # nNDSVisualEffectShield == 8. The kind is recorded on the EFStruct at
        # creation (battleship_efmanager.c:1073), so this is the effect's own
        # statement of what it is rather than an inference from its display list.
        $commands += ("condition {0} (((GObj *)`$r0)->user_data.p != 0) && " +
            "(((EFStruct *)((GObj *)`$r0)->user_data.p)->effect_vars.common.size == 8)") -f
            (2 + [int]($null -ne $drawAddress))
    }
    # A breakpoint at 0x0 is what a gc-sectioned weak twin looks like, and
    # without this its only symptom is an unexplained timeout.
    $commands += 'info breakpoints'

    $commands += @(
        'commands 1',
        'silent',
        'set $hits = $hits + 1',
        ('printf "SHIELDMAKE n=%d tr=%d ' + $countersFmt + '\n", $hits, gSCManagerBattleState->time_remain, ' + $counters),
        'if $hits < 60',
        'continue',
        'end',
        'end'
    )
    if ($null -ne $drawAddress) {
        $commands += @(
            'commands 2',
            'silent',
            'set $hits = $hits + 1',
            ('printf "SHIELDDRAW n=%d tr=%d ' + $countersFmt + '\n", $hits, gSCManagerBattleState->time_remain, ' + $counters),
            'if $hits < 60',
            'continue',
            'end',
            'end'
        )
    }
    if ($null -ne $destroyAddress) {
        $index = 2 + [int]($null -ne $drawAddress)
        $commands += @(
            ("commands {0}" -f $index),
            'silent',
            'set $hits = $hits + 1',
            ('printf "SHIELDKILL n=%d tr=%d ' + $countersFmt + '\n", $hits, gSCManagerBattleState->time_remain, ' + $counters),
            'if $hits < 60',
            'continue',
            'end',
            'end'
        )
    }
    $commands += @(
        'continue',
        ('printf "SHIELDDONE hits=%d tr=%d ' + $countersFmt + '\n", $hits, gSCManagerBattleState->time_remain, ' + $counters),
        'detach',
        'quit'
    )

    $text = $null
    try {
        $capture = Invoke-GdbMarkerScript `
            -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
            -ScriptName 'standin_shield_probe.gdb' `
            -TimeoutSeconds $TimeoutSeconds
        $text = $capture.Stdout
    }
    catch {
        $text = $_.Exception.Message
        Write-Warning ('gdb run ended without reaching the hit cap; ' +
            'reading the streamed output out of the exception.')
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    Set-Content -LiteralPath $Artifact -Value $text
    $text -split "`n" |
        Where-Object { $_ -match 'SHIELD(MAKE|DRAW|KILL|DONE)' } |
        ForEach-Object { Write-Output $_ }
    Write-Output "probe capture: $Artifact"
}
finally {
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}

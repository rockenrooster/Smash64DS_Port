[CmdletBinding()]
param(
    [string]$Build = 'build-c60-flag1',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    # A one-minute match plus boot and load runs in about 100 s of wall clock,
    # and this probe's last command is an unbounded `continue`, so it only
    # returns early if the hit cap is reached. A KO-only probe will not reach a
    # large cap -- there are a handful of KOs in a match, not forty -- so the
    # TIMEOUT is the normal exit and the budget must be sized for the match, not
    # for the cap. 900 s here meant fourteen minutes of waiting for evidence that
    # was complete after two.
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 240,
    # OFF BY DEFAULT, AND THAT IS THE POINT. lbParticleMakeScriptID is called for
    # every particle in the game, several times a frame, and a gdb CONDITION is
    # evaluated on the HOST -- one stub round trip per call. Breaking there with
    # a script-id condition slowed the guest so far that a 900-second run did not
    # reach the first KO, which is the same failure BUG_FIXING_PROCESS records
    # for probe-vfx-contracts.ps1's per-frame callback. The two makers below fire
    # only on a KO and cost nothing, so the default configuration answers "was
    # the maker reached, and at which tic" without this. Turn it on only when the
    # maker is known to run and the question is specifically whether its particle
    # half is refused.
    [switch]$IncludeParticleBreak,
    [string]$Artifact = ''
)

# WHEN DOES THE KO BLAST FIRE, AND WHICH OF ITS TWO HALVES IS REACHED?
#
# The KO row is the last of the effect cluster and it has never had a TIC. Every
# other row in the cluster got one from a maker-conditioned probe and became
# capturable that same cycle (BUGS.md, "EFFECT MOMENTS ARE NOW KNOWN"); this one
# was left out because probe-ko-vfx.ps1 targets the tickhud ROM and a build that
# predates the whole source-effect campaign.
#
# The source oracle says the KO blast is TWO effects from one call, not one:
#   * a PARTICLE, lbParticleMakeScriptID(... , dEFManagerDeadExplodeGenID[index])
#     where index = (type % 2) * 4 + player, so the script is one of
#     0x2D 0x2C 0x2B 0x2A (bottom/top KO) or 0x3F 0x3E 0x3D 0x3C (side KO)
#     -- per player, and rotated by dEFManagerDeadExplodeRotateD[type] so it
#     points inward from the blast zone it crossed (efmanager.c:4776).
#   * an EFDESC MODEL, dEFManagerDeadExplodeEffectDesc, flags
#     EFFECT_FLAG_SPECIALLINK | 0x4 on DL LINK 18 with both transforms plain
#     nGCMatrixKindTraRotRpyRSca (efmanager.c:850), whose per-player colour is
#     written into the child and sibling MObj envcolor after creation.
# Script 0x5C is the separate Star-KO sparkle from efManagerSparkleWhiteDeadMake
# Effect, driven by ftCommonDeadUpStarProcUpdate (ftcommondead.c:357).
#
# So "the blast pillar is not drawing" has at least three distinguishable
# answers, and this probe separates them before anything is theorised: the maker
# never runs, the particle half is refused, or both are made and the pixels are
# the problem. The particle counters are read at the maker rather than inside
# lbParticle so that a refusal shows as a delta across the call.
#
# Everything printed is a register at a FUNCTION ENTRY (where the ABI pins
# r0..r3) or a file-scope global. No stack object is read -- through this stub a
# stack read returns garbage that looks like data (BUGS.md, cycle 55), and
# ftCommonDeadDownSetStatus passes its Vec3f by pointer to a local, so `*pos` is
# exactly that trap. The effect's world position is therefore NOT read here; the
# capture at the tic this probe reports is what answers position.
#
# Addresses come from nm, never from gdb's name lookup: several efManager makers
# have an NDS_WEAK twin in battle_playable_compat_stubs.c that debug info still
# describes after --gc-sections drops it, and breaking on the name silently
# yields a breakpoint at 0x0 whose only symptom is a timeout.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_ko-blast-probe.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.ko-blast-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.ko-blast-probe.stderr.log'
$config_state = $null
$emulator = $null

# One absent symbol aborts every later command in a gdb batch, so the whole read
# set is checked and not only the breakpoint targets.
$required = @(
    'efManagerDeadExplodeMakeEffect',
    'efManagerSparkleWhiteDeadMakeEffect',
    'lbParticleMakeScriptID',
    'gSCManagerBattleState',
    'gNdsParticleQuadEmitCount',
    'gNdsParticleQuadMissCount',
    'gNdsParticleRejectCount',
    'gNdsParticleDrawVisibleCount',
    'gNdsParticleScriptStartCount',
    'gNdsEffectRendererDObjDrawCount',
    'gNdsEffectRendererRejectedDrawCount',
    'gNdsEffectRendererTriangleCount',
    'gNdsEffectRendererTextureReadyCount'
)
$nmLines = & $nm $elf
$symbols = $nmLines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw ("KO blast probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}
function Get-TextAddress {
    param([string]$Name)
    $line = $nmLines |
        Where-Object { $_ -match ('^([0-9a-fA-F]{8})\s+[Tt]\s+' + [regex]::Escape($Name) + '$') } |
        Select-Object -First 1
    if ($null -eq $line) { throw "$Name has no text symbol in $elf." }
    return '0x' + ($line -split '\s+')[0]
}
$explodeAddress = Get-TextAddress 'efManagerDeadExplodeMakeEffect'
$sparkleAddress = Get-TextAddress 'efManagerSparkleWhiteDeadMakeEffect'
$particleAddress = Get-TextAddress 'lbParticleMakeScriptID'

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

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        'set $hits = 0',

        ("break *{0}" -f $explodeAddress),
        ("break *{0}" -f $sparkleAddress)
    )
    if ($IncludeParticleBreak) {
        # r1 is the script id at lbParticleMakeScriptID's entry. The condition
        # restricts the STOP to the nine KO scripts, but gdb still round-trips to
        # the host on every one of the hundreds of other calls per second -- read
        # the parameter's own note before turning this on.
        $commands += ("break *{0}" -f $particleAddress)
        $commands += 'condition 3 ($r1 == 0x2D) || ($r1 == 0x2C) || ($r1 == 0x2B) || ($r1 == 0x2A) || ($r1 == 0x3F) || ($r1 == 0x3E) || ($r1 == 0x3D) || ($r1 == 0x3C) || ($r1 == 0x5C)'
    }
    # A breakpoint at 0x0 is what a gc-sectioned weak twin looks like and its
    # only other symptom is the whole run timing out with no explanation.
    $commands += 'info breakpoints'

    $commands += @(
        'commands 1',
        'silent',
        'set $hits = $hits + 1',
        'printf "KOEXPLODE n=%d tr=%d player=%d type=%d emit=%u miss=%u prej=%u vis=%u start=%u draw=%u erej=%u tris=%u texready=%u\n", $hits, gSCManagerBattleState->time_remain, $r1, $r2, gNdsParticleQuadEmitCount, gNdsParticleQuadMissCount, gNdsParticleRejectCount, gNdsParticleDrawVisibleCount, gNdsParticleScriptStartCount, gNdsEffectRendererDObjDrawCount, gNdsEffectRendererRejectedDrawCount, gNdsEffectRendererTriangleCount, gNdsEffectRendererTextureReadyCount',
        'if $hits < 40',
        'continue',
        'end',
        'end',

        'commands 2',
        'silent',
        'set $hits = $hits + 1',
        'printf "KOSTAR n=%d tr=%d emit=%u miss=%u prej=%u vis=%u start=%u\n", $hits, gSCManagerBattleState->time_remain, gNdsParticleQuadEmitCount, gNdsParticleQuadMissCount, gNdsParticleRejectCount, gNdsParticleDrawVisibleCount, gNdsParticleScriptStartCount',
        'if $hits < 40',
        'continue',
        'end',
        'end'
    )
    if ($IncludeParticleBreak) {
        $commands += @(
            'commands 3',
            'silent',
            'set $hits = $hits + 1',
            'printf "KOPART n=%d tr=%d script=0x%x emit=%u miss=%u prej=%u vis=%u start=%u\n", $hits, gSCManagerBattleState->time_remain, $r1, gNdsParticleQuadEmitCount, gNdsParticleQuadMissCount, gNdsParticleRejectCount, gNdsParticleDrawVisibleCount, gNdsParticleScriptStartCount',
            'if $hits < 40',
            'continue',
            'end',
            'end'
        )
    }

    $commands += @(
        'continue',
        'printf "KODONE hits=%d tr=%d emit=%u miss=%u prej=%u vis=%u start=%u\n", $hits, gSCManagerBattleState->time_remain, gNdsParticleQuadEmitCount, gNdsParticleQuadMissCount, gNdsParticleRejectCount, gNdsParticleDrawVisibleCount, gNdsParticleScriptStartCount',
        'detach',
        'quit'
    )

    # THE TIMEOUT IS THIS PROBE'S NORMAL EXIT, so the artifact is written from
    # the exception too. Invoke-GdbMarkerScript throws on timeout and the
    # streamed breakpoint output is inside the message; a `finally` that only
    # kills the emulator therefore threw away a complete capture. Same shape as
    # the abort-read lesson in BUG_FIXING_PROCESS.
    $text = $null
    try {
        $capture = Invoke-GdbMarkerScript `
            -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
            -ScriptName 'ko_blast_probe.gdb' `
            -TimeoutSeconds $TimeoutSeconds
        $text = $capture.Stdout
    }
    catch {
        $text = $_.Exception.Message
        Write-Warning ("gdb run ended without reaching the hit cap; " +
            "reading the streamed output out of the exception.")
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    Set-Content -LiteralPath $Artifact -Value $text
    $text -split "`n" |
        Where-Object { $_ -match 'KO(EXPLODE|STAR|PART|DONE)' } |
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

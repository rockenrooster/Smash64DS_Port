param(
    [Parameter(Mandatory = $true)][string]$Rom,
    [Parameter(Mandatory = $true)][string]$Elf,
    [Parameter(Mandatory = $true)][string]$Tag,
    [int]$RunnerSlot = 9,
    # Boot pokes (at `main`, before any scene): -1 leaves the ROM's default.
    [int]$Route = -1,
    [int]$Admit = -1,
    # More boot pokes, "global=value" (e.g. the walk ROM's
    # gNdsMenuShellCssWalkTargetKind2=11 for a CPU Ness).
    [string[]]$BootSet = @(),
    # Results is photographed at these SOURCE tics (sMNVSResultsTotalTimeTics).
    [int[]]$ResultsTics = @(200),
    # The character select after Results: photographed this many presents
    # after the screen opens, each (cumulative).
    [int[]]$CssPresents = @(150, 300),
    # A no-walk ROM needs input: a gdb snippet (keysHeld feed) that drives
    # Title -> VS -> CSS -> SSS -> battle and presses START on Results. Its
    # lines are inserted before the first `continue`. Empty for a walk ROM.
    [string]$FeedFile = '',
    # Dump every live dynamic texture-cache slot at each Results / CSS stop.
    [switch]$DumpTex,
    # Main-RAM addresses (hex strings) whose first 32 bytes are printed at
    # each Results stop.
    [string[]]$DumpMem = @(),
    # Functions to stop at (in order) between battle start and the Results
    # tics: counters, a backtrace and a capture at each.
    [string[]]$StopAt = @(),
    # After each Results capture, a second capture with 3D texturing off.
    [switch]$UntexturedShot,
    # "name:start:end" memory regions dumped (binary) at each Results stop, to
    # $env:SMASH64DS_VERIFY_TEMP_DIR/<tag>-<name>-t<tic>.bin.
    [string[]]$DumpRegions = @(),
    [int]$TimeoutSeconds = 900
)
# P2-2p8 Phase 1 slice 7 regression probe (owner playtest, 2026-09-24): VS
# battle -> Results -> character select, with window captures of Results and
# of the character select it returns to, and the render counters at each stop.
# The same ROM is run with the lean words at their default and poked to 0 at
# boot (the old path everywhere). Captures land in
# artifacts/visibility/2026-09-24_p2-2p8-phase1-slice7-results/.
$ErrorActionPreference = 'Stop'
$root = 'D:/Stuff/DevFolder/Smash64DS_Port'
$shots = "$root/artifacts/visibility/2026-09-24_p2-2p8-phase1-slice7-results"
New-Item -ItemType Directory -Force -Path $shots | Out-Null
. "$root/scripts/lib/melonds.ps1"
. "$root/scripts/lib/gdb-markers.ps1"
$context = Initialize-MelonDSVerifierContext -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$emulator = $null; $state = $null
$probeName = "results_css_s7_$Tag"
$counters = 'printf "CTR scene=%u native=%u nstatus=%u nident=0x%x nreason=%u ndomain=%u nscene=%u texup=%u pkt_rec=%u pkt_hit=%u pkt_fault=%u pkt_decl=%u route=%u admit=%u dlent=%u dtakes=%u dreturns=%u admitfail=%u heapmin=%u tint_builds=%u tint_fails=%u tint_hits=%u tint_misses=%u arena_fail=%u\n",gSCManagerSceneData.scene_curr,gNdsRendererNativeFailure.count,gNdsRendererNativeFailure.status,gNdsRendererNativeFailure.identity,gNdsRendererNativeFailure.reason,gNdsRendererNativeFailure.domain,gNdsRendererNativeFailure.scene,gNdsFighterDLAllDrawHardwareTextureUploadCount,gNdsFighterPacketRecords,gNdsFighterPacketHits,gNdsFighterPacketFaults,gNdsFighterPacketDeclines,gNdsFtrLeanRoute,gNdsFtrLeanAdmit,gNdsVramBankDLent,gNdsVramBankDTakes,gNdsVramBankDReturns,gNdsFtrLeanAdmitFail,gNdsTaskmanGeneralHeapFreeMin,gNdsR2FighterTintBuilds,gNdsR2FighterTintFails,gNdsR2FighterTintHits,gNdsR2FighterTintMisses,gNdsTaskmanArenaAllocFailCount'
$texDump = @()
$vramLine = 'printf "VRAM cnt A=%02x B=%02x C=%02x D=%02x E=%02x F=%02x G=%02x H=%02x I=%02x lock0=%d lock1=%d disp3d=%08x\n",*(unsigned char*)0x04000240,*(unsigned char*)0x04000241,*(unsigned char*)0x04000242,*(unsigned char*)0x04000243,*(unsigned char*)0x04000244,*(unsigned char*)0x04000245,*(unsigned char*)0x04000246,*(unsigned char*)0x04000248,*(unsigned char*)0x04000249,glGlobalData.vramLock[0],glGlobalData.vramLock[1],*(unsigned int*)0x04000060'
$counters = @($counters, $vramLine)
if ($DumpTex) {
    $texDump = @('printf "SERIAL %u\n",sNdsRendererHardwareFrameSerial', 'set $i = 0', 'while $i < 254',
        'set $e = &sNdsRendererHardwareTextureCache[$i]', 'if $e->name != 0',
        'set $k = &sNdsRendererHardwareTextureIdentPool[($i >= 45) ? ($i - 45) : 0]',
        'printf "TEX %u name=0x%x params=0x%08x pltt=0x%x cf=%u adm=%u rdy=%u pin=%u used=%u gen=%u img=0x%08x tlut=0x%08x\n",$i,$e->name,$e->params,$e->carve_pltt,$e->carve_flags,$e->admitted,$e->ready,$e->pinned,$e->last_used_frame,$e->key_generation,$k->image,$k->tlut_image',
        'if ($e->name > 0) && ($e->name < 0x1000)',
        'set $t = (gl_texture_data*)glGlobalData.texturePtrs.data[$e->name]',
        'if $t != 0',
        'printf "TEXLIB %u vram=0x%08x fmt=0x%08x pal=%d\n",$i,$t->vramAddr,$t->texFormat,$t->palIndex',
        'if $t->palIndex > 0',
        'set $p = (gl_palette_data*)glGlobalData.palettePtrs.data[$t->palIndex]',
        'if $p != 0',
        'printf "PALLIB %u vram=0x%08x addr=0x%x size=%u conn=%u pindex=%u\n",$i,$p->vramAddr,$p->addr,$p->palSize,$p->connectCount,$p->palIndex',
        'end', 'end', 'end', 'end',
        'end', 'set $i = $i + 1', 'end')
}
try {
    $state = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort -Persistent -BreakOnStartup -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList @($Rom) -WindowStyle Normal -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $cap = { param($name) "shell pwsh -NoProfile -File $root/scripts/capture-running-melonds-window.ps1 -EmulatorProcessId $($emulator.Id) -Output $shots/$Tag-$name.png" }
    $commands = @('set pagination off', 'set confirm off', 'set remotetimeout 60', "target remote 127.0.0.1:$($context.GdbPort)")
    if (($Route -ge 0) -or ($Admit -ge 0) -or ($BootSet.Count -gt 0)) {
        $commands += @('break main', 'continue')
        if ($Route -ge 0) { $commands += "set var gNdsFtrLeanRoute = $Route" }
        if ($Admit -ge 0) { $commands += "set var gNdsFtrLeanAdmit = $Admit" }
        foreach ($s in $BootSet) {
            $kv = $s -split '=', 2
            $commands += @("set var $($kv[0]) = $($kv[1])", "printf ""BOOTPOKE $($kv[0])=%u\n"",$($kv[0])")
        }
        $commands += @('printf "BOOTPOKE route=%u admit=%u\n",gNdsFtrLeanRoute,gNdsFtrLeanAdmit', 'delete')
    }
    $commands += @('break ndsSyMallocOverflowHalt', 'commands', 'silent', 'printf "HALT malloc-overflow\n"', 'bt 12', 'detach', 'quit 2', 'end',
        'break __excpt_entry', 'commands', 'silent', 'printf "HALT exception\n"', 'bt 12', 'detach', 'quit 3', 'end')
    if ($FeedFile -ne '') {
        $commands += (Get-Content -LiteralPath $FeedFile) | ForEach-Object { $_.Replace('@ROOT@', $root).Replace('@PID@', "$($emulator.Id)").Replace('@SHOTS@', $shots).Replace('@TAG@', $Tag) }
    }
    # Battle start, for the record.
    $commands += @('tbreak scVSBattleStartBattle', 'continue', 'printf "STOP battle-start\n"', $counters)
    foreach ($f in $StopAt) {
        $commands += @("tbreak $f", 'continue', "printf ""STOP at=$f scene=%u tic=%d\n"",gSCManagerSceneData.scene_curr,sMNVSResultsTotalTimeTics", $counters, 'bt 6', (& $cap "at-$f"))
    }
    foreach ($t in $ResultsTics) {
        $commands += @("tbreak ndsPlatformEndFrame if (gSCManagerSceneData.scene_curr == nSCKindVSResults) && (sMNVSResultsTotalTimeTics == $t)",
            'continue', "printf ""STOP results tic=%d\n"",sMNVSResultsTotalTimeTics", $counters, (& $cap "results-t$t")) + $texDump
        foreach ($r in $DumpRegions) {
            $parts = $r -split ':'
            $out = (Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR "$Tag-$($parts[0])-t$t.bin") -replace '\\', '/'
            $commands += "dump binary memory $out $($parts[1]) $($parts[2])"
        }
        if ($UntexturedShot) {
            # DISP3DCNT bit 0 off: the next frames render without texels, so
            # a pair of arms compares geometry and vertex colour alone.
            $commands += @('set *(unsigned int*)0x04000060 = (*(unsigned int*)0x04000060) & ~1',
                'break ndsPlatformEndFrame', 'ignore $bpnum 2', 'continue', 'delete $bpnum',
                "printf ""STOP results-untex tic=%d\n"",sMNVSResultsTotalTimeTics", (& $cap "results-t$t-untex"),
                'set *(unsigned int*)0x04000060 = (*(unsigned int*)0x04000060) | 1')
        }
        foreach ($m in $DumpMem) {
            $commands += "printf ""MEM $m %08x %08x %08x %08x %08x %08x %08x %08x\n"",((unsigned int*)$m)[0],((unsigned int*)$m)[1],((unsigned int*)$m)[2],((unsigned int*)$m)[3],((unsigned int*)$m)[4],((unsigned int*)$m)[5],((unsigned int*)$m)[6],((unsigned int*)$m)[7]"
        }
    }
    $commands += @('tbreak ndsMenuShellRunCharSelect', 'continue', 'printf "STOP css-enter\n"', $counters)
    $done = 0
    foreach ($p in $CssPresents) {
        $step = $p - $done
        $done = $p
        $commands += @('break ndsPlatformEndFrame', "ignore `$bpnum $($step - 1)", 'continue', 'delete $bpnum',
            "printf ""STOP css presents=$p\n""", $counters, (& $cap "css-p$p")) + $texDump
    }
    $commands += @('printf "PROBE_DONE\n"', 'detach', 'quit 0')
    Invoke-GdbMarkerScript -Gdb 'C:/devkitPro/devkitARM/bin/arm-none-eabi-gdb.exe' -Elf $Elf -Root $root -Commands $commands -ScriptName "$probeName.gdb" -TimeoutSeconds $TimeoutSeconds | Out-Null
} finally {
    $capture = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR "$probeName.gdb.out"
    if (Test-Path -LiteralPath $capture) {
        Get-Content -LiteralPath $capture | Where-Object { $_ -match '^(BOOTPOKE|STOP|CTR|VRAM|HALT|PROBE_DONE|FEED|TEX|PALLIB|SERIAL|MEM|#)' }
    }
    if ($emulator) { Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue }
    if ($state) { Restore-MelonDSGdbConfig -State $state }
}

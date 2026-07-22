[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('Fox','Mario','Natural')][string]$AuditMode,
    [string]$Gdb = '',
    [string]$Elf = '',
    [ValidateRange(1,65535)][int]$GdbPort = 4333,
    [int]$EmulatorProcessId = 0,
    [long]$WindowHandle = 0,
    [string]$OutputDirectory = '',
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 1200,
    [ValidateRange(0,218)][int]$StartMotion = 0,
    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Assert-Condition([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Get-FighterMotionRows([string]$Fighter) {
    $path = Join-Path $root 'decomp\BattleShip-main\decomp\src\ft\ftdata.c'
    $lines = Get-Content -LiteralPath $path
    $start = [Array]::FindIndex($lines, [Predicate[string]]{
        param($line)
        $line.Contains("FTMotionDesc dFT${Fighter}MotionDescs[]")
    })
    Assert-Condition ($start -ge 0) "Could not find $Fighter motion table."
    $rows = [Collections.Generic.List[object]]::new()
    $keep = $true
    for ($i = $start + 1; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -match '^\s*#if\s+defined\(REGION_US\)') { $keep = $true; continue }
        if ($line -match '^\s*#else') { $keep = $false; continue }
        if ($line -match '^\s*#endif') { $keep = $true; continue }
        if ($line -match '^\s*};') { break }
        if (-not $keep) { continue }
        $token = $null
        if ($line -match '^\s*\{\s*([^,]+),') {
            $token = $matches[1].Trim()
        } elseif ($line -match '^\s*([^,{][^,]*),\s*[^,]+,\s*[^,]+,?\s*$') {
            $token = $matches[1].Trim()
        }
        if ($null -eq $token) { continue }
        $sourceNull = $token -match '^(0|0x0+|NULL)$'
        $name = if ($sourceNull) {
            'SourceNull'
        } else {
            ($token.TrimStart('&') -replace '^ll','' -replace 'FileID$','')
        }
        $name = $name -replace '[^A-Za-z0-9_.-]', '_'
        $rows.Add([pscustomobject]@{
            Motion = $rows.Count
            Symbol = $name
            SourceNull = $sourceNull
        })
    }
    $expected = if ($Fighter -eq 'Mario') { 204 } else { 219 }
    Assert-Condition ($rows.Count -eq $expected) `
        "$Fighter motion parser found $($rows.Count), expected $expected."
    return $rows.ToArray()
}

$motionRows = @{
    Mario = @(Get-FighterMotionRows 'Mario')
    Fox = @(Get-FighterMotionRows 'Fox')
}
if ($ValidateOnly) {
    "Fighter animation audit parser passed: Mario=204 Fox=219."
    return
}

foreach ($required in @($Gdb, $Elf, $OutputDirectory)) {
    Assert-Condition (-not [string]::IsNullOrWhiteSpace($required)) `
        'Gdb, Elf, and OutputDirectory are required.'
}
Assert-Condition ($EmulatorProcessId -gt 0 -and $WindowHandle -ne 0) `
    'A live emulator process and window are required.'
$gdbPath = (Resolve-Path -LiteralPath $Gdb).Path
$elfPath = (Resolve-Path -LiteralPath $Elf).Path
$outputPath = [IO.Path]::GetFullPath($OutputDirectory)
$visibility = [IO.Path]::GetFullPath(
    (Join-Path $root 'artifacts\visibility')).TrimEnd('\','/') + '\'
Assert-Condition ($outputPath.StartsWith(
        $visibility, [StringComparison]::OrdinalIgnoreCase)) `
    "Animation screenshots must stay under $visibility"
New-Item -ItemType Directory -Force -Path $outputPath | Out-Null

$emulator = Get-Process -Id $EmulatorProcessId -ErrorAction Stop
$handle = [IntPtr]$WindowHandle
$fighter = if ($AuditMode -eq 'Natural') { $null } else { $AuditMode }
$selectedRows = if ($null -eq $fighter) { @() } else { $motionRows[$fighter] }
Assert-Condition (($AuditMode -ne 'Natural') -or ($StartMotion -eq 0)) `
    '-StartMotion is only valid for Fox or Mario.'
Assert-Condition (($null -eq $fighter) -or ($StartMotion -lt $selectedRows.Count)) `
    "Start motion $StartMotion is outside the $fighter table."
$expectedCaptures = @($selectedRows | Where-Object {
    $_.Motion -ge $StartMotion -and -not $_.SourceNull
}).Count
$temp = Join-Path $root ("artifacts\verifier-temp\task40\capture-$($AuditMode.ToLowerInvariant())")
New-Item -ItemType Directory -Force -Path $temp | Out-Null
$token = 'p{0}-{1}' -f $PID, ([guid]::NewGuid().ToString('N'))
$gdbScript = Join-Path $temp "$token.gdb"
$gdbOut = Join-Path $temp "$token.gdb.stdout.log"
$gdbErr = Join-Path $temp "$token.gdb.stderr.log"
$ready = Join-Path $temp "$token.capture.ready"
$go = Join-Path $temp "$token.capture.go"
$complete = Join-Path $temp "$token.complete.ready"
$completeGo = Join-Path $temp "$token.complete.go"
$arrays = @('Requested','Resolved','Fallback','ExpectedFrames','PlayedFrames','Flags')
$bins = @{}
foreach ($name in $arrays) { $bins[$name] = Join-Path $temp "$token.$name.bin" }
$temporary = @($gdbScript, $ready, $go, $complete, $completeGo) + @($bins.Values)
Remove-Item -LiteralPath $temporary -Force -ErrorAction SilentlyContinue

function Convert-GdbPath([string]$Path) {
    return [IO.Path]::GetRelativePath($root, $Path).Replace('\','/')
}
function Get-GdbShellWait([string]$ReadyPath, [string]$GoPath,
                          [bool]$CreateReady = $true) {
    $readyGdb = Convert-GdbPath $ReadyPath
    $goGdb = Convert-GdbPath $GoPath
    $publish = if ($CreateReady) {
        "Set-Content -LiteralPath '$readyGdb' -Value ready; "
    } else { '' }
    $waitSeconds = [Math]::Min($TimeoutSeconds, 35)
    return "shell powershell.exe -NoProfile -Command `"${publish}`$deadline=(Get-Date).AddSeconds($waitSeconds); while (-not (Test-Path -LiteralPath '$goGdb') -and (Get-Date) -lt `$deadline) { Start-Sleep -Milliseconds 25 }; if (-not (Test-Path -LiteralPath '$goGdb')) { exit 124 }`""
}

$commands = @(
    'set pagination off',
    'set confirm off',
    'set remotetimeout 15',
    "target remote 127.0.0.1:$GdbPort",
    'tbreak scVSBattleStartBattle',
    'continue',
    ('set variable gNdsBattlePlayableFoxCpuEnabled = {0}' -f
        $(if ($AuditMode -eq 'Natural') { 1 } else { 0 }))
)
if ($StartMotion -ne 0) {
    $commands += "set variable sNdsFighterAnimAuditMotion = $StartMotion"
}
if ($AuditMode -ne 'Natural') {
    $readyGdb = Convert-GdbPath $ready
    $goGdb = Convert-GdbPath $go
    $commands += @(
        'break ndsFighterAnimAuditCaptureMarker',
        'commands',
        'silent',
        "shell powershell.exe -NoProfile -Command `"Remove-Item -LiteralPath '$goGdb' -Force -ErrorAction SilentlyContinue`"",
        "dump binary memory $readyGdb &gNdsFighterAnimAuditActiveMotion ((char *)&gNdsFighterAnimAuditActiveMotion + 4)",
        (Get-GdbShellWait $ready $go $false),
        'continue',
        'end',
        'tbreak ndsFighterAnimAuditCompleteMarker',
        'commands',
        'silent'
    )
} else {
    $commands += @('tbreak mnVSResultsStartScene', 'continue')
}
foreach ($name in $arrays) {
    $binGdb = Convert-GdbPath $bins[$name]
    $symbol = "gNdsFighterAnimAudit$name"
    $commands += "dump binary memory $binGdb &$symbol`[0`]`[0`] (&$symbol`[0`]`[0`] + 438)"
}
$commands += (Get-GdbShellWait $complete $completeGo)
if ($AuditMode -ne 'Natural') { $commands += @('end','continue') }
$commands += 'set variable gNdsBattlePlayableFoxCpuEnabled = 1'
$commands += @('detach','quit')
Set-Content -LiteralPath $gdbScript -Value ($commands -join "`n")

Add-Type -AssemblyName System.Drawing
. (Join-Path $PSScriptRoot 'lib\melonds-screenshot.ps1')
if ($null -eq ('Smash64DSFighterAnimCapture' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Smash64DSFighterAnimCapture {
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rect);
}
'@
}

function Save-NativeTop([string]$Path) {
    $rect = New-Object Smash64DSFighterAnimCapture+Rect
    Assert-Condition ([Smash64DSFighterAnimCapture]::GetWindowRect(
            $handle, [ref]$rect)) 'Could not read melonDS window bounds.'
    $window = New-Object Drawing.Bitmap ($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top)
    $graphics = [Drawing.Graphics]::FromImage($window)
    $native = $null
    try {
        $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $window.Size)
        $native = Convert-MelonDSWindowTopToNativeBitmap -Bitmap $window
        $different = 0
        $samples = 0
        for ($y = 0; $y -lt 192; $y += 4) {
            for ($x = 0; $x -lt 256; $x += 4) {
                $pixel = $native.GetPixel($x, $y)
                if ($pixel.R -ne 20 -or $pixel.G -ne 28 -or $pixel.B -ne 52) {
                    $different++
                }
                $samples++
            }
        }
        Assert-Condition ($different -ge [Math]::Ceiling($samples * 0.01)) `
            "Top screen was clear at $Path."
        $native.Save($Path, [Drawing.Imaging.ImageFormat]::Png)
    } finally {
        if ($null -ne $native) { $native.Dispose() }
        $graphics.Dispose()
        $window.Dispose()
    }
}

function New-ContactSheet([object[]]$Rows) {
    if ($Rows.Count -eq 0) { return }
    $columns = 8
    $tileWidth = 128
    $tileHeight = 118
    $sheet = New-Object Drawing.Bitmap ($columns * $tileWidth), `
        ([Math]::Ceiling($Rows.Count / [double]$columns) * $tileHeight)
    $graphics = [Drawing.Graphics]::FromImage($sheet)
    $font = New-Object Drawing.Font 'Consolas', 8
    try {
        $graphics.Clear([Drawing.Color]::Black)
        for ($i = 0; $i -lt $Rows.Count; $i++) {
            $row = $Rows[$i]
            $image = [Drawing.Image]::FromFile($row.Path)
            try {
                $x = ($i % $columns) * $tileWidth
                $y = [Math]::Floor($i / $columns) * $tileHeight
                $graphics.DrawImage($image, $x, $y, 128, 96)
                $graphics.DrawString(("{0:D3} {1}" -f $row.Motion, $row.Symbol),
                    $font, [Drawing.Brushes]::White, $x + 2, $y + 98)
            } finally { $image.Dispose() }
        }
        $sheet.Save((Join-Path $outputPath "$fighter-contact-sheet.png"),
            [Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $font.Dispose()
        $graphics.Dispose()
        $sheet.Dispose()
    }
}

function Read-U16Array([string]$Path) {
    [byte[]]$bytes = [IO.File]::ReadAllBytes($Path)
    Assert-Condition ($bytes.Length -eq 876) `
        "Audit array $Path has $($bytes.Length) bytes, expected 876."
    $values = [uint16[]]::new(438)
    for ($i = 0; $i -lt $values.Length; $i++) {
        $values[$i] = [BitConverter]::ToUInt16($bytes, $i * 2)
    }
    return ,$values
}

function Get-FlagNames([int]$Flags) {
    $names = @()
    foreach ($pair in @(
        @(1,'source-null'), @(2,'load-fallback'), @(4,'extern-fail'),
        @(8,'figatree-invalid'), @(16,'unsafe'), @(32,'end'),
        @(64,'loop'), @(128,'timeout'), @(256,'duration-mismatch'),
        @(512,'expected-invalid'))) {
        if (($Flags -band $pair[0]) -ne 0) { $names += $pair[1] }
    }
    return $names -join '|'
}

$gdbProcess = $null
$captured = [Collections.Generic.HashSet[int]]::new()
$captureRows = [Collections.Generic.List[object]]::new()
foreach ($row in @($selectedRows | Where-Object {
    $_.Motion -lt $StartMotion -and -not $_.SourceNull
})) {
    $name = '{0:D3}_{1}.png' -f $row.Motion, $row.Symbol
    $path = Join-Path $outputPath $name
    Assert-Condition (Test-Path -LiteralPath $path -PathType Leaf) `
        "Resume screenshot is missing: $path"
    $captureRows.Add([pscustomobject]@{
        Motion = $row.Motion; Symbol = $row.Symbol; Path = $path
    })
}
try {
    $gdbProcess = Start-Process -FilePath $gdbPath `
        -ArgumentList @('-q','-batch','-x',$gdbScript,$elfPath) `
        -RedirectStandardOutput $gdbOut -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $progressDeadline = (Get-Date).AddSeconds(30)
    while (-not (Test-Path -LiteralPath $complete -PathType Leaf)) {
        $gdbProcess.Refresh(); $emulator.Refresh()
        if ($gdbProcess.HasExited -or $emulator.HasExited) {
            throw 'GDB or melonDS exited before the animation audit completed.'
        }
        if ((Get-Date) -ge $deadline) { throw 'Animation audit timed out.' }
        if (($AuditMode -ne 'Natural') -and
            ((Get-Date) -ge $progressDeadline)) {
            throw 'Animation audit made no capture progress for 30 seconds.'
        }
        if (($AuditMode -ne 'Natural') -and
            (Test-Path -LiteralPath $ready -PathType Leaf) -and
            ((Get-Item -LiteralPath $ready).Length -ge 4)) {
            [byte[]]$marker = [IO.File]::ReadAllBytes($ready)
            $motion = [BitConverter]::ToUInt32($marker, 0)
            Remove-Item -LiteralPath $ready -Force
            Assert-Condition ($motion -lt $selectedRows.Count) `
                "Capture motion $motion is outside $fighter table."
            Assert-Condition ($motion -ge $StartMotion) `
                "Capture motion $motion precedes start motion $StartMotion."
            Assert-Condition (-not $selectedRows[$motion].SourceNull) `
                "Source-null motion $motion requested a screenshot."
            Assert-Condition ($captured.Add($motion)) `
                "Motion $motion requested two screenshots."
            $name = '{0:D3}_{1}.png' -f $motion, $selectedRows[$motion].Symbol
            $path = Join-Path $outputPath $name
            Save-NativeTop $path
            $captureRows.Add([pscustomobject]@{
                Motion = $motion; Symbol = $selectedRows[$motion].Symbol; Path = $path
            })
            $progressDeadline = (Get-Date).AddSeconds(30)
            Set-Content -LiteralPath $go -Value go
            Write-Output "Task 40 $fighter capture $($captured.Count)/${expectedCaptures}: $name"
        }
        Start-Sleep -Milliseconds 25
    }
    Set-Content -LiteralPath $completeGo -Value go
    Assert-Condition ($gdbProcess.WaitForExit(30000)) `
        'GDB did not detach after the audit completed.'
    $gdbProcess.WaitForExit()
    $gdbText = ((Get-Content $gdbOut,$gdbErr -Raw -ErrorAction SilentlyContinue) -join "`n")
    Assert-Condition ($gdbProcess.ExitCode -eq 0) `
        "Animation audit GDB failed with exit $($gdbProcess.ExitCode).`n$gdbText"
    if ($AuditMode -ne 'Natural') {
        Assert-Condition ($captured.Count -eq $expectedCaptures) `
            "$fighter captured $($captured.Count)/$expectedCaptures non-null motions from $StartMotion."
        New-ContactSheet @($captureRows | Sort-Object Motion)
    }
    $data = @{}
    foreach ($name in $arrays) { $data[$name] = Read-U16Array $bins[$name] }
    $csvRows = foreach ($kind in 0,1) {
        $kindName = @('Mario','Fox')[$kind]
        foreach ($row in $motionRows[$kindName]) {
            $offset = ($kind * 219) + $row.Motion
            $flags = $data.Flags[$offset]
            [pscustomobject]@{
                fighter = $kindName
                motion_id = $row.Motion
                symbol = $row.Symbol
                requested = $data.Requested[$offset]
                resolved = $data.Resolved[$offset]
                fallback = $data.Fallback[$offset]
                expected_frames = $data.ExpectedFrames[$offset]
                played_frames = $data.PlayedFrames[$offset]
                flags = ('0x{0:X4}' -f $flags)
                flag_names = Get-FlagNames $flags
            }
        }
    }
    $leaf = Split-Path -Leaf $outputPath
    $csvPath = Join-Path $root "artifacts\performance\$leaf-audit.csv"
    $csvRows | Export-Csv -LiteralPath $csvPath -NoTypeInformation
    Write-Output "Task 40 $AuditMode audit complete: data=$csvPath captures=$($captureRows.Count)."
} finally {
    Set-Content -LiteralPath $go,$completeGo -Value go -ErrorAction SilentlyContinue
    if ($null -ne $gdbProcess) {
        $gdbProcess.Refresh()
        if (-not $gdbProcess.HasExited) {
            if (-not $gdbProcess.WaitForExit(2000)) {
                Stop-Process -Id $gdbProcess.Id -Force
                $gdbProcess.WaitForExit()
            }
        }
    }
    Remove-Item -LiteralPath $temporary -Force -ErrorAction SilentlyContinue
}

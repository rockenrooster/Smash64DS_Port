[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Build,
    [Parameter(Mandatory=$true)][string]$Target,
    [Parameter(Mandatory=$true)][string]$OutputWave,
    [ValidateRange(10,180)][int]$Seconds=70,
    [ValidateRange(1,8)][int]$RunnerSlot=6
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $root 'scripts/lib/melonds.ps1')
. (Join-Path $root 'scripts/lib/build-output.ps1')
$context=Initialize-MelonDSVerifierContext -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$rom=Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$wave=[IO.Path]::GetFullPath((Join-Path $root $OutputWave))
$raw=[IO.Path]::ChangeExtension($wave,'.raw')
$log=[IO.Path]::ChangeExtension($wave,'.stdout.log')
$err=[IO.Path]::ChangeExtension($wave,'.stderr.log')
$driverName='80_'+[IO.Path]::GetFileName($raw)
$driverOutput=Join-Path (Split-Path -Parent $context.MelonDSPath) $driverName
if (Test-Path -LiteralPath $raw) { throw "Capture already exists: $raw" }
if (Test-Path -LiteralPath $driverOutput) { throw "Driver capture already exists: $driverOutput" }
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $wave) | Out-Null
$state=$null
$emulator=$null
$savedDriver=$env:SDL_AUDIODRIVER
$savedFile=$env:SDL_DISKAUDIOFILE
$savedDelay=$env:SDL_DISKAUDIODELAY
try {
    $state=Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort -Arm7Port $context.Arm7Port
    $text=Get-Content -LiteralPath $state.Config -Raw
    # Disk output receives this process only and never reaches host speakers.
    # A slow consumer plus audio sync prevents host-speed underrun/padding.
    # This is an output capture, never a timing/cadence measurement.
    $text=Set-MelonDSTomlValue -Text $text -Section 'Instance0.Audio' -Key 'Volume' -Value '256'
    $text=Set-MelonDSTomlRootValue -Text $text -Key 'AudioSync' -Value 'true'
    $text=Set-MelonDSTomlRootValue -Text $text -Key 'GdbEnabled' -Value 'false'
    $text=Set-MelonDSTomlValue -Text $text -Section 'Instance0.Gdb' -Key 'Enabled' -Value 'false'
    Set-Content -LiteralPath $state.Config -Value $text -NoNewline
    $env:SDL_AUDIODRIVER='disk'
    # This SDL2 Windows build reuses getenv's temporary string for filename and
    # delay. Identical strings avoid that alias; SDL_atoi reads the 80 ms prefix.
    # Migrate this unique runner-owned file to builds immediately after exit.
    $env:SDL_DISKAUDIOFILE=$driverName
    $env:SDL_DISKAUDIODELAY=$driverName
    $emulator=Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) -WindowStyle Hidden `
        -RedirectStandardOutput $log -RedirectStandardError $err -PassThru
    Write-Output ("Disk capture process {0}; ROM {1}; output {2}" -f $emulator.Id,$rom,$raw)
    $deadline=(Get-Date).AddSeconds($Seconds)
    while ((Get-Date) -lt $deadline -and -not $emulator.HasExited) { Start-Sleep -Milliseconds 250 }
    if ($emulator.HasExited) { throw "Audio capture emulator exited early: $($emulator.ExitCode)" }
} finally {
    if ($null -ne $emulator -and -not $emulator.HasExited) {
        $emulator.CloseMainWindow() | Out-Null
        if (-not $emulator.WaitForExit(4000)) { Stop-Process -Id $emulator.Id -Force }
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state | Out-Null }
    $env:SDL_AUDIODRIVER=$savedDriver
    $env:SDL_DISKAUDIOFILE=$savedFile
    $env:SDL_DISKAUDIODELAY=$savedDelay
}
if (Test-Path -LiteralPath $driverOutput) {
    $driverHash=(Get-FileHash -LiteralPath $driverOutput -Algorithm SHA256).Hash
    Move-Item -LiteralPath $driverOutput -Destination $raw
    if ((Get-FileHash -LiteralPath $raw -Algorithm SHA256).Hash -ne $driverHash) {
        throw 'Driver capture hash changed during migration.'
    }
}
if (-not (Test-Path -LiteralPath $raw) -or (Get-Item -LiteralPath $raw).Length -lt 4096) {
    throw "SDL disk audio produced no usable PCM: $err"
}
[string]$stdout=Get-Content -LiteralPath $log -Raw
$rate=[regex]::Match($stdout,'Audio output frequency:\s*(\d+)\s*Hz')
if ($rate.Success -and [int]$rate.Groups[1].Value -ne 48000) {
    throw 'Capture format differs from melonDS 1.0 audioInit''s 48 kHz stereo S16LE request.'
}
$convert=@'
import sys,wave,hashlib,json,array
from pathlib import Path
raw,output,rom=map(Path,sys.argv[1:4]);data=raw.read_bytes()
if len(data)%4: raise SystemExit('partial stereo PCM frame')
samples=array.array('h',data)
if sys.byteorder!='little':samples.byteswap()
if not any(samples):raise SystemExit('capture is entirely silent')
with wave.open(str(output),'wb') as w:
    w.setnchannels(2);w.setsampwidth(2);w.setframerate(48000);w.writeframes(data)
report={'rom_sha256':hashlib.sha256(rom.read_bytes()).hexdigest().upper(),
        'wave':str(output),'wave_sha256':hashlib.sha256(output.read_bytes()).hexdigest().upper(),
        'rate':48000,'channels':2,'seconds':len(data)/192000,'peak':max(abs(x) for x in samples),
        'nonzero_samples':sum(x!=0 for x in samples),'timing_qualified':False,
        'format_source':'melonDS 1.0 EmuInstance::audioInit + SDL disk driver preserves requested format'}
output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
'@
& python -c $convert $raw $wave $rom
if ($LASTEXITCODE -ne 0) { throw 'PCM conversion/validation failed.' }

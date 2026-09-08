$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib/melonds.ps1')
function Assert-True([bool]$Value, [string]$Message) {
    if (-not $Value) { throw $Message }
}
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$profiles = @()
foreach ($slot in 0..11) {
    $storage = Join-Path $root "emulators/melonds-runners/slot$slot/diagnostics/test"
    $profile = Set-MelonDSAutomationProfile -Text '' `
        -GdbPort (Get-MelonDSRunnerPort -RunnerSlot $slot) `
        -Arm7Port (Get-MelonDSRunnerPort -RunnerSlot $slot -Cpu ARM7) -StorageDirectory $storage
    $image = ((Join-Path $storage 'dldi.bin') -replace '\\','/')
    Assert-True ($profile.Contains("ImagePath = `"$image`"")) "slot$slot lacks a private DLDI image"
    Assert-True ($profile.Contains('ReadOnly = true') -and $profile.Contains('FolderSync = false')) 'DLDI mode drift'
    Assert-True ($profile.Contains('ConsoleType = 0') -and $profile.Contains('Renderer = 0')) 'DS/reference renderer mode drift'
    Assert-True ((Set-MelonDSAutomationProfile -Text $profile `
        -GdbPort (Get-MelonDSRunnerPort -RunnerSlot $slot) `
        -Arm7Port (Get-MelonDSRunnerPort -RunnerSlot $slot -Cpu ARM7) `
        -StorageDirectory $storage) -ceq $profile) 'Private profile is not idempotent'
    $profiles += $profile
}
$ports = @(foreach ($slot in 0..31) {
    Get-MelonDSRunnerPort -RunnerSlot $slot -Cpu ARM9
    Get-MelonDSRunnerPort -RunnerSlot $slot -Cpu ARM7
})
Assert-True (@($ports | Select-Object -Unique).Count -eq 64) 'Runner GDB ports overlap'
Assert-True (3333 -notin $ports -and 3334 -notin $ports) 'Manual GDB ports were claimed'
$normal = Set-MelonDSAutomationProfile -Text $profiles[0] -GdbPort 4323 -Arm7Port 4324
Assert-True ($normal.Contains("ImagePath = `"$script:MelonDSCanonicalDldiImage`"")) 'Serial profile did not restore its image'
Assert-True ($normal.Contains('SaveFilePath = ""') -and $normal.Contains('SavestatePath = ""')) 'Private save paths leaked'
$previous = $env:SMASH64DS_VERIFY_STORAGE_DIR
try {
    $env:SMASH64DS_VERIFY_STORAGE_DIR = Join-Path $root 'builds/outside-runner'
    $rejected = $false
    try { Get-MelonDSVerifierStorageDirectory | Out-Null } catch { $rejected = $true }
    Assert-True $rejected 'Storage outside runner directories was accepted'
    $env:SMASH64DS_VERIFY_STORAGE_DIR = Join-Path $root 'emulators/melonds-runners/slot4/diagnostics/test'
    Assert-True ((Get-MelonDSVerifierStorageDirectory) -eq $env:SMASH64DS_VERIFY_STORAGE_DIR) 'Valid private storage rejected'
} finally { $env:SMASH64DS_VERIFY_STORAGE_DIR = $previous }
Write-Output 'PARALLEL_DIAGNOSTIC_ISOLATION_OK profiles=12 distinct_ports=64 manual_ports_reserved=1'

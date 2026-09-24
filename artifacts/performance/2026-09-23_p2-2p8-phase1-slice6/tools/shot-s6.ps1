param(
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [Parameter(Mandatory = $true)][string]$Path
)
# P2-2p8 Phase 1 slice 6: one window capture of a GDB-halted melonDS, run by
# gdb's `shell` while the core is stopped (capture-s6.ps1 writes the call), so
# the picture belongs to that stop. PrintWindow renders the window itself (no
# desktop pixels, no foreground raise); a uniform image is refused.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds-screenshot.ps1')
$p = Get-Process -Id $ProcessId
$p.Refresh()
$h = $p.MainWindowHandle
if ($h -eq [IntPtr]::Zero) { throw 'melonDS has no window handle' }
$size = Save-MelonDSWindowCapture -WindowHandle $h -Path $Path -PreferPrintWindow
$bmp = [System.Drawing.Bitmap]::FromFile($Path)
try {
    $first = $bmp.GetPixel([int]($bmp.Width / 2), [int]($bmp.Height / 4))
    $uniform = $true
    for ($y = 60; $y -lt $bmp.Height - 8; $y += 37) {
        for ($x = 10; $x -lt $bmp.Width - 10; $x += 29) {
            if ($bmp.GetPixel($x, $y).ToArgb() -ne $first.ToArgb()) { $uniform = $false; break }
        }
        if (-not $uniform) { break }
    }
} finally { $bmp.Dispose() }
if ($uniform) { Remove-Item -LiteralPath $Path -Force; throw "uniform capture refused: $Path" }
"SHOT-OK $size $Path"

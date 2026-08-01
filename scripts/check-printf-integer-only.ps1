[CmdletBinding()]
param()

# Port code formats with the INTEGER-ONLY newlib entry points, and this fails the
# build if it stops.
#
# newlib chooses its formatter by SYMBOL, not by format string. One call to
# snprintf or vsnprintf links _svfprintf_r (10,047 bytes), which pulls _dtoa_r
# (4,608) and __mprec (2,480) whether or not any caller ever passes a %f. The
# port had seven such calls -- six "%s"/"%lu" path builders and the tick-HUD line
# printer, whose 73 call sites contain no float conversion at all -- and was
# paying 17,135 bytes of image for a conversion it never performs.
#
# WHY THAT IS A CORRECTNESS GUARD AND NOT A SIZE PREFERENCE. Linked bytes come
# out of the boot-time taskman arena one-for-one: src/port/diagnostics.c searches
# down from 0x150000 in 0x1000 steps and FLOORS at 0x130000, so 17,135 bytes is
# three whole steps of that search. Measured 2026-07-31, tickhud build,
# arena 1,290,240 -> 1,302,528 with AllocFail 21 -> 18. On the same day a 33,152
# byte reference made the battle unbootable outright, dying under
# ifCommonSetMaxNumGObj's 25 KiB GObj latch. Three steps is real headroom on a
# target that currently has about eleven.
#
# If a format ever genuinely needs %f, the honest move is to format the integer
# and fractional parts separately -- the tick HUD already does exactly that for
# "FPS 24.1" -- not to relink the float formatter for one call site.

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

# Ban the float-capable spellings. iprintf/siprintf/sniprintf/vsniprintf are the
# integer-only ones and are fine. The \b before each name stops sniprintf from
# matching as "...niprintf" and vsniprintf from matching vsnprintf.
$banned = @('snprintf', 'vsnprintf', 'sprintf', 'vsprintf', 'printf', 'vprintf',
            'fprintf', 'vfprintf')
$pattern = '(?<![A-Za-z0-9_])(' + ($banned -join '|') + ')\s*\('

$searchRoots = @('src', 'include') | ForEach-Object { Join-Path $root $_ }
$offenders = @()
foreach ($searchRoot in $searchRoots) {
    $files = Get-ChildItem -LiteralPath $searchRoot -Recurse -File -Include '*.c', '*.h' |
        Where-Object { $_.FullName -notmatch '\\generated\\' }
    foreach ($file in $files) {
        $number = 0
        foreach ($line in [System.IO.File]::ReadAllLines($file.FullName)) {
            $number++
            # Skip comments so this file's own explanation cannot trip it.
            $code = $line -replace '/\*.*?\*/', '' -replace '//.*$', ''
            if ($code -match '^\s*\*') { continue }
            if ($code -match $pattern) {
                $offenders += ('{0}:{1}: {2}' -f
                    $file.FullName.Substring($root.Length + 1), $number, $line.Trim())
            }
        }
    }
}

if ($offenders.Count -gt 0) {
    throw ("Float-capable printf in port code -- use the integer-only spelling " +
        "(iprintf / siprintf / sniprintf / vsniprintf). It costs 17,135 bytes of " +
        "image, which is three taskman arena steps:`n" + ($offenders -join "`n"))
}

Write-Output ('printf policy passed: no float-capable formatter reachable from ' +
    'port source; newlib links the integer path only.')

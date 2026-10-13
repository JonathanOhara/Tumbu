# The memory check (CLAUDE.md, "Memory check"): N matches through the real UI on each renderer (devtest.ps1 -Cycles N),
# judged against the expected values and compared with the previous run of the same length. Run it (in the background,
# about 12 minutes per renderer for 40 matches) after a change that adds or changes GPU resources, render targets,
# materials or objects created per match.
# Usage: .\scripts\memcheck.ps1 [-Cycles 40] [-Renderers D3D11,GL]
#
# Per renderer it prints a block like this (numbers made up):
#   == GL: PASS (40 matches, driver 4.6.0.0, Ogre 14.6.0)
#     heap     menu 18.4 MB | after match 1 21.6 MB | last 21.5 MB | matches 21..40 21.4 .. 21.6 MB
#     growth   matches 2..21 -2 KB/match | 21..40 -4 KB/match | 2..40 -3.0 KB/match (limit +/-30)
#     objects  flat: nodes=3 entities=0 materials=119 textures=194 meshes=20 skeletons=13 programs=69 ...
#     previous 2026-10-13 01:56 (40 matches): after match 1 21.0 MB (+0.6), last 21.0 MB (+0.5), growth -1.2 KB/match
#     files    report ...\memcheck-GL.txt, game log ...\devtest-memcheck-GL.log
# and on a FAIL, under it, what failed and the evidence: which object counts changed in which match, the block sizes the
# heap grew by in the last matches and the DLLs that own sample blocks (the game logs them), plus where to look next.
#
# The checks (keep the thresholds in step with CLAUDE.md, "Memory check"):
#   both    the game exits normally after N matches, no exception; every object count is the same after every match
#           (objects=flat: a count that climbs is a leak of that kind of object)
#   OpenGL  the heap flat from match 2 on: growth (matches 2..N) within +/- $GlGrowthKB KB per match (judged from 10 matches)
#   D3D11   an NVIDIA driver pool fills for about 18 matches and then stays (not a leak): with 40 matches or more, growth
#           over the second half within $D3dSecondHalfKB KB per match and the last heap below $D3dPlateauMB MB; with fewer
#           matches the heap is still filling and is not judged
# "previous" is the last run of the same renderer and length in %USERPROFILE%\Tumbu\memory-history.csv (the game
# appends every run). A NOTE line points out a level that moved more than usual; levels drift between days for
# reasons outside the game (docs/HISTORY.md, "Memory"), so a NOTE is a hint to compare builds alternately, not a FAIL.
# Exit code 1 when a check fails.
param(
    [int]$Cycles = 40,
    [ValidateSet('D3D11', 'GL')] [string[]]$Renderers = @('D3D11', 'GL'),
    [double]$GlGrowthKB = 30,
    [double]$D3dSecondHalfKB = 300,
    [double]$D3dPlateauMB = 100,
    # NOTE when the level after match 1 (OpenGL) or the last heap (D3D11) moved more than this from the previous run
    [double]$GlLevelNoteMB = 1.0,
    [double]$D3dLevelNoteMB = 8.0
)
$ErrorActionPreference = 'Stop'
[Threading.Thread]::CurrentThread.CurrentCulture = [Globalization.CultureInfo]::InvariantCulture
$out = Join-Path $env:USERPROFILE 'Tumbu'
$historyFile = Join-Path $out 'memory-history.csv'
$failed = $false

function MB([double]$kb) { '{0:0.0} MB' -f ($kb / 1024) }
function Num([string]$s) { [double]::Parse($s, [Globalization.CultureInfo]::InvariantCulture) }

foreach ($r in $Renderers) {
    # the previous run of this renderer and length, read before this run appends its own line
    $previous = $null
    if (Test-Path $historyFile) {
        $previous = Import-Csv $historyFile | Where-Object { $_.render -eq $r -and [int]$_.matches -eq $Cycles } | Select-Object -Last 1
    }

    $report = Join-Path $out "memcheck-$r.txt"
    $gameLog = Join-Path $out "devtest-memcheck-$r.log"
    # every stream (devtest.ps1 reports with Write-Host), into the report instead of the console
    $ErrorActionPreference = 'Continue'
    $lines = & (Join-Path $PSScriptRoot 'devtest.ps1') -Cycles $Cycles -Renderer $r -Name "memcheck-$r" *>&1 | ForEach-Object { "$_" }
    $ErrorActionPreference = 'Stop'
    Set-Content $report $lines

    # the game's own lines: one per match (heap and object counts), the growth by block size, the sample DLLs
    $perMatch = @()
    $blockLines = @{}
    $moduleLines = @{}
    $summary = $null
    $current = 0
    $log = if (Test-Path $gameLog) { Get-Content $gameLog } else { @() }
    foreach ($l in $log) {
        if ($l -match '\[DEVTEST\] memory cycle (\d+) menu private=(\d+)KB heap=(\d+)KB/\d+ (.*)$') {
            $counts = [ordered]@{}
            foreach ($pair in $Matches[4].Trim().Split(' ')) {
                $kv = $pair.Split('=')
                if ($kv.Count -eq 2) { $counts[$kv[0]] = [int]$kv[1] }
            }
            $current = [int]$Matches[1]
            $perMatch += [pscustomobject]@{ n = $current; heap = [int]$Matches[3]; private = [int]$Matches[2]; counts = $counts }
        } elseif ($l -match '\[DEVTEST\] heap growth by block size: (.*)$') {
            # logged before the cycle line of the match it belongs to
            $blockLines[$current + 1] = $Matches[1]
        } elseif ($l -match '\[DEVTEST\]\s+sample modules:(.*)$') {
            $moduleLines[$current + 1] = ($moduleLines[$current + 1], $Matches[1].Trim() | Where-Object { $_ }) -join ' '
        } elseif ($l -match 'memory summary: (.*)$') {
            $summary = $Matches[1]
        }
    }
    $driver = if ($summary -match 'driver=(\S+)') { $Matches[1] } else { '?' }
    $ogre = if ($summary -match 'ogre=(\S+)') { $Matches[1] } else { '?' }

    $problems = @()
    $evidence = @()
    if (-not ($lines | Where-Object { $_ -match '^exit=0' })) { $problems += 'the game did not exit normally (crash: %USERPROFILE%\Tumbu\crash.log)' }
    $errors = $log | Where-Object { $_ -match 'EXCEPTION|Fatal' -and $_ -notmatch 'Information Queue' } | Select-Object -First 3
    if ($errors) { $problems += 'an exception or fatal error'; $evidence += $errors | ForEach-Object { "  log: $_" } }
    $played = $perMatch.Count - 1
    if ($played -lt $Cycles) { $problems += "only $([math]::Max($played, 0)) of $Cycles matches finished" }

    $heapLine = ''
    $growthLine = ''
    $objectsLine = ''
    $previousLine = ''
    $notes = @()
    if ($perMatch.Count -ge 3) {
        $heaps = $perMatch | ForEach-Object { $_.heap }
        $last = $heaps[$heaps.Count - 1]
        $mid = [int][math]::Floor(($heaps.Count + 1) / 2)
        $firstHalf = ($heaps[$mid] - $heaps[2]) / [math]::Max(1, $mid - 2)
        $secondHalf = ($last - $heaps[$mid]) / [math]::Max(1, $heaps.Count - 1 - $mid)
        $overall = ($last - $heaps[2]) / [math]::Max(1, $heaps.Count - 3)
        $tail = $heaps[$mid..($heaps.Count - 1)] | Measure-Object -Minimum -Maximum
        $heapLine = "menu $(MB $heaps[0]) | after match 1 $(MB $heaps[1]) | last $(MB $last) | matches $mid..$($heaps.Count - 1) $('{0:0.0}' -f ($tail.Minimum / 1024)) .. $(MB $tail.Maximum)"
        $growthLine = "matches 2..$mid {0:+0;-0} KB/match | $mid..$($heaps.Count - 1) {1:+0;-0} KB/match | 2..$($heaps.Count - 1) {2:+0.0;-0.0} KB/match" -f $firstHalf, $secondHalf, $overall

        if ($r -eq 'GL' -and $heaps.Count -ge 11) {
            $growthLine += " (limit +/-$GlGrowthKB)"
            if ([math]::Abs($overall) -gt $GlGrowthKB) { $problems += "the heap grows {0:0.0} KB per match from match 2 on (limit +/-$GlGrowthKB)" -f $overall }
        } elseif ($r -eq 'GL') {
            # a single step of tens of KB (OpenGL32, the Windows text input DLLs) is noise over a few matches
            $growthLine += ' (under 10 matches single steps are noise: growth not judged)'
        } elseif ($heaps.Count -ge 41) {
            $growthLine += " (second half limit $D3dSecondHalfKB; the first half fills the driver pool)"
            if ($secondHalf -gt $D3dSecondHalfKB) { $problems += "the heap still grows {0:0} KB per match in the second half (limit $D3dSecondHalfKB): the driver pool fills, then must stay" -f $secondHalf }
            if ($last / 1024 -gt $D3dPlateauMB) { $problems += "the plateau is $(MB $last), above $D3dPlateauMB MB (expected about 85 to 93 MB)" }
        } else {
            $growthLine += ' (under 40 matches the driver pool is still filling: growth not judged)'
        }

        # object counts: the same after every match from match 1 on (the menu before match 1 has fewer)
        $first = $perMatch[1].counts
        $changed = @()
        foreach ($k in $first.Keys) {
            $values = $perMatch[1..($perMatch.Count - 1)] | ForEach-Object { $_.counts[$k] }
            $distinct = $values | Sort-Object -Unique
            if (@($distinct).Count -gt 1) {
                $at = 2
                while ($at -lt $perMatch.Count -and $perMatch[$at].counts[$k] -eq $first[$k]) { $at++ }
                $changed += "$k $($first[$k]) -> $($perMatch[$perMatch.Count - 1].counts[$k]) (first change after match $at; min $(($distinct | Measure-Object -Minimum).Minimum), max $(($distinct | Measure-Object -Maximum).Maximum))"
            }
        }
        $countText = ($first.Keys | ForEach-Object { "$_=$($first[$_])" }) -join ' '
        if ($changed.Count -eq 0) {
            $objectsLine = "flat: $countText"
        } else {
            $objectsLine = "CHANGED: " + ($changed -join '; ')
            $problems += 'object counts changed between matches (a leak of that kind of object, or one made only once by a later match)'
        }

        # the previous run of the same length
        if ($previous) {
            $p1 = Num $previous.heap_cycle1_kb
            $pl = Num $previous.heap_last_kb
            $pg = Num $previous.growth_kb_per_match
            $previousLine = "$($previous.date) ($($previous.matches) matches, driver $($previous.driver)): after match 1 $(MB $p1) ({0:+0.0;-0.0}), last $(MB $pl) ({1:+0.0;-0.0}), growth 2..N {2:0.0} KB/match, objects $($previous.objects)" -f (($heaps[1] - $p1) / 1024), (($last - $pl) / 1024), $pg
            if ($r -eq 'GL' -and [math]::Abs($heaps[1] - $p1) / 1024 -gt $GlLevelNoteMB) {
                $notes += "NOTE the OpenGL level after match 1 moved {0:+0.0;-0.0} MB from the previous run (more than $GlLevelNoteMB MB): if the change should not add memory, compare the two builds alternately (CLAUDE.md, 'Memory check')" -f (($heaps[1] - $p1) / 1024)
            }
            if ($r -eq 'D3D11' -and $heaps.Count -ge 41 -and [math]::Abs($last - $pl) / 1024 -gt $D3dLevelNoteMB) {
                $notes += "NOTE the Direct3D 11 plateau moved {0:+0.0;-0.0} MB from the previous run (more than $D3dLevelNoteMB MB); single runs scatter, alternate the builds before concluding" -f (($last - $pl) / 1024)
            }
            if ($previous.driver -ne $driver) { $notes += "NOTE the driver changed since the previous run ($($previous.driver) -> $driver): levels may move" }
        } else {
            $previousLine = "none of $Cycles matches on $r in $historyFile"
        }

        # evidence for a FAIL: what the heap grew by in the last matches and which DLLs own the sample blocks
        if ($problems.Count -gt 0) {
            foreach ($m in ($heaps.Count - 3)..($heaps.Count - 1)) {
                if ($blockLines.ContainsKey($m)) { $evidence += "  match $m grew by block size (bytes+count): $($blockLines[$m])" }
            }
            $owners = @()
            foreach ($m in $mid..($heaps.Count - 1)) { if ($moduleLines.ContainsKey($m)) { $owners += $moduleLines[$m].Split(' ') } }
            if ($owners.Count -gt 0) {
                $top = $owners | ForEach-Object { $_ -replace '^\+\d+:', '' } | Group-Object | Sort-Object Count -Descending | Select-Object -First 4
                $evidence += "  sample blocks of matches $mid..$($heaps.Count - 1) are owned by: " + (($top | ForEach-Object { "$($_.Name) x$($_.Count)" }) -join ', ')
            }
            $evidence += "  next: the heap per match is in $report; nvwgf2umx.dll is the NVIDIA driver pool (docs/SPECIAL_EFFECTS.md, 'Known issue'); game DLLs or TUMBU.exe point at our code; docs/HISTORY.md, 'Memory', has the past cases"
        }
    }

    $verdict = if ($problems.Count -eq 0) { 'PASS' } else { 'FAIL'; $failed = $true }
    Write-Host "== ${r}: $verdict ($([math]::Max($played, 0)) matches, driver $driver, Ogre $ogre)"
    foreach ($p in $problems) { Write-Host "  FAILED   $p" }
    if ($heapLine) { Write-Host "  heap     $heapLine" }
    if ($growthLine) { Write-Host "  growth   $growthLine" }
    if ($objectsLine) { Write-Host "  objects  $objectsLine" }
    if ($previousLine) { Write-Host "  previous $previousLine" }
    foreach ($n in $notes) { Write-Host "  $n" }
    foreach ($e in $evidence) { Write-Host $e }
    $lines | Where-Object { $_ -match 'REMINDER' } | ForEach-Object { Write-Host "  $_" }
    Write-Host "  files    report $report, game log $gameLog"
}
if ($failed) { exit 1 }
exit 0

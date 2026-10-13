# The memory check (CLAUDE.md, "Memory check"), on both renderers, judged against the expected values: N matches
# through the real UI per renderer (devtest.ps1 -Cycles N), then one verdict line per renderer. The full report of each
# run (the heap after every match, the summary, the growth per half) goes to %USERPROFILE%\Tumbu\memcheck-<renderer>.txt:
# read it when a check fails.
# Usage: .\scripts\memcheck.ps1 [-Cycles 40] [-Renderers D3D11,GL]
# Checks (the thresholds below; keep them in step with CLAUDE.md):
#   both      the run ends normally after N matches, no exception, every object count the same each match (objects=flat)
#   OpenGL    the heap flat from match 2 on: growth within +/- $GlGrowthKB KB per match
#   D3D11     the NVIDIA driver pool fills for about 18 matches, then stays: with 40 matches or more, growth over the second
#             half within $D3dSecondHalfKB KB per match and the last heap below $D3dPlateauMB MB (fewer matches: not judged)
# Exit code 1 when a check fails. About 12 minutes per renderer for 40 matches.
param(
    [int]$Cycles = 40,
    [ValidateSet('D3D11', 'GL')] [string[]]$Renderers = @('D3D11', 'GL'),
    [double]$GlGrowthKB = 30,
    [double]$D3dSecondHalfKB = 300,
    [double]$D3dPlateauMB = 100
)
$ErrorActionPreference = 'Stop'
$out = Join-Path $env:USERPROFILE 'Tumbu'
$failed = $false
foreach ($r in $Renderers) {
    $report = Join-Path $out "memcheck-$r.txt"
    # every stream (devtest.ps1 reports with Write-Host), into the report instead of the console
    $ErrorActionPreference = 'Continue'
    $lines = & (Join-Path $PSScriptRoot 'devtest.ps1') -Cycles $Cycles -Renderer $r -Name "memcheck-$r" *>&1 | ForEach-Object { "$_" }
    $ErrorActionPreference = 'Stop'
    Set-Content $report $lines

    $problems = @()
    $heaps = @()
    foreach ($l in $lines) {
        if ($l -match '^cycle\s+\d+: heap\s+(\d+) KB') { $heaps += [int]$Matches[1] }
    }
    $summary = $lines | Where-Object { $_ -match 'memory summary' } | Select-Object -Last 1
    if (-not ($lines | Where-Object { $_ -match '^exit=0' })) { $problems += 'the game did not exit normally' }
    if ($lines | Where-Object { $_ -match 'EXCEPTION|Fatal' }) { $problems += 'an exception or fatal error' }
    if ($heaps.Count -lt $Cycles + 1) { $problems += "$($heaps.Count - 1) of $Cycles matches" }
    if (-not $summary) {
        $problems += 'no memory summary'
    } elseif ($summary -notmatch 'objects=flat') {
        $problems += 'object counts CHANGED between matches'
    }
    $growth = if ($summary -match 'growth=(-?[\d.]+)KB/match') { [double]$Matches[1] } else { [double]::NaN }
    $detail = ''
    if ($heaps.Count -ge 3) {
        $detail = "after match 1 {0:0.0} MB, last {1:0.0} MB" -f ($heaps[1] / 1024), ($heaps[$heaps.Count - 1] / 1024)
        if ($r -eq 'GL') {
            $detail += ", growth {0:0.0} KB/match" -f $growth
            if ([math]::Abs($growth) -gt $GlGrowthKB) { $problems += "heap grows $([math]::Round($growth, 1)) KB/match (limit $GlGrowthKB)" }
        } elseif ($heaps.Count -ge 41) {
            # the same halves as devtest.ps1: matches mid..N
            $mid = [int][math]::Floor(($heaps.Count + 1) / 2)
            $second = ($heaps[$heaps.Count - 1] - $heaps[$mid]) / ($heaps.Count - 1 - $mid)
            $detail += ", second half {0:0} KB/match" -f $second
            if ($second -gt $D3dSecondHalfKB) { $problems += "still growing in the second half: $([math]::Round($second)) KB/match (limit $D3dSecondHalfKB)" }
            if ($heaps[$heaps.Count - 1] / 1024 -gt $D3dPlateauMB) { $problems += "plateau above $D3dPlateauMB MB" }
        } else {
            $detail += ' (under 40 matches: the driver pool is still filling, not judged)'
        }
    }
    if ($problems.Count -eq 0) {
        Write-Host "$r PASS  objects=flat, $detail"
    } else {
        $failed = $true
        Write-Host "$r FAIL  $($problems -join '; ')  ($detail; full report: $report)"
    }
    $lines | Where-Object { $_ -match 'REMINDER' } | ForEach-Object { Write-Host "  $_" }
}
if ($failed) { exit 1 }
exit 0

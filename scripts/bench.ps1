# Frame-rate benchmark: alternates variants (A, B, A, B, ...) with devtest.ps1 -Bench on each renderer and reports the
# average fps and ms per frame, the spread between runs and the difference to the first variant.
# Usage: .\scripts\bench.ps1 -Name smaa -Variants "off=-AA 0","on=-AA 1" [-Runs 3] [-Seconds 15] [-Renderers D3D11,GL]
#        [-Common "-Hero robot001 -Hour 13"] [-VideoMode 1920x1080]
#   -Variants  label=devtest.ps1 arguments; the first one is the reference
#              "@<folder>" among the arguments runs another checkout of the game (its own devtest.ps1, exe and media),
#              to compare two builds run after run: git worktree add ..\Tumbu-before <commit>, build it there, then
#              -Variants "before=@..\Tumbu-before","after="
#   -Common    devtest.ps1 arguments given to every run (the default is the standard robot view at 13:00)
#   -VideoMode the window size, set for both renderers (their ogre.cfg sections differ otherwise; the OpenGL one was
#              640x480 while Direct3D 11 had 1024x768)
# The table is printed and saved as %USERPROFILE%\Tumbu\<Name>\fps.txt (next to the before/after images).
# ogre.cfg is switched for the OpenGL runs and always restored. The game runs muted.
param(
    [Parameter(Mandatory)] [string]$Name,
    [Parameter(Mandatory)] [string[]]$Variants,
    [int]$Runs = 3,
    [double]$Seconds = 15,
    [ValidateSet('D3D11', 'GL')] [string[]]$Renderers = @('D3D11', 'GL'),
    [string]$Common = '-Hero robot001 -Hour 13',
    [string]$VideoMode = '1024x768'
)
$ErrorActionPreference = 'Stop'
$work = Join-Path $env:USERPROFILE 'Tumbu'
$cfg = Join-Path $work 'ogre.cfg'
$systems = @{ D3D11 = 'Direct3D11 Rendering Subsystem'; GL = 'OpenGL 3+ Rendering Subsystem' }

# "-AA 0 -NoWalk" -> @{ AA = '0'; NoWalk = $true }
function ConvertTo-Splat([string]$text) {
    $splat = @{}
    $tokens = @([regex]::Matches($text, '"[^"]*"|\S+') | ForEach-Object { $_.Value.Trim('"') })
    for ($i = 0; $i -lt $tokens.Count; $i++) {
        $key = $tokens[$i].TrimStart('-')
        if ($i + 1 -lt $tokens.Count -and -not $tokens[$i + 1].StartsWith('-')) { $splat[$key] = $tokens[$i + 1]; $i++ }
        else { $splat[$key] = $true }
    }
    return $splat
}

$parsed = foreach ($v in $Variants) {
    $label, $vArgs = $v.Split('=', 2)
    [pscustomobject]@{ Label = $label; Args = $vArgs }
}
$original = Get-Content $cfg -Raw
$results = @()
try {
    foreach ($renderer in $Renderers) {
        # Render system, and the same window size in both renderers' sections.
        $w, $h = $VideoMode.Split('x')
        $section = ''
        $lines = foreach ($l in Get-Content $cfg) {
            if ($l -match '^\[(.*)\]') { $section = $Matches[1] }
            if ($l -match '^Render System=') { "Render System=$($systems[$renderer])" }
            elseif ($l -match '^Video Mode=' -and $section -eq $systems.D3D11) { "Video Mode=$w x $h @ 32-bit colour" }
            elseif ($l -match '^Video Mode=' -and $section -eq $systems.GL) { "Video Mode=$w x $h" }
            else { $l }
        }
        $lines | Set-Content $cfg
        for ($run = 1; $run -le $Runs; $run++) {
            foreach ($v in $parsed) {
                $runName = "bench-$Name-$renderer-$($v.Label)-$run"
                $vArgs = "$($v.Args)"
                $devtest = Join-Path $PSScriptRoot 'devtest.ps1'
                if ($vArgs -match '@(\S+)') {
                    $devtest = Join-Path (Resolve-Path (Join-Path (Split-Path $PSScriptRoot -Parent) $Matches[1])) 'scripts\devtest.ps1'
                    $vArgs = $vArgs -replace '@\S+', ''
                }
                $splat = ConvertTo-Splat "$Common $vArgs"
                $splat.Bench = $Seconds
                $splat.Name = $runName
                $fpsFile = Join-Path $work "devtest-$runName.fps"
                # A run whose window was hidden or minimised did not render (DevTest marks it invalid): run it again.
                for ($attempt = 1; $attempt -le 3; $attempt++) {
                    & $devtest @splat | Out-Null
                    $line = if (Test-Path $fpsFile) { Get-Content $fpsFile -Raw } else { '' }
                    if ($line -notmatch 'invalid=') { break }
                    Write-Warning "$runName : $($Matches[0]) the window did not render (hidden or minimised?), attempt $attempt of 3"
                }
                if ($line -match 'invalid=') { continue }
                $m = [regex]::Match($line, 'avg=([\d.]+) ms=([\d.]+)')
                if (-not $m.Success -or -not $line.Contains("render=$($systems[$renderer] -replace ' ', '_')")) {
                    Write-Warning "$runName : no bench result on $renderer ($line)"
                    continue
                }
                if (-not $line.Contains("size=$VideoMode")) { Write-Warning "$runName : the window was not $VideoMode ($line)" }
                $fps = [double]::Parse($m.Groups[1].Value, [cultureinfo]::InvariantCulture)
                $ms = [double]::Parse($m.Groups[2].Value, [cultureinfo]::InvariantCulture)
                Write-Host ("{0,-6} {1,-12} run {2}: {3,7:0.0} fps  {4,6:0.000} ms" -f $renderer, $v.Label, $run, $fps, $ms)
                $results += [pscustomobject]@{ Renderer = $renderer; Variant = $v.Label; Run = $run; Fps = $fps; Ms = $ms }
            }
        }
    }
} finally {
    Set-Content $cfg $original -NoNewline
}

$inv = [cultureinfo]::InvariantCulture
$lines = @("Frame-rate benchmark '$Name' ($(Get-Date -Format 'yyyy-MM-dd HH:mm')), $Runs runs x $Seconds s per variant, alternated",
    "common: $Common; variants: $($Variants -join ' | ')", '')
foreach ($renderer in $Renderers) {
    $reference = $null
    foreach ($v in $parsed) {
        $set = @($results | Where-Object { $_.Renderer -eq $renderer -and $_.Variant -eq $v.Label })
        if ($set.Count -eq 0) { continue }
        $ms = ($set | Measure-Object Ms -Average).Average
        $fps = 1000 / $ms
        $spread = (($set | Measure-Object Ms -Maximum).Maximum - ($set | Measure-Object Ms -Minimum).Minimum)
        $runsText = ($set | ForEach-Object { $_.Fps.ToString('0', $inv) }) -join ' / '
        $delta = ''
        if ($null -eq $reference) { $reference = $ms }
        else {
            $d = $ms - $reference
            $noise = if ([math]::Abs($d) -le $spread) { ', inside the run-to-run spread' } else { '' }
            $delta = [string]::Format($inv, '  {0:+0.000;-0.000} ms ({1:+0.0;-0.0} % fps){2}', $d, 100 * ($reference / $ms - 1), $noise)
        }
        $lines += [string]::Format($inv, '{0,-6} {1,-12} {2,7:0.0} fps {3,7:0.000} ms  (runs {4} fps; spread {5:0.000} ms){6}',
            $renderer, $v.Label, $fps, $ms, $runsText, $spread, $delta)
    }
}
$dir = Join-Path $work $Name
New-Item -ItemType Directory -Force $dir | Out-Null
$lines | Set-Content (Join-Path $dir 'fps.txt')

# Every run also goes to fps-history.csv, to follow the frame rate of the same view across changes.
$history = Join-Path $work 'fps-history.csv'
if (-not (Test-Path $history)) { 'date,name,renderer,variant,video_mode,run,fps,ms,common,args' | Set-Content $history }
$date = Get-Date -Format 'yyyy-MM-dd HH:mm'
foreach ($r in $results) {
    $vArgs = ($parsed | Where-Object { $_.Label -eq $r.Variant }).Args
    [string]::Format($inv, '{0},{1},{2},{3},{4},{5},{6:0.0},{7:0.0000},"{8}","{9}"', $date, $Name, $r.Renderer, $r.Variant, $VideoMode,
        $r.Run, $r.Fps, $r.Ms, ($Common -replace '"', "'"), $vArgs) | Add-Content $history
}
''; $lines

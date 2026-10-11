# Frame-rate benchmark: alternates variants with devtest.ps1 -Bench on each renderer and reports the average fps and ms
# per frame, the spread between runs and the difference to the first variant with its uncertainty.
# Usage: .\scripts\bench.ps1 -Name smaa -Variants "off=-AA 0","on=-AA 1" [-Runs 3] [-MaxRuns 6] [-Seconds 8]
#        [-Renderers D3D11,GL] [-Common "-Hero robot001 -Hour 13"] [-VideoMode 1920x1080]
#   -Variants  label=devtest.ps1 arguments; the first one is the reference
#              "@<folder>" among the arguments runs another checkout of the game (its own devtest.ps1, exe and media),
#              to compare two builds run after run: git worktree add bin\scratch\before <commit>, build it there, then
#              -Variants "before=@bin\scratch\before","after="
#              Call it from PowerShell (or powershell -Command): through powershell -File the list arrives as one string.
#   -Common    devtest.ps1 arguments given to every run (the default is the standard robot view at 13:00)
#   -VideoMode the window size, set for both renderers (their ogre.cfg sections differ otherwise; the OpenGL one was
#              640x480 while Direct3D 11 had 1024x768)
#   -Runs      rounds always run (each round runs every variant once); -MaxRuns the most rounds: after -Runs rounds it
#              stops as soon as every variant's difference to the first is clear (larger than 3 times its standard
#              error) or known to within -Resolution ms (2 standard errors), otherwise it adds rounds.
#   -Quick     for light changes: 1920x1080 and exactly 2 rounds; -Runs / -MaxRuns / -VideoMode given explicitly still win.
# How the runs are ordered (measured 2026-10-11 with two identical builds, 1920x1080):
#   - one warm-up run per renderer is thrown away: the first run of a session was always the fastest (cold GPU);
#   - the rounds alternate the order (A B, B A, A B, ...): Direct3D 11 slows down by several % over a session (the
#     GPU warming up), and a fixed order made the second variant look ~0.02 ms slower;
#   - the difference is taken round by round (each variant against the reference run next to it), so the slow drift
#     cancels out; "+/-" is 2 standard errors of that mean.
# The table is printed and saved as %USERPROFILE%\Tumbu\<Name>\fps.txt (next to the before/after images). A note follows
# a renderer's rows when a variant's own runs spread by more than 10 % of its frame time: something else was probably
# using the PC. ogre.cfg is switched for the runs and always restored. The game runs muted, without the focus.
param(
    [Parameter(Mandatory)] [string]$Name,
    [Parameter(Mandatory)] [string[]]$Variants,
    [int]$Runs = 3,
    [int]$MaxRuns = 6,
    [double]$Seconds = 8,
    [double]$Resolution = 0.02,
    [ValidateSet('D3D11', 'GL')] [string[]]$Renderers = @('D3D11', 'GL'),
    [string]$Common = '-Hero robot001 -Hour 13',
    [string]$VideoMode = '1024x768',
    [switch]$Quick,
    [switch]$NoWarmup
)
$ErrorActionPreference = 'Stop'
if ($Quick) {
    if (-not $PSBoundParameters.ContainsKey('Runs')) { $Runs = 2 }
    if (-not $PSBoundParameters.ContainsKey('MaxRuns')) { $MaxRuns = $Runs }
    if (-not $PSBoundParameters.ContainsKey('VideoMode')) { $VideoMode = '1920x1080' }
}
$MaxRuns = [math]::Max($MaxRuns, $Runs)
$work = Join-Path $env:USERPROFILE 'Tumbu'
$cfg = Join-Path $work 'ogre.cfg'
$systems = @{ D3D11 = 'Direct3D11 Rendering Subsystem'; GL = 'OpenGL 3+ Rendering Subsystem' }
$inv = [cultureinfo]::InvariantCulture

# "-AA 0 -NoWalk" -> @{ AA = '0'; NoWalk = $true }
function ConvertTo-Splat([string]$text) {
    $splat = @{}
    $tokens = @([regex]::Matches($text, '"[^"]*"|\S+') | ForEach-Object { $_.Value.Trim('"') })
    for ($i = 0; $i -lt $tokens.Count; $i++) {
        $key = $tokens[$i].TrimStart('-')
        # a value may start with '-' when it is a number ("-Camera -18.5,1,9,..."): only "-Name" is a new switch
        if ($i + 1 -lt $tokens.Count -and $tokens[$i + 1] -notmatch '^-[A-Za-z]') { $splat[$key] = $tokens[$i + 1]; $i++ }
        else { $splat[$key] = $true }
    }
    return $splat
}

# One bench run of a variant; returns its fps line, or $null when it gave no valid result.
function Invoke-BenchRun($v, [string]$renderer, [string]$runName) {
    $vArgs = "$($v.Args)"
    $devtest = Join-Path $PSScriptRoot 'devtest.ps1'
    if ($vArgs -match '@(\S+)') {
        $devtest = Join-Path (Resolve-Path (Join-Path (Split-Path $PSScriptRoot -Parent) $Matches[1])) 'scripts\devtest.ps1'
        $vArgs = $vArgs -replace '@\S+', ''
    }
    $splat = ConvertTo-Splat "$Common $vArgs"
    $splat.Bench = $Seconds
    # devtest.ps1 sets the window size itself now (default 1024x768); older checkouts (@folder) do not know it.
    if ((Get-Command $devtest).Parameters.ContainsKey('VideoMode')) { $splat.VideoMode = $VideoMode }
    $splat.Name = $runName
    $fpsFile = Join-Path $work "devtest-$runName.fps"
    # A run whose window was hidden or minimised did not render (DevTest marks it invalid): run it again.
    for ($attempt = 1; $attempt -le 3; $attempt++) {
        & $devtest @splat | Out-Null
        $line = if (Test-Path $fpsFile) { Get-Content $fpsFile -Raw } else { '' }
        if ($line -notmatch 'invalid=') { break }
        Write-Warning "$runName : $($Matches[0]) the window did not render (hidden or minimised?), attempt $attempt of 3"
    }
    if ($line -match 'invalid=') { return $null }
    if ($line -notmatch 'avg=[\d.]+ ms=[\d.]+' -or -not $line.Contains("render=$($systems[$renderer] -replace ' ', '_')")) {
        Write-Warning "$runName : no bench result on $renderer ($line)"
        return $null
    }
    if (-not $line.Contains("size=$VideoMode")) { Write-Warning "$runName : the window was not $VideoMode ($line)" }
    return $line
}

function Get-Number([string]$line, [string]$key) {
    $m = [regex]::Match($line, "$key=([\d.]+)")
    if ($m.Success) { return [double]::Parse($m.Groups[1].Value, $inv) } else { return 0 }
}

# The difference of a variant to the reference on one renderer, from the rounds that have both: mean and standard error.
function Get-Difference([string]$renderer, [string]$label, [string]$reference) {
    $diffs = @()
    foreach ($r in ($results | Where-Object { $_.Renderer -eq $renderer -and $_.Variant -eq $label })) {
        $ref = $results | Where-Object { $_.Renderer -eq $renderer -and $_.Variant -eq $reference -and $_.Run -eq $r.Run }
        if ($ref) { $diffs += $r.Ms - $ref.Ms }
    }
    if ($diffs.Count -eq 0) { return $null }
    $mean = ($diffs | Measure-Object -Average).Average
    $se = [double]::PositiveInfinity
    if ($diffs.Count -gt 1) {
        $var = (($diffs | ForEach-Object { ($_ - $mean) * ($_ - $mean) }) | Measure-Object -Sum).Sum / ($diffs.Count - 1)
        $se = [math]::Sqrt($var / $diffs.Count)
    }
    return [pscustomobject]@{ Mean = $mean; Se = $se; Count = $diffs.Count }
}

$parsed = foreach ($v in $Variants) {
    $label, $vArgs = $v.Split('=', 2)
    [pscustomobject]@{ Label = $label; Args = $vArgs }
}
if ($parsed.Count -lt 2 -and $Variants[0] -match '=.*,\w+=') {
    Write-Warning "One variant '$($Variants[0])': was the list passed through powershell -File? Call bench.ps1 from PowerShell."
}
# The game windows do not take the keyboard focus (SDL reads the hint from the environment): devtest.ps1 passes -nofocus,
# and older trees (@folder) inherit this.
$env:SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN = '1'
$original = Get-Content $cfg -Raw
$results = @()
$rounds = @{}
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
        if (-not $NoWarmup) {
            $null = Invoke-BenchRun $parsed[0] $renderer "bench-$Name-$renderer-warmup"
            Write-Host "$renderer warm-up run done (not counted)"
        }
        for ($run = 1; $run -le $MaxRuns; $run++) {
            # A B, B A, A B, ...: a slow drift over the session then weighs on every variant alike.
            $order = @($parsed)
            if ($run % 2 -eq 0) { [array]::Reverse($order) }
            foreach ($v in $order) {
                $runName = "bench-$Name-$renderer-$($v.Label)-$run"
                $line = Invoke-BenchRun $v $renderer $runName
                if (-not $line) { continue }
                $fps = Get-Number $line 'avg'
                $ms = Get-Number $line 'ms'
                Write-Host ([string]::Format($inv, "{0,-6} {1,-12} run {2}: {3,7:0.0} fps  {4,6:0.000} ms", $renderer, $v.Label, $run, $fps, $ms))
                $results += [pscustomobject]@{ Renderer = $renderer; Variant = $v.Label; Run = $run; Fps = $fps; Ms = $ms }
            }
            $rounds[$renderer] = $run
            if ($run -lt $Runs -or $run -eq $MaxRuns) { continue }
            # Stop when every difference is clear or known well enough.
            $open = @($parsed | Select-Object -Skip 1 | Where-Object {
                $d = Get-Difference $renderer $_.Label $parsed[0].Label
                -not $d -or -not ([math]::Abs($d.Mean) -gt 3 * $d.Se -or 2 * $d.Se -le $Resolution)
            })
            if ($open.Count -eq 0) { break }
            Write-Host "$renderer : $(($open | ForEach-Object { $_.Label }) -join ', ') not settled after $run rounds, one more"
        }
    }
} finally {
    Set-Content $cfg $original -NoNewline
}

$lines = @("Frame-rate benchmark '$Name' ($(Get-Date -Format 'yyyy-MM-dd HH:mm')), $Seconds s per run, rounds alternated (A B, B A, ...)",
    "common: $Common; variants: $($Variants -join ' | '); size $VideoMode", '')
foreach ($renderer in $Renderers) {
    $noisy = @()
    foreach ($v in $parsed) {
        $set = @($results | Where-Object { $_.Renderer -eq $renderer -and $_.Variant -eq $v.Label })
        if ($set.Count -eq 0) { continue }
        $ms = ($set | Measure-Object Ms -Average).Average
        $fps = 1000 / $ms
        $spread = (($set | Measure-Object Ms -Maximum).Maximum - ($set | Measure-Object Ms -Minimum).Minimum)
        $runsText = ($set | ForEach-Object { $_.Fps.ToString('0', $inv) }) -join ' / '
        $delta = ''
        if ($v -ne $parsed[0]) {
            $d = Get-Difference $renderer $v.Label $parsed[0].Label
            if ($d) {
                $sig = if ($d.Count -lt 2) { ', one round only' }
                       elseif ([math]::Abs($d.Mean) -le 2 * $d.Se) { ', not significant' } else { '' }
                $pm = if ($d.Count -gt 1) { [string]::Format($inv, ' +/- {0:0.000}', 2 * $d.Se) } else { '' }
                $delta = [string]::Format($inv, '  {0:+0.000;-0.000}{1} ms ({2:+0.0;-0.0} % fps){3}', $d.Mean, $pm,
                    100 * (1 / (1 + $d.Mean / ($ms - $d.Mean)) - 1), $sig)
            }
        }
        $lines += [string]::Format($inv, '{0,-6} {1,-12} {2,7:0.0} fps {3,7:0.000} ms  (runs {4} fps; spread {5:0.000} ms){6}',
            $renderer, $v.Label, $fps, $ms, $runsText, $spread, $delta)
        if ($set.Count -gt 1 -and $spread -gt 0.1 * $ms) {
            $noisy += [string]::Format($inv, '{0,-6} note: the runs of {1} spread by {2:0} % of its frame time (other load on the PC?): run again',
                $renderer, $v.Label, 100 * $spread / $ms)
        }
    }
    if ($rounds[$renderer]) { $lines += "$renderer  $($rounds[$renderer]) rounds" }
    $lines += $noisy
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

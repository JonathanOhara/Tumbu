# Runs TUMBU unattended with the developer test switches and prints the [DEVTEST] log lines.
# Usage: .\scripts\devtest.ps1 [-Configuration Release] [-FpsCap 60] [-QuitAfter 8] [-NoWalk] [-Hour 22] [-Name run] [-Sound] [-FaceShot [-Jyn]] [-JynWalk] [-FxTest name [-FxTime 0.3] [-FxDistance 4]] [-JynHit 6] [-Camera "x,y,z,tx,ty,tz"] [-Hero robot005] [-AA 0|1] [-Sky 0|1] [-Bench S [-BenchAI] [-BenchChase]] [-NoFx]
# The screenshot and log copies are left in %USERPROFILE%\Tumbu\devtest-<Name>.png / .log
# The game runs muted (-mute) unless -Sound is given.
# It ends with the frame rate of the run ("fps: avg=... min=... max=..."; VSync is off under DevTest), also saved as
# devtest-<Name>.fps, which compare.ps1 prints under each image.
param(
    [ValidateSet('Release', 'RelWithDebInfo')] [string]$Configuration = 'Release',
    [int]$FpsCap = 0,
    [int]$Hour = -1,
    [double]$QuitAfter = 8,
    [switch]$NoWalk,
    [switch]$Sound,
    [switch]$FaceShot,
    [switch]$Jyn,
    [switch]$JynWalk,
    [string]$FxTest = "",
    [double]$FxTime = 0.3,
    [double]$FxDistance = 0,
    [double]$JynHit = 0,
    [string]$Camera = '',
    [string]$Hero = "",
    [int]$AA = -1,
    [int]$Sky = -1,
    [double]$Bench = 0,
    [switch]$NoFx,
    [switch]$BenchAI,
    [switch]$BenchChase,
    [string]$Name = 'run',
    [int]$TimeoutSeconds = 180
)
$ErrorActionPreference = 'Stop'
$root   = Split-Path $PSScriptRoot -Parent
$binDir = Join-Path $root "bin\$Configuration"
$work   = Join-Path $env:USERPROFILE 'Tumbu'
$exe    = Join-Path $binDir 'TUMBU.exe'
if (-not (Test-Path $exe)) { throw "$exe not found. Build first (.\scripts\build.ps1)." }

$gameArgs = @("-quitafter=$QuitAfter")
if (-not $NoWalk -and $Bench -le 0) { $gameArgs += '-walktest' }
if ($FpsCap -gt 0) { $gameArgs += "-fpscap=$FpsCap" }
if ($Hour -ge 0)   { $gameArgs += "-hour=$Hour" }
if (-not $Sound)  { $gameArgs += '-mute' }
if ($FaceShot)    { $gameArgs += $(if ($Jyn) { '-faceshot=jyn' } else { '-faceshot' }) }
if ($JynWalk)     { $gameArgs += "-jynwalk" }
if ($JynHit -gt 0) { $gameArgs += "-jynhit=" + $JynHit.ToString([cultureinfo]::InvariantCulture) }
if ($FxTest)      { $gameArgs += "-fxtest=$FxTest" }
if ($FxTest -or $JynHit -gt 0) { $gameArgs += "-fxtime=" + $FxTime.ToString([cultureinfo]::InvariantCulture) }
if ($FxDistance -gt 0) { $gameArgs += "-fxdistance=" + $FxDistance.ToString([cultureinfo]::InvariantCulture) }
if ($Camera)      { $gameArgs += "-camera=$Camera" }
if ($Hero)        { $gameArgs += "-hero=$Hero" }
if ($AA -ge 0)     { $gameArgs += "-aa=$AA" }
if ($Sky -ge 0)    { $gameArgs += "-sky=$Sky" }
if ($NoFx)        { $gameArgs += '-nofx' }
if ($BenchAI)     { $gameArgs += '-benchai' }
if ($BenchChase)  { $gameArgs += '-benchchase' }
if ($Bench -gt 0)  { $gameArgs += "-bench=" + $Bench.ToString([cultureinfo]::InvariantCulture) }

Remove-Item "$work\devtest.png" -ErrorAction SilentlyContinue
$proc = Start-Process $exe -ArgumentList $gameArgs -WorkingDirectory $binDir -PassThru
if (-not $proc.WaitForExit($TimeoutSeconds * 1000)) {
    Stop-Process $proc -Force
    Write-Warning "Timed out after $TimeoutSeconds s; process killed."
}

Copy-Item "$work\ogre.log" "$work\devtest-$Name.log" -Force
if (Test-Path "$work\devtest.png") { Copy-Item "$work\devtest.png" "$work\devtest-$Name.png" -Force }
Write-Host "exit=$($proc.ExitCode)  log=$work\devtest-$Name.log  screenshot=$(Test-Path "$work\devtest-$Name.png")"
Select-String "$work\devtest-$Name.log" -Pattern '\[DEVTEST\]|EXCEPTION|Error|Fatal' |
    Where-Object { $_.Line -notmatch 'white\.png|city_6_|sample\.fontdef' } | ForEach-Object { $_.Line }

# Frame rate of the run: the [DEVTEST] fps= samples (every 0.5 s) after a 2 s warm-up. Saved next to the screenshot
# (devtest-<Name>.fps), where compare.ps1 picks it up for its labels.
$fps = Select-String "$work\devtest-$Name.log" -Pattern '\[DEVTEST\] t=([\d.]+) fps=([\d.]+)' |
    Where-Object { [double]::Parse($_.Matches[0].Groups[1].Value, [cultureinfo]::InvariantCulture) -ge 2 } |
    ForEach-Object { [double]::Parse($_.Matches[0].Groups[2].Value, [cultureinfo]::InvariantCulture) }
Remove-Item "$work\devtest-$Name.fps" -ErrorAction SilentlyContinue
if ($fps) {
    $m = $fps | Measure-Object -Average -Minimum -Maximum
    $cap = if ($FpsCap -gt 0) { " capped=$FpsCap" } else { '' }
    $summary = "avg={0:0} min={1:0} max={2:0} samples={3}{4}" -f $m.Average, $m.Minimum, $m.Maximum, $m.Count, $cap
    Set-Content "$work\devtest-$Name.fps" $summary
    Write-Host "fps: $summary"
}
# -Bench: the steady measurement replaces the summary (fps over the measured seconds, no AI, fixed camera).
$benchLine = Select-String "$work\devtest-$Name.log" -Pattern '\[DEVTEST\] bench: (.*)$' | Select-Object -Last 1
if ($benchLine) {
    $summary = $benchLine.Matches[0].Groups[1].Value -replace '^fps=', 'avg='
    Set-Content "$work\devtest-$Name.fps" $summary
    Write-Host "bench: $summary"
}

# Runs TUMBU unattended with the developer test switches and prints the [DEVTEST] log lines.
# Usage: .\scripts\devtest.ps1 [-Configuration Release] [-FpsCap 60] [-QuitAfter 8] [-NoWalk] [-Hour 22] [-Name run] [-Sound] [-FaceShot [-Jyn]] [-JynWalk] [-FxTest name [-FxTime 0.3] [-FxDistance 4]] [-JynHit 6] [-Camera "x,y,z,tx,ty,tz"] [-Hero robot005] [-AA 0|1] [-Sky 0|1] [-Bench S [-BenchAI] [-BenchChase]] [-NoFx] [-Renderer D3D11|GL]
#        .\scripts\devtest.ps1 -Cycles 40 [-Renderer GL] [-Name mem]   # memory check (CLAUDE.md, -cycles)
# The screenshot and log copies are left in %USERPROFILE%\Tumbu\devtest-<Name>.png / .log
# -Renderer D3D11 or GL runs this one game with that render system (ogre.cfg is switched for the run and always
# restored); without it the game uses whatever ogre.cfg says.
# -Cycles N runs the memory check instead (TUMBU.exe -cycles=N: N matches through the real UI) and prints a short
# report: the heap after each match, the summary line, and for 20 matches or more the heap growth per match in the
# first and the second half (the Direct3D 11 driver pool fills over ~18 matches, then the second half is near 0).
# The game runs muted (-mute) unless -Sound is given.
# -Clean moves the old per-run files (devtest-*.png/.log/.fps, also the bench runs') to the Recycle Bin first; given
# alone (.\scripts\devtest.ps1 -Clean) it only cleans.
# It ends with the frame rate of the run ("fps: avg=... min=... max=..."; VSync is off under DevTest), also saved as
# devtest-<Name>.fps, which compare.ps1 prints under each image.
param(
    [ValidateSet('Release', 'RelWithDebInfo')] [string]$Configuration = 'Release',
    [int]$FpsCap = 0,
    [double]$Hour = -1,
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
    [ValidateSet('', 'D3D11', 'GL')] [string]$Renderer = '',
    [int]$Cycles = 0,
    [switch]$Clean,
    [int]$TimeoutSeconds = 180
)
$ErrorActionPreference = 'Stop'
$root   = Split-Path $PSScriptRoot -Parent
$binDir = Join-Path $root "bin\$Configuration"
$work   = Join-Path $env:USERPROFILE 'Tumbu'
$exe    = Join-Path $binDir 'TUMBU.exe'

if ($Clean) {
    Add-Type -AssemblyName Microsoft.VisualBasic
    $old = @(Get-ChildItem $work -File -Filter 'devtest-*' -ErrorAction SilentlyContinue)
    foreach ($f in $old) {
        [Microsoft.VisualBasic.FileIO.FileSystem]::DeleteFile($f.FullName, 'OnlyErrorDialogs', 'SendToRecycleBin')
    }
    Write-Host "clean: $($old.Count) old devtest files moved to the Recycle Bin"
    if ($PSBoundParameters.Count -eq 1) { return }
}
if (-not (Test-Path $exe)) { throw "$exe not found. Build first (.\scripts\build.ps1)." }

$gameArgs = @("-quitafter=$QuitAfter")
if (-not $NoWalk -and $Bench -le 0) { $gameArgs += '-walktest' }
if ($FpsCap -gt 0) { $gameArgs += "-fpscap=$FpsCap" }
if ($Hour -ge 0)   { $gameArgs += "-hour=" + $Hour.ToString([cultureinfo]::InvariantCulture) }    # fractional: 20.5 = 20:30
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
if ($Cycles -gt 0) {
    # The memory check drives the game itself (menu, match, kills, back to the menu) and quits when done.
    $gameArgs = @("-cycles=$Cycles")
    if (-not $Sound) { $gameArgs += '-mute' }
    $TimeoutSeconds = [math]::Max($TimeoutSeconds, 30 * $Cycles + 120)
}

Remove-Item "$work\devtest.png" -ErrorAction SilentlyContinue
$cfgFile = Join-Path $work 'ogre.cfg'
$cfgSaved = $null
if ($Renderer) {
    $systems = @{ D3D11 = 'Direct3D11 Rendering Subsystem'; GL = 'OpenGL 3+ Rendering Subsystem' }
    $cfgSaved = [IO.File]::ReadAllText($cfgFile)
    (Get-Content $cfgFile) -replace '^Render System=.*', "Render System=$($systems[$Renderer])" | Set-Content $cfgFile
}
try {
    $proc = Start-Process $exe -ArgumentList $gameArgs -WorkingDirectory $binDir -PassThru
    $finished = $proc.WaitForExit($TimeoutSeconds * 1000)
} finally {
    if ($null -ne $cfgSaved) { [IO.File]::WriteAllText($cfgFile, $cfgSaved) }
}
if (-not $finished) {
    Stop-Process $proc -Force
    Write-Warning "Timed out after $TimeoutSeconds s; process killed."
}

Copy-Item "$work\ogre.log" "$work\devtest-$Name.log" -Force
if (Test-Path "$work\devtest.png") { Copy-Item "$work\devtest.png" "$work\devtest-$Name.png" -Force }
if ($Cycles -gt 0) {
    Write-Host "exit=$($proc.ExitCode)  log=$work\devtest-$Name.log"
    $heaps = @()
    foreach ($l in Select-String "$work\devtest-$Name.log" -Pattern '\[DEVTEST\] memory cycle (\d+) menu private=(\d+)KB heap=(\d+)KB') {
        $g = $l.Matches[0].Groups
        $heaps += [int]$g[3].Value
        Write-Host ("cycle {0,3}: heap {1,7} KB  private {2,8} KB" -f $g[1].Value, $g[3].Value, $g[2].Value)
    }
    Select-String "$work\devtest-$Name.log" -Pattern 'memory summary|REMINDER|EXCEPTION|Fatal' |
        Where-Object { $_.Line -notmatch 'Information Queue' } | ForEach-Object { $_.Line }
    # heaps[0] is the menu before any match; the growth is measured from match 2, as in the summary.
    if ($heaps.Count -ge 21) {
        $mid = [int][math]::Floor(($heaps.Count + 1) / 2)
        $first = ($heaps[$mid] - $heaps[2]) / ($mid - 2)
        $second = ($heaps[$heaps.Count - 1] - $heaps[$mid]) / ($heaps.Count - 1 - $mid)
        Write-Host ("heap growth per match: matches 2..{0} {1:0} KB, matches {0}..{2} {3:0} KB (near 0 = it has levelled off)" -f
            $mid, $first, ($heaps.Count - 1), $second)
    }
    return
}
Write-Host "exit=$($proc.ExitCode)  log=$work\devtest-$Name.log  screenshot=$(Test-Path "$work\devtest-$Name.png")"
Select-String "$work\devtest-$Name.log" -Pattern '\[DEVTEST\]|EXCEPTION|Error|Fatal' |
    # Known and harmless: the 2011 terrain page leaves one chunk open when Ogre reads it, and old resource lookups.
    Where-Object { $_.Line -notmatch 'white\.png|city_6_|sample\.fontdef|was not fully read|Information Queue' } | ForEach-Object { $_.Line }

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

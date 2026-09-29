# Runs TUMBU unattended with the developer test switches and prints the [DEVTEST] log lines.
# Usage: .\scripts\devtest.ps1 [-Configuration Release] [-FpsCap 60] [-QuitAfter 8] [-NoWalk] [-Hour 22] [-Name run]
# The screenshot and log copies are left in %USERPROFILE%\Tumbu\devtest-<Name>.png / .log
param(
    [ValidateSet('Release', 'RelWithDebInfo')] [string]$Configuration = 'Release',
    [int]$FpsCap = 0,
    [int]$Hour = -1,
    [double]$QuitAfter = 8,
    [switch]$NoWalk,
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
if (-not $NoWalk)  { $gameArgs += '-walktest' }
if ($FpsCap -gt 0) { $gameArgs += "-fpscap=$FpsCap" }
if ($Hour -ge 0)   { $gameArgs += "-hour=$Hour" }

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

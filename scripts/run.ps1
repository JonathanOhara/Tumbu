# Launches TUMBU from bin\<Configuration> (the build copies every runtime file there).
# Usage: .\scripts\run.ps1 [-Configuration Release|RelWithDebInfo] [-Wait] [-Mute] [-- extra game switches]
param(
    [ValidateSet('Release', 'RelWithDebInfo')] [string]$Configuration = 'Release',
    [switch]$Wait,
    [switch]$Mute,
    [Parameter(ValueFromRemainingArguments = $true)] [string[]]$GameArgs
)
$ErrorActionPreference = 'Stop'
$root   = Split-Path $PSScriptRoot -Parent
$binDir = Join-Path $root "bin\$Configuration"
$exe    = Join-Path $binDir 'TUMBU.exe'
if (-not (Test-Path $exe)) { throw "$exe not found. Run .\scripts\build.ps1 -Configuration $Configuration first." }

if ($Mute) { $GameArgs = @('-mute') + $GameArgs }
$startArgs = @{ FilePath = $exe; WorkingDirectory = $binDir; PassThru = $true }
if ($GameArgs) { $startArgs.ArgumentList = $GameArgs }
$proc = Start-Process @startArgs
Write-Host "Started TUMBU ($Configuration), PID $($proc.Id). Logs: $env:USERPROFILE\Tumbu\ogre.log"
if ($Wait) { $proc.WaitForExit(); Write-Host "Exited with code $($proc.ExitCode)" }

# Prepares bin\<Configuration> with runtime DLLs/configs and launches TUMBU.
# Usage: .\scripts\run.ps1 [-Configuration Release|Debug] [-Wait]
param(
    [ValidateSet('Release', 'Debug')] [string]$Configuration = 'Release',
    [switch]$Wait,
    [switch]$NoLaunch
)
$ErrorActionPreference = 'Stop'
$root   = Split-Path $PSScriptRoot -Parent
$binDir = Join-Path $root "bin\$Configuration"
$exe    = Join-Path $binDir 'TUMBU.exe'
if (-not (Test-Path $exe)) { throw "$exe not found. Run .\scripts\build.ps1 -Configuration $Configuration first." }

# Runtime DLLs: the version-matched set from the 2011 build (same Ogre 1.7.3 / CEGUI 0.7.5 / OgreAL / SkyX builds
# we link against). Override with $env:TUMBU_RUNTIME_DIR if they move.
$runtimeDir = if ($env:TUMBU_RUNTIME_DIR) { $env:TUMBU_RUNTIME_DIR } else { "E:\WorkSpaces\workspace tcc\Tumbu\bin\$Configuration" }
if (-not (Test-Path $runtimeDir)) { throw "Runtime DLL folder $runtimeDir not found." }
Get-ChildItem $runtimeDir -Filter *.dll | Where-Object { -not (Test-Path (Join-Path $binDir $_.Name)) } |
    Copy-Item -Destination $binDir

# Configs: plugins cfg from the runtime set; resource cfg always refreshed from the repo root tumbu.cfg.
$suffix = if ($Configuration -eq 'Debug') { '_d' } else { '' }
if (-not (Test-Path "$binDir\plugins$suffix.cfg")) { Copy-Item "$runtimeDir\plugins$suffix.cfg" $binDir }
Copy-Item "$root\tumbu.cfg" "$binDir\tumbu$suffix.cfg" -Force

# Per-user folder for ogre.cfg / ogre.log / cegui.log (the game throws without it).
New-Item -ItemType Directory -Force "$env:USERPROFILE\Tumbu" | Out-Null
if ($NoLaunch) { return }

$proc = Start-Process $exe -WorkingDirectory $binDir -PassThru
Write-Host "Started TUMBU ($Configuration), PID $($proc.Id). Logs: $env:USERPROFILE\Tumbu\ogre.log"
if ($Wait) { $proc.WaitForExit(); Write-Host "Exited with code $($proc.ExitCode)" }

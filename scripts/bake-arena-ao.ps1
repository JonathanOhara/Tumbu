# Bakes the arena's ambient occlusion and exports its meshes (scripts/blender/arena_ao.py).
# Usage: .\scripts\bake-arena-ao.ps1 [-Rebuild] [-Only <mesh>] [-ExportOnly] [-Blender <path to blender.exe>]
#   -Only torches: bakes and exports only that mesh of the Export collection (the others keep their AO and .mesh).
#   -ExportOnly: exports the meshes without baking the AO again (the AO maps on disk stay as they are).
#   First run (or -Rebuild): creates art/arena/Arena.blend from the 2011 coliseum.blend and gym.blend.
#   Every run: bakes media/tumbu/arena/<mesh>_ao.png and exports media/tumbu/arena/<mesh>.mesh.
# Needs blender2ogre installed in that Blender (docs/DEV_SETUP.md, Part B).
param(
    [switch]$Rebuild,
    [string]$Only = '',
    [switch]$ExportOnly,
    [string]$Blender = 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path $Blender)) { throw "Blender not found: $Blender (pass -Blender)" }

$scriptArgs = @('--background', '--python', (Join-Path $PSScriptRoot 'blender\arena_ao.py'), '--', $root)
if ($Rebuild) { $scriptArgs += '--rebuild' }
if ($Only) { $scriptArgs += @('--only', $Only) }
if ($ExportOnly) { $scriptArgs += '--export-only' }

# Blender prints to stderr; Windows PowerShell 5.1 would turn that into errors under 'Stop' (the exit code decides).
$ErrorActionPreference = 'Continue'
& $Blender @scriptArgs 2>&1 | ForEach-Object {
    $line = "$_"
    if ($line -match '^\[AO\]|Error|Traceback|^\s+File ') { Write-Host $line }
}
$ErrorActionPreference = 'Stop'
if ($LASTEXITCODE -ne 0) { throw "Blender failed (exit $LASTEXITCODE)" }

# The game uses compressed copies (DDS) of these PNGs.
& (Join-Path $PSScriptRoot 'compress-arena-textures.ps1')

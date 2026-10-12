# The arena's field (scripts/blender/arena_field.py): builds the "field" object of art/arena/Arena.blend from the 2011
# terrain page the first time, then bakes its AO and exports media/tumbu/arena/field.mesh (scripts/bake-arena-ao.ps1).
# Usage: .\scripts\arena-field.ps1 [-Rebuild] [-NewMap] [-NoBake] [-Blender <path to blender.exe>]
#   -Rebuild  builds the field again from the 2011 page: DISCARDS hand edits of the field and of field_map.png
#   -NewMap   writes field_map.png again from the rules in arena_field.py: DISCARDS painting in it
#   -NoBake   only the Blender step (no AO bake, no export)
# After editing the field by hand in Blender, only the bake and export are needed: .\scripts\bake-arena-ao.ps1 -Only field
param(
    [switch]$Rebuild,
    [switch]$NewMap,
    [switch]$NoBake,
    [string]$Blender = 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path $Blender)) { throw "Blender not found: $Blender (pass -Blender)" }

$scriptArgs = @('--background', (Join-Path $root 'art\arena\Arena.blend'), '--python-exit-code', '1', '--python', (Join-Path $PSScriptRoot 'blender\arena_field.py'), '--', $root)
if ($Rebuild) { $scriptArgs += '--rebuild' }
if ($NewMap) { $scriptArgs += '--new-map' }

# Blender prints to stderr; Windows PowerShell 5.1 would turn that into errors under 'Stop' (the exit code decides).
$ErrorActionPreference = 'Continue'
& $Blender @scriptArgs 2>&1 | ForEach-Object {
    $line = "$_"
    if ($line -match '^\[FIELD\]|Error|Traceback|^\s+File ') { Write-Host $line }
}
$ErrorActionPreference = 'Stop'
if ($LASTEXITCODE -ne 0) { throw "Blender failed (exit $LASTEXITCODE)" }

if (-not $NoBake) {
    & (Join-Path $PSScriptRoot 'bake-arena-ao.ps1') -Only field -Blender $Blender
} else {
    & (Join-Path $PSScriptRoot 'compress-arena-textures.ps1')
}

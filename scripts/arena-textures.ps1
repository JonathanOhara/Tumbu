# Generates the arena remake textures (scripts/blender/arena_textures.py) into media/tumbu/arena/.
# Usage: .\scripts\arena-textures.ps1 [-Preview <folder>] [-Blender <path to blender.exe>]
#   -Preview  also writes 2x2 tiled copies there, to check the seams by eye
param(
    [string]$Preview = '',
    [string]$Blender = 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path $Blender)) { throw "Blender not found: $Blender (pass -Blender)" }

$scriptArgs = @('--background', '--factory-startup', '--python', (Join-Path $PSScriptRoot 'blender\arena_textures.py'), '--', $root)
if ($Preview) { New-Item -ItemType Directory -Force $Preview | Out-Null; $scriptArgs += @('--preview', $Preview) }

# Blender prints to stderr; Windows PowerShell 5.1 would turn that into errors under 'Stop' (the exit code decides).
$ErrorActionPreference = 'Continue'
& $Blender @scriptArgs 2>&1 | ForEach-Object {
    $line = "$_"
    if ($line -match '^\[TEX\]|Error|Traceback|^\s+File ') { Write-Host $line }
}
$ErrorActionPreference = 'Stop'
if ($LASTEXITCODE -ne 0) { throw "Blender failed (exit $LASTEXITCODE)" }

# The game uses compressed copies (DDS) of these PNGs.
& (Join-Path $PSScriptRoot 'compress-arena-textures.ps1')

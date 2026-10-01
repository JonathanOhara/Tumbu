# Converts 2011 (Blender 2.49) .blend files into working files under art/, with Blender 4.5 LTS: the last
# version that converts pre-2.50 animation data (Blender 5.x opens 2.49 files but drops their animations,
# and crashes on robot002.blend). The originals in media/ are never written.
# Usage: .\scripts\convert-legacy-blend.ps1 media\tumbu\robot001\robot001.blend art\robots\robot001.blend
#        .\scripts\convert-legacy-blend.ps1 -Robots      # all five robots -> art\robots\robot00N.blend
param(
    [string]$Source,
    [string]$Target,
    [switch]$Robots,
    [string]$Blender = 'D:\TumbuDeps\tools\blender-4.5.14-windows-x64\blender.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path $Blender)) {
    throw "Blender 4.5 not found: $Blender. Download the portable zip from download.blender.org/release/Blender4.5/ into D:\TumbuDeps\tools."
}
$jobs = @()
if ($Robots) {
    foreach ($n in 1..5) {
        $jobs += ,@((Join-Path $root "media\tumbu\robot00$n\robot00$n.blend"), (Join-Path $root "art\robots\robot00$n.blend"))
    }
} elseif ($Source -and $Target) {
    $jobs += ,@((Resolve-Path $Source).Path, [IO.Path]::GetFullPath((Join-Path (Get-Location) $Target)))
} else {
    throw 'Pass -Source and -Target, or -Robots.'
}
foreach ($job in $jobs) {
    if (Test-Path $job[1]) { throw "$($job[1]) already exists: it may have manual edits. Delete it first to convert again." }
    & $Blender --background --factory-startup $job[0] --python (Join-Path $PSScriptRoot 'blender\convert_legacy.py') -- $job[1] 2>&1 |
        ForEach-Object { $line = "$_"; if ($line -match '^\[CONVERT\]|Error|Traceback') { Write-Host $line } }
    if (-not (Test-Path $job[1])) { throw "Conversion failed: $($job[0])" }
}

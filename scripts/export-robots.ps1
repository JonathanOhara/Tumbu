# Exports the robots from Blender (art/robots/robot00N.blend) to the game's .mesh + .skeleton files.
# Usage: .\scripts\export-robots.ps1 [-Robot 1,3] [-Save] [-Install] [-Blender <path to Blender 4.5 blender.exe>]
#   (no switch)  export to %TEMP%\tumbu-robot-export, check it against Blender and compare it with the game's files
#   -Save        also write the source fixes (fps, weights, material names) back into the .blend
#   -Install     also copy the new .mesh/.skeleton files into media/tumbu/robot00N/ (only when every check passed)
# scripts/blender/robot_export.py does the export and checks that every vertex weight and every bone at every frame of
# every action matches Blender; scripts/blender/robot_compare.py then compares the skinned vertices with the files the
# game uses now, frame by frame. Needs Blender 4.5 LTS with blender2ogre (docs/DEV_SETUP.md, Part B): blender2ogre's
# animation export does not work with Blender 5's actions.
param(
    [int[]]$Robot = @(1, 2, 3, 4, 5),
    [switch]$Save,
    [switch]$Install,
    [string]$Blender = 'D:\TumbuDeps\tools\blender-4.5.14-windows-x64\blender.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$converter = 'D:\TumbuDeps\modern\install\bin\OgreXMLConverter.exe'
if (-not (Test-Path $Blender)) { throw "Blender not found: $Blender (pass -Blender)" }
$out = Join-Path $env:TEMP 'tumbu-robot-export'
$parts = 'head', 'body', 'leftArm', 'rightArm', 'legs'

# Blender and the converter print to stderr; Windows PowerShell 5 would turn that into errors under 'Stop'.
$ErrorActionPreference = 'Continue'

foreach ($r in $Robot) {
    $name = 'robot{0:D3}' -f $r
    $number = '{0:D3}' -f $r
    $blend = Join-Path $root "art\robots\$name.blend"
    $media = Join-Path $root "media\tumbu\$name"
    Remove-Item (Join-Path $out $name) -Recurse -Force -ErrorAction SilentlyContinue

    # 1. Export and check against Blender.
    $exportArgs = @('--background', $blend, '--python', (Join-Path $PSScriptRoot 'blender\robot_export.py'), '--', $out)
    if ($Save) { $exportArgs += '--save' }
    $ok = $false
    & $Blender @exportArgs 2>&1 | ForEach-Object {
        $line = "$_"
        if ($line -match '^\[ROBOT-EXPORT\]') { Write-Host $line; if ($line -match 'all parts match Blender') { $ok = $true } }
        elseif ($line -match 'SystemExit|robot_(export|check)\.py", line') { Write-Host $line }
    }
    if (-not $ok) { throw "$name : the export does not match Blender; nothing installed" }

    # 2. Compare with the files the game uses now (the skinned vertices, frame by frame).
    $old = Join-Path $out "$name-installed"
    New-Item -ItemType Directory -Force $old | Out-Null
    foreach ($p in $parts) {
        foreach ($ext in 'mesh', 'skeleton') {
            $file = Join-Path $media "${p}_$number.$ext"
            & $converter -q -log (Join-Path $old 'converter.log') $file (Join-Path $old "${p}_$number.$ext.xml") | Out-Null
        }
    }
    $names = $parts | ForEach-Object { "${_}_$number" }
    & $Blender --background --python (Join-Path $PSScriptRoot 'blender\robot_compare.py') -- $old (Join-Path $out $name) @names 2>&1 |
        ForEach-Object { if ("$_" -match '^\[COMPARE\].*(largest|vertices \d|only in)') { Write-Host "$_" } }

    # 3. Install.
    if ($Install) {
        foreach ($p in $parts) {
            foreach ($ext in 'mesh', 'skeleton') {
                Copy-Item (Join-Path $out "$name\${p}_$number.$ext") $media -Force
            }
        }
        Write-Host "$name : installed into $media"
    }
}

# Upgrades every .mesh / .skeleton under media/ from the Ogre 1.7 binary formats (MeshSerializer_v1.41,
# Serializer_v1.10) to the current Ogre 14 formats, in place.
#
#   Ogre 1.7 OgreXMLConverter:  binary 1.7  -> XML
#   Ogre 14  OgreXMLConverter:  XML         -> binary 14
#
# Needs the Ogre 1.7.2 command line tools (D:\Backup\OgreSDK\OgreCommandLineTools_1.7.2.zip, extracted to
# -LegacyTools) and the Ogre 14 install from scripts\deps.ps1. Files already in the new format are skipped.
# The 2011 binaries remain in git history (tag legacy-2011).
param(
    [string]$LegacyTools = 'D:\TumbuDeps\ogretools',
    [string]$DepsDir = $(if ($env:TUMBU_DEPS_DIR) { $env:TUMBU_DEPS_DIR } else { 'D:\TumbuDeps\modern' })
)
$ErrorActionPreference = 'Stop'
$root    = Split-Path $PSScriptRoot -Parent
$old     = Join-Path $LegacyTools 'OgreXMLConverter.exe'
$new     = Join-Path $DepsDir 'install\bin\OgreXMLConverter.exe'
$temp    = Join-Path $env:TEMP 'tumbu-mesh-upgrade'
foreach ($tool in $old, $new) { if (-not (Test-Path $tool)) { throw "Missing $tool" } }
New-Item -ItemType Directory -Force $temp | Out-Null

function Get-Header([string]$file) {
    $bytes = [System.IO.File]::ReadAllBytes($file)
    return [System.Text.Encoding]::ASCII.GetString($bytes, 2, [Math]::Min(40, $bytes.Length - 2))
}

$files = Get-ChildItem (Join-Path $root 'media') -Recurse -Include *.mesh, *.skeleton
$upgraded = 0
foreach ($file in $files) {
    $header = Get-Header $file.FullName
    if ($header -notmatch '\[(MeshSerializer_v1\.4\d|Serializer_v1\.10)\]') {
        Write-Host "skip (already current): $($file.Name)" -ForegroundColor DarkGray
        continue
    }
    $xml = Join-Path $temp ($file.Name + '.xml')
    # The converters resolve their DLLs from their own folder.
    Push-Location $LegacyTools
    & $old -q $file.FullName $xml | Out-Null
    Pop-Location
    if (-not (Test-Path $xml)) { throw "Ogre 1.7 converter failed on $($file.FullName)" }

    Push-Location (Split-Path $new)
    & $new -q $xml $file.FullName | Out-Null
    Pop-Location
    if ($LASTEXITCODE -ne 0) { throw "Ogre 14 converter failed on $xml" }

    Write-Host "upgraded: $($file.FullName.Substring($root.Length + 1))  ($header -> $(Get-Header $file.FullName))"
    Remove-Item $xml
    $upgraded++
}
Write-Host "$upgraded file(s) upgraded." -ForegroundColor Green

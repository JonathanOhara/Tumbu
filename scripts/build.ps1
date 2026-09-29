# Builds TUMBU with the legacy toolchain (VC++ 2010 / v100) through VS2022's MSBuild.
# Usage: .\scripts\build.ps1 [-Configuration Release|Debug] [-Rebuild]
param(
    [ValidateSet('Release', 'Debug')] [string]$Configuration = 'Release',
    [switch]$Rebuild
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

# Pick up the dependency env vars (set as User variables) even in shells opened before they were set.
foreach ($name in 'OGRE_HOME','BULLET_HOME','OGRE_BULLET_HOME','OGREAL_SDK','OPENAL_SDK','LIB_OGG_HOME',
                  'LIB_VORBIS_HOME','CEGUI_HOME','SKYX_HOME','PAGED_GEOMETRY_HOME') {
    $value = [Environment]::GetEnvironmentVariable($name, 'User')
    if (-not $value) { throw "Environment variable $name is not set. See docs/DEV_SETUP.md, Part 2.3." }
    Set-Item "Env:$name" $value
}

# The VC++ 2010 compiler from Windows SDK 7.1 needs mspdb100.dll, which lives in Common7\IDE. Without a full
# VS2010 install nothing puts that folder on the PATH, and CL.exe dies with 0xC0000135 (DLL not found).
$vs2010Ide = "${env:ProgramFiles(x86)}\Microsoft Visual Studio 10.0\Common7\IDE"
if (-not (Test-Path "$vs2010Ide\mspdb100.dll")) { throw 'VC++ 2010 compiler not found. See docs/DEV_SETUP.md, Part 2.1.' }
$env:PATH = "$vs2010Ide;$env:PATH"

$vswhere ="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'MSBuild not found. Install Visual Studio 2022 with the C++ workload.' }

$target = if ($Rebuild) { 'Rebuild' } else { 'Build' }
& $msbuild "$root\TUMBU.sln" /t:$target /p:Configuration=$Configuration /p:Platform=Win32 /m /nologo /v:minimal
if ($LASTEXITCODE -ne 0) { throw "Build failed (exit $LASTEXITCODE)." }
Write-Host "Built: $root\bin\$Configuration\TUMBU.exe"

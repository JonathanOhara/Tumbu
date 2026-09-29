# Builds TUMBU (Ogre 14, CMake, Visual Studio 2022 x64).
# Usage: .\scripts\build.ps1 [-Configuration Release|RelWithDebInfo] [-Clean]
# First time: build the dependencies with .\scripts\deps.ps1 (see docs/DEV_SETUP.md).
param(
    [ValidateSet('Release', 'RelWithDebInfo')] [string]$Configuration = 'Release',
    [switch]$Clean
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath  = & $vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw 'Visual Studio 2022 with the C++ workload is required.' }
$cmake = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'

Push-Location $root
try {
    if ($Clean -or -not (Test-Path 'build\vs2022\TUMBU.sln')) {
        & $cmake --preset vs2022
        if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed (are the dependencies built? run .\scripts\deps.ps1).' }
    }
    $target = if ($Clean) { @('--clean-first') } else { @() }
    & $cmake --build build/vs2022 --config $Configuration @target -- /m /v:minimal
    if ($LASTEXITCODE -ne 0) { throw "Build failed (exit $LASTEXITCODE)." }
    Write-Host "Built: $root\bin\$Configuration\TUMBU.exe" -ForegroundColor Green
} finally {
    Pop-Location
}

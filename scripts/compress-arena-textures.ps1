# Compresses the arena's textures (media/tumbu/arena/*.png) into DDS with a full mip chain, which the materials use:
#   *_col.png, ring_emblem.png, gym_arena.png  -> BC7  (colour, or three masks)
#   *_nrm.png, *_hgt.png                         -> BC5  (two channels: normal x/y, the shader rebuilds z; height + moss)
#   *_ao.png                                     -> BC4  (one channel)
# About 4x less video memory than the PNGs (decoded to RGBA8), faster loading, no visible change. The PNGs stay as the
# sources (scripts/arena-textures.ps1, scripts/bake-arena-ao.ps1 call this script after writing them).
# Usage: .\scripts\compress-arena-textures.ps1 [-Texconv <path>]
#   texconv: Microsoft DirectXTex (https://github.com/microsoft/DirectXTex), installed by scripts/deps.ps1 in
#   D:\TumbuDeps\tools\texconv. Game textures are in display (gamma) space, so no sRGB conversion.
param(
    [string]$Texconv = 'D:\TumbuDeps\tools\texconv\texconv.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$dir = Join-Path $root 'media\tumbu\arena'
if (-not (Test-Path $Texconv)) { throw "texconv not found: $Texconv (run scripts\deps.ps1, or pass -Texconv)" }

$jobs = foreach ($png in Get-ChildItem $dir -Filter *.png) {
    $n = $png.BaseName
    if ($n -like '*_nrm' -or $n -like '*_hgt') { $format = 'BC5_UNORM'; $wrap = $true }
    elseif ($n -like '*_ao') { $format = 'BC4_UNORM'; $wrap = $false }
    else { $format = 'BC7_UNORM'; $wrap = $n -like '*_col' }
    [pscustomobject]@{ File = $png.FullName; Format = $format; Wrap = $wrap }
}
foreach ($j in $jobs) {
    # -m 0: every mip level; -dx10: the DX10 header (Ogre reads BC4/5/7 from it); -wrap: tiling textures filter their
    # mips across the edges; -nologo -y: quiet, overwrite. --ignore-srgb: the PNGs carry an sRGB tag, and texconv
    # converted them to linear values (everything darker, the moss field below its threshold); the game samples raw values.
    $texArgs = @('-nologo', '-y', '-m', '0', '-dx10', '--ignore-srgb', '-f', $j.Format, '-o', $dir)
    if ($j.Wrap) { $texArgs += '-wrap' }
    $out = & $Texconv @texArgs $j.File 2>&1
    if ($LASTEXITCODE -ne 0) { throw "texconv failed on $($j.File): $out" }
    $dds = [IO.Path]::ChangeExtension($j.File, '.dds')
    Write-Host ("{0,-22} {1,-10} {2,8:N0} KB" -f (Split-Path $dds -Leaf), $j.Format, ((Get-Item $dds).Length / 1KB))
}

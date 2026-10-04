# Side-by-side BEFORE | AFTER image of two screenshots, for reviewing a visual change.
# Usage: .\scripts\compare.ps1 -Before a.png -After b.png -Out c.png [-Crop "x,y,w,h"] [-Zoom 2]
#   -Crop  the same region of both screenshots (pixels), e.g. the robot: "330,0,380,720"
#   -Zoom  enlarges the crop (nearest pixel, so edges stay sharp), e.g. 2 or 4 for close-ups of edges
# Typical flow (see CLAUDE.md "Before/after comparisons"): devtest.ps1 -Name feature-before ..., change, devtest.ps1
# -Name feature-after ..., then compare into %USERPROFILE%\Tumbu\<feature>\.
param(
    [Parameter(Mandatory)] [string]$Before,
    [Parameter(Mandatory)] [string]$After,
    [Parameter(Mandatory)] [string]$Out,
    [string]$Crop = '',
    [double]$Zoom = 1,
    [string]$BeforeLabel = 'BEFORE',
    [string]$AfterLabel = 'AFTER'
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$imgBefore = [Drawing.Image]::FromFile((Resolve-Path $Before))
$imgAfter = [Drawing.Image]::FromFile((Resolve-Path $After))
try {
    if ($Crop) {
        $c = $Crop.Split(',') | ForEach-Object { [int]$_ }
        $src = New-Object Drawing.Rectangle $c[0], $c[1], $c[2], $c[3]
    } else {
        $src = New-Object Drawing.Rectangle 0, 0, $imgBefore.Width, $imgBefore.Height
    }
    $w = [int]($src.Width * $Zoom); $h = [int]($src.Height * $Zoom); $gap = 8
    $bitmap = New-Object Drawing.Bitmap ($w * 2 + $gap), $h
    $g = [Drawing.Graphics]::FromImage($bitmap)
    if ($Zoom -ne 1) { $g.InterpolationMode = 'NearestNeighbor'; $g.PixelOffsetMode = 'Half' }
    $g.Clear([Drawing.Color]::White)
    $g.DrawImage($imgBefore, (New-Object Drawing.Rectangle 0, 0, $w, $h), $src, [Drawing.GraphicsUnit]::Pixel)
    $g.DrawImage($imgAfter, (New-Object Drawing.Rectangle ($w + $gap), 0, $w, $h), $src, [Drawing.GraphicsUnit]::Pixel)
    $font = New-Object Drawing.Font 'Arial', 16, ([Drawing.FontStyle]::Bold)
    $suffix = if ($Zoom -ne 1) { " (${Zoom}x)" } else { '' }
    $g.DrawString($BeforeLabel + $suffix, $font, [Drawing.Brushes]::Yellow, 8, $h - 30)
    $g.DrawString($AfterLabel + $suffix, $font, [Drawing.Brushes]::Yellow, $w + $gap + 8, $h - 30)
    $dir = Split-Path $Out -Parent
    if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Force $dir | Out-Null }
    $bitmap.Save($Out, [Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bitmap.Dispose()
    Write-Host "comparison: $Out"
} finally {
    $imgBefore.Dispose(); $imgAfter.Dispose()
}

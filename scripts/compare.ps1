# Side-by-side BEFORE | AFTER image of two screenshots, for reviewing a visual change.
# Usage: .\scripts\compare.ps1 -Before a.png -After b.png -Out c.png [-Crop "x,y,w,h"] [-Zoom 2]
#   -Crop  the same region of both screenshots (pixels), e.g. the robot: "330,0,380,720"
#   -Zoom  enlarges the crop (nearest pixel, so edges stay sharp), e.g. 2 or 4 for close-ups of edges
# Typical flow (see CLAUDE.md "Before/after comparisons"): devtest.ps1 -Name feature-before ..., change, devtest.ps1
# -Name feature-after ..., then compare into %USERPROFILE%\Tumbu\<feature>\.
# When a screenshot has the frame rate of its run next to it (devtest-<Name>.fps, written by devtest.ps1), the label
# shows the average fps, and the AFTER label the change in percent: "(bench)" for a steady -Bench run, "~... (1 run)" and
# "indicative" for a normal screenshot run (identical runs differ by 15-30 %; the real cost comes from bench.ps1).
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

# Average fps of the run that took the screenshot, or 0 when unknown; Steady = it came from a -Bench run (no AI, fixed
# camera), which compares; a normal screenshot run varies by 15-30 % between identical runs, so it is marked "1 run".
function Get-RunFps([string]$png) {
    $file = [IO.Path]::ChangeExtension((Resolve-Path $png).Path, '.fps')
    if ((Test-Path $file) -and ((Get-Content $file -Raw) -match 'avg=([\d.]+)')) {
        $text = Get-Content $file -Raw
        return [pscustomobject]@{ Fps = [double]::Parse($Matches[1], [cultureinfo]::InvariantCulture); Steady = $text -match 'render_ms=' }
    }
    return [pscustomobject]@{ Fps = 0; Steady = $false }
}
function Format-Fps($r) {
    $value = [math]::Round($r.Fps).ToString([cultureinfo]::InvariantCulture)
    if ($r.Steady) { return "  $value fps (bench)" }
    return "  ~$value fps (1 run)"
}
$fpsBefore = Get-RunFps $Before
$fpsAfter = Get-RunFps $After
if ($fpsBefore.Fps -gt 0) { $BeforeLabel += Format-Fps $fpsBefore }
if ($fpsAfter.Fps -gt 0) {
    $AfterLabel += Format-Fps $fpsAfter
    if ($fpsBefore.Fps -gt 0) {
        $change = [string]::Format([cultureinfo]::InvariantCulture, "{0:+0.0;-0.0}%", 100 * ($fpsAfter.Fps / $fpsBefore.Fps - 1))
        # only bench numbers compare; between single runs the change is only a hint (use scripts\bench.ps1)
        $AfterLabel += if ($fpsBefore.Steady -and $fpsAfter.Steady) { " ($change)" } else { " ($change, indicative)" }
    }
}

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
    # A dark band under the labels keeps them readable over bright pixels.
    $band = New-Object Drawing.SolidBrush ([Drawing.Color]::FromArgb(150, 0, 0, 0))
    $g.FillRectangle($band, 0, $h - 34, $w * 2 + $gap, 34)
    $g.DrawString($BeforeLabel + $suffix, $font, [Drawing.Brushes]::Yellow, 8, $h - 30)
    $g.DrawString($AfterLabel + $suffix, $font, [Drawing.Brushes]::Yellow, $w + $gap + 8, $h - 30)
    $dir = Split-Path $Out -Parent
    if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Force $dir | Out-Null }
    $bitmap.Save($Out, [Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bitmap.Dispose(); $band.Dispose()
    Write-Host "comparison: $Out  ($BeforeLabel | $AfterLabel)"

    # Pixel difference of the compared region: how many pixels differ, and by how much at most (0..255 per channel).
    $diffs = foreach ($img in $imgBefore, $imgAfter) {
        $bmp = New-Object Drawing.Bitmap $src.Width, $src.Height, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $gb = [Drawing.Graphics]::FromImage($bmp)
        $gb.DrawImage($img, (New-Object Drawing.Rectangle 0, 0, $src.Width, $src.Height), $src, [Drawing.GraphicsUnit]::Pixel)
        $gb.Dispose()
        $data = $bmp.LockBits((New-Object Drawing.Rectangle 0, 0, $src.Width, $src.Height), 'ReadOnly', $bmp.PixelFormat)
        $bytes = New-Object byte[] ($data.Stride * $src.Height)
        [Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $bytes.Length)
        $bmp.UnlockBits($data); $bmp.Dispose()
        , $bytes
    }
    $a = $diffs[0]; $b = $diffs[1]; $count = 0; $max = 0
    for ($i = 0; $i -lt $a.Length; $i += 4) {
        $d = [math]::Max([math]::Max([math]::Abs($a[$i] - $b[$i]), [math]::Abs($a[$i + 1] - $b[$i + 1])), [math]::Abs($a[$i + 2] - $b[$i + 2]))
        if ($d -gt 0) { $count++; if ($d -gt $max) { $max = $d } }
    }
    Write-Host ("pixels that differ: {0} of {1} ({2:0.00} %), largest difference {3}/255" -f $count, ($a.Length / 4), (100 * $count / ($a.Length / 4)), $max)
} finally {
    $imgBefore.Dispose(); $imgAfter.Dispose()
}

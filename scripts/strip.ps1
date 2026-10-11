# Compares frame strips (devtest.ps1 -Strip N): flicker and crawl of small bright details while the camera pans.
# Usage: .\scripts\strip.ps1 -Before <name> -After <name> -Crop "x,y,w,h" -Out "%USERPROFILE%\Tumbu\<feature>\<case>-strip.png"
#        [-Zoom 2] [-Frames 8] [-BeforeLabel BEFORE] [-AfterLabel AFTER]
#   -Before/-After  devtest run names: the frames are %USERPROFILE%\Tumbu\devtest-<name>-strip-NN.png
#   -Crop           the region to compare, in window pixels (the strips must have the same window size)
#   -Frames         how many frames the image shows (the measurement uses them all)
# The image has, for each strip, a row of frames and below it the differences between consecutive frames x8 (bright =
# changed). It also prints the crawl of each strip: how much the brightest value of each pixel column of the crop
# changes from one frame to the next, as % of its mean. A glow that moves smoothly with the camera stays low; one that
# jumps between the pixels of a small texture (bloom flicker) is high. Only compare strips of the same view: the
# camera moves about a third of a pixel per frame, so thin edges always change a little.
# Like compare.ps1, the frames are kept in the folder's source\ (devtest.ps1 -Clean removes the loose devtest-* files).
param(
    [Parameter(Mandatory)] [string]$Before,
    [Parameter(Mandatory)] [string]$After,
    [Parameter(Mandatory)] [string]$Crop,
    [Parameter(Mandatory)] [string]$Out,
    [double]$Zoom = 2,
    [int]$Frames = 8,
    [string]$BeforeLabel = 'BEFORE',
    [string]$AfterLabel = 'AFTER'
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$work = Join-Path $env:USERPROFILE 'Tumbu'
$inv = [cultureinfo]::InvariantCulture
$c = $Crop.Split(',') | ForEach-Object { [int]$_ }
$src = New-Object Drawing.Rectangle $c[0], $c[1], $c[2], $c[3]

# The crop of one frame as BGRA bytes.
function Read-Crop([string]$file) {
    $img = [Drawing.Image]::FromFile($file)
    try {
        $bmp = New-Object Drawing.Bitmap $src.Width, $src.Height, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $g = [Drawing.Graphics]::FromImage($bmp)
        $g.DrawImage($img, (New-Object Drawing.Rectangle 0, 0, $src.Width, $src.Height), $src, [Drawing.GraphicsUnit]::Pixel)
        $g.Dispose()
        $data = $bmp.LockBits((New-Object Drawing.Rectangle 0, 0, $src.Width, $src.Height), 'ReadOnly', $bmp.PixelFormat)
        $bytes = New-Object byte[] ($data.Stride * $src.Height)
        [Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $bytes.Length)
        $bmp.UnlockBits($data); $bmp.Dispose()
        return , $bytes
    } finally { $img.Dispose() }
}

# The brightest value (sum of R, G and B) of each pixel column.
function Get-Profile([byte[]]$bytes) {
    $w = $src.Width; $profile = New-Object double[] $w
    for ($y = 0; $y -lt $src.Height; $y++) {
        $row = $y * $w * 4
        for ($x = 0; $x -lt $w; $x++) {
            $i = $row + $x * 4
            $v = [double]$bytes[$i] + $bytes[$i + 1] + $bytes[$i + 2]
            if ($v -gt $profile[$x]) { $profile[$x] = $v }
        }
    }
    return , $profile
}

$strips = foreach ($pair in @(@($Before, $BeforeLabel), @($After, $AfterLabel))) {
    $files = @(Get-ChildItem "$work\devtest-$($pair[0])-strip-*.png" | Sort-Object Name)
    if ($files.Count -lt 2) { throw "No strip for '$($pair[0])': run devtest.ps1 -Name $($pair[0]) -Strip N (with -Camera or -Bench)." }
    $crops = @($files | ForEach-Object { , (Read-Crop $_.FullName) })
    $profiles = @($crops | ForEach-Object { , (Get-Profile $_) })
    $change = 0.0; $total = 0.0
    for ($k = 1; $k -lt $profiles.Count; $k++) {
        for ($x = 0; $x -lt $src.Width; $x++) { $change += [math]::Abs($profiles[$k][$x] - $profiles[$k - 1][$x]); $total += $profiles[$k][$x] }
    }
    $crawl = if ($total -gt 0) { 100 * $change / $total } else { 0 }
    [pscustomobject]@{ Name = $pair[0]; Label = $pair[1]; Files = $files; Crops = $crops; Crawl = $crawl }
}

# The image: per strip, a row of frames and a row of differences x8.
$n = [math]::Min($Frames, [math]::Min($strips[0].Files.Count, $strips[1].Files.Count))
$w = [int]($src.Width * $Zoom); $h = [int]($src.Height * $Zoom); $gap = 4; $labelH = 26
$bitmap = New-Object Drawing.Bitmap ($n * ($w + $gap) - $gap), (2 * ($labelH + 2 * $h + $gap))
$g = [Drawing.Graphics]::FromImage($bitmap)
$g.InterpolationMode = 'NearestNeighbor'; $g.PixelOffsetMode = 'Half'
$g.Clear([Drawing.Color]::FromArgb(24, 24, 28))
$font = New-Object Drawing.Font 'Arial', 13, ([Drawing.FontStyle]::Bold)
$top = 0
foreach ($s in $strips) {
    $g.DrawString([string]::Format($inv, "{0}: frames (top), differences between frames x8 (bottom); crawl {1:0.00} %", $s.Label, $s.Crawl), $font, [Drawing.Brushes]::Yellow, 4, $top + 4)
    $top += $labelH
    for ($k = 0; $k -lt $n; $k++) {
        $img = [Drawing.Image]::FromFile($s.Files[$k].FullName)
        $g.DrawImage($img, (New-Object Drawing.Rectangle ($k * ($w + $gap)), $top, $w, $h), $src, [Drawing.GraphicsUnit]::Pixel)
        $img.Dispose()
        if ($k + 1 -lt $s.Crops.Count) {
            $a = $s.Crops[$k]; $b = $s.Crops[$k + 1]
            $diff = New-Object Drawing.Bitmap $src.Width, $src.Height, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
            $data = $diff.LockBits((New-Object Drawing.Rectangle 0, 0, $src.Width, $src.Height), 'WriteOnly', $diff.PixelFormat)
            $bytes = New-Object byte[] ($data.Stride * $src.Height)
            for ($i = 0; $i -lt $bytes.Length; $i += 4) {
                for ($ch = 0; $ch -lt 3; $ch++) { $bytes[$i + $ch] = [byte][math]::Min(255, 8 * [math]::Abs([int]$a[$i + $ch] - $b[$i + $ch])) }
                $bytes[$i + 3] = 255
            }
            [Runtime.InteropServices.Marshal]::Copy($bytes, 0, $data.Scan0, $bytes.Length)
            $diff.UnlockBits($data)
            $g.DrawImage($diff, (New-Object Drawing.Rectangle ($k * ($w + $gap)), ($top + $h), $w, $h), (New-Object Drawing.Rectangle 0, 0, $src.Width, $src.Height), [Drawing.GraphicsUnit]::Pixel)
            $diff.Dispose()
        }
    }
    $top += 2 * $h + $gap
}
$dir = Split-Path $Out -Parent
if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Force $dir | Out-Null }
$bitmap.Save($Out, [Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bitmap.Dispose(); $font.Dispose()
Write-Host "strip comparison: $Out"
foreach ($s in $strips) { Write-Host ([string]::Format($inv, "crawl {0,-10} {1,6:0.00} %  ({2} frames, {3})", $s.Label, $s.Crawl, $s.Files.Count, $s.Name)) }

# Keep the frames next to the image (source\<case>-before-strip-NN.png / -after-).
$keep = Join-Path (Resolve-Path -LiteralPath $dir).Path 'source'
New-Item -ItemType Directory -Force $keep | Out-Null
$base = [IO.Path]::GetFileNameWithoutExtension($Out)
foreach ($pair in @(@($strips[0], 'before'), @($strips[1], 'after'))) {
    foreach ($f in $pair[0].Files) { Copy-Item -LiteralPath $f.FullName (Join-Path $keep ("$base-$($pair[1])-" + ($f.Name -replace '^devtest-.*-(strip-\d+\.png)$', '$1'))) -Force }
}

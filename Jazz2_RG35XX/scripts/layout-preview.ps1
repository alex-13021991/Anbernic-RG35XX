param(
    [string]$Source = "$PSScriptRoot/../assets/jazz2-cover-generated.png",
    [string]$Destination = "$PSScriptRoot/../assets/Jazz Jackrabbit 2.png"
)
Add-Type -AssemblyName System.Drawing
$sourceImage = [System.Drawing.Bitmap]::new([IO.Path]::GetFullPath($Source))
# Pixel placement matches the supplied Jazz Jackrabbit CD launcher preview.
$preview = [System.Drawing.Bitmap]::new(640, 480, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($preview)
$graphics.Clear([System.Drawing.Color]::Transparent)
$graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$target = [System.Drawing.Rectangle]::new(381,56,240,360)
$graphics.SetClip($target)
$graphics.FillRectangle([System.Drawing.Brushes]::Black,$target)
# The AI artwork is normalized here to the exact launcher canvas and rectangle.
$crop = [System.Drawing.Rectangle]::new(862,124,552,824)
$graphics.DrawImage($sourceImage,$target,$crop,[System.Drawing.GraphicsUnit]::Pixel)
$graphics.Dispose()
$preview.Save([IO.Path]::GetFullPath($Destination),[System.Drawing.Imaging.ImageFormat]::Png)
$preview.Dispose()
$sourceImage.Dispose()

param(
    [Parameter(Position=0)][string]$Game,
    [Parameter(Position=1)][string]$Output = "$PSScriptRoot/Content"
)
$ErrorActionPreference = 'Stop'
if (-not $Game) { $Game = Read-Host 'Path to installed GOG game (folder with Anims.j2a)' }
$gamePath = (Resolve-Path -LiteralPath $Game.Trim('"')).Path
if (-not (Test-Path -LiteralPath (Join-Path $gamePath 'Anims.j2a'))) { throw 'Anims.j2a not found' }
$outputPath = [IO.Path]::GetFullPath($Output)
if (Test-Path -LiteralPath $outputPath) { throw 'Output exists. Choose a new folder to avoid mixing files.' }
$packer = Join-Path $PSScriptRoot 'AssetPacker.exe'
$engine = Join-Path $PSScriptRoot 'EngineContent'
if (-not (Test-Path -LiteralPath $packer) -or -not (Test-Path -LiteralPath "$engine/Metadata")) { throw 'Converter files missing' }
$staging = Join-Path ([IO.Path]::GetTempPath()) ('jazz2-convert-' + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path $staging | Out-Null
    Copy-Item -LiteralPath $gamePath -Destination (Join-Path $staging 'Source') -Recurse
    Copy-Item -LiteralPath $engine -Destination (Join-Path $staging 'Content') -Recurse
    & $packer $staging $outputPath --target=console --originals-only --skip-non-episode-levels --video-downscale=2
    if ($LASTEXITCODE -ne 0) { throw "AssetPacker exit code $LASTEXITCODE" }
    New-Item -ItemType Directory -Path (Join-Path $outputPath 'Music') -Force | Out-Null
    foreach ($track in @('menu','bonus2','bonus3','intro','ending')) {
        $file = Join-Path $gamePath "$track.j2b"
        if (Test-Path -LiteralPath $file) { Copy-Item -LiteralPath $file -Destination (Join-Path $outputPath "Music/$track.j2b") }
        else { Write-Warning "Optional music missing: $track.j2b" }
    }
    foreach ($file in @('Prebaked.pak','Prebaked.version')) {
        if (-not (Test-Path -LiteralPath (Join-Path $outputPath $file))) { throw "Conversion incomplete: $file" }
    }
    Write-Output "Content ready: $outputPath"
} finally {
    # Delete only the unique directory just created beneath the OS temporary folder.
    $resolvedStaging = [IO.Path]::GetFullPath($staging)
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if ($resolvedStaging.StartsWith($tempRoot,[StringComparison]::OrdinalIgnoreCase) -and
        [IO.Path]::GetFileName($resolvedStaging).StartsWith('jazz2-convert-')) {
        Remove-Item -LiteralPath $resolvedStaging -Recurse -Force -ErrorAction SilentlyContinue
    }
}

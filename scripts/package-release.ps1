param(
    [string] $BuildDir = "build/runner-mingw",
    [string] $Version = "0.1.0-preview",
    [string] $OutputDir = "dist"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildRoot = Join-Path $repoRoot $BuildDir
$outRoot = Join-Path $repoRoot $OutputDir
$stageName = "SacredStonesRecomp-$Version-win64"
$stageRoot = Join-Path $outRoot $stageName
$zipPath = Join-Path $outRoot "$stageName.zip"

if (-not (Test-Path -LiteralPath $buildRoot -PathType Container)) {
    throw "Build directory not found: $buildRoot"
}

$exe = Join-Path $buildRoot "SacredStonesRecomp.exe"
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    throw "Executable not found: $exe"
}

New-Item -ItemType Directory -Force -Path $outRoot | Out-Null
$outFull = [System.IO.Path]::GetFullPath($outRoot)
$stageFull = [System.IO.Path]::GetFullPath($stageRoot)
if (-not $stageFull.StartsWith($outFull, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to remove a staging directory outside the output directory: $stageFull"
}
if (Test-Path -LiteralPath $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stageRoot | Out-Null

$files = @(
    "SacredStonesRecomp.exe",
    "SDL2.dll",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll"
)

foreach ($file in $files) {
    $source = Join-Path $buildRoot $file
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Required release file not found: $source"
    }
    Copy-Item -LiteralPath $source -Destination (Join-Path $stageRoot $file) -Force
}

$assets = Join-Path $buildRoot "assets"
if (-not (Test-Path -LiteralPath $assets -PathType Container)) {
    throw "Launcher assets directory not found: $assets"
}
Copy-Item -LiteralPath $assets -Destination (Join-Path $stageRoot "assets") -Recurse -Force

Copy-Item -LiteralPath (Join-Path $repoRoot "game.toml") -Destination (Join-Path $stageRoot "game.toml") -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "README.md") -Destination (Join-Path $stageRoot "README.md") -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "docs/release.md") -Destination (Join-Path $stageRoot "RELEASE_NOTES.md") -Force

if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
Compress-Archive -Path (Join-Path $stageRoot "*") -DestinationPath $zipPath -Force

Write-Host "Release package: $zipPath"
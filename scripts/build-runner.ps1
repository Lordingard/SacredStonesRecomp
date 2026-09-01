param(
    [string] $BuildDir = "build/runner-mingw",
    [string] $BiosPath,
    [string] $MingwBin
)

. "$PSScriptRoot/common.ps1"

function Resolve-RepoPath {
    param([Parameter(Mandatory = $true)][string] $Path)
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $script:RepoRoot $Path))
}

$resolvedBuildDir = Resolve-RepoPath $BuildDir
$resolvedBiosPath = Resolve-Setting $BiosPath "GBA_BIOS" "BiosPath" ""
$resolvedMingwBin = Resolve-Setting $MingwBin "MINGW_BIN" "MingwBin" ""

if (-not $resolvedMingwBin) {
    $cxx = Get-Command c++.exe -ErrorAction SilentlyContinue
    if (-not $cxx) {
        throw "MinGW c++.exe was not found. Start an MSYS2 MINGW64 shell or pass -MingwBin."
    }
    $resolvedMingwBin = Split-Path -Parent $cxx.Source
}

Assert-File -Path (Join-Path $resolvedMingwBin "c++.exe") -Label "MinGW c++.exe"
Assert-File -Path (Join-Path $resolvedMingwBin "cc.exe") -Label "MinGW cc.exe"

$env:PATH = "$resolvedMingwBin;$env:PATH"

$cmakeArgs = @(
    "-S", $script:RepoRoot,
    "-B", $resolvedBuildDir,
    "-G", "Ninja",
    "-DGBARECOMP_MINGW_RUNTIME_BIN=$resolvedMingwBin"
)

Write-Host "Configuring runner: $resolvedBuildDir"
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($resolvedBiosPath) {
    Assert-File -Path $resolvedBiosPath -Label "GBA BIOS"

    Write-Host "Building BIOS recompiler"
    & cmake --build $resolvedBuildDir --target gba_recompile --parallel
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    $gbaRecompile = Join-Path $resolvedBuildDir "gbarecomp-core/gba_recompile.exe"
    Assert-File -Path $gbaRecompile -Label "gba_recompile"

    Write-Host "Generating local recompiled BIOS output"
    Push-Location (Join-Path $script:RepoRoot "extern/gbarecomp")
    & $gbaRecompile --bios $resolvedBiosPath
    $exitCode = $LASTEXITCODE
    Pop-Location
    if ($exitCode -ne 0) { exit $exitCode }

    Write-Host "Reconfiguring runner with recompiled BIOS output"
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} else {
    Write-Warning "No BIOS path configured. The runner will use GBARecomp's HLE fallback, which is not the recommended release path for this project."
}

Write-Host "Building SacredStonesRecomp"
& cmake --build $resolvedBuildDir --target SacredStonesRecomp --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Built: $(Join-Path $resolvedBuildDir 'SacredStonesRecomp.exe')"
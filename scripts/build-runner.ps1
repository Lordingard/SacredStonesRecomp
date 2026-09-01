param(
    [string] $BuildDir = "build/runner-mingw",
    [string] $GeneratedProjectPath,
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
$resolvedGeneratedProject = Resolve-RepoPath (Resolve-Setting $GeneratedProjectPath "SACREDSTONES_RECOMP_OUTPUT" "GeneratedProjectPath" $script:DefaultGeneratedProject)
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
Assert-File -Path (Join-Path $resolvedGeneratedProject "CMakeLists.txt") -Label "Generated GBARecomp project"
Assert-File -Path (Join-Path $resolvedGeneratedProject "generated/dispatch_table.cpp") -Label "Generated dispatch table"

$env:PATH = "$resolvedMingwBin;$env:PATH"

$generatedBuildDir = Join-Path $resolvedGeneratedProject "build-mingw"
$generatedLib = Join-Path $generatedBuildDir "libgbarecomp_game.a"
$generatedBiosDir = Join-Path $resolvedBuildDir "generated-bios"

Write-Host "Configuring generated game library: $generatedBuildDir"
& cmake -S $resolvedGeneratedProject -B $generatedBuildDir -G Ninja
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Building generated game library with MinGW"
& cmake --build $generatedBuildDir --target gbarecomp_game --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Assert-File -Path $generatedLib -Label "Generated MinGW game library"

$cmakeArgs = @(
    "-S", $script:RepoRoot,
    "-B", $resolvedBuildDir,
    "-G", "Ninja",
    "-U", "GBARECOMP_TOMLPP_INCLUDE_DIR",
    "-DGBARECOMP_MINGW_RUNTIME_BIN=$resolvedMingwBin",
    "-DSACREDSTONES_GENERATED_PROJECT=$resolvedGeneratedProject",
    "-DSACREDSTONES_GAME_LIB=$generatedLib",
    "-DGBARECOMP_GENERATED_BIOS_DIR=$generatedBiosDir"
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

    Write-Host "Generating local recompiled BIOS output: $generatedBiosDir"
    New-Item -ItemType Directory -Force -Path $generatedBiosDir | Out-Null
    & $gbaRecompile --bios $resolvedBiosPath --out $generatedBiosDir
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

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
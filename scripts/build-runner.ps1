param(
    [string] $BuildDir = "build/runner-mingw",
    [string] $GeneratedProjectPath,
    [string] $BiosPath,
    [string] $MingwBin,
    [ValidateRange(1, 64)][int] $Jobs = 2
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

if (-not $resolvedBiosPath) {
    throw "A GBA BIOS is required. Pass -BiosPath or configure BiosPath in config/project.local.ps1."
}
$resolvedBiosPath = Resolve-RepoPath $resolvedBiosPath
Assert-File -Path $resolvedBiosPath -Label "GBA BIOS"

if (-not $resolvedMingwBin) {
    $cxx = Get-Command c++.exe -ErrorAction SilentlyContinue
    if (-not $cxx) {
        throw "MinGW c++.exe was not found. Start an MSYS2 MINGW64 shell or pass -MingwBin."
    }
    $resolvedMingwBin = Split-Path -Parent $cxx.Source
}

Assert-File -Path (Join-Path $resolvedMingwBin "c++.exe") -Label "MinGW c++.exe"
Assert-File -Path (Join-Path $resolvedMingwBin "cc.exe") -Label "MinGW cc.exe"
$mingwCxx = (Join-Path $resolvedMingwBin "c++.exe").Replace('\', '/')
$mingwCc = (Join-Path $resolvedMingwBin "cc.exe").Replace('\', '/')
Assert-File -Path (Join-Path $resolvedGeneratedProject "CMakeLists.txt") -Label "Generated GBARecomp project"
Assert-File -Path (Join-Path $resolvedGeneratedProject "generated/dispatch_table.cpp") -Label "Generated dispatch table"

$env:PATH = "$resolvedMingwBin;$env:PATH"

$generatedBuildDir = Join-Path $resolvedGeneratedProject "build-mingw"
$generatedLib = Join-Path $generatedBuildDir "libgbarecomp_game.a"
$generatedBiosDir = Join-Path $resolvedBuildDir "generated-bios"

Write-Host "Configuring generated game library: $generatedBuildDir"
& cmake -S $resolvedGeneratedProject -B $generatedBuildDir -G Ninja "-DCMAKE_CXX_COMPILER=$mingwCxx"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Building generated game library with MinGW"
& cmake --build $generatedBuildDir --target gbarecomp_game --parallel $Jobs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Assert-File -Path $generatedLib -Label "Generated MinGW game library"

$cmakeArgs = @(
    "-S", $script:RepoRoot,
    "-B", $resolvedBuildDir,
    "-G", "Ninja",
    "-DCMAKE_C_COMPILER=$mingwCc",
    "-DCMAKE_CXX_COMPILER=$mingwCxx",
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
    & cmake --build $resolvedBuildDir --target gba_recompile --parallel $Jobs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    $gbaRecompile = Join-Path $resolvedBuildDir "gbarecomp-core/gba_recompile.exe"
    Assert-File -Path $gbaRecompile -Label "gba_recompile"

    Write-Host "Generating local recompiled BIOS output: $generatedBiosDir"
    New-Item -ItemType Directory -Force -Path $generatedBiosDir | Out-Null
    $biosConfig = Join-Path $script:RepoRoot "extern/gbarecomp/bios/gba_bios.toml"
    Assert-File -Path $biosConfig -Label "GBA BIOS recompilation config"
    & $gbaRecompile --bios $resolvedBiosPath --config $biosConfig --out $generatedBiosDir
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "Reconfiguring runner with recompiled BIOS output"
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Write-Host "Building SacredStonesRecomp"
& cmake --build $resolvedBuildDir --target SacredStonesRecomp --parallel $Jobs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Built: $(Join-Path $resolvedBuildDir 'SacredStonesRecomp.exe')"
